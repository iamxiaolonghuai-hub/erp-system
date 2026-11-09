#pragma once
#include "BusinessSupport.h"
namespace erp::business {Json::Value moveCost(const Tx &tx,int64_t inventory,int64_t before,int64_t after,int64_t delta,const Json::Value &details);void validateUnitCost(const std::string &value);}
