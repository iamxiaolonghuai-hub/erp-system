#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 财务控制器：收付款登记
class FinanceController : public drogon::HttpController<FinanceController>
{
  public:
    METHOD_LIST_BEGIN
    // 收付款登记
    ADD_METHOD_TO(FinanceController::addPayment, "/api/finance/payments",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    METHOD_LIST_END

    void addPayment(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
