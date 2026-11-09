#include "CostService.h"
#include <regex>
namespace erp::business {
void validateUnitCost(const std::string &value){require(std::regex_match(value,std::regex("[0-9]{1,12}(\\.[0-9]{1,6})?")),"单位成本应为非负数，最多6位小数");}
Json::Value moveCost(const Tx &tx,int64_t id,int64_t before,int64_t after,int64_t delta,const Json::Value &details){
 auto rows=tx->execSqlSync("SELECT cost_initialized,cost_value FROM inventory WHERE id=? FOR UPDATE",id);bool known=rows[0]["cost_initialized"].as<int>()||before==0;Json::Value result;
 if(delta>0){
  if(!details.isMember("incoming_cost_value")){tx->execSqlSync("UPDATE inventory SET cost_initialized=0 WHERE id=?",id);return result;}
  auto value=details["incoming_cost_value"].asString();require(std::regex_match(value,std::regex("[0-9]{1,18}(\\.[0-9]{1,6})?")),"入库成本金额格式错误");
  if(known){tx->execSqlSync("UPDATE inventory SET cost_value=IF(?=0,0,cost_value)+CAST(? AS DECIMAL(26,6)),cost_initialized=1 WHERE id=?",before,value,id);result=value;}
 }else if(known){
  auto value=tx->execSqlSync("SELECT IF(?=0,cost_value,ROUND(cost_value*?/?,6)) AS value FROM inventory WHERE id=?",after,-delta,before,id)[0]["value"].as<std::string>();tx->execSqlSync("UPDATE inventory SET cost_value=IF(?=0,0,cost_value-CAST(? AS DECIMAL(26,6))),cost_initialized=1 WHERE id=?",after,value,id);result=value;
 }
 if(after==0)tx->execSqlSync("UPDATE inventory SET cost_value=0,cost_initialized=1 WHERE id=?",id);
 return result;
}
}
