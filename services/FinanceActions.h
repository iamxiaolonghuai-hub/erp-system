#pragma once
#include "BusinessSupport.h"
namespace erp {
void submitFinance(const Json::Value &data,int64_t user,business::Callback callback,const business::RequestIdentity &request={});
void reviewFinance(int64_t id,bool approve,const std::string &remark,int64_t user,business::Callback callback);
}
