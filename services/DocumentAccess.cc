#include <map>
#include "DocumentAccess.h"
namespace erp::business {
namespace {
const std::set<std::string> tables={"purchase_orders","sales_orders","purchase_order_items","sales_order_items","purchase_returns","sales_returns","purchase_return_items","sales_return_items","stock_operations","stock_checks","stock_check_items","stock_records","inventory","applications","accounts","payments"};
bool role(const std::string &value,const std::string &name){return (","+value+",").find(","+name+",")!=std::string::npos;}
}
std::string documentScope(const std::string &table,const std::string &a,const drogon::HttpRequestPtr &req) {
 return documentScope(table, a, std::stoll(req->getAttributes()->get<std::string>("user_id")), *drogon::app().getDbClient());
}
std::string documentScope(const std::string &table, const std::string &a, int64_t uid, drogon::orm::DbClient &db) {
 require(tables.count(table),"单据类型错误");
 auto id=std::to_string(uid);
 auto users=db.execSqlSync("SELECT u.department,u.warehouse_id,COALESCE(GROUP_CONCAT(r.code),'') AS roles FROM user_directory u LEFT JOIN user_roles ur ON ur.user_id=u.id LEFT JOIN roles r ON r.id=ur.role_id WHERE u.id=? AND u.status=1 GROUP BY u.id",uid);
 require(!users.empty(),"账号已停用",ErrorCode::FORBIDDEN);auto user=users[0];auto roles=user["roles"].as<std::string>(),dept=user["department"].as<std::string>();
 if(role(roles,"admin"))return "1=1";
 auto isLeader=role(roles,"leader");auto wid=user["warehouse_id"].isNull()?"0":std::to_string(user["warehouse_id"].as<int64_t>());
 if(table=="accounts" || table=="payments")return dept=="财务部"?"1=1":"1=0";
 if(table=="purchase_order_items" || table=="sales_order_items" || table=="purchase_return_items" || table=="sales_return_items" || table=="stock_check_items"){
  std::string parent=table=="purchase_order_items"?"purchase_orders":table=="sales_order_items"?"sales_orders":table=="purchase_return_items"?"purchase_returns":table=="sales_return_items"?"sales_returns":"stock_checks";
  std::string key=table=="stock_check_items"?"check_id":table=="purchase_return_items" || table=="sales_return_items"?"return_id":"order_id";
  return "EXISTS(SELECT 1 FROM "+parent+" scope_parent WHERE scope_parent.id="+a+"."+key+" AND ("+documentScope(parent,"scope_parent",uid,db)+"))";
 }
 if(table=="inventory" || table=="stock_records") {
  if(dept=="仓库部" && !isLeader)return a+".warehouse_id="+wid;
  return (dept=="销售部" || dept=="采购部" || dept=="仓库部" || dept=="财务部")?"1=1":"1=0";
 }
 bool app=table=="applications",op=table=="stock_operations",check=table=="stock_checks";
 std::string creator=(app||op)?"applicant_id":"created_by";
 std::string scope=a+"."+creator+"="+id;
 if(!check)scope+=" OR "+a+".approved_by="+id+" OR "+a+".assigned_approver_id="+id;
 if(check)scope+=" OR "+a+".reviewed_by="+id;
 if(!app && !op && !check)scope+=" OR "+a+".warehouse_confirmed_by="+id;
 if(table=="sales_orders")scope+=" OR "+a+".warehouse_approved_by="+id;
 if(isLeader){
  std::string expected=table.find("purchase")==0?"采购部":table.find("sales")==0?"销售部":"仓库部";
  if(!app && dept==expected)scope+=" OR 1=1";
  if(app && dept=="销售部")scope+=" OR "+a+".type='customer'";
  if(app && dept=="采购部")scope+=" OR "+a+".type IN ('supplier','product')";
 }
 if(dept=="仓库部" && !app){
  if(op)scope+=" OR "+a+".from_warehouse_id="+wid+" OR "+a+".to_warehouse_id="+wid;
  else scope+=" OR "+a+".warehouse_id="+wid;
  if(isLeader)scope+=" OR 1=1";
 }
 if(dept=="财务部" && !app && !op && !check)scope+=" OR 1=1";
 return "("+scope+")";
}
Json::Value rowJson(const drogon::orm::Result &result,const drogon::orm::Row &row){
 Json::Value out(Json::objectValue);const std::set<std::string> integers={"id","status","quantity","qty","change_qty","before_qty","after_qty","min_quantity","actual_qty","book_qty","diff_qty","received_qty","shipped_qty","has_serial","pending_qty","reserved_qty","available_qty","batch_enabled","expiry_enabled","expired","cost_initialized"};
 for(size_t i=0;i<result.columns();++i){std::string key=result.columnName(i);auto field=row[i];if(field.isNull())out[key]=Json::Value();else if(integers.count(key) || key=="created_by" || key=="approved_by" || key=="reviewed_by" || key=="counted_by" || key=="warehouse_confirmed_by" || key=="warehouse_approved_by" || (key.size()>3 && key.substr(key.size()-3)=="_id"))out[key]=Json::Int64(field.as<int64_t>());else out[key]=field.as<std::string>();}return out;
}
void readDocuments(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> cb,const std::string &table,int64_t id){
 try {
  std::string select="d.*",joins;
  if(table=="inventory")select+=",d.quantity-d.reserved_qty AS available_qty";
  if(table=="stock_operations") {select+=",p.name AS product_name,wf.name AS from_warehouse_name,wt.name AS to_warehouse_name,COALESCE(NULLIF(ua.real_name,''),ua.username) AS applicant_name,COALESCE(NULLIF(ub.real_name,''),ub.username) AS approver_name";joins=" LEFT JOIN products p ON p.id=d.product_id LEFT JOIN warehouses wf ON wf.id=d.from_warehouse_id LEFT JOIN warehouses wt ON wt.id=d.to_warehouse_id LEFT JOIN user_directory ua ON ua.id=d.applicant_id LEFT JOIN user_directory ub ON ub.id=d.approved_by";}
  if(table=="applications") {select+=",COALESCE(NULLIF(ua.real_name,''),ua.username) AS applicant_name,COALESCE(NULLIF(ub.real_name,''),ub.username) AS approver_name";joins=" LEFT JOIN user_directory ua ON ua.id=d.applicant_id LEFT JOIN user_directory ub ON ub.id=d.approved_by";}
  if(table=="payments") {select+=",COALESCE(NULLIF(ua.real_name,''),ua.username) AS created_by_name,COALESCE(NULLIF(ub.real_name,''),ub.username) AS approved_by_name,ac.biz_no,ac.partner_name";joins=" LEFT JOIN user_directory ua ON ua.id=d.created_by LEFT JOIN user_directory ub ON ub.id=d.approved_by LEFT JOIN accounts ac ON ac.id=d.account_id";}
  if(table=="stock_checks") {select+=",COALESCE(NULLIF(ua.real_name,''),ua.username) AS created_by_name,COALESCE(NULLIF(ub.real_name,''),ub.username) AS reviewer_name,COALESCE(NULLIF(uc.real_name,''),uc.username,IF(d.counted_by IS NULL,'未记录',CONCAT('用户#',d.counted_by))) AS counter_name";joins=" LEFT JOIN user_directory ua ON ua.id=d.created_by LEFT JOIN user_directory ub ON ub.id=d.reviewed_by LEFT JOIN user_directory uc ON uc.id=d.counted_by";}
  if(table=="purchase_returns" || table=="sales_returns") {bool p=table=="purchase_returns";auto partner=std::string(p?"supplier_id":"customer_id");select+=",original_order."+partner+" AS "+partner+",party.name AS "+std::string(p?"supplier_name":"customer_name")+",COALESCE(NULLIF(ua.real_name,''),ua.username) AS created_by_name,original_order.order_no AS order_no";joins=" LEFT JOIN "+std::string(p?"purchase_orders":"sales_orders")+" original_order ON original_order.id=d.order_id LEFT JOIN "+std::string(p?"suppliers":"customers")+" party ON party.id=original_order."+partner+" LEFT JOIN user_directory ua ON ua.id=d.created_by";}
  if(table.find("items")!=std::string::npos){select+=",product.name AS product_name,product.sku AS sku,product.batch_enabled,product.expiry_enabled";joins+=" LEFT JOIN products product ON product.id=d.product_id";}
  auto sql="SELECT "+select+" FROM "+table+" d"+joins+" WHERE ("+documentScope(table,"d",req)+")";
  if(id)sql+=" AND d.id="+std::to_string(id);
  Json::Value filter;auto raw=req->getParameter("filter");if(!raw.empty()){Json::Reader reader;require(reader.parse(raw,filter) && filter.isObject(),"筛选条件格式错误");}
  for(const auto &key:{"order_id","return_id","check_id"}){
   if((table.find("items")!=std::string::npos) && ((std::string(key)=="order_id" && table.find("order_items")!=std::string::npos) || (std::string(key)=="return_id" && table.find("return_items")!=std::string::npos) || (std::string(key)=="check_id" && table=="stock_check_items"))){
    auto rawId=req->getParameter(key);int64_t v=filter.isMember(key)?filter[key].asInt64():rawId.empty()?0:std::stoll(rawId);if(v)sql+=" AND d."+std::string(key)+"="+std::to_string(v);
   }
  }
  if ((table=="purchase_returns" || table=="sales_returns") && req->getParameter("warehouse_queue")=="1") {
   const auto uid=std::stoll(req->getAttributes()->get<std::string>("user_id"));
   sql+=" AND d.status=1 AND EXISTS(SELECT 1 FROM user_directory w WHERE w.id="+std::to_string(uid)+
        " AND w.status=1 AND (EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=w.id AND r.code='admin')"
        " OR (w.department='仓库部' AND (w.warehouse_id=d.warehouse_id OR EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=w.id AND r.code='leader')))))";
  }
  auto state=req->getParameter("status");if(!state.empty() && table!="inventory" && table!="stock_records" && table.find("items")==std::string::npos)sql+=" AND d.status="+std::to_string(std::stoi(state));
  if(table=="payments" && filter.isMember("account_id"))sql+=" AND d.account_id="+std::to_string(filter["account_id"].asInt64());
  if(table=="stock_operations"){auto type=req->getParameter("type");if(!type.empty()){require(type=="in" || type=="out" || type=="transfer","操作类型错误");sql+=" AND d.type='"+type+"'";}}
  auto op=req->getParameter("operator_id");if(!op.empty() && table.find("items")==std::string::npos && table!="inventory" && table!="stock_records" && table!="accounts" && table!="payments"){
   auto who=std::to_string(std::stoll(op));auto creator=(table=="stock_operations" || table=="applications")?"applicant_id":"created_by";
   sql+=" AND (d."+std::string(creator)+"="+who;
   if(table!="stock_checks")sql+=" OR d.approved_by="+who;
   if(table=="purchase_orders" || table=="sales_orders" || table=="purchase_returns" || table=="sales_returns")sql+=" OR d.warehouse_confirmed_by="+who;
   if(table=="sales_orders")sql+=" OR d.warehouse_approved_by="+who;
   sql+=")";
  }
  if(table=="applications"){auto type=req->getParameter("type");if(!type.empty()){require(type=="supplier" || type=="customer" || type=="product","申报类型错误");sql+=" AND d.type='"+type+"'";}}
  sql+=" ORDER BY d.id DESC";auto rows=drogon::app().getDbClient()->execSqlSync(sql);std::map<int64_t,std::string> serialText;if(table=="stock_operations"&&!rows.empty()){std::string ids;for(auto row:rows){if(!ids.empty())ids+=",";ids+=std::to_string(row["id"].as<int64_t>());}auto serials=drogon::app().getDbClient()->execSqlSync("SELECT operation_id,serial_no FROM stock_operation_serials WHERE operation_id IN ("+ids+") ORDER BY operation_id,line_no");for(auto serial:serials){auto &text=serialText[serial["operation_id"].as<int64_t>()];if(!text.empty())text+=",";text+=serial["serial_no"].as<std::string>();}}Json::Value data(Json::arrayValue);for(auto row:rows){auto value=rowJson(rows,row);if(table=="stock_operations")value["serial_nos"]=serialText[row["id"].as<int64_t>()];data.append(value);}
  if(id){require(!data.empty(),"单据不存在或无权查看",ErrorCode::NOT_FOUND);cb(drogon::HttpResponse::newHttpJsonResponse(data[0]));}else cb(drogon::HttpResponse::newHttpJsonResponse(data));
 }catch(const Error &e){cb(ApiResponse::error(e.code,e.what()));}catch(const std::exception &e){LOG_ERROR<<e.what();cb(ApiResponse::error(ErrorCode::INTERNAL_ERROR,"查询失败"));}
}
}
