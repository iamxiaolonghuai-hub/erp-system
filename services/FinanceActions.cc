#include "FinanceActions.h"
#include "FinancePeriod.h"
#include <algorithm>
#include <regex>
namespace erp {
using namespace business;
namespace {
int64_t amount(const drogon::orm::Row &row,const std::string &column){return cents(row[column].as<std::string>());}
void date(const Tx &tx,const std::string &value){
 require(std::regex_match(value,std::regex("[0-9]{4}-[0-9]{2}-[0-9]{2}")),"请选择合法日期");
 int year=std::stoi(value.substr(0,4)),month=std::stoi(value.substr(5,2)),day=std::stoi(value.substr(8,2));
 const int days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
 require(year>=1900 && month>=1 && month<=12 && day>=1 && day<=days[month]+(month==2 && (year%400==0 || (year%4==0 && year%100!=0))),"日期无效");
}
void reviewer(const Tx &tx,int64_t id,int64_t user){
 if(!id)return;require(id!=user,"不能指定自己复核");
 require(!tx->execSqlSync("SELECT u.id FROM user_directory u WHERE u.id=? AND u.status=1 AND (u.department='财务部' OR EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='admin')) FOR UPDATE",id).empty(),"复核人必须是启用的财务人员或管理员");
}
void updateAccount(const Tx &tx,int64_t id,int64_t paid,int64_t offset,int64_t total){
 require((total>=0 && paid>=0 && offset>=0 && paid+offset<=total) || (total<0 && paid<=0 && offset<=0 && paid+offset>=total),"核销金额超出账目范围，请刷新后重试",ErrorCode::CONFLICT);
 tx->execSqlSync("UPDATE accounts SET paid_amount=?,offset_amount=?,status=? WHERE id=?",money(paid),money(offset),paid+offset==total?1:0,id);
}
// All movements for one partner/type serialize, including refunds and reversals.
void partnerLock(const Tx &tx,const drogon::orm::Row &account){
 auto table=account["account_type"].as<std::string>()=="receivable"?"customers":"suppliers";
 require(!tx->execSqlSync("SELECT id FROM "+std::string(table)+" WHERE id=? FOR UPDATE",account["partner_id"].as<int64_t>()).empty(),"往来单位不存在");
}
void cashAvailable(const Tx &tx,const drogon::orm::Row &account,int64_t reduction){
 // Locking reads use current committed values, not the transaction's earlier snapshot.
 auto rows=tx->execSqlSync("SELECT paid_amount FROM accounts WHERE partner_id=? AND account_type=? ORDER BY id FOR UPDATE",account["partner_id"].as<int64_t>(),account["account_type"].as<std::string>());
 int64_t net=0;for(auto row:rows)net+=amount(row,"paid_amount");
 require(net>=reduction,"该往来单位可退现金不足，未收付款部分请使用抵扣；已退款时不能直接冲正原收付款");
}
}
void submitFinance(const Json::Value &data,int64_t user,Callback callback,const RequestIdentity &request){
 transaction(std::move(callback),[=](const Tx &tx){
  actor(tx,user,"财务部");auto kind=data.get("kind","payment").asString();require(kind=="payment" || kind=="refund" || kind=="offset" || kind=="reverse","不支持的财务操作");
  auto id=data.get("account_id",0).asInt64(),target=data.get("target_account_id",0).asInt64(),payment=data.get("payment_id",0).asInt64(),source=data.get("source_action_id",0).asInt64(),assigned=data.get("assigned_approver_id",0).asInt64();
  auto remark=data.get("remark","").asString(),method=data.get("pay_method","").asString(),when=data.get("pay_date","").asString();
  require(remark.size()<=255 && method.size()<=32,"备注或支付方式过长");require(!remark.empty() || kind=="payment","退款、抵扣和冲正需要填写原因");date(tx,when);lockFinancePosting(tx,when);reviewer(tx,assigned,user);
  int64_t value=0;
  if(kind=="reverse"){
   require((payment>0)!=(source>0),"请选择一条收付款记录或抵扣记录进行冲正");
   if(payment){auto rows=tx->execSqlSync("SELECT * FROM payments WHERE id=?",payment);require(!rows.empty() && rows[0]["reversal_of"].isNull() && rows[0]["reversed_by"].isNull(),"收付款记录不存在或已经冲正");id=rows[0]["account_id"].as<int64_t>();value=amount(rows[0],"amount");}
   else {auto rows=tx->execSqlSync("SELECT * FROM finance_actions WHERE id=?",source);require(!rows.empty() && rows[0]["kind"].as<std::string>()=="offset" && rows[0]["status"].as<int>()==1 && rows[0]["reversed_by"].isNull(),"请选择已批准且未冲正的抵扣");id=rows[0]["account_id"].as<int64_t>();target=rows[0]["target_account_id"].as<int64_t>();value=amount(rows[0],"amount");}
  } else {value=cents(data.get("amount",0).asDouble());require(value>0,"金额必须大于零");}
  auto rows=tx->execSqlSync("SELECT * FROM accounts WHERE id=?",id);require(!rows.empty(),"账目不存在");auto account=rows[0];auto total=amount(account,"amount"),remaining=total-amount(account,"paid_amount")-amount(account,"offset_amount");
  if(kind=="payment")require(total>0 && remaining>=value,"请选择未结清的正金额账目，且金额不能超过余额");
  if(kind=="refund" || kind=="offset")require(total<0 && -remaining>=value,"请选择未结清的退货贷项，且金额不能超过可用贷项");
  if(kind=="offset"){
   auto targets=tx->execSqlSync("SELECT * FROM accounts WHERE id=?",target);require(!targets.empty(),"抵扣目标不存在");auto other=targets[0];
   require(target!=id && other["account_type"].as<std::string>()==account["account_type"].as<std::string>() && other["partner_id"].as<int64_t>()==account["partner_id"].as<int64_t>() && amount(other,"amount")>0,"只能抵扣同一往来单位、同一应收应付类型的正金额账目");
   require(amount(other,"amount")-amount(other,"paid_amount")-amount(other,"offset_amount")>=value,"抵扣金额超过目标未结余额");
  }else if(kind!="reverse")require(!target && !payment && !source,"该操作不能关联其他记录");
  auto result=tx->execSqlSync("INSERT INTO finance_actions (action_no,kind,account_id,target_account_id,payment_id,source_action_id,amount,pay_method,pay_date,remark,created_by,assigned_approver_id) VALUES (?,?,?,NULLIF(?,0),NULLIF(?,0),NULLIF(?,0),?,?,?,?,?,NULLIF(?,0))",number("FIN"),kind,id,target,payment,source,money(value),method,when,remark,user,assigned);
  event(tx,"finance_actions",result.insertId(),"submit",user,-1,0,remark);Outcome out;out.data["action_id"]=Json::UInt64(result.insertId());out.data["status"]=0;out.message="已提交财务复核，复核通过后生效";return out;
 },request);
}
void reviewFinance(int64_t id,bool approve,const std::string &remark,int64_t user,Callback callback){
 transaction(std::move(callback),[=](const Tx &tx){
  if (approve) lockFinancePosting(tx);
  actor(tx,user,"财务部");require(remark.size()<=255,"复核备注过长");auto rows=tx->execSqlSync("SELECT * FROM finance_actions WHERE id=? FOR UPDATE",id);require(!rows.empty(),"财务申请不存在");auto action=rows[0];require(action["status"].as<int>()==0,"申请已处理",ErrorCode::CONFLICT);require(action["created_by"].as<int64_t>()!=user,"经办人与复核人必须不同",ErrorCode::FORBIDDEN);
  auto admin=tx->execSqlSync("SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=? AND r.code='admin'",user);
  require(!admin.empty() || action["assigned_approver_id"].isNull() || action["assigned_approver_id"].as<int64_t>()==user,"该申请已指定其他复核人",ErrorCode::FORBIDDEN);
  auto aid=action["account_id"].as<int64_t>(),target=action["target_account_id"].isNull()?0:action["target_account_id"].as<int64_t>();auto kind=action["kind"].as<std::string>();int64_t paymentId=0;
  if(approve){
   lockFinancePosting(tx,action["pay_date"].as<std::string>());
   const auto actionNo = action["action_no"].as<std::string>();
   auto preliminary=tx->execSqlSync("SELECT * FROM accounts WHERE id=?",aid);require(!preliminary.empty(),"账目不存在");partnerLock(tx,preliminary[0]);
   // Lock all this partner's accounts in a stable order to avoid refund/offset races.
   auto accounts=tx->execSqlSync("SELECT * FROM accounts WHERE partner_id=? AND account_type=? ORDER BY id FOR UPDATE",preliminary[0]["partner_id"].as<int64_t>(),preliminary[0]["account_type"].as<std::string>());
   auto find=[&](int64_t accountId)->drogon::orm::Row{for(auto row:accounts)if(row["id"].as<int64_t>()==accountId)return row;throw Error(ErrorCode::BAD_REQUEST,"目标账目不属于同一往来单位和应收应付类型");};
   auto account=find(aid);auto total=amount(account,"amount"),paid=amount(account,"paid_amount"),offset=amount(account,"offset_amount"),value=amount(action,"amount");
   int64_t delta=0,cash=0;std::string direction;
   if(kind=="offset"){
    auto other=find(target);require(total<0 && amount(other,"amount")>0,"抵扣方向错误");
    updateAccount(tx,aid,paid,offset-value,total);updateAccount(tx,target,amount(other,"paid_amount"),amount(other,"offset_amount")+value,amount(other,"amount"));
    financePosting(tx,aid,kind,actionNo,0,0,-value);
    financePosting(tx,target,kind,actionNo,0,0,value);
   }else if(kind=="reverse" && !action["source_action_id"].isNull()){
    auto sourceId=action["source_action_id"].as<int64_t>();auto originals=tx->execSqlSync("SELECT * FROM finance_actions WHERE id=? FOR UPDATE",sourceId);require(!originals.empty(),"原抵扣不存在");auto original=originals[0];
    require(original["kind"].as<std::string>()=="offset" && original["status"].as<int>()==1 && original["reversed_by"].isNull() && original["account_id"].as<int64_t>()==aid && original["target_account_id"].as<int64_t>()==target && amount(original,"amount")==value,"原抵扣已冲正或发生变化",ErrorCode::CONFLICT);
    auto other=find(target);updateAccount(tx,aid,paid,offset+value,total);updateAccount(tx,target,amount(other,"paid_amount"),amount(other,"offset_amount")-value,amount(other,"amount"));
    financePosting(tx,aid,kind,actionNo,0,0,value);
    financePosting(tx,target,kind,actionNo,0,0,-value);
    tx->execSqlSync("UPDATE finance_actions SET reversed_by=? WHERE id=?",id,sourceId);
   }else{
    if(kind=="payment"){require(total>0,"正向收付款只能用于正金额账目");delta=value;cash=value;direction=account["account_type"].as<std::string>()=="receivable"?"receive":"pay";}
    else if(kind=="refund"){require(total<0,"退款只能用于退货贷项");cashAvailable(tx,account,value);delta=-value;cash=value;direction=account["account_type"].as<std::string>()=="receivable"?"pay":"receive";}
    else {
     auto originalId=action["payment_id"].as<int64_t>();auto originals=tx->execSqlSync("SELECT * FROM payments WHERE id=? FOR UPDATE",originalId);require(!originals.empty(),"原收付款不存在");auto original=originals[0];
     require(original["account_id"].as<int64_t>()==aid && original["reversal_of"].isNull() && original["reversed_by"].isNull() && amount(original,"amount")==value,"原收付款已冲正或发生变化",ErrorCode::CONFLICT);
     auto settled=original["settlement_amount"].isNull()?(total<0?-value:value):amount(original,"settlement_amount");delta=-settled;cash=-value;direction=original["payment_type"].as<std::string>();
     if(delta<0)cashAvailable(tx,account,-delta);
    }
    updateAccount(tx,aid,paid+delta,offset,total);
    financePosting(tx,aid,kind,actionNo,0,delta,0);
    auto inserted=tx->execSqlSync("INSERT INTO payments (payment_no,account_id,payment_type,amount,pay_method,pay_date,remark,created_by,approved_by,action_id,reversal_of,settlement_amount) VALUES (?,?,?,?,?,?,?,?,?,?,NULLIF(?,0),?)",number(kind=="reverse"?"REV":"PAY"),aid,direction,money(cash),action["pay_method"].as<std::string>(),action["pay_date"].as<std::string>(),action["remark"].as<std::string>(),action["created_by"].as<int64_t>(),user,id,kind=="reverse"?action["payment_id"].as<int64_t>():0,money(delta));paymentId=inserted.insertId();
    if(kind=="reverse")tx->execSqlSync("UPDATE payments SET reversed_by=? WHERE id=?",paymentId,action["payment_id"].as<int64_t>());
    event(tx,"payments",paymentId,kind,user,-1,1,remark);
   }
  }
  tx->execSqlSync("UPDATE finance_actions SET status=?,approved_by=?,approved_at=NOW(),review_remark=? WHERE id=?",approve?1:2,user,remark,id);
  event(tx,"finance_actions",id,approve?"approve":"reject",user,0,approve?1:2,remark);Outcome out;out.data["action_id"]=Json::Int64(id);out.data["payment_id"]=Json::Int64(paymentId);out.message=approve?"复核通过，账目及相关记录已一起生效":"已驳回，账目未改变";return out;
 });
}
}
