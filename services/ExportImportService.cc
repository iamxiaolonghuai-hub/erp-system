#include "ExportImportService.h"
#include "PartnerCsv.h"

#include "../models/Products.h"
#include "../models/Suppliers.h"
#include "../models/Customers.h"
#include "../models/Warehouses.h"
#include "../models/Categories.h"
#include "../utils/ApiResponse.h"

#include <drogon/orm/Mapper.h>
#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>

#include <memory>
#include <sstream>
#include <vector>
#include <string>
#include <map>

using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::erp_system;

namespace erp
{

// CSV 单元格转义：含逗号/引号/换行则加引号
static std::string csvEscape(const std::string &s)
{
    if (s.find(',') == std::string::npos && s.find('"') == std::string::npos &&
        s.find('\n') == std::string::npos && s.find('\r') == std::string::npos)
        return s;
    std::string out = "\"";
    for (char c : s)
    {
        if (c == '"')
            out += "\"\"";
        else
            out += c;
    }
    out += "\"";
    return out;
}

// 解析一行 CSV（支持引号转义）
static std::vector<std::string> parseCsvLine(const std::string &line)
{
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); i++)
    {
        char c = line[i];
        if (inQuotes)
        {
            if (c == '"')
            {
                if (i + 1 < line.size() && line[i + 1] == '"')
                {
                    cur += '"';
                    i++;
                }
                else
                {
                    inQuotes = false;
                }
            }
            else
            {
                cur += c;
            }
        }
        else
        {
            if (c == '"')
            {
                inQuotes = true;
            }
            else if (c == ',')
            {
                fields.push_back(cur);
                cur.clear();
            }
            else
            {
                cur += c;
            }
        }
    }
    fields.push_back(cur);
    return fields;
}

