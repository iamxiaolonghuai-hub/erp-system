#pragma once
#include "BusinessSupport.h"
namespace erp::business {
Json::Value changeLots(const Tx &tx,int64_t product,int64_t warehouse,int64_t delta,int64_t book,const std::string &type,const std::string &number,int64_t user,const Json::Value &details);
void validateLotDates(const std::string &manufactured,const std::string &expires,bool required);
Json::Value salesReturnDetails(const Tx &tx,int64_t order,const std::string &orderNo,int64_t product,int64_t quantity);
}
