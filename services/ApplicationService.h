#include "BusinessSupport.h"
#pragma once

#include <string>
#include <functional>
#include <json/json.h>

namespace erp
{
using ApplicationCallback =
    std::function<void(int, const std::string &, const Json::Value &)>;

// 申报服务：供应商/客户申报（采购部申报供应商 → 采购部领导审批；销售部申报客户 → 销售部领导审批）
class ApplicationService
{
  public:
    // 创建申报单（type: supplier/customer；payload 为申报字段 JSON）
    static void createApplication(const std::string &type,
                                  const std::string &title,
                                  const Json::Value &payload,
                                  int64_t applicantId,
                                  const std::string &remark,
                                  int64_t assignedApproverId,
                                  ApplicationCallback &&cb, const business::RequestIdentity &request={});

    // 审批申报单（approve=true 批准 → 写入 suppliers/products；false 驳回）
    static void approveApplication(int64_t applicationId,
                                   bool approve,
                                   int64_t approverId,
                                   const std::string &remark,
                                   ApplicationCallback &&cb);

    // 申报单列表（可传 type/status 过滤）
    static void listApplications(const std::string &type,
                                 int status,
                                 ApplicationCallback &&cb);

    // 被驳回的申报单修改后重新提交（status: 2→0）
    static void resubmitApplication(int64_t applicationId,
                                    const std::string &title,
                                    const Json::Value &payload,
                                    int64_t applicantId,
                                    ApplicationCallback &&cb);
};

}  // namespace erp
