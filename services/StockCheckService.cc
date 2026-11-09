#include "StockCheckService.h"
#include "BusinessSupport.h"
#include <set>
#include <climits>
#include <vector>
#include <algorithm>
namespace erp {
void StockCheckService::createStockCheck(int64_t wid,const std::string &remark,int64_t user,const Json::Value &items,StockCheckCallback &&cb, const business::RequestIdentity &request){
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;warehouseAccess(tx,user,wid);require(items.isArray() && !items.empty() && items.size()<=500,"盘点明细应包含 1 至 500 项");
  auto no=number("CK");auto r=tx->execSqlSync("INSERT INTO stock_checks (check_no,warehouse_id,status,remark,created_by,counted_by) VALUES (?,?,0,?,?,?)",no,wid,remark,user,user);auto id=r.insertId();
  std::vector<Json::Value> sorted;for(const auto &item:items)sorted.push_back(item);std::sort(sorted.begin(),sorted.end(),[](const auto &a,const auto &b){return a["product_id"].asInt64()<b["product_id"].asInt64();});std::set<int64_t> seen;
  for(auto item:sorted){auto pid=item.get("product_id",0).asInt64();require(item["actual_qty"].isIntegral(),"实盘数量必须为整数");auto actual=item["actual_qty"].asInt64();require(pid>0 && actual>=0 && actual<=INT_MAX && seen.insert(pid).second,"商品重复或实盘数量错误");require(!tx->execSqlSync("SELECT id FROM products WHERE id=? AND status=1",pid).empty(),"商品不存在或已停用");
   tx->execSqlSync("INSERT INTO inventory (product_id,warehouse_id,quantity,min_quantity) VALUES (?,?,0,0) ON DUPLICATE KEY UPDATE id=id",pid,wid);
   auto stock=tx->execSqlSync("SELECT quantity FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);auto book=stock[0]["quantity"].as<int64_t>();
   require(item.get("book_qty",book).asInt64()==book,"库存已变化，请刷新后重新盘点",ErrorCode::CONFLICT);
   tx->execSqlSync("INSERT INTO stock_check_items (check_id,product_id,book_qty,actual_qty,diff_qty) VALUES (?,?,?,?,?)",id,pid,book,actual,actual-book);
  }
  event(tx,"stock_checks",id,"submit",user,-1,0,remark);Outcome out;out.data["check_id"]=Json::UInt64(id);out.data["check_no"]=no;out.message="盘点已提交，等待仓库领导复核";return out;
 },request);
}
void StockCheckService::completeStockCheck(int64_t id,int64_t user,StockCheckCallback &&cb){
 business::transaction(std::move(cb),[=](const business::Tx &tx){
  using namespace business;auto rows=tx->execSqlSync("SELECT * FROM stock_checks WHERE id=? FOR UPDATE",id);require(!rows.empty(),"盘点单不存在",ErrorCode::NOT_FOUND);auto row=rows[0];warehouseAccess(tx,user,row["warehouse_id"].as<int64_t>(),true);
  require(row["status"].as<int>()==0,"盘点已完成",ErrorCode::CONFLICT);require((row["created_by"].isNull() || row["created_by"].as<int64_t>()!=user) && (row["counted_by"].isNull() || row["counted_by"].as<int64_t>()!=user),"盘点人和复核人必须不同",ErrorCode::FORBIDDEN);
  auto lines=tx->execSqlSync("SELECT * FROM stock_check_items WHERE check_id=? ORDER BY product_id",id);require(!lines.empty(),"盘点无明细");int adjusted=0;auto wid=row["warehouse_id"].as<int64_t>();
  for(auto line:lines){auto pid=line["product_id"].as<int64_t>();auto current=tx->execSqlSync("SELECT quantity FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);
   require(!current.empty() && current[0]["quantity"].as<int64_t>()==line["book_qty"].as<int64_t>(),"盘点后发生出入库，请重新盘点，整单未调整",ErrorCode::CONFLICT);
   auto diff=line["diff_qty"].as<int64_t>();if(diff){stock(tx,pid,wid,diff,"stock_check",row["check_no"].as<std::string>(),user);++adjusted;}
  }
  tx->execSqlSync("UPDATE stock_checks SET status=1,reviewed_by=? WHERE id=?",user,id);event(tx,"stock_checks",id,"review",user,0,1);Outcome out;out.data["check_id"]=Json::Int64(id);out.data["adjust_count"]=adjusted;out.message="复核通过，所有盘点差异已一起调整";return out;
 });
}
}
