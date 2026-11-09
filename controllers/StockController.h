#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 库存操作控制器：出入库（业务联动，自动登记流水）
class StockController : public drogon::HttpController<StockController>
{
  public:
    METHOD_LIST_BEGIN
    // 入库
    // 手工出入库统一提交库存操作单并审批，旧的直接过账入口停止注册。
    // 出库
    METHOD_LIST_END

    void stockIn(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void stockOut(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
