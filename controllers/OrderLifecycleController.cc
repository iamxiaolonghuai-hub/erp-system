#include "OrderLifecycleController.h"
#include "../services/BusinessSupport.h"
namespace erp {
void OrderLifecycleController::change(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb){
 auto data=req->getJsonObject();if(!data || !data->isObject()){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"请提交合法参数"));return;}
 auto user=std::stoll(req->getAttributes()->get<std::string>("user_id"));
 business::transaction([cb](int code,const std::string &msg,const Json::Value &value){cb(code==ErrorCode::OK?ApiResponse::ok(msg,value):ApiResponse::error(code,msg));},[=](const business::Tx &tx){
  using namespace business;auto kind=data->get("type","").asString(),action=data->get("action","").asString(),reason=data->get("reason","").asString();
  require(kind=="purchase" || kind=="sales","单据类型错误");require(action=="withdraw" || action=="void" || action=="close","操作类型错误");require(!reason.empty() && reason.size()<=255,"请填写原因，最多255字节");
  auto table=kind=="purchase"?"purchase_orders":"sales_orders";auto detail=kind=="purchase"?"purchase_order_items":"sales_order_items";auto qty=kind=="purchase"?"received_qty":"shipped_qty";auto id=data->get("order_id",0).asInt64();
  actor(tx,user,kind=="purchase"?"采购部":"销售部");auto rows=tx->execSqlSync("SELECT * FROM "+std::string(table)+" WHERE id=? FOR UPDATE",id);require(!rows.empty(),"单据不存在");auto row=rows[0];auto state=row["status"].as<int>();
  if(action=="withdraw")require(!row["created_by"].isNull() && row["created_by"].as<int64_t>()==user && state==0,"仅发起人可撤回待审批单据",ErrorCode::FORBIDDEN);
  else {actor(tx,user,kind=="purchase"?"采购部":"销售部",true);require(state==0 || state==1 || state==2 || state==4 || state==5 || state==6,"当前状态不可作废或关闭");}
  if(action=="close"){
   require(state==1 || state==5,"仅未全部收发的已批准订单可关闭剩余数量");
   require(!tx->execSqlSync("SELECT id FROM "+std::string(detail)+" WHERE order_id=? AND "+qty+">0",id).empty(),"未发生收发请作废单据");
   require(!tx->execSqlSync("SELECT id FROM "+std::string(detail)+" WHERE order_id=? AND "+qty+"<quantity",id).empty(),"订单没有剩余数量");
  }else{
   require(tx->execSqlSync("SELECT id FROM "+std::string(detail)+" WHERE order_id=? AND "+qty+">0",id).empty(),"已有实物收发，不能撤回或作废，请走退货流程");
   require(tx->execSqlSync("SELECT id FROM accounts WHERE biz_no=?",row["order_no"].as<std::string>()).empty(),"已有账务记录，不能直接作废");
  }
  if(kind=="sales"){auto lines=tx->execSqlSync("SELECT product_id,pending_qty FROM sales_order_items WHERE order_id=? ORDER BY product_id FOR UPDATE",id);for(auto line:lines){auto q=line["pending_qty"].as<int64_t>();if(q){auto r=tx->execSqlSync("UPDATE inventory SET reserved_qty=reserved_qty-? WHERE product_id=? AND warehouse_id=? AND reserved_qty>=?",q,line["product_id"].as<int64_t>(),row["warehouse_id"].as<int64_t>(),q);require(r.affectedRows()==1,"预占库存异常");}}tx->execSqlSync("UPDATE sales_order_items SET pending_qty=0 WHERE order_id=?",id);}
  int next=action=="withdraw"?6:action=="close"?8:7;tx->execSqlSync("UPDATE "+std::string(table)+" SET status=? WHERE id=?",next,id);event(tx,table,id,action,user,state,next,reason);Outcome out;out.data["status"]=next;out.message=next==6?"已撤回，可修改重提":next==8?"已关闭剩余数量，保留已过账库存和账务":"已作废，保留原单和操作轨迹";return out;
 },business::requestIdentity(req));
}
}
