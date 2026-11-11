#include "BusinessSupport.h"
#include "StockOperationService.h"
#include "CostService.h"

#include "../utils/ApiResponse.h"

#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>

#include <chrono>
#include <sstream>
#include <iomanip>
#include <set>
#include <vector>
#include <algorithm>

using namespace drogon;
using namespace drogon::orm;

namespace erp
{

static std::string genOpNo(const std::string &prefix)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    std::ostringstream oss;
    oss << prefix << ms;
    return oss.str();
}

void StockOperationService::create(const std::string &type,
                                   int64_t productId,
                                   int64_t fromWarehouseId,
                                   int64_t toWarehouseId,
                                   int qty,
                                   const std::string &bizType,
                                   const std::string &bizNo,
                                   const std::string &logisticsCompany,
                                   const std::string &logisticsNo,
                                   const std::string &serialNos,
                                   const std::string &remark,
                                   int64_t applicantId,
                                   int64_t assignedApproverId,
                                  StockOpCallback &&cb, const business::RequestIdentity &request,const Json::Value &lot)
{
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;
  actor(tx,applicantId,"仓库部");assigned(tx,"仓库部",applicantId,assignedApproverId);
  require(type=="in" || type=="out" || type=="transfer","操作类型错误");require(qty>0 && productId>0,"商品或数量错误");require(bizType!="scrap" || (type=="out"&&!remark.empty()),"报损必须为出库并填写原因");
  if(type!="in")warehouseAccess(tx,applicantId,fromWarehouseId);
  if(type!="out") {warehouse(tx,toWarehouseId);if(type=="in")warehouseAccess(tx,applicantId,toWarehouseId);}
  require(type!="transfer" || fromWarehouseId!=toWarehouseId,"调拨的来源和目标仓库不能相同");
  auto product=tx->execSqlSync("SELECT has_serial FROM products WHERE id=? AND status=1",productId);require(!product.empty(),"商品不存在或已停用");
  std::set<std::string> uniqueSerials;std::vector<std::string> orderedSerials;std::istringstream serialInput(serialNos);std::string serial;
  while(std::getline(serialInput,serial,',')){auto first=serial.find_first_not_of(" \t\r\n"),last=serial.find_last_not_of(" \t\r\n");require(first!=std::string::npos,"序列号不能为空");auto value=serial.substr(first,last-first+1);require(value.size()<=512&&std::count_if(value.begin(),value.end(),[](unsigned char ch){return (ch&0xc0)!=0x80;})<=128&&uniqueSerials.insert(value).second,"序列号过长或重复");orderedSerials.push_back(value);}
  if(product[0]["has_serial"].as<int>())require(orderedSerials.size()==static_cast<size_t>(qty),"序列号数量必须与商品数量一致");
  require(remark.size()<=255 && logisticsCompany.size()<=64 && logisticsNo.size()<=64,"备注或物流信息过长");
  auto no=number("SO");auto r=tx->execSqlSync("INSERT INTO stock_operations (operation_no,type,product_id,from_warehouse_id,to_warehouse_id,qty,biz_type,biz_no,logistics_company,logistics_no,status,applicant_id,remark,assigned_approver_id) VALUES (?,?,?,?,?,?,?,?,?,?,0,?,?,NULLIF(?,0))",no,type,productId,fromWarehouseId,toWarehouseId,qty,bizType,bizNo,logisticsCompany,logisticsNo,applicantId,remark,assignedApproverId);
  auto id=r.insertId();int serialLine=0;for(const auto &value:orderedSerials)tx->execSqlSync("INSERT INTO stock_operation_serials(operation_id,line_no,serial_no) VALUES(?,?,?)",id,++serialLine,value);Json::Value metadata;for(auto key:{"lot_no","manufactured_on","expires_on","unit_cost"})metadata[key]=lot.get(key,"");Json::StreamWriterBuilder writer;tx->execSqlSync("UPDATE stock_operations SET lot_details=? WHERE id=?",Json::writeString(writer,metadata),id);tx->execSqlSync("INSERT INTO stock_operation_logs (operation_id,action,operator_id,remark) VALUES (?,'submit',?,?)",id,applicantId,remark);event(tx,"stock_operations",id,"submit",applicantId,-1,0,remark);
  Outcome out;out.data["operation_id"]=Json::UInt64(id);out.data["operation_no"]=no;out.data["status"]=0;out.message="申请已提交，等待领导审批";return out;
 },request);
}

