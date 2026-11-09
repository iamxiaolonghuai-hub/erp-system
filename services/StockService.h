#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using StockCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 库存服务：出入库操作（自动登记流水，保证数据一致性）
class StockService
{
  public:
    // 入库：增加库存（采购入库、其他入库等）
    // productId 商品ID, warehouseId 仓库ID, qty 数量(正), bizType 业务类型, bizNo 业务单号, operatorId 操作人
    static void stockIn(int64_t productId,
                        int64_t warehouseId,
                        int qty,
                        const std::string &bizType,
                        const std::string &bizNo,
                        int64_t operatorId,
                        StockCallback &&cb);

    // 出库：扣减库存（销售出库等），校验可用量，不足则失败
    static void stockOut(int64_t productId,
                         int64_t warehouseId,
                         int qty,
                         const std::string &bizType,
                         const std::string &bizNo,
                         int64_t operatorId,
                         StockCallback &&cb);
};

}  // namespace erp