void ExportImportService::exportCsv(const std::string &table,
                                    ExportCallback &&cb)
{
    auto callback = std::make_shared<ExportCallback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    if (table == "products")
    {
        Mapper<Products> mapper(clientPtr);
        mapper.findAll(
            [callback](const std::vector<Products> &v) {
                std::ostringstream oss;
                oss << "sku,name,category_id,unit,spec,price,cost_price,status,description\n";
                for (const auto &p : v)
                {
                    oss << csvEscape(p.getValueOfSku()) << ","
                        << csvEscape(p.getValueOfName()) << ","
                        << p.getValueOfCategoryId() << ","
                        << csvEscape(p.getValueOfUnit()) << ","
                        << csvEscape(p.getValueOfSpec()) << ","
                        << p.getValueOfPrice() << ","
                        << p.getValueOfCostPrice() << ","
                        << (int)p.getValueOfStatus() << ","
                        << csvEscape(p.getValueOfDescription()) << "\n";
                }
                Json::Value data;
                data["csv"] = oss.str();
                (*callback)(ErrorCode::OK, "success", data);
            },
            [callback](const DrogonDbException &e) {
                (*callback)(ErrorCode::INTERNAL_ERROR,
                            "导出失败: " + std::string(e.base().what()),
                            Json::Value());
            });
    }
    else if (table == "suppliers")
    {
        Mapper<Suppliers> mapper(clientPtr);
        mapper.findAll(
            [callback](const std::vector<Suppliers> &v) {
                std::ostringstream oss;
                oss << "name,contact,phone,email,address,status,remark\n";
                for (const auto &s : v)
                {
                    oss << csvEscape(s.getValueOfName()) << ","
                        << csvEscape(s.getValueOfContact()) << ","
                        << csvEscape(s.getValueOfPhone()) << ","
                        << csvEscape(s.getValueOfEmail()) << ","
                        << csvEscape(s.getValueOfAddress()) << ","
                        << (int)s.getValueOfStatus() << ","
                        << csvEscape(s.getValueOfRemark()) << "\n";
                }
                Json::Value data;
                data["csv"] = oss.str();
                (*callback)(ErrorCode::OK, "success", data);
            },
            [callback](const DrogonDbException &e) {
                (*callback)(ErrorCode::INTERNAL_ERROR,
                            "导出失败: " + std::string(e.base().what()),
                            Json::Value());
            });
    }
    else if (table == "customers")
    {
        Mapper<Customers> mapper(clientPtr);
        mapper.findAll(
            [callback](const std::vector<Customers> &v) {
                std::ostringstream oss;
                oss << "name,contact,phone,email,address,status,remark\n";
                for (const auto &c : v)
                {
                    oss << csvEscape(c.getValueOfName()) << ","
                        << csvEscape(c.getValueOfContact()) << ","
                        << csvEscape(c.getValueOfPhone()) << ","
                        << csvEscape(c.getValueOfEmail()) << ","
                        << csvEscape(c.getValueOfAddress()) << ","
                        << (int)c.getValueOfStatus() << ","
                        << csvEscape(c.getValueOfRemark()) << "\n";
                }
                Json::Value data;
                data["csv"] = oss.str();
                (*callback)(ErrorCode::OK, "success", data);
            },
            [callback](const DrogonDbException &e) {
                (*callback)(ErrorCode::INTERNAL_ERROR,
                            "导出失败: " + std::string(e.base().what()),
                            Json::Value());
            });
    }
    else if (table == "warehouses")
    {
        Mapper<Warehouses> mapper(clientPtr);
        mapper.findAll(
            [callback](const std::vector<Warehouses> &v) {
                std::ostringstream oss;
                oss << "name,code,address,manager,status\n";
                for (const auto &w : v)
                {
                    oss << csvEscape(w.getValueOfName()) << ","
                        << csvEscape(w.getValueOfCode()) << ","
                        << csvEscape(w.getValueOfAddress()) << ","
                        << csvEscape(w.getValueOfManager()) << ","
                        << (int)w.getValueOfStatus() << "\n";
                }
                Json::Value data;
                data["csv"] = oss.str();
                (*callback)(ErrorCode::OK, "success", data);
            },
            [callback](const DrogonDbException &e) {
                (*callback)(ErrorCode::INTERNAL_ERROR,
                            "导出失败: " + std::string(e.base().what()),
                            Json::Value());
            });
    }
    else if (table == "categories")
    {
        Mapper<Categories> mapper(clientPtr);
        mapper.findAll(
            [callback](const std::vector<Categories> &v) {
                std::ostringstream oss;
                oss << "parent_id,name,sort_order\n";
                for (const auto &c : v)
                {
                    oss << c.getValueOfParentId() << ","
                        << csvEscape(c.getValueOfName()) << ","
                        << c.getValueOfSortOrder() << "\n";
                }
                Json::Value data;
                data["csv"] = oss.str();
                (*callback)(ErrorCode::OK, "success", data);
            },
            [callback](const DrogonDbException &e) {
                (*callback)(ErrorCode::INTERNAL_ERROR,
                            "导出失败: " + std::string(e.base().what()),
                            Json::Value());
            });
    }
    else if (table == "purchase_orders")
    {
        // 采购单导出：主表 + 供应商名 + 明细（多行，同一单号展开）
        try
        {
            auto rows = clientPtr->execSqlSync(
                "SELECT po.id, po.order_no, po.total_amount, po.status, "
                "po.remark, po.created_at, s.name AS partner_name "
                "FROM purchase_orders po "
                "LEFT JOIN suppliers s ON s.id = po.supplier_id "
                "ORDER BY po.id DESC");
            auto items = clientPtr->execSqlSync(
                "SELECT i.order_id, p.name AS product_name, p.sku, i.quantity, "
                "i.price, i.amount "
                "FROM purchase_order_items i "
                "LEFT JOIN products p ON p.id = i.product_id "
                "ORDER BY i.order_id, i.id");

            // 构建 order_id -> 明细列表
            std::map<int64_t, std::vector<std::vector<std::string>>> itemMap;
            for (const auto &r : items)
            {
                int64_t oid = (int64_t)r["order_id"].as<uint64_t>();
                std::vector<std::string> line;
                line.push_back(r["product_name"].isNull() ? "" : r["product_name"].as<std::string>());
                line.push_back(r["sku"].isNull() ? "" : r["sku"].as<std::string>());
                line.push_back(r["quantity"].as<std::string>());
                line.push_back(r["price"].as<std::string>());
                line.push_back(r["amount"].as<std::string>());
                itemMap[oid].push_back(line);
            }

            std::ostringstream oss;
            oss << "单号,类型,供应商,商品名,SKU,数量,单价,小计,总金额,状态,备注,创建时间\n";
            for (const auto &r : rows)
            {
                int64_t oid = (int64_t)r["id"].as<uint64_t>();
                std::string orderNo = r["order_no"].as<std::string>();
                std::string partner = r["partner_name"].isNull() ? "" : r["partner_name"].as<std::string>();
                std::string total = r["total_amount"].as<std::string>();
                std::string status = r["status"].as<std::string>();
                std::string remark = r["remark"].as<std::string>();
                std::string createdAt = r["created_at"].as<std::string>();

                auto it = itemMap.find(oid);
                if (it == itemMap.end() || it->second.empty())
                {
                    // 无明细，输出一行
                    oss << csvEscape(orderNo) << ",采购," << csvEscape(partner)
                        << ",,,,,," << total << "," << status << ","
                        << csvEscape(remark) << "," << createdAt << "\n";
                }
                else
                {
                    for (const auto &line : it->second)
                    {
                        oss << csvEscape(orderNo) << ",采购," << csvEscape(partner)
                            << "," << csvEscape(line[0]) << "," << csvEscape(line[1])
                            << "," << line[2] << "," << line[3] << "," << line[4]
                            << "," << total << "," << status << ","
                            << csvEscape(remark) << "," << createdAt << "\n";
                    }
                }
            }
            Json::Value data;
            data["csv"] = oss.str();
            (*callback)(ErrorCode::OK, "success", data);
        }
        catch (const std::exception &e)
        {
            (*callback)(ErrorCode::INTERNAL_ERROR,
                        std::string("导出采购单失败: ") + e.what(),
                        Json::Value());
        }
    }
    else if (table == "sales_orders")
    {
        // 销售单导出：主表 + 客户名 + 明细
        try
        {
            auto rows = clientPtr->execSqlSync(
                "SELECT so.id, so.order_no, so.total_amount, so.status, "
                "so.remark, so.created_at, c.name AS partner_name "
                "FROM sales_orders so "
                "LEFT JOIN customers c ON c.id = so.customer_id "
                "ORDER BY so.id DESC");
            auto items = clientPtr->execSqlSync(
                "SELECT i.order_id, p.name AS product_name, p.sku, i.quantity, "
                "i.price, i.amount "
                "FROM sales_order_items i "
                "LEFT JOIN products p ON p.id = i.product_id "
                "ORDER BY i.order_id, i.id");

            std::map<int64_t, std::vector<std::vector<std::string>>> itemMap;
            for (const auto &r : items)
            {
                int64_t oid = (int64_t)r["order_id"].as<uint64_t>();
                std::vector<std::string> line;
                line.push_back(r["product_name"].isNull() ? "" : r["product_name"].as<std::string>());
                line.push_back(r["sku"].isNull() ? "" : r["sku"].as<std::string>());
                line.push_back(r["quantity"].as<std::string>());
                line.push_back(r["price"].as<std::string>());
                line.push_back(r["amount"].as<std::string>());
                itemMap[oid].push_back(line);
            }

            std::ostringstream oss;
            oss << "单号,类型,客户,商品名,SKU,数量,单价,小计,总金额,状态,备注,创建时间\n";
            for (const auto &r : rows)
            {
                int64_t oid = (int64_t)r["id"].as<uint64_t>();
                std::string orderNo = r["order_no"].as<std::string>();
                std::string partner = r["partner_name"].isNull() ? "" : r["partner_name"].as<std::string>();
                std::string total = r["total_amount"].as<std::string>();
                std::string status = r["status"].as<std::string>();
                std::string remark = r["remark"].as<std::string>();
                std::string createdAt = r["created_at"].as<std::string>();

                auto it = itemMap.find(oid);
                if (it == itemMap.end() || it->second.empty())
                {
                    oss << csvEscape(orderNo) << ",销售," << csvEscape(partner)
                        << ",,,,,," << total << "," << status << ","
                        << csvEscape(remark) << "," << createdAt << "\n";
                }
                else
                {
                    for (const auto &line : it->second)
                    {
                        oss << csvEscape(orderNo) << ",销售," << csvEscape(partner)
                            << "," << csvEscape(line[0]) << "," << csvEscape(line[1])
                            << "," << line[2] << "," << line[3] << "," << line[4]
                            << "," << total << "," << status << ","
                            << csvEscape(remark) << "," << createdAt << "\n";
                    }
                }
            }
            Json::Value data;
            data["csv"] = oss.str();
            (*callback)(ErrorCode::OK, "success", data);
        }
        catch (const std::exception &e)
        {
            (*callback)(ErrorCode::INTERNAL_ERROR,
                        std::string("导出销售单失败: ") + e.what(),
                        Json::Value());
        }
    }
    else if (table == "sales_returns")
    {
        // 销售售后单导出（退货/换货）：主表 + 客户名 + 明细方向
        try
        {
            auto rows = clientPtr->execSqlSync(
                "SELECT sr.id, sr.return_no, sr.return_type, sr.total_amount, "
                "sr.status, sr.remark, sr.created_at, c.name AS partner_name, "
                "u.real_name AS applicant "
                "FROM sales_returns sr "
                "LEFT JOIN customers c ON c.id = sr.customer_id "
                "LEFT JOIN user_directory u ON u.id = sr.created_by "
                "ORDER BY sr.id DESC");
            auto items = clientPtr->execSqlSync(
                "SELECT i.return_id, i.direction, p.name AS product_name, "
                "p.sku, i.quantity, i.price, i.amount "
                "FROM sales_return_items i "
                "LEFT JOIN products p ON p.id = i.product_id "
                "ORDER BY i.return_id, i.id");

            std::map<int64_t, std::vector<std::vector<std::string>>> itemMap;
            for (const auto &r : items)
            {
                int64_t rid = (int64_t)r["return_id"].as<uint64_t>();
                std::vector<std::string> line;
                line.push_back(r["direction"].as<std::string>());
                line.push_back(r["product_name"].isNull() ? "" : r["product_name"].as<std::string>());
                line.push_back(r["sku"].isNull() ? "" : r["sku"].as<std::string>());
                line.push_back(r["quantity"].as<std::string>());
                line.push_back(r["price"].as<std::string>());
                line.push_back(r["amount"].as<std::string>());
                itemMap[rid].push_back(line);
            }

            std::ostringstream oss;
            oss << "单号,类型,客户,商品名,SKU,方向,数量,单价,小计,售后金额,状态,申请人,备注,创建时间\n";
            for (const auto &r : rows)
            {
                int64_t rid = (int64_t)r["id"].as<uint64_t>();
                std::string orderNo = r["return_no"].as<std::string>();
                std::string rtype = r["return_type"].isNull() ? "return" : r["return_type"].as<std::string>();
                std::string typeText = (rtype == "exchange") ? "换货" : "退货";
                std::string partner = r["partner_name"].isNull() ? "" : r["partner_name"].as<std::string>();
                std::string total = r["total_amount"].as<std::string>();
                std::string status = r["status"].as<std::string>();
                std::string remark = r["remark"].as<std::string>();
                std::string applicant = r["applicant"].isNull() ? "" : r["applicant"].as<std::string>();
                std::string createdAt = r["created_at"].as<std::string>();

                auto it = itemMap.find(rid);
                if (it == itemMap.end() || it->second.empty())
                {
                    oss << csvEscape(orderNo) << "," << typeText << ","
                        << csvEscape(partner) << ",,,,," << total << ","
                        << status << "," << csvEscape(applicant) << ","
                        << csvEscape(remark) << "," << createdAt << "\n";
                }
                else
                {
                    for (const auto &line : it->second)
                    {
                        std::string dirText = (line[0] == "replace") ? "换出" : "退入";
                        oss << csvEscape(orderNo) << "," << typeText << ","
                            << csvEscape(partner) << "," << csvEscape(line[1])
                            << "," << csvEscape(line[2]) << "," << dirText << ","
                            << line[3] << "," << line[4] << "," << line[5]
                            << "," << total << "," << status << ","
                            << csvEscape(applicant) << "," << csvEscape(remark)
                            << "," << createdAt << "\n";
                    }
                }
            }
            Json::Value data;
            data["csv"] = oss.str();
            (*callback)(ErrorCode::OK, "success", data);
        }
        catch (const std::exception &e)
        {
            (*callback)(ErrorCode::INTERNAL_ERROR,
                        std::string("导出售后单失败: ") + e.what(),
                        Json::Value());
        }
    }
    else if (table == "purchase_returns")
    {
        // 采购退货单导出：主表 + 供应商名 + 明细
        try
        {
            auto rows = clientPtr->execSqlSync(
                "SELECT pr.id, pr.return_no, pr.total_amount, pr.status, "
                "pr.remark, pr.created_at, s.name AS partner_name, "
                "u.real_name AS applicant "
                "FROM purchase_returns pr "
                "LEFT JOIN suppliers s ON s.id = pr.supplier_id "
                "LEFT JOIN user_directory u ON u.id = pr.created_by "
                "ORDER BY pr.id DESC");
            auto items = clientPtr->execSqlSync(
                "SELECT i.return_id, p.name AS product_name, p.sku, "
                "i.quantity, i.price, i.amount "
                "FROM purchase_return_items i "
                "LEFT JOIN products p ON p.id = i.product_id "
                "ORDER BY i.return_id, i.id");

            std::map<int64_t, std::vector<std::vector<std::string>>> itemMap;
            for (const auto &r : items)
            {
                int64_t rid = (int64_t)r["return_id"].as<uint64_t>();
                std::vector<std::string> line;
                line.push_back(r["product_name"].isNull() ? "" : r["product_name"].as<std::string>());
                line.push_back(r["sku"].isNull() ? "" : r["sku"].as<std::string>());
                line.push_back(r["quantity"].as<std::string>());
                line.push_back(r["price"].as<std::string>());
                line.push_back(r["amount"].as<std::string>());
                itemMap[rid].push_back(line);
            }

            std::ostringstream oss;
            oss << "单号,供应商,商品名,SKU,数量,单价,小计,退货金额,状态,申请人,备注,创建时间\n";
            for (const auto &r : rows)
            {
                int64_t rid = (int64_t)r["id"].as<uint64_t>();
                std::string orderNo = r["return_no"].as<std::string>();
                std::string partner = r["partner_name"].isNull() ? "" : r["partner_name"].as<std::string>();
                std::string total = r["total_amount"].as<std::string>();
                std::string status = r["status"].as<std::string>();
                std::string remark = r["remark"].as<std::string>();
                std::string applicant = r["applicant"].isNull() ? "" : r["applicant"].as<std::string>();
                std::string createdAt = r["created_at"].as<std::string>();

                auto it = itemMap.find(rid);
                if (it == itemMap.end() || it->second.empty())
                {
                    oss << csvEscape(orderNo) << "," << csvEscape(partner)
                        << ",,,,,," << total << "," << status << ","
                        << csvEscape(applicant) << "," << csvEscape(remark)
                        << "," << createdAt << "\n";
                }
                else
                {
                    for (const auto &line : it->second)
                    {
                        oss << csvEscape(orderNo) << "," << csvEscape(partner)
                            << "," << csvEscape(line[0]) << "," << csvEscape(line[1])
                            << "," << line[2] << "," << line[3] << "," << line[4]
                            << "," << total << "," << status << ","
                            << csvEscape(applicant) << "," << csvEscape(remark)
                            << "," << createdAt << "\n";
                    }
                }
            }
            Json::Value data;
            data["csv"] = oss.str();
            (*callback)(ErrorCode::OK, "success", data);
        }
        catch (const std::exception &e)
        {
            (*callback)(ErrorCode::INTERNAL_ERROR,
                        std::string("导出采购退货单失败: ") + e.what(),
                        Json::Value());
        }
    }
    else
    {
        (*callback)(ErrorCode::BAD_REQUEST, "不支持的表类型", Json::Value());
    }
}

