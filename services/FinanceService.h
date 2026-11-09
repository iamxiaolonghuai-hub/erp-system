#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
// 财务服务回调：(code, message, data)
using FinanceCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 财务服务：收付款登记，更新往来账已收/已付金额并自动结清
class FinanceService
{
  public:
    // 收付款登记
    // accountId 往来账ID, amount 收付款金额, payMethod 支付方式, payDate 收付款日期, remark 备注, operatorId 操作人
    // 逻辑：校验往来账 -> 累加已收/已付金额(不超过账目金额) -> 达到则结清 -> 写收付款记录
    static void addPayment(int64_t accountId,
                           double amount,
                           const std::string &payMethod,
                           const std::string &payDate,
                           const std::string &remark,
                           int64_t operatorId,
                           FinanceCallback &&cb, const business::RequestIdentity &request={});
};

}  // namespace erp
