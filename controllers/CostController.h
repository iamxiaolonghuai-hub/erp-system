#pragma once
#include <drogon/HttpController.h>
namespace erp {class CostController:public drogon::HttpController<CostController>{public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(CostController::opening,"/api/inventory/cost/opening",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 METHOD_LIST_END
 void opening(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
};}
