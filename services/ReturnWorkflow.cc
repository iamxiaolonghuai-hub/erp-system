#include "ReturnWorkflow.h"
#include "LotService.h"
#include "FinancePeriod.h"
#include "DocumentAccess.h"
#include <set>
#include <vector>
#include <climits>
namespace erp::business {
namespace {
std::string rt(bool p){return p?"purchase_returns":"sales_returns";}
std::string ri(bool p){return p?"purchase_return_items":"sales_return_items";}
std::string ot(bool p){return p?"purchase_orders":"sales_orders";}
std::string oi(bool p){return p?"purchase_order_items":"sales_order_items";}
std::string department(bool p){return p?"采购部":"销售部";}
struct Line {int64_t product,qty,price;std::string direction;};
int64_t validateReturned(const Tx &tx,bool p,int64_t order,int64_t product,int64_t qty,int64_t exclude=0){
 auto source=tx->execSqlSync(std::string("SELECT SUM(")+(p?"received_qty":"shipped_qty")+") AS qty,MIN(price) AS price,MAX(price) AS max_price FROM "+oi(p)+" WHERE order_id=? AND product_id=?",order,product);
 require(!source[0]["qty"].isNull(),"退回商品不属于原订单");require(source[0]["price"].as<std::string>()==source[0]["max_price"].as<std::string>(),"原订单同商品存在不同价格，请拆分处理");
 std::string direction=p?"":" AND i.direction='return'";
 auto used=tx->execSqlSync("SELECT COALESCE(SUM(i.quantity),0) AS qty FROM "+ri(p)+" i JOIN "+rt(p)+" r ON r.id=i.return_id WHERE r.order_id=? AND i.product_id=? AND r.status IN (0,1,3) AND r.id<>?"+direction,order,product,exclude);
 require(qty>0 && qty<=INT_MAX && qty<=source[0]["qty"].as<int64_t>()-used[0]["qty"].as<int64_t>(),"累计退货数量超过原单可退数量（含待审批退货）");
 return cents(source[0]["price"].as<std::string>());
}
Outcome result(int64_t id,int state,const std::string &msg){Outcome out;out.data["return_id"]=Json::Int64(id);out.data["status"]=state;out.message=msg;return out;}
}
void createReturn(bool p,int64_t order,int64_t wid,const std::string &remark,const std::string &company,const std::string &tracking,int64_t user,const std::string &type,const Json::Value &items,int64_t approver,Callback cb,const RequestIdentity &request){
 transaction(std::move(cb),[=](const Tx &tx){
  actor(tx,user,department(p));warehouse(tx,wid);assigned(tx,department(p),user,approver);
  require(type=="return" || (!p && type=="exchange"),"退换货类型错误");require(items.isArray() && !items.empty() && items.size()<=500,"请填写退换货明细");
  const auto scope = documentScope(ot(p), "o", user, *tx);
  auto orders = tx->execSqlSync("SELECT o.* FROM " + ot(p) + " o WHERE o.id=? AND (" + scope + ") FOR UPDATE", order);
  require(!orders.empty(),"原订单不存在或无权发起退换货",ErrorCode::FORBIDDEN);
  auto source=orders[0];
  require(source["status"].as<int>()==3 || source["status"].as<int>()==8,"只有已完成订单可以退换货");
  std::set<std::pair<int64_t,std::string>> unique;std::vector<Line> lines;int64_t returned=0,replaced=0;
  for(auto item:items){
   require(item.isObject() && item["quantity"].isIntegral() && item["product_id"].isIntegral(),"商品和数量必须为整数");
   auto pid=item.get("product_id",0).asInt64(),qty=item.get("quantity",0).asInt64();auto dir=p?"return":item.get("direction","return").asString();require(dir=="return" || (type=="exchange" && dir=="replace"),"明细方向错误");require(pid>0 && qty>0 && qty<=INT_MAX && unique.emplace(pid,dir).second,"商品重复或数量错误");
   int64_t price=0;
   if(dir=="return")price=validateReturned(tx,p,order,pid,qty);
   else {require(!tx->execSqlSync("SELECT id FROM products WHERE id=? AND status=1",pid).empty(),"换出商品不存在或停用");price=cents(item.get("price",0).asDouble());require(price>=0 && price<=999999999999LL,"换出商品单价错误");}
   require(!price || qty<=99999999999999LL/price,"退货金额超出范围");auto amount=qty*price;
   auto &total=dir=="return"?returned:replaced;require(total<=99999999999999LL-amount,"退货金额超出范围");total+=amount;lines.push_back({pid,qty,price,dir});
  }
  if(type=="exchange")require(returned==replaced && unique.size()>=2,"目前仅支持等价换货，请保证退入和换出金额一致");
  auto partner=source[p?"supplier_id":"customer_id"].as<int64_t>();auto no=number(p?"PR":"SR");
  auto r=p?tx->execSqlSync("INSERT INTO purchase_returns (return_no,order_id,warehouse_id,total_amount,status,remark,logistics_company,logistics_no,created_by,assigned_approver_id) VALUES (?,?,?,?,0,?,?,?,?,NULLIF(?,0))",no,order,wid,money(returned),remark,company,tracking,user,approver):tx->execSqlSync("INSERT INTO sales_returns (return_no,return_type,order_id,warehouse_id,total_amount,status,remark,logistics_company,logistics_no,created_by,assigned_approver_id) VALUES (?,?,?,?,?,0,?,?,?,?,NULLIF(?,0))",no,type,order,wid,money(returned),remark,company,tracking,user,approver);
  auto id=r.insertId();for(auto line:lines){
   if(p)tx->execSqlSync("INSERT INTO purchase_return_items (return_id,product_id,quantity,price,amount) VALUES (?,?,?,?,?)",id,line.product,line.qty,money(line.price),money(line.qty*line.price));
   else tx->execSqlSync("INSERT INTO sales_return_items (return_id,direction,product_id,quantity,price,amount) VALUES (?,?,?,?,?,?)",id,line.direction,line.product,line.qty,money(line.price),money(line.qty*line.price));
  }
  event(tx,rt(p),id,"submit",user,-1,0,remark);auto out=result(id,0,"退换货已提交，金额按原单价格核算");out.data["return_no"]=no;return out;
 },request);
}
void approveReturn(bool p,int64_t id,bool approve,int64_t user,Callback cb){
 transaction(std::move(cb),[=](const Tx &tx){
  auto rows=tx->execSqlSync("SELECT * FROM "+rt(p)+" WHERE id=? FOR UPDATE",id);require(!rows.empty(),"退货单不存在",ErrorCode::NOT_FOUND);auto row=rows[0];approval(tx,row,user,department(p),"created_by");require(row["status"].as<int>()==0,"该单据已处理",ErrorCode::CONFLICT);
  int after=approve?1:2;tx->execSqlSync("UPDATE "+rt(p)+" SET status=?,approved_by=? WHERE id=?",after,user,id);event(tx,rt(p),id,approve?"approve":"reject",user,0,after);return result(id,after,approve?"已批准，等待仓库处理":"已驳回");
 });
}
void warehouseReturn(bool p,int64_t id,bool approve,int64_t user,const std::string &remark,const std::string &company,const std::string &tracking,Callback cb,const Json::Value &serialItems){
 transaction(std::move(cb),[=](const Tx &tx){
  if (approve) lockFinancePosting(tx);
  auto rows=tx->execSqlSync("SELECT * FROM "+rt(p)+" WHERE id=? FOR UPDATE",id);require(!rows.empty(),"退货单不存在",ErrorCode::NOT_FOUND);auto row=rows[0];auto wid=row["warehouse_id"].as<int64_t>();warehouseAccess(tx,user,wid);require(row["status"].as<int>()==1,"该单据已处理或未审批",ErrorCode::CONFLICT);
  if(!approve){tx->execSqlSync("UPDATE "+rt(p)+" SET status=4,remark=?,warehouse_confirmed_by=? WHERE id=?",remark,user,id);event(tx,rt(p),id,"warehouse_reject",user,1,4,remark);return result(id,4,"仓库已打回");}
  auto order=row["order_id"].as<int64_t>();auto source=tx->execSqlSync("SELECT * FROM "+ot(p)+" WHERE id=? FOR UPDATE",order);require(!source.empty(),"原订单不存在");
  auto items=tx->execSqlSync("SELECT * FROM "+ri(p)+" WHERE return_id=? ORDER BY product_id",id);require(!items.empty(),"明细不能为空");
  bool exchange=!p && row["return_type"].as<std::string>()=="exchange";
  std::map<int64_t,std::string> serials;
  if (!serialItems.isNull()) {
   require(serialItems.isArray() && serialItems.size()<=500,"序列号明细格式错误");
   for (const auto &line : serialItems) {
    require(line["item_id"].isInt64() && line["serial_nos"].isString()
            && serials.emplace(line["item_id"].asInt64(),line["serial_nos"].asString()).second,
            "序列号明细无效或重复");
   }
  }
  for(auto item:items){auto pid=item["product_id"].as<int64_t>(),qty=item["quantity"].as<int64_t>();bool replace=!p && item["direction"].as<std::string>()=="replace";if(!replace)validateReturned(tx,p,order,pid,qty,id);
   Json::Value metadata;if(!p&&!replace){tx->execSqlSync("INSERT INTO inventory(product_id,warehouse_id,quantity,min_quantity) VALUES(?,?,0,0) ON DUPLICATE KEY UPDATE id=id",pid,wid);tx->execSqlSync("SELECT id FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);metadata=salesReturnDetails(tx,order,source[0]["order_no"].as<std::string>(),pid,qty);}
   const auto lineId = item["id"].as<int64_t>();
   if (serials.count(lineId)) {
    metadata["serial_nos"] = serials.at(lineId);
    serials.erase(lineId);
   }
   metadata["original_order"] = source[0]["order_no"].as<std::string>();
   stock(tx,pid,wid,(p||replace)?-qty:qty,p?"purchase_return":replace?"sales_exchange_out":"sales_return",row["return_no"].as<std::string>(),user,metadata);
  }
  require(serials.empty(),"序列号明细不属于本单");
  if(!exchange){auto partner=source[0][p?"supplier_id":"customer_id"].as<int64_t>();auto party=tx->execSqlSync("SELECT name FROM "+std::string(p?"suppliers":"customers")+" WHERE id=?",partner);require(!party.empty(),"往来单位不存在");auto credit=tx->execSqlSync("INSERT INTO accounts (account_type,biz_type,biz_no,partner_id,partner_name,amount,paid_amount,status) VALUES (?,?,?,?,?,?,0,0)",p?"payable":"receivable",p?"purchase_return":"sales_return",row["return_no"].as<std::string>(),partner,party[0]["name"].as<std::string>(),money(-cents(row["total_amount"].as<std::string>())));
   financePosting(tx,credit.insertId(),p?"purchase_return":"sales_return",row["return_no"].as<std::string>(),
                  -cents(row["total_amount"].as<std::string>()),0,0);
  }
  tx->execSqlSync("UPDATE "+rt(p)+" SET status=3,warehouse_confirmed_by=?,logistics_company=?,logistics_no=? WHERE id=?",user,company,tracking,id);event(tx,rt(p),id,"warehouse_complete",user,1,3);return result(id,3,"退换货完成，库存、流水及贷项已一起保存");
 });
}
}
