#include "FinanceService.h"
#include "FinanceActions.h"
namespace erp {
void FinanceService::addPayment(int64_t id,double amount,const std::string &method,const std::string &date,const std::string &remark,int64_t user,FinanceCallback &&cb, const business::RequestIdentity &request) {
 Json::Value data;data["kind"]="payment";data["account_id"]=Json::Int64(id);data["amount"]=amount;data["pay_method"]=method;data["pay_date"]=date;data["remark"]=remark;
 submitFinance(data,user,std::move(cb),request);
}
}