void StockOperationService::approve(int64_t operationId,
                                    bool approve,
                                    int64_t approverId,
                                    const std::string &remark,
                                    StockOpCallback &&cb)
{
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;
  auto rows=tx->execSqlSync("SELECT * FROM stock_operations WHERE id=? FOR UPDATE",operationId);require(!rows.empty(),"操作单不存在",ErrorCode::NOT_FOUND);auto row=rows[0];
  approval(tx,row,approverId,"仓库部","applicant_id");require(row["status"].as<int>()==0,"该操作单已处理，请刷新",ErrorCode::CONFLICT);
  const auto type=row["type"].as<std::string>();auto pid=row["product_id"].as<int64_t>(),qty=row["qty"].as<int64_t>();auto from=row["from_warehouse_id"].as<int64_t>(),to=row["to_warehouse_id"].as<int64_t>();
  if(type!="in")warehouseAccess(tx,approverId,from,true);if(type!="out")warehouseAccess(tx,approverId,to,true);
  require(type=="in" || type=="out" || type=="transfer","单据类型错误");require(qty>0,"数量错误");auto no=row["operation_no"].as<std::string>();
  Json::Value lot;if(!row["lot_details"].isNull()){Json::Reader reader;require(reader.parse(row["lot_details"].as<std::string>(),lot),"批次资料格式错误");}
  if(approve){
   const auto serialRows = tx->execSqlSync("SELECT serial_no FROM stock_operation_serials WHERE operation_id=? ORDER BY line_no",operationId);
   std::string serialText;
   for (const auto &serial : serialRows) {
    if (!serialText.empty()) serialText += ",";
    serialText += serial["serial_no"].as<std::string>();
   }
   lot["serial_nos"] = serialText;
   if(type=="in" && lot.isMember("unit_cost") && !lot["unit_cost"].asString().empty()){validateUnitCost(lot["unit_cost"].asString());auto v=tx->execSqlSync("SELECT CAST(? AS DECIMAL(18,6))*? AS value",lot["unit_cost"].asString(),qty);lot["incoming_cost_value"]=v[0]["value"].as<std::string>();}
   if(type=="in")stock(tx,pid,to,qty,"stock_in",no,approverId,lot);
   else if(type=="out")stock(tx,pid,from,-qty,row["biz_type"].as<std::string>()=="scrap"?"scrap_out":"stock_out",no,approverId,lot);
   else {
    require(from!=to,"调拨仓库必须不同");auto first=std::min(from,to),second=std::max(from,to);
    tx->execSqlSync("INSERT INTO inventory (product_id,warehouse_id,quantity,min_quantity) VALUES (?,?,0,0) ON DUPLICATE KEY UPDATE id=id",pid,first);
    tx->execSqlSync("INSERT INTO inventory (product_id,warehouse_id,quantity,min_quantity) VALUES (?,?,0,0) ON DUPLICATE KEY UPDATE id=id",pid,second);
    const auto destination = tx->execSqlSync(
        "SELECT quantity FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,to);
    require(destination[0]["quantity"].as<int64_t>() + qty <= 2147483647,
            "目标仓库存容量不足");
    const auto moved = stock(tx,pid,from,-qty,"transfer_out",no,approverId,lot);
    Json::Value shipment(Json::objectValue);
    shipment["lot_allocations"] = moved["lot_allocations"];
    shipment["serial_nos"] = serialText;
    if (!moved["cost_value"].isNull()) shipment["incoming_cost_value"] = moved["cost_value"];
    Json::StreamWriterBuilder writer;
    tx->execSqlSync("UPDATE stock_operations SET lot_details=? WHERE id=?",
                    Json::writeString(writer,shipment),operationId);
   }
  }
  int after=approve?(type=="transfer"?3:1):2;tx->execSqlSync("UPDATE stock_operations SET status=?,approved_by=?,approved_at=NOW() WHERE id=?",after,approverId,operationId);
  tx->execSqlSync("INSERT INTO stock_operation_logs (operation_id,action,operator_id,remark) VALUES (?,?,?,?)",operationId,approve?"approve":"reject",approverId,remark);
  event(tx,"stock_operations",operationId,approve?"approve":"reject",approverId,0,after,remark);Outcome out;out.data["operation_id"]=Json::Int64(operationId);out.data["status"]=after;out.message=after==3?"已从来源仓发运，等待目标仓签收":approve?"已批准，库存与流水已一起提交":"已驳回";return out;
 });
}

