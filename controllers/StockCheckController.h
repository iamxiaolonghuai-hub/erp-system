#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 盘点控制器：创建盘点单、完成盘点
class StockCheckController : public drogon::HttpController<StockCheckController>
{
  public:
    METHOD_LIST_BEGIN
    // 创建盘点单
    ADD_METHOD_TO(StockCheckController::create, "/api/stockcheck/create",
                  drogon::Post, "erp::JwtAuthMiddleware");
    // 完成盘点（差异调整库存）
    ADD_METHOD_TO(StockCheckController::complete, "/api/stockcheck/complete",
                  drogon::Post, "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void complete(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
