#pragma once

#include "BusinessSupport.h"

namespace erp::business {
struct HandoverSource {
    const char *table;
    const char *department;
    const char *creator;
    const char *condition;
};
inline constexpr HandoverSource handoverSources[] = {
    {"purchase_orders", "采购部", "created_by", ""},
    {"sales_orders", "销售部", "created_by", ""},
    {"purchase_returns", "采购部", "created_by", ""},
    {"sales_returns", "销售部", "created_by", ""},
    {"applications", "采购部", "applicant_id", " AND type IN ('supplier','product')"},
    {"applications", "销售部", "applicant_id", " AND type='customer'"},
    {"stock_operations", "仓库部", "applicant_id", ""},
    {"finance_actions", "财务部", "created_by", ""},
};
inline const std::string pendingAssigned = " WHERE assigned_approver_id=? AND status=0";

inline std::string handoverEligibility(const std::string &department) {
    const std::string admin =
        "EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id "
        "WHERE ur.user_id=u.id AND r.code='admin')";
    if (department == "财务部") return "(u.department=? OR " + admin + ")";
    return "((u.department=? AND EXISTS(SELECT 1 FROM user_roles ur JOIN roles r "
           "ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='leader')) OR " + admin + ")";
}

inline int64_t assigneeAfterResubmit(const Tx &tx, const drogon::orm::Row &row,
                                    const std::string &department, int64_t creator,
                                    const std::string &table, int64_t documentId, int status) {
    if (row["assigned_approver_id"].isNull()) return 0;
    const auto assignee = row["assigned_approver_id"].as<int64_t>();
    if (assignee != creator && !tx->execSqlSync(
        "SELECT u.id FROM user_directory u WHERE u.id=? AND u.status=1 AND " +
        handoverEligibility(department) + " FOR UPDATE", assignee, department).empty()) {
        return assignee;
    }
    event(tx,table,documentId,"approval_handover",creator,status,status,
          "原审批人账号 #" + std::to_string(assignee) + " 已失去审批资格，重提后进入部门审批队列");
    return 0;
}

// The caller holds the source account lock. New assignments lock the same account.
inline void requirePendingHandover(const Tx &tx, int64_t userId) {
    for (const auto &source : handoverSources) {
        const auto rows = tx->execSqlSync("SELECT id FROM " + std::string(source.table) +
            pendingAssigned + source.condition + " LIMIT 1 FOR UPDATE", userId);
        require(rows.empty(), "该账号还有待审批单据，请先在员工调度中移交待办", ErrorCode::CONFLICT);
    }
}
}
