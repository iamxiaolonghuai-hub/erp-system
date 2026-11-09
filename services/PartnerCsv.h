#pragma once
#include "BusinessSupport.h"
namespace erp {
void importPartners(const std::string &table,const std::string &csv,int64_t user,business::Callback callback);
}
