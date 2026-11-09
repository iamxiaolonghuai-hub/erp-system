#pragma once
#include <drogon/HttpController.h>
namespace erp {class BatchController:public drogon::HttpController<BatchController>{public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(BatchController::process,"/api/orders/batch",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 METHOD_LIST_END
 void process(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
};}
