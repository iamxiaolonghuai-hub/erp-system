#include "LotService.h"
#include <regex>
#include <algorithm>
namespace erp::business {
namespace {
void validDate(const std::string &value){
 require(std::regex_match(value,std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}")),"批次日期格式错误");int y=std::stoi(value.substr(0,4)),m=std::stoi(value.substr(5,2)),d=std::stoi(value.substr(8,2));const int days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};require(y>=1900&&m>=1&&m<=12&&d>=1&&d<=days[m]+(m==2&&(y%400==0||(y%4==0&&y%100!=0))),"批次日期无效");
}
}
void validateLotDates(const std::string &made,const std::string &expiry,bool required){
 require(!required||(!made.empty()&&!expiry.empty()),"有效期商品必须提供生产日期和有效期");if(!made.empty())validDate(made);if(!expiry.empty())validDate(expiry);require(made.empty()||expiry.empty()||made<=expiry,"生产日期不能晚于有效期");
}
Json::Value salesReturnDetails(const Tx &tx,int64_t order,const std::string &orderNo,int64_t product,int64_t quantity){
 Json::Value details;auto costs=tx->execSqlSync("SELECT COUNT(*) AS n,SUM(cost_value IS NULL) AS missing,ROUND(SUM(cost_value)/SUM(-change_qty)*?,6) AS value FROM stock_records WHERE biz_type='sale_out' AND biz_no=? AND product_id=?",quantity,orderNo,product);
 if(costs[0]["n"].as<int64_t>()>0 && costs[0]["missing"].as<int64_t>()==0)details["incoming_cost_value"]=costs[0]["value"].as<std::string>();
 auto source=tx->execSqlSync("SELECT l.id,l.lot_no,l.manufactured_on,l.expires_on,SUM(-m.change_qty) AS sent FROM lot_movements m JOIN inventory_lots l ON l.id=m.lot_id WHERE m.biz_type='sale_out' AND m.biz_no=? AND l.product_id=? GROUP BY l.id ORDER BY l.id",orderNo,product);
 Json::Value allocations(Json::arrayValue);int64_t remaining=quantity;
 for(auto lot:source){if(!remaining)break;auto previous=tx->execSqlSync("SELECT m.change_qty FROM lot_movements m JOIN inventory_lots l ON l.id=m.lot_id JOIN sales_returns r ON r.return_no=m.biz_no WHERE r.order_id=? AND l.product_id=? AND l.lot_no=? AND m.biz_type='sales_return' FOR UPDATE",order,product,lot["lot_no"].as<std::string>());int64_t used=0;for(auto row:previous)used+=row["change_qty"].as<int64_t>();auto available=lot["sent"].as<int64_t>()-used;auto take=std::min(remaining,std::max<int64_t>(0,available));if(!take)continue;Json::Value item;item["lot_no"]=lot["lot_no"].as<std::string>();item["manufactured_on"]=lot["manufactured_on"].isNull()?"":lot["manufactured_on"].as<std::string>();item["expires_on"]=lot["expires_on"].isNull()?"":lot["expires_on"].as<std::string>();item["quantity"]=Json::Int64(take);allocations.append(item);remaining-=take;}
 if(!source.empty()){require(remaining==0,"原销售批次可退数量不足");details["lot_allocations"]=allocations;}
 return details;
}
Json::Value changeLots(const Tx &tx,int64_t pid,int64_t wid,int64_t delta,int64_t book,const std::string &type,const std::string &no,int64_t user,const Json::Value &details){
 Json::Value allocations(Json::arrayValue);auto product=tx->execSqlSync("SELECT batch_enabled,expiry_enabled FROM products WHERE id=?",pid);if(product.empty()||!product[0]["batch_enabled"].as<int>())return allocations;
 auto lots=tx->execSqlSync("SELECT *,expires_on<CURDATE() AS expired FROM inventory_lots WHERE product_id=? AND warehouse_id=? ORDER BY expires_on IS NULL,expires_on,id FOR UPDATE",pid,wid);int64_t total=0;for(auto lot:lots)total+=lot["quantity"].as<int64_t>();require(total==book,"已有库存尚未完成期初批次分配，或批次数量与库存不一致");
 if(delta>0 && details["lot_allocations"].isArray()){require(details["lot_allocations"].size()<=500,"一次最多分配500个批次");
  int64_t assigned=0;for(auto item:details["lot_allocations"]){require(!item.isMember("lot_allocations"),"不支持嵌套批次分配");require(item["quantity"].isIntegral(),"批次数量必须为整数");auto q=item["quantity"].asInt64();require(q>0&&q<=delta-assigned,"批次分配数量错误");changeLots(tx,pid,wid,q,book+assigned,type,no,user,item);assigned+=q;}require(assigned==delta,"批次分配总数不一致");return details["lot_allocations"];
 }
 if(delta>0){
  auto name=details.get("lot_no","").asString(),made=details.get("manufactured_on","").asString(),expiry=details.get("expires_on","").asString();require(!name.empty()&&name.size()<=64,"请填写批次号，最多64字节");validateLotDates(made,expiry,product[0]["expiry_enabled"].as<int>()!=0);
  auto found=tx->execSqlSync("SELECT * FROM inventory_lots WHERE product_id=? AND warehouse_id=? AND lot_no=? FOR UPDATE",pid,wid,name);int64_t id;
  if(found.empty()){auto r=tx->execSqlSync("INSERT INTO inventory_lots(product_id,warehouse_id,lot_no,manufactured_on,expires_on,quantity,created_by) VALUES(?,?,?,NULLIF(?,''),NULLIF(?,''),?,?)",pid,wid,name,made,expiry,delta,user);id=r.insertId();}
  else{auto lot=found[0];require((lot["manufactured_on"].isNull()?"":lot["manufactured_on"].as<std::string>())==made&&(lot["expires_on"].isNull()?"":lot["expires_on"].as<std::string>())==expiry,"同一批次日期不能改变");id=lot["id"].as<int64_t>();tx->execSqlSync("UPDATE inventory_lots SET quantity=quantity+? WHERE id=?",delta,id);}
  tx->execSqlSync("INSERT INTO lot_movements(lot_id,change_qty,biz_type,biz_no,operator_id) VALUES(?,?,?,?,?)",id,delta,type,no,user);
 }else{
  if (details["lot_allocations"].isArray()) {
   int64_t assigned=0;
   for (const auto &allocation : details["lot_allocations"]) {
    const auto name=allocation["lot_no"].asString();
    const auto qty=allocation["quantity"].asInt64();
    auto matched=std::find_if(lots.begin(),lots.end(),[&](const auto &row){return row["lot_no"].template as<std::string>()==name;});
    require(matched!=lots.end() && qty>0 && qty<=-delta-assigned,"序列号批次分配错误");
    const auto row=*matched;
    const bool expired=!row["expired"].isNull() && row["expired"].as<int>();
    require(!expired || type=="scrap_out" || type=="purchase_return" || type=="transfer_out",
            "过期批次禁止普通出库");
    require(qty<=row["quantity"].as<int64_t>(),"序列号所在批次库存不足");
    const auto lotId=row["id"].as<int64_t>();
    tx->execSqlSync("UPDATE inventory_lots SET quantity=quantity-? WHERE id=?",qty,lotId);
    tx->execSqlSync("INSERT INTO lot_movements(lot_id,change_qty,biz_type,biz_no,operator_id) VALUES (?,?,?,?,?)",
                    lotId,-qty,type,no,user);
    assigned+=qty;
   }
   require(assigned==-delta,"序列号批次分配数量不一致");
   return details["lot_allocations"];
  }
  int64_t needed=-delta;for(auto lot:lots){if(!needed)break;auto q=lot["quantity"].as<int64_t>();bool expired=!lot["expired"].isNull()&&lot["expired"].as<int>();if(expired&&type!="scrap_out"&&type!="purchase_return"&&type!="transfer_out")continue;auto take=std::min(q,needed);if(!take)continue;Json::Value assigned;assigned["lot_no"]=lot["lot_no"].as<std::string>();assigned["manufactured_on"]=lot["manufactured_on"].isNull()?"":lot["manufactured_on"].as<std::string>();assigned["expires_on"]=lot["expires_on"].isNull()?"":lot["expires_on"].as<std::string>();assigned["quantity"]=Json::Int64(take);allocations.append(assigned);auto id=lot["id"].as<int64_t>();tx->execSqlSync("UPDATE inventory_lots SET quantity=quantity-? WHERE id=?",take,id);tx->execSqlSync("INSERT INTO lot_movements(lot_id,change_qty,biz_type,biz_no,operator_id) VALUES(?,?,?,?,?)",id,-take,type,no,user);needed-=take;}require(needed==0,"可出库批次数量不足，过期批次禁止普通销售或普通调整出库",ErrorCode::STOCK_NOT_ENOUGH);
 }
 return allocations;
}
}
