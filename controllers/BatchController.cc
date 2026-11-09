#include "BatchController.h"
#include "../services/OrderWorkflow.h"
namespace erp {void BatchController::process(const drogon::HttpRequestPtr &req,std::function<void(const drogon::HttpResponsePtr &)> &&cb){
 auto json=req->getJsonObject();if(!json || !json->isObject()){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"参数错误"));return;}
 try{auto type=json->get("type","").asString(),step=json->get("step","").asString();business::require((type=="purchase" && step=="receive")||(type=="sales"&&(step=="prepare"||step=="release")),"操作类型错误");business::processOrder(type=="purchase",step=="prepare",json->get("order_id",0).asInt64(),true,std::stoll(req->getAttributes()->get<std::string>("user_id")),json->get("warehouse_id",0).asInt64(),json->get("remark","").asString(),json->get("logistics_company","").asString(),json->get("logistics_no","").asString(),[cb](int code,const std::string &msg,const Json::Value &data){cb(code==ErrorCode::OK?ApiResponse::ok(msg,data):ApiResponse::error(code,msg));},json->get("items",Json::Value()),business::requestIdentity(req));}
 catch(const std::exception &e){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));}
}}
