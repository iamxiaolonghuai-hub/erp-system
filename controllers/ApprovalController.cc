#include "ApprovalController.h"

#include <regex>

namespace erp {
void ApprovalController::myActions(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback)
{
    using namespace business;
    try {
        const auto userId = std::stoll(req->getAttributes()->get<std::string>("user_id"));
        const auto mode = req->getParameter("mode");
        const auto action = req->getParameter("action");
        const auto number = req->getParameter("number");
        auto from = req->getParameter("from");
        auto to = req->getParameter("to");
        const auto pageText = req->getParameter("page");
        const int page = pageText.empty() ? 1 : std::stoi(pageText);
        require(page >= 1 && page <= 10000 && number.size() <= 128, "分页或单号筛选条件错误");
        require(mode.empty() || mode == "warehouse", "查询类型错误");
        require(action.empty() || std::set<std::string>{
            "approve", "reject", "review", "receive", "prepare", "ship",
            "warehouse_complete", "warehouse_reject"
        }.count(action), "处理动作错误");
        const std::regex day("[0-9]{4}-[0-9]{2}-[0-9]{2}");
        require((from.empty() || std::regex_match(from, day)) &&
                (to.empty() || std::regex_match(to, day)) &&
                (from.empty() || to.empty() || from <= to), "日期范围错误");
        if (from.empty()) from = "1000-01-01";
        if (to.empty()) to = "9999-12-31";

        // Every result belongs to the caller's own action. Current order assignees
        // can change, and must not determine which historical batches appear here.
        const std::string user = std::to_string(userId);
        std::string entries;
        auto add = [&](const std::string &select) {
            if (!entries.empty()) entries += " UNION ALL ";
            entries += select;
        };
        for (const std::string kind : {"purchase", "sales"}) {
            const bool purchase = kind == "purchase";
            const std::string event = purchase ? "receive" : "ship";
            add("SELECT '" + kind + "_orders' AS document_type,o.order_no AS document_no,"
                "d.delivery_no AS batch_no,'" + event + "' AS action,d.created_at AS action_at,"
                "d.id AS sort_id,o.created_by AS initiator_id,d.created_by AS handler_id,"
                "d.approved_by AS approver_id FROM order_deliveries d JOIN " + kind +
                "_orders o ON o.id=d.order_id WHERE d.order_type='" + kind + "' AND "
                + (purchase ? "d.created_by=" : "d.approved_by=") + user);
        }
        struct EventSource {
            const char *table;
            const char *number;
            const char *creator;
            const char *actions;
            const char *warehouseActions;
        };
        const EventSource sources[] = {
            {"purchase_orders", "order_no", "created_by", "'approve','reject','warehouse_reject'", "'warehouse_reject'"},
            {"sales_orders", "order_no", "created_by", "'approve','reject','prepare','warehouse_reject'", "'prepare','warehouse_reject'"},
            {"purchase_returns", "return_no", "created_by", "'approve','reject','warehouse_complete','warehouse_reject'", "'warehouse_complete','warehouse_reject'"},
            {"sales_returns", "return_no", "created_by", "'approve','reject','warehouse_complete','warehouse_reject'", "'warehouse_complete','warehouse_reject'"},
            {"stock_operations", "operation_no", "applicant_id", "'approve','reject'", "'approve','reject'"},
            {"stock_checks", "check_no", "created_by", "'review'", "'review'"},
            {"applications", "application_no", "applicant_id", "'approve','reject'", nullptr},
            {"finance_actions", "action_no", "created_by", "'approve','reject'", nullptr},
        };
        for (const auto &source : sources) {
            const char *actions = mode == "warehouse" ? source.warehouseActions : source.actions;
            if (!actions) continue;
            const std::string table = source.table;
            add("SELECT e.document_type,d." + std::string(source.number) +
                ",NULL,e.action,e.created_at,e.id,d." + source.creator +
                ",e.operator_id,IF(e.action IN ('approve','reject','review'),e.operator_id,NULL)"
                " FROM business_events e JOIN " + table + " d ON d.id=e.document_id"
                " WHERE e.operator_id=" + user + " AND e.document_type='" + table +
                "' AND e.action IN (" + actions + ")");
        }
        auto rows = drogon::app().getDbClient()->execSqlSync(
            "SELECT h.document_type,h.document_no,h.batch_no,h.action,h.action_at,"
            "COALESCE(NULLIF(i.real_name,''),i.username) AS initiator_name,"
            "COALESCE(NULLIF(w.real_name,''),w.username) AS handler_name,"
            "COALESCE(NULLIF(a.real_name,''),a.username) AS approver_name FROM (" + entries +
            ") h LEFT JOIN users i ON i.id=h.initiator_id LEFT JOIN users w ON w.id=h.handler_id"
            " LEFT JOIN users a ON a.id=h.approver_id"
            " WHERE h.action_at>=? AND DATE(h.action_at)<=?"
            " AND (?='' OR h.action=?) AND (?='' OR LOCATE(?,h.document_no)>0 OR LOCATE(?,h.batch_no)>0)"
            " ORDER BY h.action_at DESC,h.document_type,h.action,h.sort_id DESC LIMIT 51 OFFSET " +
            std::to_string((page - 1) * 50), from, to, action, action, number, number, number);
        Json::Value data;
        data["rows"] = Json::Value(Json::arrayValue);
        for (const auto &row : rows) {
            if (data["rows"].size() == 50) break;
            data["rows"].append(rowJson(rows, row));
        }
        data["has_more"] = rows.size() > 50;
        callback(ApiResponse::ok("查询成功", data));
    } catch (const Error &e) {
        callback(ApiResponse::error(e.code, e.what()));
    } catch (const std::logic_error &) {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "查询条件错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        callback(ApiResponse::error(ErrorCode::INTERNAL_ERROR, "处理记录查询失败"));
    }
}
}
