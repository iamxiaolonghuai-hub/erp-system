#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 报表控制器：统计报表接口
class ReportController : public drogon::HttpController<ReportController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ReportController::dashboard, "/api/report/dashboard",
                  drogon::Get, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    ADD_METHOD_TO(ReportController::purchaseSummary, "/api/report/purchase",
                  drogon::Get, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    ADD_METHOD_TO(ReportController::salesSummary, "/api/report/sales",
                  drogon::Get, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    ADD_METHOD_TO(ReportController::inventorySummary, "/api/report/inventory",
                  drogon::Get, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    METHOD_LIST_END

    void dashboard(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void purchaseSummary(const drogon::HttpRequestPtr &req,
                         std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void salesSummary(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void inventorySummary(const drogon::HttpRequestPtr &req,
                          std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
