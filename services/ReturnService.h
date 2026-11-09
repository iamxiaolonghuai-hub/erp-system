#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using ReturnCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 退货服务：采购退货 + 销售退货（发起退货 → 领导审批 → 仓库审核退货）
// 状态机：0 待审批 → 1 待仓库审核 → 3 已完成；2 驳回；4 打回
class ReturnService
{
  public:
    // ===== 采购退货 =====
    // 发起采购退货（关联已完成采购单，status=0 待审批）
    static void createPurchaseReturn(int64_t orderId,
                                     int64_t warehouseId,
                                     const std::string &remark,
                                     const std::string &logisticsCompany,
                                     const std::string &logisticsNo,
                                     int64_t operatorId,
                                     const Json::Value &items,
                                     int64_t assignedApproverId,
                                  ReturnCallback &&cb, const business::RequestIdentity &request={});

    // 审批采购退货（领导，0→1 或 0→2）
    static void approvePurchaseReturn(int64_t returnId,
                                      bool approve,
                                      int64_t approverId,
                                      ReturnCallback &&cb);

    // 仓库审核采购退货（1→3 联动库存减少+冲减应付账；1→4 打回）
    static void warehousePurchaseReturn(int64_t returnId,
                                        bool approve,
                                        int64_t operatorId,
                                        const std::string &remark,
                                        const std::string &logisticsCompany,
                                        const std::string &logisticsNo,
                                        ReturnCallback &&cb);

    // ===== 销售退货 / 换货 =====
    // returnType: 'return'退货 / 'exchange'换货
    // items: 明细数组，每项可带 direction（return退入 / replace换出，默认 return）
    static void createSalesReturn(int64_t orderId,
                                  int64_t warehouseId,
                                  const std::string &remark,
                                  const std::string &logisticsCompany,
                                  const std::string &logisticsNo,
                                  int64_t operatorId,
                                  const std::string &returnType,
                                  const Json::Value &items,
                                  int64_t assignedApproverId,
                                  ReturnCallback &&cb, const business::RequestIdentity &request={});

    static void approveSalesReturn(int64_t returnId,
                                   bool approve,
                                   int64_t approverId,
                                   ReturnCallback &&cb);

    static void warehouseSalesReturn(int64_t returnId,
                                     bool approve,
                                     int64_t operatorId,
                                     const std::string &remark,
                                     const std::string &logisticsCompany,
                                     const std::string &logisticsNo,
                                     ReturnCallback &&cb);

    // 采购退货列表（含供应商名）；operatorId>0 时仅返回该用户经手的单据
    static void listPurchaseReturns(ReturnCallback &&cb);
    static void listPurchaseReturns(int64_t operatorId, ReturnCallback &&cb);

    // 销售退货列表（含客户名）；operatorId>0 时仅返回该用户经手的单据
    static void listSalesReturns(ReturnCallback &&cb);
    static void listSalesReturns(int64_t operatorId, ReturnCallback &&cb);

    // 退货明细（含商品名）
    static void listPurchaseReturnItems(int64_t returnId, ReturnCallback &&cb);
    static void listSalesReturnItems(int64_t returnId, ReturnCallback &&cb);
};

}  // namespace erp
