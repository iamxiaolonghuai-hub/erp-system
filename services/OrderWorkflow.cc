#include "ApprovalHandover.h"
#include "FinancePeriod.h"
#include "OrderWorkflow.h"
#include <set>
#include <map>
#include <climits>
namespace erp::business {
namespace {
std::string table(bool p) { return p?"purchase_orders":"sales_orders"; }
std::string details(bool p) { return p?"purchase_order_items":"sales_order_items"; }
std::string dept(bool p) { return p?"采购部":"销售部"; }
int64_t lines(const Tx &tx,bool p,int64_t id,int64_t partner,const Json::Value &items) {
    require(items.isArray() && !items.empty() && items.size()<=500,"明细应包含 1 至 500 行商品");
    std::set<int64_t> products; int64_t total=0;
    for(const auto &item:items) {
        require(item.isObject() && item["quantity"].isIntegral() && item["product_id"].isIntegral(),"商品和数量必须为整数");
        auto pid=item.get("product_id",0).asInt64(),qty=item.get("quantity",0).asInt64();
        require(pid>0 && qty>0 && qty<=INT_MAX && products.insert(pid).second,"明细商品不可重复，数量必须为正整数");
        auto price=cents(item.get("price",0).asDouble()); require(price>=0 && price<=999999999999LL,"商品单价错误");
        require(!price || qty<=99999999999999LL/price,"明细金额超出范围");auto amount=qty*price;
        require(total<=99999999999999LL-amount,"单据金额超出范围"); total+=amount;
        auto product=tx->execSqlSync("SELECT status,supplier_id FROM products WHERE id=?",pid);
        require(!product.empty() && product[0]["status"].as<int>()==1,"明细包含不存在或停用商品");
        if(p)require(product[0]["supplier_id"].isNull() || product[0]["supplier_id"].as<int64_t>()==partner,"采购商品与供应商不一致");
        tx->execSqlSync("INSERT INTO "+details(p)+" (order_id,product_id,quantity,price,amount) VALUES (?,?,?,?,?)",id,pid,qty,money(price),money(amount));
    }return total;
}
Outcome response(int64_t id,int status,const std::string &msg) { Outcome out;out.data["order_id"]=Json::Int64(id);out.data["status"]=status;out.message=msg;return out; }
}
void createOrder(bool p,int64_t partner,int64_t wid,const std::string &remark,int64_t user,const Json::Value &items,int64_t approver,Callback cb,const RequestIdentity &request) {
 transaction(std::move(cb),[=](const Tx &tx){
    actor(tx,user,dept(p));warehouse(tx,wid);assigned(tx,dept(p),user,approver);require(remark.size()<=255,"备注过长");
    require(!tx->execSqlSync("SELECT id FROM "+std::string(p?"suppliers":"customers")+" WHERE id=? AND status=1",partner).empty(),"往来单位不存在或已停用");
    auto no=number(p?"PO":"SO");auto r=tx->execSqlSync("INSERT INTO "+table(p)+" (order_no,"+(p?"supplier_id":"customer_id")+",warehouse_id,total_amount,status,remark,created_by,assigned_approver_id) VALUES (?,?,?,0,0,?,?,NULLIF(?,0))",no,partner,wid,remark,user,approver);
    auto id=r.insertId();auto total=lines(tx,p,id,partner,items);tx->execSqlSync("UPDATE "+table(p)+" SET total_amount=? WHERE id=?",money(total),id);
    event(tx,table(p),id,"submit",user,-1,0,remark);auto out=response(id,0,"单据已提交，等待部门领导审批");out.data["order_no"]=no;out.data["total_amount"]=money(total);return out;
 },request);
}
void approveOrder(bool p,int64_t id,bool approve,int64_t user,Callback cb) {
 transaction(std::move(cb),[=](const Tx &tx){
    auto rows=tx->execSqlSync("SELECT * FROM "+table(p)+" WHERE id=? FOR UPDATE",id);require(!rows.empty(),"单据不存在",ErrorCode::NOT_FOUND);auto row=rows[0];approval(tx,row,user,dept(p),"created_by");
    int before=row["status"].as<int>();require(before==0,"单据已处理，请刷新",ErrorCode::CONFLICT);int after=approve?1:2;
    tx->execSqlSync("UPDATE "+table(p)+" SET status=?,approved_by=? WHERE id=?",after,user,id);event(tx,table(p),id,approve?"approve":"reject",user,before,after);return response(id,after,approve?"已批准，等待仓库处理":"已驳回");
 });
}
void processOrder(bool p,bool prepare,int64_t id,bool approve,int64_t user,int64_t requested,const std::string &remark,const std::string &company,const std::string &tracking,Callback cb,const Json::Value &batch,const RequestIdentity &request) {
 transaction(std::move(cb),[=](const Tx &tx){
  if (approve && !prepare) lockFinancePosting(tx);
  auto rows=tx->execSqlSync("SELECT * FROM "+table(p)+" WHERE id=? FOR UPDATE",id);require(!rows.empty(),"单据不存在");auto row=rows[0];
  auto wid=row["warehouse_id"].isNull()?requested:row["warehouse_id"].as<int64_t>();require(wid>0 && (!requested || requested==wid),"处理仓库必须与单据仓库一致");warehouseAccess(tx,user,wid,!p&&!prepare);
  int before=row["status"].as<int>();require(before==(p||prepare?1:5),"单据状态已变化",ErrorCode::CONFLICT);
  if(!p&&!prepare){require(row["created_by"].as<int64_t>()!=user && row["warehouse_confirmed_by"].as<int64_t>()!=user,"备货、发起与放行人员必须分离",ErrorCode::FORBIDDEN);}
  require(remark.size()<=255 && company.size()<=64 && tracking.size()<=64,"备注或物流信息过长");
  auto items=tx->execSqlSync("SELECT * FROM "+details(p)+" WHERE order_id=? ORDER BY product_id FOR UPDATE",id);require(!items.empty(),"明细为空");
  if(!approve){
   bool partial=false;for(auto line:items){if(line[p?"received_qty":"shipped_qty"].as<int64_t>()>0)partial=true;if(!p && line["pending_qty"].as<int64_t>()>0){auto q=line["pending_qty"].as<int64_t>();tx->execSqlSync("UPDATE inventory SET reserved_qty=reserved_qty-? WHERE product_id=? AND warehouse_id=? AND reserved_qty>=?",q,line["product_id"].as<int64_t>(),wid,q);}}
   if(!p)tx->execSqlSync("UPDATE sales_order_items SET pending_qty=0 WHERE order_id=?",id);
   int state=partial?1:4;tx->execSqlSync("UPDATE "+table(p)+" SET status=? WHERE id=?",state,id);event(tx,table(p),id,"warehouse_reject",user,before,state,remark);return response(id,state,"已打回并释放本批预占，已过账批次保留");
  }
  std::map<int64_t,int64_t> selected;std::map<int64_t,Json::Value> lotDetails;
  if(!batch.isNull()){require(batch.isArray()&&!batch.empty(),"请选择本批商品");for(auto item:batch){require(item["quantity"].isIntegral(),"数量必须为整数");auto pid=item.get("product_id",0).asInt64(),q=item["quantity"].asInt64();lotDetails[pid]=item;require(q>=0 && selected.emplace(pid,q).second,"数量错误或商品重复");}}
  int64_t total=0;int count=0;Json::Value posted(Json::arrayValue);bool complete=true;
  for(auto line:items){auto pid=line["product_id"].as<int64_t>(),done=line[p?"received_qty":"shipped_qty"].as<int64_t>(),remaining=line["quantity"].as<int64_t>()-done;
   int64_t qty=(!p&&!prepare)?line["pending_qty"].as<int64_t>():batch.isNull()?remaining:selected.count(pid)?selected[pid]:0;
   if(selected.count(pid))selected.erase(pid);require(qty>=0 && qty<=remaining,"本批数量超过剩余数量");if(done+qty<line["quantity"].as<int64_t>())complete=false;if(!qty)continue;++count;
   if(prepare){
    auto stockRows=tx->execSqlSync("SELECT quantity,reserved_qty FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);require(!stockRows.empty() && stockRows[0]["quantity"].as<int64_t>()-stockRows[0]["reserved_qty"].as<int64_t>()>=qty,"可用库存不足（含其他订单预占）",ErrorCode::STOCK_NOT_ENOUGH);
    auto product=tx->execSqlSync("SELECT batch_enabled FROM products WHERE id=?",pid);if(product[0]["batch_enabled"].as<int>()){auto lots=tx->execSqlSync("SELECT quantity,expires_on IS NULL OR expires_on>=CURDATE() AS valid FROM inventory_lots WHERE product_id=? AND warehouse_id=? ORDER BY id FOR UPDATE",pid,wid);int64_t all=0,valid=0;for(auto l:lots){all+=l["quantity"].as<int64_t>();if(l["valid"].as<int>())valid+=l["quantity"].as<int64_t>();}require(all==stockRows[0]["quantity"].as<int64_t>(),"请先完成期初批次分配");require(valid-stockRows[0]["reserved_qty"].as<int64_t>()>=qty,"未过期的可备货批次数量不足",ErrorCode::STOCK_NOT_ENOUGH);}
    tx->execSqlSync("UPDATE inventory SET reserved_qty=reserved_qty+? WHERE product_id=? AND warehouse_id=?",qty,pid,wid);tx->execSqlSync("UPDATE sales_order_items SET pending_qty=? WHERE id=?",qty,line["id"].as<int64_t>());
   }else{
    if(!p){auto released=tx->execSqlSync("UPDATE inventory SET reserved_qty=reserved_qty-? WHERE product_id=? AND warehouse_id=? AND reserved_qty>=?",qty,pid,wid,qty);require(released.affectedRows()==1,"预占记录异常，请核对库存");}
    if(p)lotDetails[pid]["incoming_cost_value"]=money(qty*cents(line["price"].as<std::string>()));
    stock(tx,pid,wid,p?qty:-qty,p?"purchase_in":"sale_out",row["order_no"].as<std::string>(),user,lotDetails[pid]);
    tx->execSqlSync("UPDATE "+details(p)+" SET "+(p?"received_qty":"shipped_qty")+"=?"+(p?"":",pending_qty=0")+" WHERE id=?",done+qty,line["id"].as<int64_t>());
    total+=qty*cents(line["price"].as<std::string>());Json::Value item;item["product_id"]=Json::Int64(pid);item["quantity"]=Json::Int64(qty);item["unit_price"]=line["price"].as<std::string>();posted.append(item);
   }
  }
  require(selected.empty(),"商品不属于该单据");require(count>0,"本批数量不能全部为零");
  if(prepare){tx->execSqlSync("UPDATE sales_orders SET warehouse_id=?,status=5,warehouse_confirmed_by=?,logistics_company=?,logistics_no=? WHERE id=?",wid,user,company,tracking,id);event(tx,table(p),id,"prepare",user,before,5,remark);return response(id,5,"本批已备货并预占库存，待领导放行");}
  auto partner=row[p?"supplier_id":"customer_id"].as<int64_t>();auto party=tx->execSqlSync("SELECT name FROM "+std::string(p?"suppliers":"customers")+" WHERE id=? FOR UPDATE",partner);require(!party.empty(),"往来单位不存在");
  auto accounts=tx->execSqlSync("SELECT * FROM accounts WHERE biz_type=? AND biz_no=? FOR UPDATE",p?"purchase":"sale",row["order_no"].as<std::string>());
  int64_t accountId;
  if(accounts.empty())accountId=tx->execSqlSync("INSERT INTO accounts(account_type,biz_type,biz_no,partner_id,partner_name,amount,status) VALUES(?,?,?,?,?,?,?)",p?"payable":"receivable",p?"purchase":"sale",row["order_no"].as<std::string>(),partner,party[0]["name"].as<std::string>(),money(total),total==0?1:0).insertId();
  else {require(accounts.size()==1,"原单存在重复账目");auto account=accounts[0];accountId=account["id"].as<int64_t>();auto next=cents(account["amount"].as<std::string>())+total;tx->execSqlSync("UPDATE accounts SET amount=?,status=? WHERE id=?",money(next),next==cents(account["paid_amount"].as<std::string>())+cents(account["offset_amount"].as<std::string>())?1:0,account["id"].as<int64_t>());}
  auto delivery=number(p?"IN":"OUT");
  financePosting(tx,accountId,p?"purchase":"sale",delivery,total,0,0);
  auto header=tx->execSqlSync("INSERT INTO order_deliveries(delivery_no,order_type,order_id,warehouse_id,amount,created_by,approved_by) VALUES(?,?,?,?,?,?,?)",delivery,p?"purchase":"sales",id,wid,money(total),p?user:row["warehouse_confirmed_by"].as<int64_t>(),user);for(const auto &item:posted)tx->execSqlSync("INSERT INTO order_delivery_items(delivery_id,product_id,quantity,unit_price) VALUES(?,?,?,?)",header.insertId(),item["product_id"].asInt64(),item["quantity"].asInt64(),item["unit_price"].asString());
  int state=complete?3:1;tx->execSqlSync("UPDATE "+table(p)+" SET warehouse_id=?,status=?,"+(p?"warehouse_confirmed_by":"warehouse_approved_by")+"=? WHERE id=?",wid,state,user,id);event(tx,table(p),id,p?"receive":"ship",user,before,state,delivery+" "+remark);auto out=response(id,state,complete?"本批完成，订单已全部收发":"本批已过账，剩余数量待后续处理");out.data["delivery_no"]=delivery;return out;
 },request);

}
void resubmitOrder(bool p,int64_t id,const Json::Value &items,int64_t user,Callback cb) {
 transaction(std::move(cb),[=](const Tx &tx){
    actor(tx,user,dept(p));auto rows=tx->execSqlSync("SELECT *,updated_at>=DATE_SUB(NOW(),INTERVAL 7 DAY) AS within_window FROM "+table(p)+" WHERE id=? FOR UPDATE",id);require(!rows.empty(),"单据不存在",ErrorCode::NOT_FOUND);auto row=rows[0];
    require(!row["created_by"].isNull() && row["created_by"].as<int64_t>()==user,"只有发起人可以修改重提",ErrorCode::FORBIDDEN);auto before=row["status"].as<int>();require(before==2 || before==4 || before==6,"当前状态不可重提",ErrorCode::CONFLICT);require(row["within_window"].as<int>()==1,"已超过七天修改期限",ErrorCode::CONFLICT);
    tx->execSqlSync("DELETE FROM "+details(p)+" WHERE order_id=?",id);auto total=lines(tx,p,id,row[p?"supplier_id":"customer_id"].as<int64_t>(),items);
    const auto assignee = assigneeAfterResubmit(tx,row,dept(p),user,table(p),id,before);
    if(p)tx->execSqlSync("UPDATE purchase_orders SET status=0,total_amount=?,approved_by=NULL,warehouse_confirmed_by=NULL,assigned_approver_id=NULLIF(?,0) WHERE id=?",money(total),assignee,id);
    else tx->execSqlSync("UPDATE sales_orders SET status=0,total_amount=?,approved_by=NULL,warehouse_confirmed_by=NULL,warehouse_approved_by=NULL,logistics_company='',logistics_no='',assigned_approver_id=NULLIF(?,0) WHERE id=?",money(total),assignee,id);
    event(tx,table(p),id,"resubmit",user,before,0);return response(id,0,"已重新提交，等待审批");
 });
}
}
