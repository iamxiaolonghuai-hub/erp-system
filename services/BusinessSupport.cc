#include "BusinessSupport.h"
#include "SerialInventory.h"
#include "LotService.h"
#include "CostService.h"
#include <atomic>
#include <cmath>
#include <climits>
#include <iomanip>
#include <sstream>
#include <drogon/utils/Utilities.h>
#include <openssl/sha.h>
namespace erp::business {
RequestIdentity requestIdentity(const drogon::HttpRequestPtr &req) {
    RequestIdentity identity;identity.user=std::stoll(req->getAttributes()->get<std::string>("user_id"));
    identity.key=req->getHeader("Idempotency-Key");auto json=req->getJsonObject();
    if(identity.key.empty() && json && (*json).isMember("request_id") && (*json)["request_id"].isString())identity.key=(*json)["request_id"].asString();
    auto content=req->getPath()+":"+std::string(req->getBody());unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(reinterpret_cast<const unsigned char *>(content.data()),content.size(),hash);
    std::ostringstream out;for(auto byte:hash)out<<std::hex<<std::setw(2)<<std::setfill('0')<<int(byte);identity.fingerprint=out.str();return identity;
}
void transaction(Callback callback,const std::function<Outcome(const Tx &)> &work,const RequestIdentity &identity) {
    auto result=std::make_shared<Outcome>();
    auto replied=std::make_shared<std::atomic_bool>(false);
    auto reply=[callback=std::move(callback),replied](int code,const std::string &msg,const Json::Value &data) { if(!replied->exchange(true)) callback(code,msg,data); };
    Tx tx;
    try {
        tx=drogon::app().getDbClient("default")->newTransaction([result,reply](bool ok){
            if(ok) reply(ErrorCode::OK,result->message,result->data);
            else reply(ErrorCode::INTERNAL_ERROR,"事务提交失败，请刷新单据状态后重试",Json::Value());
        });
        require(bool(tx),"数据库暂不可用",ErrorCode::INTERNAL_ERROR);
        if(!identity.key.empty()) {
            require(identity.key.size()>=16 && identity.key.size()<=128 && identity.key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")==std::string::npos,"请求标识格式错误");
            tx->execSqlSync("INSERT INTO business_requests (user_id,request_key,fingerprint) VALUES (?,?,?) ON DUPLICATE KEY UPDATE request_key=request_key",identity.user,identity.key,identity.fingerprint);
            auto rows=tx->execSqlSync("SELECT fingerprint,response FROM business_requests WHERE user_id=? AND request_key=? FOR UPDATE",identity.user,identity.key);
            require(rows[0]["fingerprint"].as<std::string>()==identity.fingerprint,"同一请求标识不能用于不同内容，请重新发起",ErrorCode::CONFLICT);
            if(!rows[0]["response"].isNull()) {
                Json::Value saved;Json::Reader reader;require(reader.parse(rows[0]["response"].as<std::string>(),saved),"请求记录无法读取",ErrorCode::INTERNAL_ERROR);
                result->data=saved["data"];result->message=saved["message"].asString();tx.reset();return;
            }
        }
        *result=work(tx);
        if(!identity.key.empty()) {
            Json::Value saved;saved["message"]=result->message;saved["data"]=result->data;Json::StreamWriterBuilder writer;
            tx->execSqlSync("UPDATE business_requests SET response=? WHERE user_id=? AND request_key=?",Json::writeString(writer,saved),identity.user,identity.key);
        }
        tx.reset(); // Drogon commits when the final transaction owner releases it.
    } catch(const Error &e) {
        if(tx) tx->rollback();
        tx.reset(); reply(e.code,e.what(),Json::Value());
    } catch(const std::exception &e) {
        if(tx) tx->rollback();
        tx.reset(); LOG_ERROR<<"Business transaction rolled back: "<<e.what();
        reply(ErrorCode::INTERNAL_ERROR,"操作未完成，数据库变更已回滚，请刷新后重试",Json::Value());
    }
}
int64_t cents(const std::string &input) {
    require(!input.empty(),"金额不能为空");
    size_t pos=0; bool negative=input[0]=='-'; if(negative) ++pos;
    require(pos<input.size(),"金额格式错误");
    int64_t whole=0; size_t digits=0;
    while(pos<input.size() && input[pos]>='0' && input[pos]<='9') {
        require(whole<1000000000000LL,"金额超出范围"); whole=whole*10+(input[pos++]-'0');++digits;
    }
    require(digits>0,"金额格式错误"); int64_t fraction=0;
    if(pos<input.size() && input[pos]=='.') {
        ++pos; int decimals=0;
        while(pos<input.size() && input[pos]>='0' && input[pos]<='9') {
            require(decimals<2,"金额最多两位小数"); fraction=fraction*10+(input[pos++]-'0');++decimals;
        }
        if(decimals==1) fraction*=10;
    }
    require(pos==input.size() && whole<=999999999999LL,"金额格式或范围错误");
    return (whole*100+fraction)*(negative?-1:1);
}
int64_t cents(double value) {
    require(std::isfinite(value) && std::abs(value)<=999999999999.99,"金额格式或范围错误");
    auto v=std::llround(value*100);
    require(std::abs(value*100-v)<0.0001,"金额最多两位小数"); return v;
}
std::string money(int64_t value) {
    bool negative=value<0; if(negative)value=-value;
    std::ostringstream out; if(negative)out<<'-';out<<value/100<<'.'<<std::setw(2)<<std::setfill('0')<<value%100;return out.str();
}
std::string number(const std::string &prefix) { return prefix+drogon::utils::getUuid(); }
void actor(const Tx &tx,int64_t id,const std::string &department,bool leader) {
    auto users=tx->execSqlSync("SELECT u.status,u.department,EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='admin') AS admin,EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='leader') AS leader FROM user_directory u WHERE u.id=?",id);
    require(!users.empty() && users[0]["status"].as<int>()==1,"账号已停用或不存在",ErrorCode::FORBIDDEN);
    if(users[0]["admin"].as<int>())return;
    require((department.empty() || users[0]["department"].as<std::string>()==department) && (!leader || users[0]["leader"].as<int>()),"无该业务的操作权限",ErrorCode::FORBIDDEN);
}
void warehouse(const Tx &tx,int64_t id) { require(id>0 && !tx->execSqlSync("SELECT id FROM warehouses WHERE id=? AND status=1",id).empty(),"仓库不存在或已停用"); }
void warehouseAccess(const Tx &tx,int64_t id,int64_t wid,bool leader) {
    actor(tx,id,"仓库部",leader); warehouse(tx,wid);
    auto allowed=tx->execSqlSync("SELECT u.id FROM user_directory u WHERE u.id=? AND (u.warehouse_id=? OR EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code IN ('admin','leader')))",id,wid);
    require(!allowed.empty(),"无权处理该仓库单据",ErrorCode::FORBIDDEN);
}
void assigned(const Tx &tx,const std::string &dept,int64_t applicant,int64_t approver) {
    if(!approver)return;
    require(approver!=applicant,"不能指定自己审批");
    auto rows=tx->execSqlSync("SELECT u.id FROM user_directory u JOIN user_roles ur ON ur.user_id=u.id JOIN roles r ON r.id=ur.role_id WHERE u.id=? AND u.status=1 AND u.department=? AND r.code='leader' FOR UPDATE",approver,dept);
    require(!rows.empty(),"指定审批人不属于该部门或已停用");
}
void approval(const Tx &tx,const drogon::orm::Row &row,int64_t id,const std::string &dept,const std::string &creator) {
    actor(tx,id,dept,true);
    require(row[creator].isNull() || row[creator].as<int64_t>()!=id,"不能审批自己发起的单据",ErrorCode::FORBIDDEN);
    auto admin=tx->execSqlSync("SELECT ur.user_id FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=? AND r.code='admin'",id);
    require(!admin.empty() || row["assigned_approver_id"].isNull() || row["assigned_approver_id"].as<int64_t>()==id,"该单据已指定其他领导审批",ErrorCode::FORBIDDEN);
}
Json::Value stock(const Tx &tx,int64_t pid,int64_t wid,int64_t delta,const std::string &type,const std::string &no,int64_t id,const Json::Value &lot) {
    require(pid>0 && wid>0 && delta && delta>=-INT_MAX && delta<=INT_MAX,"库存变更参数错误");
    const auto product = tx->execSqlSync("SELECT has_serial,batch_enabled FROM products WHERE id=? FOR SHARE",pid);
    require(!product.empty(),"商品不存在");
    // Upsert obtains the unique product/warehouse row lock even for a new inventory row.
    tx->execSqlSync("INSERT INTO inventory (product_id,warehouse_id,quantity,min_quantity) VALUES (?,?,0,0) ON DUPLICATE KEY UPDATE id=id",pid,wid);
    auto rows=tx->execSqlSync("SELECT id,quantity,reserved_qty FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);
    int64_t before=rows[0]["quantity"].as<int64_t>(),after=before+delta;
    require(after>=rows[0]["reserved_qty"].as<int64_t>(),"可用库存不足，不能占用已备货库存",ErrorCode::STOCK_NOT_ENOUGH);
    require(after<=INT_MAX,"库存数量超出范围");
    Json::Value details = lot.isObject() ? lot : Json::Value(Json::objectValue);
    if (delta < 0 || (product[0]["has_serial"].as<int>() && type!="transfer_in" && type!="sales_return"))
        details.removeMember("lot_allocations");
    if (product[0]["has_serial"].as<int>())
        changeSerialStock(tx,pid,wid,delta,before,type,no,id,product[0]["batch_enabled"].as<int>()!=0,details);
    auto allocations=changeLots(tx,pid,wid,delta,before,type,no,id,details);
    tx->execSqlSync("UPDATE inventory SET quantity=? WHERE id=?",after,rows[0]["id"].as<int64_t>());
    auto cost=moveCost(tx,rows[0]["id"].as<int64_t>(),before,after,delta,lot);
    auto r=tx->execSqlSync("INSERT INTO stock_records (product_id,warehouse_id,change_type,change_qty,before_qty,after_qty,biz_type,biz_no,operator_id) VALUES (?,?,?,?,?,?,?,?,?)",pid,wid,delta>0?"in":"out",delta,before,after,type,no,id);
    if(!cost.isNull())tx->execSqlSync("UPDATE stock_records SET cost_value=? WHERE id=?",cost.asString(),r.insertId());
    Json::Value data;data["lot_allocations"]=allocations;data["cost_value"]=cost;data["after_qty"]=Json::Int64(after);data["record_id"]=Json::UInt64(r.insertId());return data;
}
void event(const Tx &tx,const std::string &type,int64_t id,const std::string &action,int64_t uid,int before,int after,const std::string &remark) {
    tx->execSqlSync("INSERT INTO business_events (document_type,document_id,action,operator_id,before_status,after_status,remark) VALUES (?,?,?,?,?,?,?)",type,id,action,uid,before,after,remark);
}
}
