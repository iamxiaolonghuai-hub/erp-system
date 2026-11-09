#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
// 销售服务回调：(code, message, data)
using SalesCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 销售服务：员工下单（待审批） + 领导审批（联动库存 + 应收账）
class SalesService
{
  public:
    // 创建销售单（员工下单，status=0 待审批，不联动库存）
    static void createSalesOrder(int64_t customerId,
                                 int64_t warehouseId,
                                 const std::string &remark,
                                 int64_t operatorId,
                                 const Json::Value &items,
                                 int64_t assignedApproverId,
                                  SalesCallback &&cb, const business::RequestIdentity &request={});

    // 审批销售单（领导操作）
    // approve=true 批准（status: 0→1 待仓库出库）；false 驳回（status=2）
    static void approveSalesOrder(int64_t orderId,
                                  bool approve,
                                  int64_t approverId,
                                  int64_t warehouseId,
                                  SalesCallback &&cb);

    // 仓管员确认出库（仓库部操作）
    // approve=true 确认（status: 1→5 填写物流信息，不联动库存）
    // approve=false 打回（status: 1→4 待修改）
    static void warehouseConfirm(int64_t orderId,
                                 bool approve,
                                 int64_t operatorId,
                                 int64_t warehouseId,
                                 const std::string &remark,
                                 const std::string &logisticsCompany,
                                 const std::string &logisticsNo,
                                 SalesCallback &&cb);

    // 仓库领导批准出库（仓库部领导操作）
    // approve=true 批准（status: 5→3 联动库存+生成应收账）
    // approve=false 打回（status: 5→4 待修改）
    static void warehouseOutbound(int64_t orderId,
                                  bool approve,
                                  int64_t operatorId,
                                  int64_t warehouseId,
                                  const std::string &remark,
                                  const std::string &logisticsCompany,
                                  const std::string &logisticsNo,
                                  SalesCallback &&cb);

    // 员工修改后重新提交（status: 4→0）
    static void resubmitSalesOrder(int64_t orderId,
                                   const Json::Value &items,
                                   int64_t operatorId,
                                   SalesCallback &&cb);
};

}  // namespace erp
