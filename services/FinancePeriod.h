#pragma once
#include "BusinessSupport.h"

namespace erp::business {
// Posting and close share one transaction lock. Keep the lock order before any account write.
inline void lockFinancePosting(const Tx &tx, const std::string &cashDate = "") {
    const auto rows = tx->execSqlSync(
        "SELECT DATE_FORMAT(closed_through,'%Y-%m-%d') AS cutoff,"
        "DATE_FORMAT(CURRENT_DATE,'%Y-%m-%d') AS today "
        "FROM finance_period_control WHERE id=1 FOR UPDATE");
    require(!rows.empty(), "请先执行财务期间迁移", ErrorCode::CONFLICT);
    if (!rows[0]["cutoff"].isNull()) {
        const auto cutoff = rows[0]["cutoff"].as<std::string>();
        require(rows[0]["today"].as<std::string>() > cutoff && (cashDate.empty() || cashDate > cutoff),
                "单据日期属于已结账期间", ErrorCode::CONFLICT);
    }
}

inline void financePosting(const Tx &tx, int64_t accountId, const std::string &kind,
                           const std::string &source, int64_t amount, int64_t cash, int64_t offset) {
    tx->execSqlSync(
        "INSERT INTO finance_postings(account_id,account_type,partner_id,partner_name,biz_no,"
        "source_kind,source_no,posting_day,amount_change,cash_change,offset_change) "
        "SELECT id,account_type,partner_id,partner_name,biz_no,?,?,CURRENT_DATE,?,?,? "
        "FROM accounts WHERE id=?",
        kind,source,money(amount),money(cash),money(offset),accountId);
}
}