void StockOperationService::list(const std::string &type,
                                 int status,
                                 int64_t operatorId,
                                 StockOpCallback &&cb)
{
    auto clientPtr = app().getDbClient("default");
    try
    {
        std::string sql =
            "SELECT so.*, p.name AS product_name, p.sku, "
            "wf.name AS from_warehouse_name, wt.name AS to_warehouse_name, "
            "ua.real_name AS applicant_name, ub.real_name AS approver_name "
            "FROM stock_operations so "
            "LEFT JOIN products p ON so.product_id = p.id "
            "LEFT JOIN warehouses wf ON so.from_warehouse_id = wf.id "
            "LEFT JOIN warehouses wt ON so.to_warehouse_id = wt.id "
            "LEFT JOIN user_directory ua ON so.applicant_id = ua.id "
            "LEFT JOIN user_directory ub ON so.approved_by = ub.id WHERE 1=1";
        std::vector<Json::Value> params;
        if (!type.empty())
        {
            sql += " AND so.type = ?";
            params.push_back(type);
        }
        if (status >= 0 && status <= 2)
        {
            sql += " AND so.status = ?";
            params.push_back(status);
        }
        if (operatorId > 0)
        {
            sql += " AND (so.applicant_id = ? OR so.approved_by = ?)";
            params.push_back(operatorId);
            params.push_back(operatorId);
        }
        sql += " ORDER BY so.id DESC";

        Result result = [&]() -> Result {
            switch (params.size())
            {
                case 1:
                    return clientPtr->execSqlSync(sql, params[0]);
                case 2:
                    return clientPtr->execSqlSync(sql, params[0],
                                                  params[1]);
                case 3:
                    return clientPtr->execSqlSync(sql, params[0], params[1],
                                                  params[2]);
                case 4:
                    return clientPtr->execSqlSync(sql, params[0], params[1],
                                                  params[2], params[3]);
                default:
                    return clientPtr->execSqlSync(sql);
            }
        }();

        Json::Value arr(Json::arrayValue);
        for (const auto &r : result)
        {
            Json::Value o;
            o["id"] = (Json::Int64)r["id"].as<int64_t>();
            o["operation_no"] = r["operation_no"].as<std::string>();
            o["type"] = r["type"].as<std::string>();
            o["product_id"] = (Json::Int64)r["product_id"].as<int64_t>();
            o["product_name"] = r["product_name"].as<std::string>();
            o["sku"] = r["sku"].as<std::string>();
            o["from_warehouse_id"] =
                (Json::Int64)r["from_warehouse_id"].as<int64_t>();
            o["to_warehouse_id"] =
                (Json::Int64)r["to_warehouse_id"].as<int64_t>();
            o["from_warehouse_name"] =
                r["from_warehouse_name"].as<std::string>();
            o["to_warehouse_name"] = r["to_warehouse_name"].as<std::string>();
            o["qty"] = r["qty"].as<int>();
            o["biz_type"] = r["biz_type"].as<std::string>();
            o["biz_no"] = r["biz_no"].as<std::string>();
            o["logistics_company"] = r["logistics_company"].as<std::string>();
            o["logistics_no"] = r["logistics_no"].as<std::string>();
            std::string serialText;auto serialRows=clientPtr->execSqlSync("SELECT serial_no FROM stock_operation_serials WHERE operation_id=? ORDER BY line_no",r["id"].as<int64_t>());for(auto serialRow:serialRows){if(!serialText.empty())serialText+=",";serialText+=serialRow["serial_no"].as<std::string>();}o["serial_nos"]=serialText;
            o["status"] = r["status"].as<int>();
            o["applicant_id"] = r["applicant_id"].isNull()
                                    ? Json::Value()
                                    : Json::Value((Json::Int64)r["applicant_id"]
                                                      .as<int64_t>());
            o["applicant_name"] =
                r["applicant_name"].isNull() ? Json::Value()
                                             : r["applicant_name"].as<std::string>();
            o["approved_by"] = r["approved_by"].isNull()
                                   ? Json::Value()
                                   : Json::Value((Json::Int64)r["approved_by"]
                                                     .as<int64_t>());
            o["approver_name"] =
                r["approver_name"].isNull() ? Json::Value()
                                            : r["approver_name"].as<std::string>();
            o["approved_at"] = r["approved_at"].isNull()
                                   ? Json::Value()
                                   : r["approved_at"].as<std::string>();
            o["remark"] = r["remark"].as<std::string>();
            o["created_at"] = r["created_at"].as<std::string>();
            arr.append(o);
        }
        cb(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        cb(ErrorCode::INTERNAL_ERROR,
           std::string("查询库存操作单失败: ") + e.what(), Json::Value());
    }
}

}  // namespace erp
