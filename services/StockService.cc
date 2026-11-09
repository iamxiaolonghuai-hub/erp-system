#include "StockService.h"
#include "BusinessSupport.h"
namespace erp {
void StockService::stockIn(int64_t product,int64_t warehouse,int qty,const std::string &type,const std::string &no,int64_t user,StockCallback &&cb) {
 business::transaction(std::move(cb),[=](const business::Tx &tx){business::require(qty>0,"数量必须为正数");business::warehouseAccess(tx,user,warehouse,true);business::Outcome out;out.data=business::stock(tx,product,warehouse,qty,type,no,user);return out;});
}
void StockService::stockOut(int64_t product,int64_t warehouse,int qty,const std::string &type,const std::string &no,int64_t user,StockCallback &&cb) {
 business::transaction(std::move(cb),[=](const business::Tx &tx){business::require(qty>0,"数量必须为正数");business::warehouseAccess(tx,user,warehouse,true);business::Outcome out;out.data=business::stock(tx,product,warehouse,-int64_t(qty),type,no,user);return out;});
}
}
