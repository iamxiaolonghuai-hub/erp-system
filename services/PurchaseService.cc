#include "PurchaseService.h"
#include "OrderWorkflow.h"
namespace erp {
void PurchaseService::createPurchaseOrder(int64_t partner,int64_t warehouse,const std::string &remark,int64_t user,const Json::Value &items,int64_t approver,PurchaseCallback &&cb, const business::RequestIdentity &request) { business::createOrder(true,partner,warehouse,remark,user,items,approver,std::move(cb),request); }
void PurchaseService::approvePurchaseOrder(int64_t id,bool approve,int64_t user,int64_t warehouse,PurchaseCallback &&cb) { business::approveOrder(true,id,approve,user,std::move(cb)); }
void PurchaseService::warehouseInbound(int64_t id,bool approve,int64_t user,int64_t warehouse,const std::string &remark,PurchaseCallback &&cb) { business::processOrder(true,false,id,approve,user,warehouse,remark,"","",std::move(cb)); }
void PurchaseService::resubmitPurchaseOrder(int64_t id,const Json::Value &items,int64_t user,PurchaseCallback &&cb) { business::resubmitOrder(true,id,items,user,std::move(cb)); }
}
