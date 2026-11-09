#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
// 采购服务回调：(code, message, data)
using PurchaseCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 采购服务：员工下单（待审批） + 领导审批（联动库存 + 应付账）
class PurchaseService
{
  public:
    // 创建采购单（员工下单，status=0 待审批，不联动库存）
    // supplierId 供应商ID, warehouseId 入库仓库ID, remark 备注, operatorId 操作人
    // items 为商品明细数组: [{product_id, quantity, price}]
    static void createPurchaseOrder(int64_t supplierId,
                                    int64_t warehouseId,
                                    const std::string &remark,
                                    int64_t operatorId,
                                    const Json::Value &items,
                                    int64_t assignedApproverId,
                                  PurchaseCallback &&cb, const business::RequestIdentity &request={});

    // 审批采购单（领导操作）
    // approve=true 批准（status: 0→1 待仓库入库）；false 驳回（status=2）
    static void approvePurchaseOrder(int64_t orderId,
                                     bool approve,
                                     int64_t approverId,
                                     int64_t warehouseId,
                                     PurchaseCallback &&cb);

    // 仓库入库审核（仓库部操作）
    // approve=true 入库确认（status: 1→3 联动库存+生成应付账）
    // approve=false 打回（status: 1→4 待修改）
    static void warehouseInbound(int64_t orderId,
                                 bool approve,
                                 int64_t operatorId,
                                 int64_t warehouseId,
                                 const std::string &remark,
                                 PurchaseCallback &&cb);

    // 员工修改后重新提交（status: 4→0）
    static void resubmitPurchaseOrder(int64_t orderId,
                                      const Json::Value &items,
                                      int64_t operatorId,
                                      PurchaseCallback &&cb);
};

}  // namespace erp
