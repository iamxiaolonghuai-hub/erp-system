#include "SalesService.h"
#include "OrderWorkflow.h"
namespace erp {
void SalesService::createSalesOrder(int64_t partner,int64_t warehouse,const std::string &remark,int64_t user,const Json::Value &items,int64_t approver,SalesCallback &&cb, const business::RequestIdentity &request) { business::createOrder(false,partner,warehouse,remark,user,items,approver,std::move(cb),request); }
void SalesService::approveSalesOrder(int64_t id,bool approve,int64_t user,int64_t warehouse,SalesCallback &&cb) { business::approveOrder(false,id,approve,user,std::move(cb)); }
void SalesService::warehouseConfirm(int64_t id,bool approve,int64_t user,int64_t warehouse,const std::string &remark,const std::string &company,const std::string &tracking,SalesCallback &&cb) { business::processOrder(false,true,id,approve,user,warehouse,remark,company,tracking,std::move(cb)); }
void SalesService::warehouseOutbound(int64_t id,bool approve,int64_t user,int64_t warehouse,const std::string &remark,const std::string &company,const std::string &tracking,SalesCallback &&cb) { business::processOrder(false,false,id,approve,user,warehouse,remark,company,tracking,std::move(cb)); }
void SalesService::resubmitSalesOrder(int64_t id,const Json::Value &items,int64_t user,SalesCallback &&cb) { business::resubmitOrder(false,id,items,user,std::move(cb)); }
}
