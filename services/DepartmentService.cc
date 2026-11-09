#include "DepartmentService.h"

#include "../utils/ApiResponse.h"
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <ctime>
#include <cstdio>
#include <cstdlib>

using namespace drogon;
using namespace drogon::orm;

namespace erp
{

void DepartmentService::listDepartments(Callback &&cb)
{
    auto callback = std::make_shared<Callback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    try
    {
        auto result =
            clientPtr->execSqlSync("SELECT id, name, code, description, status FROM "
                                   "departments ORDER BY id");
        Json::Value arr(Json::arrayValue);
        for (const auto &row : result)
        {
            Json::Value item;
            item["id"] = (Json::UInt64)row["id"].as<uint64_t>();
            item["name"] = row["name"].as<std::string>();
            item["code"] = row["code"].as<std::string>();
            item["description"] = row["description"].as<std::string>();
            item["status"] = row["status"].as<int>();
            arr.append(item);
        }
        (*callback)(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR, std::string("查询部门失败: ") +
                                                    e.what(),
                    Json::Value());
    }
}

void DepartmentService::listDepartmentEmployees(const std::string &department,
                                                Callback &&cb)
{
    auto callback = std::make_shared<Callback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    try
    {
        auto result = clientPtr->execSqlSync(
            "SELECT u.id, u.username, u.real_name, u.department, u.department_id, u.email, "
            "u.phone, u.status, COALESCE((SELECT r2.code FROM user_roles ur2 JOIN roles r2 ON r2.id=ur2.role_id WHERE ur2.user_id=u.id ORDER BY FIELD(r2.code,'admin','leader','staff') LIMIT 1),'') AS role_code "
            "FROM user_directory u "
            "WHERE u.department = ? "
            "ORDER BY u.id",
            department);
        Json::Value arr(Json::arrayValue);
        for (const auto &row : result)
        {
            Json::Value item;
            item["id"] = (Json::UInt64)row["id"].as<uint64_t>();
            item["username"] = row["username"].as<std::string>();
            item["real_name"] = row["real_name"].as<std::string>();
            item["department"] = row["department"].as<std::string>();
            item["department_id"] = row["department_id"].isNull()?Json::Value():Json::Value(Json::UInt64(row["department_id"].as<uint64_t>()));
            item["email"] = row["email"].as<std::string>();
            item["phone"] = row["phone"].as<std::string>();
            item["status"] = (int)row["status"].as<int8_t>();
            item["role_code"] = row["role_code"].isNull()
                                    ? ""
                                    : row["role_code"].as<std::string>();
            arr.append(item);
        }
        (*callback)(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR,
                    std::string("查询部门员工失败: ") + e.what(),
                    Json::Value());
    }
}

void DepartmentService::listAllUsers(Callback &&cb)
{
    auto callback = std::make_shared<Callback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    try
    {
        auto result = clientPtr->execSqlSync(
            "SELECT u.id, u.username, u.real_name, u.department, u.department_id, u.email, "
            "u.phone, u.status, u.warehouse_id, w.name AS warehouse_name, "
            "COALESCE((SELECT r2.code FROM user_roles ur2 JOIN roles r2 ON r2.id=ur2.role_id WHERE ur2.user_id=u.id ORDER BY FIELD(r2.code,'admin','leader','staff') LIMIT 1),'') AS role_code "
            "FROM user_directory u "
            "LEFT JOIN warehouses w ON w.id = u.warehouse_id "
            "ORDER BY u.id");
        Json::Value arr(Json::arrayValue);
        for (const auto &row : result)
        {
            Json::Value item;
            item["id"] = (Json::UInt64)row["id"].as<uint64_t>();
            item["username"] = row["username"].as<std::string>();
            item["real_name"] = row["real_name"].as<std::string>();
            item["department"] = row["department"].as<std::string>();
            item["department_id"] = row["department_id"].isNull()?Json::Value():Json::Value(Json::UInt64(row["department_id"].as<uint64_t>()));
            item["email"] = row["email"].as<std::string>();
            item["phone"] = row["phone"].as<std::string>();
            item["status"] = (int)row["status"].as<int8_t>();
            item["warehouse_id"] =
                row["warehouse_id"].isNull()
                    ? Json::Value()
                    : Json::Value((Json::UInt64)row["warehouse_id"].as<uint64_t>());
            item["warehouse_name"] =
                row["warehouse_name"].isNull()
                    ? ""
                    : row["warehouse_name"].as<std::string>();
            item["role_code"] = row["role_code"].isNull()
                                    ? ""
                                    : row["role_code"].as<std::string>();
            arr.append(item);
        }
        (*callback)(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR,
                    std::string("查询用户失败: ") + e.what(), Json::Value());
    }
}

void DepartmentService::assignWarehouse(int64_t userId, int64_t warehouseId,
                                        Callback &&cb)
{
    auto callback = std::make_shared<Callback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    try
    {
        auto existing=clientPtr->execSqlSync("SELECT id FROM user_directory WHERE id=?",userId);
        if(existing.empty()){(*callback)(ErrorCode::NOT_FOUND,"User not found",Json::Value());return;}
        if (warehouseId <= 0)
        {
            auto result = clientPtr->execSqlSync(
                "UPDATE users SET warehouse_id = NULL WHERE id = ?", userId);
            if (!existing.empty())
            {
                Json::Value data;
                data["user_id"] = (Json::UInt64)userId;
                data["warehouse_id"] = Json::Value();
                (*callback)(ErrorCode::OK, "已清除所属仓库", data);
            }
            else
            {
                (*callback)(ErrorCode::NOT_FOUND, "用户不存在", Json::Value());
            }
            return;
        }

        auto w = clientPtr->execSqlSync(
            "SELECT id, name FROM warehouses WHERE id = ?", warehouseId);
        if (w.size() == 0)
        {
            (*callback)(ErrorCode::NOT_FOUND, "仓库不存在", Json::Value());
            return;
        }

        auto result = clientPtr->execSqlSync(
            "UPDATE users SET warehouse_id = ? WHERE id = ?", warehouseId,
            userId);
        if (!existing.empty())
        {
            Json::Value data;
            data["user_id"] = (Json::UInt64)userId;
            data["warehouse_id"] = (Json::UInt64)warehouseId;
            data["warehouse_name"] = w[0]["name"].as<std::string>();
            (*callback)(ErrorCode::OK, "分配仓库成功", data);
        }
        else
        {
            (*callback)(ErrorCode::NOT_FOUND, "用户不存在", Json::Value());
        }
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR,
                    std::string("分配仓库失败: ") + e.what(), Json::Value());
    }
}

