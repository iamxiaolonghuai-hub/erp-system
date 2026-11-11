#pragma once
#include "BusinessSupport.h"
namespace erp::business {
void createReturn(bool purchase,int64_t orderId,int64_t warehouse,const std::string &remark,const std::string &company,const std::string &tracking,int64_t user,const std::string &type,const Json::Value &items,int64_t approver,Callback callback,const RequestIdentity &request);
void approveReturn(bool purchase,int64_t id,bool approve,int64_t user,Callback callback);
void warehouseReturn(bool purchase,int64_t id,bool approve,int64_t user,const std::string &remark,const std::string &company,const std::string &tracking,Callback callback,const Json::Value &serialItems=Json::Value());
}
