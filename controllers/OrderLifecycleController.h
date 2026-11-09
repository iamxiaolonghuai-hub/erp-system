#pragma once
#include <drogon/HttpController.h>
namespace erp {
class OrderLifecycleController:public drogon::HttpController<OrderLifecycleController>{
public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(OrderLifecycleController::change,"/api/orders/lifecycle",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 METHOD_LIST_END
 void change(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
};
}