void DepartmentService::departmentDashboard(const std::string &department,
                                            int64_t userId,
                                            bool onlySelf,
                                            Callback &&cb)
{
    auto callback = std::make_shared<Callback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    try
    {
        Json::Value data;

        // 员工总数
        auto empCount = clientPtr->execSqlSync(
            "SELECT COUNT(*) AS c FROM user_directory WHERE department = ?",
            department);
        data["employee_count"] =
            (Json::UInt64)empCount[0]["c"].as<uint64_t>();

        // 部门相关业务统计：根据部门类型统计
        if (department == "销售部")
        {
            std::string whereSelf = onlySelf ? " WHERE created_by = ?" : "";
            auto r = onlySelf
                         ? clientPtr->execSqlSync(
                               "SELECT COUNT(*) AS c, COALESCE(SUM(total_amount),0) AS amt "
                               "FROM sales_orders" + whereSelf, userId)
                         : clientPtr->execSqlSync(
                               "SELECT COUNT(*) AS c, COALESCE(SUM(total_amount),0) AS amt "
                               "FROM sales_orders");
            data["order_count"] = (Json::UInt64)r[0]["c"].as<uint64_t>();
            data["total_amount"] = r[0]["amt"].as<std::string>();

            // 售出金额（按时间维度：今日 / 本周 / 当年），排除驳回单
            std::string timeSql =
                std::string(
                    "SELECT "
                    "COALESCE(SUM(CASE WHEN created_at >= CURDATE() THEN "
                    "total_amount ELSE 0 END),0) AS today_amt, "
                    "COALESCE(SUM(CASE WHEN created_at >= "
                    "DATE_SUB(CURDATE(), INTERVAL WEEKDAY(CURDATE()) DAY) THEN "
                    "total_amount ELSE 0 END),0) AS week_amt, "
                    "COALESCE(SUM(CASE WHEN created_at >= "
                    "DATE_FORMAT(CURDATE(), '%Y-01-01') THEN total_amount ELSE "
                    "0 END),0) AS year_amt "
                    "FROM sales_orders WHERE status NOT IN (2,6,7)") +
                (onlySelf ? " AND created_by = ?" : "");
            auto timeR = onlySelf
                             ? clientPtr->execSqlSync(timeSql, userId)
                             : clientPtr->execSqlSync(timeSql);
            data["today_amount"] = timeR[0]["today_amt"].as<std::string>();
            data["week_amount"] = timeR[0]["week_amt"].as<std::string>();
            data["year_amount"] = timeR[0]["year_amt"].as<std::string>();

            // 待审批单数（员工：自己的；领导：部门全部）
            auto pending = onlySelf
                               ? clientPtr->execSqlSync(
                                     "SELECT COUNT(*) AS c FROM sales_orders "
                                     "WHERE created_by = ? AND status = 0",
                                     userId)
                               : clientPtr->execSqlSync(
                                     "SELECT COUNT(*) AS c FROM sales_orders "
                                     "WHERE status = 0");
            data["pending_count"] = (Json::UInt64)pending[0]["c"].as<uint64_t>();

            // 最近销售单（员工：自己的；领导：部门全部）
            std::string recentSql =
                "SELECT so.order_no, so.total_amount, so.status, so.created_at, "
                "c.name AS customer_name "
                "FROM sales_orders so "
                "LEFT JOIN customers c ON c.id = so.customer_id ";
            if (onlySelf)
                recentSql += "WHERE so.created_by = ? ";
            recentSql += "ORDER BY so.id DESC LIMIT 5";
            auto recent = onlySelf
                              ? clientPtr->execSqlSync(recentSql, userId)
                              : clientPtr->execSqlSync(recentSql);
            Json::Value recentArr(Json::arrayValue);
            for (const auto &row : recent)
            {
                Json::Value item;
                item["order_no"] = row["order_no"].as<std::string>();
                item["total_amount"] = row["total_amount"].as<std::string>();
                item["status"] = (int)row["status"].as<int8_t>();
                item["customer_name"] = row["customer_name"].isNull()
                                            ? ""
                                            : row["customer_name"].as<std::string>();
                item["created_at"] = row["created_at"].as<std::string>();
                recentArr.append(item);
            }
            data["recent_sales"] = recentArr;

            // 员工销售数据（仅领导）：按销售员分组，部门全量
            if (!onlySelf)
            {
                auto empSales = clientPtr->execSqlSync(
                    "SELECT u.id AS user_id, u.real_name, "
                    "COUNT(so.id) AS order_count, "
                    "COALESCE(SUM(so.total_amount),0) AS amt, "
                    "SUM(CASE WHEN so.status=0 THEN 1 ELSE 0 END) AS pending "
                    "FROM sales_orders so "
                    "JOIN user_directory u ON u.id = so.created_by "
                    "WHERE so.created_by IS NOT NULL "
                    "GROUP BY u.id, u.real_name "
                    "ORDER BY amt DESC");
                Json::Value empArr(Json::arrayValue);
                for (const auto &row : empSales)
                {
                    Json::Value item;
                    item["user_id"] =
                        (Json::UInt64)row["user_id"].as<uint64_t>();
                    item["real_name"] = row["real_name"].as<std::string>();
                    item["order_count"] =
                        (Json::UInt64)row["order_count"].as<uint64_t>();
                    item["total_amount"] = row["amt"].as<std::string>();
                    item["pending_count"] =
                        (Json::UInt64)row["pending"].as<uint64_t>();
                    empArr.append(item);
                }
                data["employee_sales"] = empArr;
            }
        }
        else if (department == "采购部")
        {
            auto r = clientPtr->execSqlSync(
                "SELECT COUNT(*) AS c, COALESCE(SUM(total_amount),0) AS amt "
                "FROM purchase_orders");
            data["order_count"] = (Json::UInt64)r[0]["c"].as<uint64_t>();
            data["total_amount"] = r[0]["amt"].as<std::string>();
        }
        else if (department == "仓库部")
        {
            if (onlySelf)
            {
                // 仓库部员工：只看自己所属仓库 + 与自己有关的内容
                int64_t wid = 0;
                std::string wname = "";
                auto uw = clientPtr->execSqlSync(
                    "SELECT u.warehouse_id, w.name FROM user_directory u "
                    "LEFT JOIN warehouses w ON w.id = u.warehouse_id "
                    "WHERE u.id = ?",
                    userId);
                if (uw.size() > 0 && !uw[0]["warehouse_id"].isNull())
                {
                    wid = (int64_t)uw[0]["warehouse_id"].as<uint64_t>();
                    wname = uw[0]["name"].as<std::string>();
                }
                data["warehouse_id"] =
                    wid > 0 ? (Json::Int64)wid : Json::Value();
                data["warehouse_name"] = wname;
                data["has_warehouse"] = (wid > 0);

                if (wid > 0)
                {
                    auto r = clientPtr->execSqlSync(
                        "SELECT COUNT(*) AS c, COALESCE(SUM(quantity),0) AS qty, COALESCE(SUM(quantity<=min_quantity),0) AS low_stock "
                        "FROM inventory WHERE warehouse_id = ?",
                        wid);
                    data["inventory_count"] =
                        (Json::UInt64)r[0]["c"].as<uint64_t>();
                    data["total_qty"] = (Json::UInt64)r[0]["qty"].as<uint64_t>();
                    data["low_stock"] = (Json::UInt64)r[0]["low_stock"].as<uint64_t>();

                    // 本仓库待出库确认（销售领导已批准、仓管员要确认）
                    auto pending = clientPtr->execSqlSync(
                        "SELECT COUNT(*) AS c FROM sales_orders "
                        "WHERE status = 1 AND warehouse_id = ?",wid);
                    data["pending_confirm"] =
                        (Json::UInt64)pending[0]["c"].as<uint64_t>();
                }
                else
                {
                    data["inventory_count"] = 0;
                    data["total_qty"] = 0;
                    data["low_stock"] = 0;
                    data["pending_confirm"] = 0;
                }

                // 与我有关的出入库申请（我提交的 + 待我处理的）
                auto myOps = clientPtr->execSqlSync(
                    "SELECT COUNT(*) AS c FROM stock_operations "
                    "WHERE applicant_id = ?",
                    userId);
                data["my_operations"] =
                    (Json::UInt64)myOps[0]["c"].as<uint64_t>();

                auto myPending = clientPtr->execSqlSync(
                    "SELECT COUNT(*) AS c FROM stock_operations "
                    "WHERE applicant_id = ? AND status = 0",
                    userId);
                data["my_pending_ops"] =
                    (Json::UInt64)myPending[0]["c"].as<uint64_t>();

                // 我的出入库申请按类型分布
                data["my_ops_in"] = 0;
                data["my_ops_out"] = 0;
                data["my_ops_transfer"] = 0;
                auto opsByType = clientPtr->execSqlSync(
                    "SELECT type, COUNT(*) AS c FROM stock_operations "
                    "WHERE applicant_id = ? GROUP BY type",
                    userId);
                for (const auto &row : opsByType)
                {
                    std::string t = row["type"].as<std::string>();
                    if (t == "in")
                        data["my_ops_in"] =
                            (Json::UInt64)row["c"].as<uint64_t>();
                    else if (t == "out")
                        data["my_ops_out"] =
                            (Json::UInt64)row["c"].as<uint64_t>();
                    else if (t == "transfer")
                        data["my_ops_transfer"] =
                            (Json::UInt64)row["c"].as<uint64_t>();
                }

                // 我的最近出入库操作单
                auto opsRecent = clientPtr->execSqlSync(
                    "SELECT so.operation_no, so.type, so.qty, so.status, "
                    "so.created_at, p.name AS product_name, "
                    "fw.name AS from_warehouse, tw.name AS to_warehouse "
                    "FROM stock_operations so "
                    "LEFT JOIN products p ON p.id = so.product_id "
                    "LEFT JOIN warehouses fw ON fw.id = so.from_warehouse_id "
                    "LEFT JOIN warehouses tw ON tw.id = so.to_warehouse_id "
                    "WHERE so.applicant_id = ? "
                    "ORDER BY so.id DESC LIMIT 5",
                    userId);
                Json::Value opsArr(Json::arrayValue);
                for (const auto &row : opsRecent)
                {
                    Json::Value item;
                    item["operation_no"] =
                        row["operation_no"].as<std::string>();
                    item["type"] = row["type"].as<std::string>();
                    item["qty"] = row["qty"].as<int>();
                    item["status"] = (int)row["status"].as<int8_t>();
                    item["product_name"] = row["product_name"].as<std::string>();
                    item["from_warehouse"] =
                        row["from_warehouse"].isNull()
                            ? ""
                            : row["from_warehouse"].as<std::string>();
                    item["to_warehouse"] = row["to_warehouse"].isNull()
                                               ? ""
                                               : row["to_warehouse"].as<std::string>();
                    item["created_at"] = row["created_at"].as<std::string>();
                    opsArr.append(item);
                }
                data["my_ops_recent"] = opsArr;
            }
            else
            {
                // 领导/管理员：全仓库统计
                auto r = clientPtr->execSqlSync(
                    "SELECT COUNT(*) AS c, COALESCE(SUM(quantity),0) AS qty, COALESCE(SUM(quantity<=min_quantity),0) AS low_stock "
                    "FROM inventory");
                data["inventory_count"] =
                    (Json::UInt64)r[0]["c"].as<uint64_t>();
                data["total_qty"] = (Json::UInt64)r[0]["qty"].as<uint64_t>();
                data["low_stock"] = (Json::UInt64)r[0]["low_stock"].as<uint64_t>();
                auto pending = clientPtr->execSqlSync(
                    "SELECT COUNT(*) AS c FROM sales_orders "
                    "WHERE status = 1");
                data["pending_confirm"] =
                    (Json::UInt64)pending[0]["c"].as<uint64_t>();

                // 待审批库存操作单（领导要审批的）
                auto opsPending = clientPtr->execSqlSync(
                    "SELECT COUNT(*) AS c FROM stock_operations "
                    "WHERE status = 0");
                data["ops_pending_approve"] =
                    (Json::UInt64)opsPending[0]["c"].as<uint64_t>();
            }
        }
        else if (department == "财务部")
        {
            auto r = clientPtr->execSqlSync(
                "SELECT COALESCE(SUM(CASE WHEN account_type='receivable' AND amount>0 "
                "THEN amount-paid_amount-offset_amount ELSE 0 END),0) AS recv, "
                "COALESCE(SUM(CASE WHEN account_type='payable' AND amount>0 THEN "
                "amount-paid_amount-offset_amount ELSE 0 END),0) AS pay "
                "FROM accounts");
            data["receivable"] = r[0]["recv"].as<std::string>();
            data["payable"] = r[0]["pay"].as<std::string>();
        }
        else
        {
            data["order_count"] = 0;
            data["total_amount"] = "0";
        }

        if(department=="销售部" || department=="采购部") {
            auto source=department=="销售部"?"posted_sales_activity":"posted_purchase_activity";
            std::string scope=onlySelf?" WHERE created_by="+std::to_string(userId):"";
            auto posted=clientPtr->execSqlSync("SELECT COALESCE(SUM(total_amount),0) AS total,COALESCE(SUM(CASE WHEN created_at>=CURDATE() THEN total_amount ELSE 0 END),0) AS today,COALESCE(SUM(CASE WHEN created_at>=DATE_SUB(CURDATE(),INTERVAL WEEKDAY(CURDATE()) DAY) THEN total_amount ELSE 0 END),0) AS week,COALESCE(SUM(CASE WHEN YEAR(created_at)=YEAR(CURDATE()) THEN total_amount ELSE 0 END),0) AS year FROM "+std::string(source)+scope);
            data["total_amount"]=posted[0]["total"].as<std::string>();data["today_amount"]=posted[0]["today"].as<std::string>();data["week_amount"]=posted[0]["week"].as<std::string>();data["year_amount"]=posted[0]["year"].as<std::string>();
        }
        (*callback)(ErrorCode::OK, "success", data);
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR,
                    std::string("查询部门仪表盘失败: ") + e.what(),
                    Json::Value());
    }
}

