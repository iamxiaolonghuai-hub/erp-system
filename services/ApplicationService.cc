#include "ApprovalHandover.h"
#include "BusinessSupport.h"
#include "ApplicationService.h"

#include "../utils/ApiResponse.h"
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

#include <memory>
#include <chrono>
#include <sstream>
#include <vector>
#include <string>

using namespace drogon;
using namespace drogon::orm;

namespace erp
{

static std::string genApplicationNo()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    std::ostringstream oss;
    oss << "APP" << ms;
    return oss.str();
}

static std::string jsonStr(const Json::Value &v, const std::string &key)
{
    if (!v.isMember(key) || v[key].isNull())
        return "";
    return v[key].asString();
}

void ApplicationService::createApplication(const std::string &type,
                                           const std::string &title,
                                           const Json::Value &payload,
                                           int64_t applicantId,
                                           const std::string &remark,
                                           int64_t assignedApproverId,
                                  ApplicationCallback &&cb, const business::RequestIdentity &request)
{
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;require(type=="supplier" || type=="customer" || type=="product","申报类型错误");auto department=type=="customer"?"销售部":"采购部";actor(tx,applicantId,department);assigned(tx,department,applicantId,assignedApproverId);
  require(!title.empty() && title.size()<=128,"请填写名称，长度不超过128字节");require(payload.isObject(),"申报内容格式错误");
  Json::StreamWriterBuilder writer;auto serialized=Json::writeString(writer,payload);auto no=number("APP");
  auto r=tx->execSqlSync("INSERT INTO applications (application_no,type,title,payload,status,applicant_id,remark,assigned_approver_id) VALUES (?,?,?,?,0,?,?,NULLIF(?,0))",no,type,title,serialized,applicantId,remark,assignedApproverId);
  event(tx,"applications",r.insertId(),"submit",applicantId,-1,0,remark);Outcome out;out.data["application_id"]=Json::UInt64(r.insertId());out.data["application_no"]=no;out.message="申报已提交";return out;
 },request);
}

void ApplicationService::approveApplication(int64_t applicationId,
                                            bool approve,
                                            int64_t approverId,
                                            const std::string &remark,
                                            ApplicationCallback &&cb)
{
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;auto rows=tx->execSqlSync("SELECT * FROM applications WHERE id=? FOR UPDATE",applicationId);require(!rows.empty(),"申报不存在",ErrorCode::NOT_FOUND);auto row=rows[0];auto type=row["type"].as<std::string>();approval(tx,row,approverId,type=="customer"?"销售部":"采购部","applicant_id");require(row["status"].as<int>()==0,"该申报已处理",ErrorCode::CONFLICT);
  if(approve){
   Json::Value data;Json::Reader reader;require(reader.parse(row["payload"].as<std::string>(),data) && data.isObject(),"申报内容格式错误");auto title=row["title"].as<std::string>();
   if(type=="supplier" || type=="customer")tx->execSqlSync("INSERT INTO "+std::string(type=="supplier"?"suppliers":"customers")+" (name,contact,phone,email,address,status) VALUES (?,?,?,?,?,1)",title,data.get("contact","").asString(),data.get("phone","").asString(),data.get("email","").asString(),data.get("address","").asString());
   else {
    require(type=="product","申报类型错误");auto supplier=data.get("supplier_id",0).asInt64();require(!tx->execSqlSync("SELECT id FROM suppliers WHERE id=? AND status=1",supplier).empty(),"请选择启用供应商");auto price=cents(data.get("price","0").asString()),cost=cents(data.get("cost_price","0").asString());require(price>=0 && cost>=0,"价格不能为负数");auto sku=data.get("sku","").asString();require(!sku.empty(),"商品编码不能为空");
    tx->execSqlSync("INSERT INTO products (sku,name,category_id,unit,spec,price,cost_price,status,description,image_url,supplier_id,has_serial) VALUES (?,?,?,?,?,?,?,1,?,?,?,?)",sku,title,data.get("category_id",0).asInt64(),data.get("unit","件").asString(),data.get("spec","").asString(),money(price),money(cost),data.get("description","").asString(),data.get("image_url","").asString(),supplier,data.get("has_serial",0).asInt()?1:0);
   }
  }
  int after=approve?1:2;tx->execSqlSync("UPDATE applications SET status=?,approved_by=?,remark=? WHERE id=?",after,approverId,remark,applicationId);event(tx,"applications",applicationId,approve?"approve":"reject",approverId,0,after,remark);Outcome out;out.data["status"]=after;out.message=approve?"已批准，正式资料已一起保存":"已驳回";return out;
 });
}