void ExportImportService::importCsv(const std::string &table,
                                    const std::string &csv,
                                    ExportCallback &&cb,int64_t operatorId)
{
    if(table=="suppliers" || table=="customers") { importPartners(table,csv,operatorId,std::move(cb));return; }
    auto callback = std::make_shared<ExportCallback>(std::move(cb));
    auto clientPtr = app().getDbClient("default");

    // 按行拆分
    std::vector<std::string> lines;
    std::string cur;
    std::istringstream iss(csv);
    while (std::getline(iss, cur))
    {
        // 去掉 \r
        if (!cur.empty() && cur.back() == '\r')
            cur.pop_back();
        if (!cur.empty())
            lines.push_back(cur);
    }
    if (lines.empty())
    {
        (*callback)(ErrorCode::BAD_REQUEST, "CSV 内容为空", Json::Value());
        return;
    }

    // 第一行是表头，跳过
    std::vector<std::string> header = parseCsvLine(lines[0]);

    int success = 0;
    int failed = 0;
    std::string errMsg;

    if (table == "products")
    {
        for (size_t i = 1; i < lines.size(); i++)
        {
            auto f = parseCsvLine(lines[i]);
            if (f.size() < 9)
            {
                failed++;
                continue;
            }
            try
            {
                int64_t categoryId = std::stoll(f[2]);
                int status = std::stoi(f[7]);

                // 用原生 SQL 同步插入（导入场景）
                std::string sql =
                    "INSERT INTO products (sku, name, category_id, unit, spec, "
                    "price, cost_price, status, description) VALUES (?, ?, "
                    "?, ?, ?, ?, ?, ?, ?)";
                clientPtr->execSqlSync(
                    sql, f[0], f[1], categoryId, f[3], f[4], f[5], f[6],
                    status, f[8]);
                success++;
            }
            catch (const std::exception &e)
            {
                if (errMsg.empty())
                    errMsg = e.what();
                failed++;
            }
        }
    }
    else
    {
        (*callback)(ErrorCode::BAD_REQUEST, "暂仅支持导入商品表", Json::Value());
        return;
    }

    Json::Value data;
    data["success"] = success;
    data["failed"] = failed;
    if (!errMsg.empty())
        data["error"] = errMsg;
    (*callback)(ErrorCode::OK, "导入完成", data);
}

}  // namespace erp
