#pragma once
#include "BusinessSupport.h"
namespace erp::business {
void createOrder(bool purchase,int64_t partner,int64_t warehouseId,const std::string &remark,int64_t user,const Json::Value &items,int64_t approver,Callback callback,const RequestIdentity &request);
void approveOrder(bool purchase,int64_t orderId,bool approve,int64_t user,Callback callback);
void processOrder(bool purchase,bool prepare,int64_t orderId,bool approve,int64_t user,int64_t requestedWarehouse,const std::string &remark,const std::string &logisticsCompany,const std::string &logisticsNo,Callback callback,const Json::Value &batch=Json::Value(),const RequestIdentity &request={});
void resubmitOrder(bool purchase,int64_t orderId,const Json::Value &items,int64_t user,Callback callback);
}
