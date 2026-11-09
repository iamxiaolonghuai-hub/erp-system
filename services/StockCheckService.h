#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using StockCheckCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 盘点服务：创建盘点单、完成盘点（差异调整库存）
class StockCheckService
{
  public:
    // 创建盘点单（进行中）
    // warehouseId 仓库ID, remark 备注, operatorId 操作人
    // items: [{product_id, book_qty, actual_qty}]
    // 逻辑：插入盘点单 -> 逐条插入盘点明细（计算 diff_qty）
    static void createStockCheck(int64_t warehouseId,
                                 const std::string &remark,
                                 int64_t operatorId,
                                 const Json::Value &items,
                                 StockCheckCallback &&cb, const business::RequestIdentity &request={});

    // 完成盘点：对每条有差异的明细调整库存，并更新盘点单状态
    static void completeStockCheck(int64_t checkId,
                                   int64_t operatorId,
                                   StockCheckCallback &&cb);
};

}  // namespace erp
