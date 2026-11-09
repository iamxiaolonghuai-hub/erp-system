#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 退货控制器：采购退货 + 销售退货（发起 → 领导审批 → 仓库审核）
class ReturnController : public drogon::HttpController<ReturnController>
{
  public:
    METHOD_LIST_BEGIN
    // ===== 采购退货 =====
    ADD_METHOD_TO(ReturnController::createPurchaseReturn,
                  "/api/purchase/return", drogon::Post,
                  "erp::JwtAuthMiddleware");
    ADD_METHOD_TO(ReturnController::approvePurchaseReturn,
                  "/api/purchase/return/approve", drogon::Post,
                  "erp::JwtAuthMiddleware", "erp::RoleAuthMiddleware");
    ADD_METHOD_TO(ReturnController::warehousePurchaseReturn,
                  "/api/purchase/return/warehouse", drogon::Post,
                  "erp::JwtAuthMiddleware", "erp::RoleAuthMiddleware");
    // ===== 销售退货 =====
    ADD_METHOD_TO(ReturnController::createSalesReturn, "/api/sales/return",
                  drogon::Post, "erp::JwtAuthMiddleware");
    ADD_METHOD_TO(ReturnController::approveSalesReturn,
                  "/api/sales/return/approve", drogon::Post,
                  "erp::JwtAuthMiddleware", "erp::RoleAuthMiddleware");
    ADD_METHOD_TO(ReturnController::warehouseSalesReturn,
                  "/api/sales/return/warehouse", drogon::Post,
                  "erp::JwtAuthMiddleware", "erp::RoleAuthMiddleware");
    // ===== 退货列表 =====
    ADD_METHOD_TO(ReturnController::listPurchaseReturns,
                  "/api/purchase/returns", drogon::Get,
                  "erp::JwtAuthMiddleware");
    ADD_METHOD_TO(ReturnController::listSalesReturns, "/api/sales/returns",
                  drogon::Get, "erp::JwtAuthMiddleware");
    ADD_METHOD_TO(ReturnController::listPurchaseReturnItems,
                  "/api/purchase/return/items", drogon::Get,
                  "erp::JwtAuthMiddleware");
    ADD_METHOD_TO(ReturnController::listSalesReturnItems,
                  "/api/sales/return/items", drogon::Get,
                  "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void createPurchaseReturn(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void approvePurchaseReturn(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void warehousePurchaseReturn(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void createSalesReturn(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void approveSalesReturn(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void warehouseSalesReturn(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);

    void listPurchaseReturns(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void listSalesReturns(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void listPurchaseReturnItems(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void listSalesReturnItems(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
