#include "../services/DocumentAccess.h"
#pragma once
#include <drogon/HttpController.h>
#include "../services/ApprovalAssignment.h"
namespace erp {
class ApprovalController: public drogon::HttpController<ApprovalController> {
public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(ApprovalController::leaders,"/api/approval/leaders",drogon::Get,"erp::JwtAuthMiddleware");
 ADD_METHOD_TO(ApprovalController::assignments,"/api/approval/assignments",drogon::Get,"erp::JwtAuthMiddleware");
 ADD_METHOD_TO(ApprovalController::myActions,"/api/approval/my-actions",drogon::Get,"erp::JwtAuthMiddleware");
 ADD_METHOD_TO(ApprovalController::history,"/api/approval/history",drogon::Get,"erp::JwtAuthMiddleware");
 METHOD_LIST_END
 void myActions(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void history(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  try {
   auto type=req->getParameter("document_type");auto id=std::stoll(req->getParameter("document_id"));
   auto scope=business::documentScope(type,"d",req);
   auto db=drogon::app().getDbClient();auto visible=db->execSqlSync("SELECT d.id FROM "+type+" d WHERE d.id=? AND ("+scope+")",id);
   business::require(!visible.empty(),"单据不存在或无权查看",ErrorCode::NOT_FOUND);
   auto rows=db->execSqlSync("SELECT e.*,COALESCE(NULLIF(u.real_name,''),u.username,CONCAT('用户#',e.operator_id)) AS operator_name FROM business_events e LEFT JOIN users u ON u.id=e.operator_id WHERE document_type=? AND document_id=? ORDER BY e.id",type,id);
   Json::Value out(Json::arrayValue);for(auto row:rows)out.append(business::rowJson(rows,row));cb(drogon::HttpResponse::newHttpJsonResponse(out));
  }catch(const business::Error &e){cb(ApiResponse::error(e.code,e.what()));}catch(const std::exception &e){LOG_ERROR<<e.what();cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"查询条件错误"));}
 }
 void assignments(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  const auto table=req->getParameter("table");
  const std::set<std::string> allowed={"purchase_orders","sales_orders","applications","stock_operations","purchase_returns","sales_returns"};
  if(!allowed.count(table)){auto r=drogon::HttpResponse::newHttpResponse();r->setStatusCode(drogon::k400BadRequest);cb(r);return;}
  try {
   const std::string creator=(table=="applications" || table=="stock_operations")?"applicant_id":"created_by";
   const bool warehouse=(table=="purchase_orders" || table=="sales_orders" || table=="purchase_returns" || table=="sales_returns");
   const std::string wh=warehouse ? "d.warehouse_confirmed_by" : "NULL";
   const std::string wa=table=="sales_orders" ? "d.warehouse_approved_by" : "NULL";
   const std::string orderWarehouse=warehouse?"d.warehouse_id":"NULL";
   auto rows=drogon::app().getDbClient()->execSqlSync("SELECT d.id,d.approved_by,d.assigned_approver_id, "+wh+" AS wh_id, "+wa+" AS wa_id, "+orderWarehouse+" AS order_warehouse_id, COALESCE(NULLIF(u.real_name,''),u.username,'部门审批队列') AS name, COALESCE(NULLIF(c.real_name,''),c.username,IF(d."+creator+" IS NULL,'—',CONCAT('用户#',d."+creator+"))) AS creator_name, COALESCE(NULLIF(a.real_name,''),a.username,IF(d.approved_by IS NULL,'待审批',CONCAT('用户#',d.approved_by))) AS approver_name, COALESCE(NULLIF(w.real_name,''),w.username,IF("+wh+" IS NULL,'待处理',CONCAT('用户#',"+wh+"))) AS warehouse_name, COALESCE(NULLIF(x.real_name,''),x.username,IF("+wa+" IS NULL,'待批准',CONCAT('用户#',"+wa+"))) AS warehouse_approver_name FROM "+table+" d LEFT JOIN users u ON u.id=d.assigned_approver_id LEFT JOIN users c ON c.id=d."+creator+" LEFT JOIN users a ON a.id=d.approved_by LEFT JOIN users w ON w.id="+wh+" LEFT JOIN users x ON x.id="+wa+" WHERE "+business::documentScope(table,"d",req));
   Json::Value out(Json::arrayValue);for(auto &r:rows){Json::Value v;v["id"]=Json::Int64(r["id"].as<int64_t>());v["assigned_approver_id"]=r["assigned_approver_id"].isNull()?Json::Value():Json::Value(Json::Int64(r["assigned_approver_id"].as<int64_t>()));v["approved_by"]=r["approved_by"].isNull()?Json::Value():Json::Value(Json::Int64(r["approved_by"].as<int64_t>()));v["assigned_approver_name"]=r["name"].as<std::string>();v["warehouse_id"]=r["order_warehouse_id"].isNull()?Json::Value():Json::Value(Json::Int64(r["order_warehouse_id"].as<int64_t>()));v["created_by_name"]=r["creator_name"].as<std::string>();v["approver_name"]=r["approver_name"].as<std::string>();v["warehouse_confirmed_by"]=r["wh_id"].isNull()?Json::Value():Json::Value(Json::Int64(r["wh_id"].as<int64_t>()));v["warehouse_approved_by"]=r["wa_id"].isNull()?Json::Value():Json::Value(Json::Int64(r["wa_id"].as<int64_t>()));v["warehouse_confirmed_name"]=r["warehouse_name"].as<std::string>();v["warehouse_approver_name"]=r["warehouse_approver_name"].as<std::string>();out.append(v);}cb(drogon::HttpResponse::newHttpJsonResponse(out));
  }catch(const std::exception &e){LOG_ERROR<<e.what();auto r=drogon::HttpResponse::newHttpResponse();r->setStatusCode(drogon::k500InternalServerError);cb(r);}
 }
 void leaders(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  try {
   auto id=std::stoll(req->getAttributes()->get<std::string>("user_id"));
   auto rows=drogon::app().getDbClient()->execSqlSync("SELECT DISTINCT l.id,l.real_name,l.username FROM users l JOIN user_roles ur ON ur.user_id=l.id JOIN roles r ON r.id=ur.role_id JOIN users u ON u.id=? WHERE l.department_id=u.department_id AND l.status=1 AND r.code='leader' AND l.id<>u.id ORDER BY l.id",id);
   Json::Value out(Json::arrayValue);for(auto &r:rows){Json::Value v;v["id"]=Json::Int64(r["id"].as<int64_t>());v["name"]=r["real_name"].as<std::string>();if(v["name"].asString().empty())v["name"]=r["username"].as<std::string>();out.append(v);}cb(drogon::HttpResponse::newHttpJsonResponse(out));
  } catch(const std::exception &e){LOG_ERROR<<e.what();auto r=drogon::HttpResponse::newHttpResponse();r->setStatusCode(drogon::k500InternalServerError);cb(r);}
 }
};
}
