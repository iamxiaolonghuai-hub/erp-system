#pragma once
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>
#include <json/json.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <cstdint>
#include "../utils/ApiResponse.h"

namespace erp::business {
using Tx = std::shared_ptr<drogon::orm::Transaction>;
using Callback = std::function<void(int,const std::string &,const Json::Value &)>;
struct Error : std::runtime_error {
    int code;
    Error(int c,const std::string &message):std::runtime_error(message),code(c){}
};
inline void require(bool ok,const std::string &message,int code=ErrorCode::BAD_REQUEST) { if(!ok) throw Error(code,message); }
struct Outcome { Json::Value data{Json::objectValue}; std::string message="操作成功"; };
struct RequestIdentity { int64_t user=0; std::string key,fingerprint; };
RequestIdentity requestIdentity(const drogon::HttpRequestPtr &request);
void transaction(Callback callback,const std::function<Outcome(const Tx &)> &work,const RequestIdentity &identity={});
int64_t cents(const std::string &value);
int64_t cents(double value);
std::string money(int64_t value);
std::string number(const std::string &prefix);
void actor(const Tx &tx,int64_t id,const std::string &department="",bool leader=false);
void warehouseAccess(const Tx &tx,int64_t id,int64_t warehouse,bool leader=false);
void warehouse(const Tx &tx,int64_t id);
void assigned(const Tx &tx,const std::string &department,int64_t applicant,int64_t approver);
void approval(const Tx &tx,const drogon::orm::Row &row,int64_t operatorId,const std::string &department,const std::string &creatorColumn);
Json::Value stock(const Tx &tx,int64_t product,int64_t warehouse,int64_t delta,const std::string &type,const std::string &no,int64_t operatorId,const Json::Value &lot=Json::Value());
void event(const Tx &tx,const std::string &type,int64_t id,const std::string &action,int64_t actor,int before,int after,const std::string &remark="");
}
