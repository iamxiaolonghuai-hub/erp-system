#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using StockOpCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 库存操作服务：入库/出库/调拨（统一走审批）
// 状态机：0 待审批 → 1 已批准（执行库存变更+流水）/ 2 已驳回
class StockOperationService
{
  public:
    // 提交库存操作单（status=0 待审批）
    // type: in/out/transfer；from/to warehouse 按类型使用
    static void create(const std::string &type,
                       int64_t productId,
                       int64_t fromWarehouseId,
                       int64_t toWarehouseId,
                       int qty,
                       const std::string &bizType,
                       const std::string &bizNo,
                       const std::string &logisticsCompany,
                       const std::string &logisticsNo,
                       const std::string &serialNos,
                       const std::string &remark,
                       int64_t applicantId,
                       int64_t assignedApproverId,
                                  StockOpCallback &&cb, const business::RequestIdentity &request={},const Json::Value &lot=Json::Value());

    // 审批库存操作单（仓库领导）
    // approve=true 批准（执行库存变更）；false 驳回；remark 为驳回原因/备注
    static void approve(int64_t operationId,
                        bool approve,
                        int64_t approverId,
                        const std::string &remark,
                        StockOpCallback &&cb);

    // 库存操作单列表（可按 type/status 过滤；operatorId>0 时仅返回该用户经手：申请人/审批人）
    static void list(const std::string &type, int status,
                     int64_t operatorId, StockOpCallback &&cb);
};

}  // namespace erp
