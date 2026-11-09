#pragma once
#include <drogon/HttpController.h>
namespace erp {
class FinanceActionController:public drogon::HttpController<FinanceActionController>{
public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(FinanceActionController::advice,"/api/finance/settlement-advice",drogon::Get,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(FinanceActionController::list,"/api/finance/actions",drogon::Get,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(FinanceActionController::submit,"/api/finance/actions",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(FinanceActionController::review,"/api/finance/actions/review",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(FinanceActionController::reviewers,"/api/finance/reviewers",drogon::Get,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 METHOD_LIST_END
 void list(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void advice(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void submit(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void review(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void reviewers(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
};
}