void ApplicationService::listApplications(const std::string &type,
                                          int status,
                                          ApplicationCallback &&cb)
{
    auto callback = std::make_shared<ApplicationCallback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    try
    {
        std::string sql =
            "SELECT a.id, a.application_no, a.type, a.title, a.payload, "
            "a.status, a.applicant_id, a.approved_by, a.remark, a.created_at, "
            "u.real_name AS applicant_name, u2.real_name AS approver_name "
            "FROM applications a "
            "LEFT JOIN user_directory u ON u.id = a.applicant_id "
            "LEFT JOIN user_directory u2 ON u2.id = a.approved_by "
            "WHERE 1=1";
        std::vector<std::string> paramStr;

        if (!type.empty())
        {
            sql += " AND a.type = ?";
            paramStr.push_back(type);
        }
        if (status >= 0 && status <= 2)
        {
            sql += " AND a.status = ?";
            paramStr.push_back(std::to_string(status));
        }
        sql += " ORDER BY a.id DESC";

        // 按参数个数分派查询
        Result result = [&]() -> Result {
            if (paramStr.size() == 0)
                return clientPtr->execSqlSync(sql);
            else if (paramStr.size() == 1)
                return clientPtr->execSqlSync(sql, paramStr[0]);
            else
                return clientPtr->execSqlSync(sql, paramStr[0], paramStr[1]);
        }();

        Json::Value arr(Json::arrayValue);
        for (const auto &row : result)
        {
            Json::Value item;
            item["id"] = (Json::UInt64)row["id"].as<uint64_t>();
            item["application_no"] = row["application_no"].as<std::string>();
            item["type"] = row["type"].as<std::string>();
            item["title"] = row["title"].as<std::string>();
            item["payload"] = row["payload"].isNull()
                                  ? ""
                                  : row["payload"].as<std::string>();
            item["status"] = (int)row["status"].as<int8_t>();
            item["applicant_id"] = row["applicant_id"].isNull()
                                       ? 0
                                       : (Json::UInt64)row["applicant_id"].as<uint64_t>();
            item["approved_by"] = row["approved_by"].isNull()
                                      ? 0
                                      : (Json::UInt64)row["approved_by"].as<uint64_t>();
            item["remark"] = row["remark"].as<std::string>();
            item["created_at"] = row["created_at"].as<std::string>();
            item["applicant_name"] = row["applicant_name"].isNull()
                                         ? ""
                                         : row["applicant_name"].as<std::string>();
            item["approver_name"] = row["approver_name"].isNull()
                                        ? ""
                                        : row["approver_name"].as<std::string>();
            arr.append(item);
        }

        (*callback)(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR,
                    std::string("查询申报单失败: ") + e.what(), Json::Value());
    }
}

void ApplicationService::resubmitApplication(int64_t applicationId,
                                             const std::string &title,
                                             const Json::Value &payload,
                                             int64_t applicantId,
                                             ApplicationCallback &&cb)
{
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;auto rows=tx->execSqlSync("SELECT * FROM applications WHERE id=? FOR UPDATE",applicationId);require(!rows.empty(),"申报不存在",ErrorCode::NOT_FOUND);auto row=rows[0];actor(tx,applicantId,row["type"].as<std::string>()=="customer"?"销售部":"采购部");require(!row["applicant_id"].isNull() && row["applicant_id"].as<int64_t>()==applicantId,"只有发起人可修改重提",ErrorCode::FORBIDDEN);require(row["status"].as<int>()==2,"该申报不能修改",ErrorCode::CONFLICT);require(!title.empty() && payload.isObject(),"申报内容不能为空");Json::StreamWriterBuilder writer;
  const auto assignee = assigneeAfterResubmit(tx,row,row["type"].as<std::string>()=="customer"?"销售部":"采购部",applicantId,"applications",applicationId,2);
  tx->execSqlSync("UPDATE applications SET title=?,payload=?,status=0,approved_by=NULL,remark='',assigned_approver_id=NULLIF(?,0) WHERE id=?",title,Json::writeString(writer,payload),assignee,applicationId);event(tx,"applications",applicationId,"resubmit",applicantId,2,0);Outcome out;out.data["application_id"]=Json::Int64(applicationId);out.data["status"]=0;out.message="已重新提交";return out;
 });
}

}  // namespace erp