void DepartmentService::departmentTrend(const std::string &department,
                                         const std::string &period,
                                         int64_t userId,
                                         bool onlySelf,
                                         Callback &&cb)
{
    auto callback = std::make_shared<Callback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    // 确定分组格式与时间范围（返回 label 分组）
    // period: day -> 近30天按天；week -> 近12周按周；month -> 近12月按月；year -> 近5年按年
    std::string dateFormat;
    int buckets = 0;
    std::string intervalUnit;
    if (period == "week")
    {
        // MySQL 周分组用 YEARWEEK
        dateFormat = "YEARWEEK(created_at, 1)";
        buckets = 12;
        intervalUnit = "week";
    }
    else if (period == "month")
    {
        dateFormat = "DATE_FORMAT(created_at, '%Y-%m')";
        buckets = 12;
        intervalUnit = "month";
    }
    else if (period == "year")
    {
        dateFormat = "DATE_FORMAT(created_at, '%Y')";
        buckets = 5;
        intervalUnit = "year";
    }
    else
    {
        // 默认 day
        dateFormat = "DATE_FORMAT(created_at, '%Y-%m-%d')";
        buckets = 30;
        intervalUnit = "day";
    }

    // 根据部门选择统计的序列（支持多序列，如仓库部入库+出库）
    // 每个序列：{名称, SQL, 是否带 userId 参数}
    struct SeriesDef
    {
        std::string name;
        std::string sql;
        bool hasUserFilter;
        int64_t filterVal = 0;  // 过滤参数值（0 表示用 userId）
    };
    std::vector<SeriesDef> seriesDefs;

    if (department == "销售部")
    {
        // 员工只看自己的数据（onlySelf），领导/管理员看全部门
        std::string selfWhere = onlySelf ? " AND created_by = ?" : "";
        std::string sqlAmount = "SELECT " + dateFormat +
                                " AS bucket, COALESCE(SUM(total_amount),0) AS "
                                "val FROM posted_sales_activity WHERE status NOT IN (2,6,7)" +
                                selfWhere + " GROUP BY bucket";
        std::string sqlCount = "SELECT " + dateFormat +
                               " AS bucket, COUNT(*) AS val FROM "
                               "sales_orders WHERE status NOT IN (2,6,7)" +
                               selfWhere + " GROUP BY bucket";
        std::string sqlQty =
            "SELECT " + dateFormat +
            " AS bucket, COALESCE(SUM(soi.quantity),0) AS val FROM "
            "sales_orders so JOIN sales_order_items soi ON so.id = "
            "soi.order_id WHERE so.status NOT IN (2,6,7)" +
            (onlySelf ? " AND so.created_by = ?" : "") + " GROUP BY bucket";
        // 售后（退货+换货）情况
        std::string afterWhere = onlySelf ? " WHERE status=3 AND created_by = ?" : " WHERE status=3";
        std::string sqlAfterCount =
            "SELECT " + dateFormat + " AS bucket, COUNT(*) AS val FROM "
            "sales_returns" + afterWhere + " GROUP BY bucket";
        std::string sqlAfterAmount =
            "SELECT " + dateFormat +
            " AS bucket, COALESCE(SUM(total_amount),0) AS val FROM "
            "sales_returns" + afterWhere + " GROUP BY bucket";
        seriesDefs.push_back({"售卖金额", sqlAmount, onlySelf});
        seriesDefs.push_back({"销售单数", sqlCount, onlySelf});
        seriesDefs.push_back({"订单商品件数", sqlQty, onlySelf});
        seriesDefs.push_back({"售后单数", sqlAfterCount, onlySelf});
        seriesDefs.push_back({"售后金额", sqlAfterAmount, onlySelf});
    }
    else if (department == "采购部")
    {
        std::string sql = "SELECT " + dateFormat +
                          " AS bucket, COALESCE(SUM(total_amount),0) AS val "
                          "FROM posted_purchase_activity WHERE status NOT IN (2,6,7) GROUP BY bucket";
        seriesDefs.push_back({"采购金额", sql, false});
    }
    else if (department == "仓库部")
    {
        // 员工按所属仓库过滤；领导/管理员看全仓库
        int64_t filterWh = 0;
        if (onlySelf)
        {
            auto uw = clientPtr->execSqlSync(
                "SELECT warehouse_id FROM user_directory WHERE id = ?", userId);
            if (uw.size() > 0 && !uw[0]["warehouse_id"].isNull())
                filterWh = (int64_t)uw[0]["warehouse_id"].as<uint64_t>();
        }
        std::string whWhere = filterWh > 0 ? " AND warehouse_id = ?" : "";
        std::string sqlIn = "SELECT " + dateFormat +
                            " AS bucket, COALESCE(SUM(change_qty),0) AS val "
                            "FROM stock_records WHERE change_type = 'in'" +
                            whWhere + " GROUP BY bucket";
        std::string sqlOut = "SELECT " + dateFormat +
                             " AS bucket, COALESCE(SUM(ABS(change_qty)),0) AS val "
                             "FROM stock_records WHERE change_type = 'out'" +
                             whWhere + " GROUP BY bucket";
        seriesDefs.push_back({"入库数量", sqlIn, filterWh > 0, filterWh});
        seriesDefs.push_back({"出库数量", sqlOut, filterWh > 0, filterWh});
    }
    else if (department == "财务部")
    {
        std::string sql = "SELECT " + dateFormat +
                          " AS bucket, COALESCE(SUM(amount),0) AS val "
                          "FROM accounts GROUP BY bucket";
        seriesDefs.push_back({"往来账金额", sql, false});
    }
    else
    {
        std::string sql = "SELECT " + dateFormat +
                          " AS bucket, COALESCE(SUM(total_amount),0) AS val "
                          "FROM posted_sales_activity WHERE status NOT IN (2,6,7) GROUP BY bucket";
        seriesDefs.push_back({"金额", sql, false});
    }

    try
    {
        // 分别查询每个序列的聚合结果
        std::vector<std::map<std::string, double>> seriesAgg;
        for (const auto &def : seriesDefs)
        {
            std::map<std::string, double> agg;
            Result result = def.hasUserFilter
                                ? clientPtr->execSqlSync(
                                      def.sql,
                                      def.filterVal > 0 ? def.filterVal : userId)
                                : clientPtr->execSqlSync(def.sql);
            for (const auto &row : result)
            {
                std::string bucket = row["bucket"].as<std::string>();
                double val = 0;
                try { val = std::stod(row["val"].as<std::string>()); }
                catch (...) {}
                agg[bucket] = val;
            }
            seriesAgg.push_back(agg);
        }

        // 生成时间桶序列（补齐缺失的桶，值为 0）
        Json::Value labels(Json::arrayValue);
        Json::Value series(Json::arrayValue);
        // 先初始化每个序列的 values 数组
        std::vector<Json::Value> seriesValues(seriesDefs.size());
        for (auto &sv : seriesValues)
            sv = Json::Value(Json::arrayValue);

        // 获取当前时间（本地时间）
        std::time_t now = std::time(nullptr);
        std::tm nowTm;
#ifdef _WIN32
        localtime_s(&nowTm, &now);
#else
        localtime_r(&now, &nowTm);
#endif

        auto fmtBucket = [](int y, int m, int d, const std::string &unit,
                            int week) -> std::string {
            char buf[64];
            if (unit == "year")
                snprintf(buf, sizeof(buf), "%04d", y);
            else if (unit == "month")
                snprintf(buf, sizeof(buf), "%04d-%02d", y, m);
            else if (unit == "week")
                snprintf(buf, sizeof(buf), "%04d%02d", y, week);
            else
                snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
            return std::string(buf);
        };

        // 生成标签序列（从旧到新）
        std::vector<std::string> bucketKeys;
        std::vector<std::string> bucketLabels;
        if (intervalUnit == "year")
        {
            for (int i = buckets - 1; i >= 0; --i)
            {
                int y = nowTm.tm_year + 1900 - i;
                bucketKeys.push_back(fmtBucket(y, 1, 1, "year", 0));
                bucketLabels.push_back(fmtBucket(y, 1, 1, "year", 0));
            }
        }
        else if (intervalUnit == "month")
        {
            // 从 now 往前推 buckets-1 个月
            std::tm t = nowTm;
            for (int i = buckets - 1; i >= 0; --i)
            {
                std::tm cur = t;
                int y = cur.tm_year + 1900;
                int m = cur.tm_mon + 1;
                // 往前推 i 个月
                int totalMonths = y * 12 + (m - 1) - i;
                int ry = totalMonths / 12;
                int rm = totalMonths % 12 + 1;
                bucketKeys.push_back(fmtBucket(ry, rm, 1, "month", 0));
                bucketLabels.push_back(fmtBucket(ry, rm, 1, "month", 0));
            }
        }
        else if (intervalUnit == "week")
        {
            for (int i = buckets - 1; i >= 0; --i)
            {
                std::tm t = nowTm;
                std::time_t shifted = now - (std::time_t)i * 7 * 24 * 3600;
                std::tm shiftedTm;
#ifdef _WIN32
                localtime_s(&shiftedTm, &shifted);
#else
                localtime_r(&shifted, &shiftedTm);
#endif
                // 计算 ISO 周号（简化：用一年中的周）
                char ybuf[8];
                strftime(ybuf, sizeof(ybuf), "%Y", &shiftedTm);
                char weekBuf[8];
                strftime(weekBuf, sizeof(weekBuf), "%V", &shiftedTm);
                int y = atoi(ybuf);
                int wk = atoi(weekBuf);
                bucketKeys.push_back(fmtBucket(y, 1, 1, "week", wk));
                bucketLabels.push_back(std::string(ybuf) + "-W" + weekBuf);
            }
        }
        else
        {
            // day
            for (int i = buckets - 1; i >= 0; --i)
            {
                std::time_t shifted = now - (std::time_t)i * 24 * 3600;
                std::tm shiftedTm;
#ifdef _WIN32
                localtime_s(&shiftedTm, &shifted);
#else
                localtime_r(&shifted, &shiftedTm);
#endif
                int y = shiftedTm.tm_year + 1900;
                int m = shiftedTm.tm_mon + 1;
                int d = shiftedTm.tm_mday;
                bucketKeys.push_back(fmtBucket(y, m, d, "day", 0));
                bucketLabels.push_back(fmtBucket(y, m, d, "day", 0));
            }
        }

        for (size_t i = 0; i < bucketKeys.size(); ++i)
        {
            labels.append(bucketLabels[i]);
            for (size_t s = 0; s < seriesDefs.size(); ++s)
            {
                auto it = seriesAgg[s].find(bucketKeys[i]);
                seriesValues[s].append(it != seriesAgg[s].end() ? it->second : 0.0);
            }
        }

        // 组装 series 数组
        for (size_t s = 0; s < seriesDefs.size(); ++s)
        {
            Json::Value item;
            item["name"] = seriesDefs[s].name;
            item["values"] = seriesValues[s];
            series.append(item);
        }

        Json::Value data;
        data["period"] = period;
        data["labels"] = labels;
        data["series"] = series;
        // 兼容旧字段：values 指向第一个序列
        if (!seriesValues.empty())
            data["values"] = seriesValues[0];
        data["unit"] = (department == "仓库部" ? "数量"
                        : department == "销售部" ? "金额(¥)/数量"
                                                 : "金额(¥)");
        (*callback)(ErrorCode::OK, "success", data);
    }
    catch (const std::exception &e)
    {
        (*callback)(ErrorCode::INTERNAL_ERROR,
                    std::string("查询部门趋势失败: ") + e.what(),
                    Json::Value());
    }
}


}  // namespace erp
