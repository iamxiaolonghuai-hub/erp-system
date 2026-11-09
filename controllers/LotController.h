#pragma once
#include <drogon/HttpController.h>
namespace erp {class LotController:public drogon::HttpController<LotController>{public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(LotController::list,"/api/lots",drogon::Get,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(LotController::opening,"/api/lots/opening",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 METHOD_LIST_END
 void list(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void opening(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
};}
