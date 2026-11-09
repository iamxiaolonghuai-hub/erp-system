#include "FinanceActionController.h"
#include <algorithm>
#include "../services/FinanceActions.h"
#include "../services/DocumentAccess.h"
namespace erp {
using namespace drogon;
void FinanceActionController::advice(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb){
 try{
  auto db=app().getDbClient();auto id=std::stoll(req->getParameter("account_id"));auto rows=db->execSqlSync("SELECT * FROM accounts WHERE id=?",id);business::require(!rows.empty(),"账目不存在");auto credit=rows[0];Json::Value result;result["source_account_id"]=Json::Value();result["source_biz_no"]="";result["suggested_offset"]=0;result["suggested_refund"]=0;
  auto remainder=business::cents(credit["amount"].as<std::string>())-business::cents(credit["paid_amount"].as<std::string>())-business::cents(credit["offset_amount"].as<std::string>());business::require(remainder<0,"请选择未处理的退货贷项");
  bool purchase=credit["account_type"].as<std::string>()=="payable";auto source=db->execSqlSync("SELECT a.* FROM "+std::string(purchase?"purchase_returns":"sales_returns")+" r JOIN "+(purchase?"purchase_orders":"sales_orders")+" o ON o.id=r.order_id JOIN accounts a ON a.biz_no=o.order_no AND a.biz_type=? WHERE r.return_no=? AND a.partner_id=?",purchase?"purchase":"sale",credit["biz_no"].as<std::string>(),credit["partner_id"].as<int64_t>());
  auto cash=db->execSqlSync("SELECT COALESCE(SUM(paid_amount),0) AS total FROM accounts WHERE partner_id=? AND account_type=?",credit["partner_id"].as<int64_t>(),credit["account_type"].as<std::string>());int64_t available=std::max<int64_t>(0,business::cents(cash[0]["total"].as<std::string>())),offset=0;
  if(source.size()==1){auto original=source[0];offset=std::min(-remainder,std::max<int64_t>(0,business::cents(original["amount"].as<std::string>())-business::cents(original["paid_amount"].as<std::string>())-business::cents(original["offset_amount"].as<std::string>())));result["source_account_id"]=Json::Int64(original["id"].as<int64_t>());result["source_biz_no"]=original["biz_no"].as<std::string>();}
  result["credit_remaining"]=business::money(-remainder);result["suggested_offset"]=business::money(offset);result["suggested_refund"]=business::money(std::min(-remainder-offset,available));result["note"]="优先抵扣原账单，退款参考同一往来单位的现金净结算；仅为建议，复核时重新检查余额。";cb(ApiResponse::ok("结算建议",result));
 }catch(const std::exception &e){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));}
}
void FinanceActionController::submit(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb){
 auto json=req->getJsonObject();if(!json || !json->isObject()){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"请提交合法JSON对象"));return;}
 try {submitFinance(*json,std::stoll(req->getAttributes()->get<std::string>("user_id")),[cb](int code,const std::string &msg,const Json::Value &data){cb(code==ErrorCode::OK?ApiResponse::ok(msg,data):ApiResponse::error(code,msg));},business::requestIdentity(req));}
 catch(const std::exception &){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"财务参数格式错误"));}
}
void FinanceActionController::review(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb){
 auto json=req->getJsonObject();if(!json || !(*json)["approve"].isBool() || !(*json)["action_id"].isIntegral()){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"请提供申请编号和复核结果"));return;}
 try {reviewFinance((*json)["action_id"].asInt64(),(*json)["approve"].asBool(),json->get("remark","").asString(),std::stoll(req->getAttributes()->get<std::string>("user_id")),[cb](int code,const std::string &msg,const Json::Value &data){cb(code==ErrorCode::OK?ApiResponse::ok(msg,data):ApiResponse::error(code,msg));});}
 catch(const std::exception &){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"复核参数格式错误"));}
}
void FinanceActionController::list(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb){
 try {
  auto db=app().getDbClient();auto sql=std::string("SELECT a.*,ac.biz_no,ac.partner_name,ac.account_type,ac.amount AS account_amount,t.biz_no AS target_biz_no,p.payment_no,COALESCE(NULLIF(u.real_name,''),u.username) AS created_by_name,COALESCE(NULLIF(v.real_name,''),v.username) AS approved_by_name,COALESCE(NULLIF(w.real_name,''),w.username) AS assigned_approver_name FROM finance_actions a JOIN accounts ac ON ac.id=a.account_id LEFT JOIN accounts t ON t.id=a.target_account_id LEFT JOIN payments p ON p.id=a.payment_id LEFT JOIN user_directory u ON u.id=a.created_by LEFT JOIN user_directory v ON v.id=a.approved_by LEFT JOIN user_directory w ON w.id=a.assigned_approver_id WHERE 1=1");
  auto status=req->getParameter("status");if(!status.empty()){business::require(status=="0" || status=="1" || status=="2","状态筛选错误");sql+=" AND a.status="+status;}
  auto id=req->getParameter("account_id");if(!id.empty())sql+=" AND (a.account_id="+std::to_string(std::stoll(id))+" OR a.target_account_id="+std::to_string(std::stoll(id))+")";
  sql+=" ORDER BY a.id DESC LIMIT 1000";auto rows=db->execSqlSync(sql);Json::Value data(Json::arrayValue);for(auto row:rows)data.append(business::rowJson(rows,row));cb(ApiResponse::ok("查询成功",data));
 }catch(const business::Error &e){cb(ApiResponse::error(e.code,e.what()));}catch(const std::exception &){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"财务查询失败或筛选格式错误"));}
}
void FinanceActionController::reviewers(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb){
 try {auto rows=app().getDbClient()->execSqlSync("SELECT u.id,COALESCE(NULLIF(u.real_name,''),u.username) AS name FROM user_directory u WHERE u.status=1 AND u.id<>? AND (u.department='财务部' OR EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='admin')) ORDER BY u.id",std::stoll(req->getAttributes()->get<std::string>("user_id")));Json::Value data(Json::arrayValue);for(auto row:rows){Json::Value item;item["id"]=Json::Int64(row["id"].as<int64_t>());item["name"]=row["name"].as<std::string>();data.append(item);}cb(ApiResponse::ok("查询成功",data));}
 catch(const std::exception &){cb(ApiResponse::error(ErrorCode::INTERNAL_ERROR,"复核人查询失败"));}
}
}
