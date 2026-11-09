#include "LotController.h"
#include "../services/LotService.h"
#include "../services/DocumentAccess.h"
namespace erp {
void LotController::list(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb){
 try{auto db=drogon::app().getDbClient();auto scope=business::documentScope("inventory","i",req);auto rows=db->execSqlSync("SELECT l.*,p.name AS product_name,w.name AS warehouse_name,l.expires_on<CURDATE() AS expired FROM inventory_lots l JOIN inventory i ON i.product_id=l.product_id AND i.warehouse_id=l.warehouse_id JOIN products p ON p.id=l.product_id JOIN warehouses w ON w.id=l.warehouse_id WHERE "+scope+" ORDER BY l.id DESC");Json::Value data(Json::arrayValue);for(auto row:rows)data.append(business::rowJson(rows,row));cb(ApiResponse::ok("批次库存",data));}catch(const std::exception &){cb(ApiResponse::error(ErrorCode::INTERNAL_ERROR,"批次查询失败"));}
}
void LotController::opening(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb){
 auto j=req->getJsonObject();if(!j||!j->isObject()){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"参数错误"));return;}
 auto user=std::stoll(req->getAttributes()->get<std::string>("user_id"));business::transaction([cb](int code,const std::string &msg,const Json::Value &data){cb(code==ErrorCode::OK?ApiResponse::ok(msg,data):ApiResponse::error(code,msg));},[=](const business::Tx &tx){
  using namespace business;auto pid=j->get("product_id",0).asInt64(),wid=j->get("warehouse_id",0).asInt64();warehouseAccess(tx,user,wid,true);require((*j)["quantity"].isIntegral(),"数量须为整数");auto qty=(*j)["quantity"].asInt64();require(qty>0,"数量须大于零");auto reason=j->get("reason","").asString();require(!reason.empty()&&reason.size()<=255,"请填写期初分配依据");
  auto stocks=tx->execSqlSync("SELECT quantity FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);require(!stocks.empty(),"没有可分配的历史库存");auto product=tx->execSqlSync("SELECT batch_enabled,expiry_enabled FROM products WHERE id=?",pid);require(!product.empty()&&product[0]["batch_enabled"].as<int>(),"商品未启用批次管理");auto name=j->get("lot_no","").asString(),made=j->get("manufactured_on","").asString(),expiry=j->get("expires_on","").asString();require(!name.empty()&&name.size()<=64,"请填写批次号");validateLotDates(made,expiry,product[0]["expiry_enabled"].as<int>());
  auto lots=tx->execSqlSync("SELECT quantity FROM inventory_lots WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);int64_t allocated=0;for(auto row:lots)allocated+=row["quantity"].as<int64_t>();require(allocated+qty<=stocks[0]["quantity"].as<int64_t>(),"分配数量超过未分配历史库存");
  auto r=tx->execSqlSync("INSERT INTO inventory_lots(product_id,warehouse_id,lot_no,manufactured_on,expires_on,quantity,created_by) VALUES(?,?,?,NULLIF(?,''),NULLIF(?,''),?,?)",pid,wid,name,made,expiry,qty,user);tx->execSqlSync("INSERT INTO lot_movements(lot_id,change_qty,biz_type,biz_no,operator_id) VALUES(?,?,'opening',?,?)",r.insertId(),qty,number("OPEN"),user);event(tx,"inventory_lots",r.insertId(),"opening",user,-1,1,reason);Outcome out;out.message="期初批次已登记，实物库存数量未改变";return out;
 },business::requestIdentity(req));
}
}
