#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 销售控制器：员工下单（待审批） + 领导审批（联动库存）
class SalesController : public drogon::HttpController<SalesController>
{
  public:
    METHOD_LIST_BEGIN
    // 创建销售单（员工下单，待审批）
    ADD_METHOD_TO(SalesController::create, "/api/sales/orders",
                  drogon::Post, "erp::JwtAuthMiddleware");
    // 审批销售单（领导操作，0→1 或 0→2）
    ADD_METHOD_TO(SalesController::approve, "/api/sales/approve",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 仓管员确认出库（仓库部，1→5 填物流 或 1→4）
    ADD_METHOD_TO(SalesController::warehouseConfirm, "/api/sales/confirm",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 仓库领导批准出库（仓库部领导，5→3 扣库存 或 5→4）
    ADD_METHOD_TO(SalesController::warehouseOutbound, "/api/sales/outbound",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 员工修改后重新提交（4→0）
    ADD_METHOD_TO(SalesController::resubmit, "/api/sales/resubmit",
                  drogon::Post, "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void approve(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void warehouseConfirm(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void warehouseOutbound(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void resubmit(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
