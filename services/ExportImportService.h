#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using ExportCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 导入导出服务：基础数据 CSV 导出/导入
class ExportImportService
{
  public:
    // 导出指定表为 CSV 文本
    // table: products / suppliers / customers / warehouses / categories
    //         purchase_orders / sales_orders（含明细）
    static void exportCsv(const std::string &table, ExportCallback &&cb);

    // 导入 CSV 文本到指定表
    // table: products / suppliers / customers / warehouses / categories
    static void importCsv(const std::string &table,
                          const std::string &csv,
                          ExportCallback &&cb,int64_t operatorId=0);
};

}  // namespace erp
