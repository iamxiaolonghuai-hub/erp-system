#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 采购控制器：员工下单（待审批） + 领导审批（联动库存）
class PurchaseController : public drogon::HttpController<PurchaseController>
{
  public:
    METHOD_LIST_BEGIN
    // 创建采购单（员工下单，待审批）
    ADD_METHOD_TO(PurchaseController::create, "/api/purchase/orders",
                  drogon::Post, "erp::JwtAuthMiddleware");
    // 审批采购单（领导操作，0→1 或 0→2）
    ADD_METHOD_TO(PurchaseController::approve, "/api/purchase/approve",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 仓库入库审核（仓库部，1→3 或 1→4）
    ADD_METHOD_TO(PurchaseController::warehouseInbound, "/api/purchase/inbound",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 员工修改后重新提交（4→0）
    ADD_METHOD_TO(PurchaseController::resubmit, "/api/purchase/resubmit",
                  drogon::Post, "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void approve(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void warehouseInbound(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void resubmit(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
