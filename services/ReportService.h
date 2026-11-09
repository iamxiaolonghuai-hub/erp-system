#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using ReportCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 报表服务：利润统计、采购/销售汇总、库存概况
class ReportService
{
  public:
    // 综合看板：商品总数、库存总量、低库存数、采购总额、销售总额、毛利、应收/应付
    static void dashboard(ReportCallback &&cb);

    // 采购汇总：按供应商统计采购金额和次数
    static void purchaseSummary(ReportCallback &&cb);

    // 销售汇总：按客户统计销售金额和次数
    static void salesSummary(ReportCallback &&cb);

    // 库存概况：按仓库统计库存商品数和总量
    static void inventorySummary(ReportCallback &&cb);
};

}  // namespace erp
