#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 库存操作控制器：入库/出库/调拨（走审批）
class StockOperationController
    : public drogon::HttpController<StockOperationController>
{
  public:
    METHOD_LIST_BEGIN
    // 提交库存操作单（仓库员）
    ADD_METHOD_TO(StockOperationController::create, "/api/stock/operation",
                  drogon::Post, "erp::JwtAuthMiddleware");
    // 审批库存操作单（仓库领导）
    ADD_METHOD_TO(StockOperationController::approve,
                  "/api/stock/operation/approve", drogon::Post,
                  "erp::JwtAuthMiddleware", "erp::RoleAuthMiddleware");
    // 库存操作单列表
    ADD_METHOD_TO(StockOperationController::list, "/api/stock/operations",
                  drogon::Get, "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void approve(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void list(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
