#include "ReturnWorkflow.h"
#include "BusinessSupport.h"
#include "ReturnService.h"

#include "../utils/ApiResponse.h"

#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>

#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace drogon;
using namespace drogon::orm;

namespace erp
{

static std::string genReturnNo(const std::string &prefix)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch())
                  .count();
    std::ostringstream oss;
    oss << prefix << ms;
    return oss.str();
}

static std::string fmtAmount(double v)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << v;
    return oss.str();
}

static std::string jsonStr(const Json::Value &v, const std::string &key)
{
    return v.isMember(key) && !v[key].isNull() ? v[key].asString() : "";
}

// ===================== 采购退货 =====================

void ReturnService::createPurchaseReturn(int64_t orderId,
                                         int64_t warehouseId,
                                         const std::string &remark,
                                         const std::string &logisticsCompany,
                                         const std::string &logisticsNo,
                                         int64_t operatorId,
                                         const Json::Value &items,
                                         int64_t assignedApproverId,
                                  ReturnCallback &&cb, const business::RequestIdentity &request)
{ business::createReturn(true,orderId,warehouseId,remark,logisticsCompany,logisticsNo,operatorId,"return",items,assignedApproverId,std::move(cb),request); }

void ReturnService::approvePurchaseReturn(int64_t returnId,
                                          bool approve,
                                          int64_t approverId,
                                          ReturnCallback &&cb)
{ business::approveReturn(true,returnId,approve,approverId,std::move(cb)); }

void ReturnService::warehousePurchaseReturn(int64_t returnId,
                                            bool approve,
                                            int64_t operatorId,
                                            const std::string &remark,
                                            const std::string &logisticsCompany,
                                            const std::string &logisticsNo,
                                            ReturnCallback &&cb)
{ business::warehouseReturn(true,returnId,approve,operatorId,remark,logisticsCompany,logisticsNo,std::move(cb)); }

// ===================== 销售退货 =====================

void ReturnService::createSalesReturn(int64_t orderId,
                                      int64_t warehouseId,
                                      const std::string &remark,
                                      const std::string &logisticsCompany,
                                      const std::string &logisticsNo,
                                      int64_t operatorId,
                                      const std::string &returnType,
                                      const Json::Value &items,
                                      int64_t assignedApproverId,
                                  ReturnCallback &&cb, const business::RequestIdentity &request)
{ business::createReturn(false,orderId,warehouseId,remark,logisticsCompany,logisticsNo,operatorId,returnType,items,assignedApproverId,std::move(cb),request); }

void ReturnService::approveSalesReturn(int64_t returnId,
                                       bool approve,
                                       int64_t approverId,
                                       ReturnCallback &&cb)
{ business::approveReturn(false,returnId,approve,approverId,std::move(cb)); }

void ReturnService::warehouseSalesReturn(int64_t returnId,
                                         bool approve,
                                         int64_t operatorId,
                                         const std::string &remark,
                                         const std::string &logisticsCompany,
                                         const std::string &logisticsNo,
                                         ReturnCallback &&cb)
{ business::warehouseReturn(false,returnId,approve,operatorId,remark,logisticsCompany,logisticsNo,std::move(cb)); }

void ReturnService::listPurchaseReturns(ReturnCallback &&cb)
{
    listPurchaseReturns(0, std::move(cb));
}

void ReturnService::listPurchaseReturns(int64_t operatorId,
                                        ReturnCallback &&cb)
{
    auto clientPtr = app().getDbClient("default");
    try
    {
        std::string sql =
            "SELECT pr.id, pr.return_no, pr.order_id, source_order.supplier_id, "
            "pr.warehouse_id, pr.total_amount, pr.status, pr.remark, "
            "pr.logistics_company, pr.logistics_no, "
            "pr.created_by, pr.approved_by, pr.warehouse_confirmed_by, "
            "pr.created_at, s.name AS "
            "supplier_name, u.real_name AS created_by_name "
            "FROM purchase_returns pr "
            "LEFT JOIN purchase_orders source_order ON source_order.id=pr.order_id LEFT JOIN suppliers s ON source_order.supplier_id = s.id "
            "LEFT JOIN user_directory u ON u.id = pr.created_by ";
        if (operatorId > 0)
            sql += "WHERE (pr.created_by = ? OR pr.approved_by = ? OR "
                   "pr.warehouse_confirmed_by = ?) ";
        sql += "ORDER BY pr.id DESC";
        Result rows = operatorId > 0
                          ? clientPtr->execSqlSync(sql, operatorId,
                                                    operatorId, operatorId)
                          : clientPtr->execSqlSync(sql);
        Json::Value arr(Json::arrayValue);
        for (const auto &r : rows)
        {
            Json::Value o;
            o["id"] = (Json::Int64)r["id"].as<int64_t>();
            o["return_no"] = r["return_no"].as<std::string>();
            o["order_id"] = (Json::Int64)r["order_id"].as<int64_t>();
            o["supplier_id"] = (Json::Int64)r["supplier_id"].as<int64_t>();
            o["warehouse_id"] = (Json::Int64)r["warehouse_id"].as<int64_t>();
            o["total_amount"] = r["total_amount"].as<std::string>();
            o["status"] = r["status"].as<int>();
            o["remark"] = r["remark"].as<std::string>();
            o["logistics_company"] = r["logistics_company"].as<std::string>();
            o["logistics_no"] = r["logistics_no"].as<std::string>();
            o["supplier_name"] = r["supplier_name"].as<std::string>();
            o["created_by"] =
                r["created_by"].isNull()
                    ? Json::Value()
                    : Json::Value((Json::Int64)r["created_by"].as<int64_t>());
            o["created_by_name"] =
                r["created_by_name"].isNull()
                    ? Json::Value()
                    : r["created_by_name"].as<std::string>();
            o["created_at"] = r["created_at"].as<std::string>();
            arr.append(o);
        }
        cb(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        cb(ErrorCode::INTERNAL_ERROR,
           std::string("查询采购退货单失败: ") + e.what(), Json::Value());
    }
}

void ReturnService::listSalesReturns(ReturnCallback &&cb)
{
    listSalesReturns(0, std::move(cb));
}

void ReturnService::listSalesReturns(int64_t operatorId,
                                     ReturnCallback &&cb)
{
    auto clientPtr = app().getDbClient("default");
    try
    {
        std::string sql =
            "SELECT sr.id, sr.return_no, sr.return_type, sr.order_id, "
            "source_order.customer_id, "
            "sr.warehouse_id, sr.total_amount, sr.status, sr.remark, "
            "sr.logistics_company, sr.logistics_no, "
            "sr.created_by, sr.approved_by, sr.warehouse_confirmed_by, "
            "sr.created_at, c.name AS "
            "customer_name, u.real_name AS created_by_name "
            "FROM sales_returns sr "
            "LEFT JOIN sales_orders source_order ON source_order.id=sr.order_id LEFT JOIN customers c ON source_order.customer_id = c.id "
            "LEFT JOIN user_directory u ON u.id = sr.created_by ";
        if (operatorId > 0)
            sql += "WHERE (sr.created_by = ? OR sr.approved_by = ? OR "
                   "sr.warehouse_confirmed_by = ?) ";
        sql += "ORDER BY sr.id DESC";
        Result rows = operatorId > 0
                          ? clientPtr->execSqlSync(sql, operatorId,
                                                    operatorId, operatorId)
                          : clientPtr->execSqlSync(sql);
        Json::Value arr(Json::arrayValue);
        for (const auto &r : rows)
        {
            Json::Value o;
            o["id"] = (Json::Int64)r["id"].as<int64_t>();
            o["return_no"] = r["return_no"].as<std::string>();
            o["return_type"] = r["return_type"].isNull()
                                   ? "return"
                                   : r["return_type"].as<std::string>();
            o["order_id"] = (Json::Int64)r["order_id"].as<int64_t>();
            o["customer_id"] = (Json::Int64)r["customer_id"].as<int64_t>();
            o["warehouse_id"] = (Json::Int64)r["warehouse_id"].as<int64_t>();
            o["total_amount"] = r["total_amount"].as<std::string>();
            o["status"] = r["status"].as<int>();
            o["remark"] = r["remark"].as<std::string>();
            o["logistics_company"] = r["logistics_company"].as<std::string>();
            o["logistics_no"] = r["logistics_no"].as<std::string>();
            o["customer_name"] = r["customer_name"].as<std::string>();
            o["created_by"] =
                r["created_by"].isNull()
                    ? Json::Value()
                    : Json::Value((Json::Int64)r["created_by"].as<int64_t>());
            o["created_by_name"] =
                r["created_by_name"].isNull()
                    ? Json::Value()
                    : r["created_by_name"].as<std::string>();
            o["created_at"] = r["created_at"].as<std::string>();
            arr.append(o);
        }
        cb(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        cb(ErrorCode::INTERNAL_ERROR,
           std::string("查询销售退货单失败: ") + e.what(), Json::Value());
    }
}

void ReturnService::listPurchaseReturnItems(int64_t returnId, ReturnCallback &&cb)
{
    auto clientPtr = app().getDbClient("default");
    try
    {
        auto rows = clientPtr->execSqlSync(
            "SELECT ri.id, ri.product_id, ri.quantity, ri.price, ri.amount, "
            "p.name AS product_name, p.sku "
            "FROM purchase_return_items ri LEFT JOIN products p ON "
            "ri.product_id = p.id WHERE ri.return_id = ?",
            returnId);
        Json::Value arr(Json::arrayValue);
        for (const auto &r : rows)
        {
            Json::Value o;
            o["id"] = (Json::Int64)r["id"].as<int64_t>();
            o["product_id"] = (Json::Int64)r["product_id"].as<int64_t>();
            o["quantity"] = r["quantity"].as<int>();
            o["price"] = r["price"].as<std::string>();
            o["amount"] = r["amount"].as<std::string>();
            o["product_name"] = r["product_name"].as<std::string>();
            o["sku"] = r["sku"].as<std::string>();
            arr.append(o);
        }
        cb(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        cb(ErrorCode::INTERNAL_ERROR,
           std::string("查询采购退货明细失败: ") + e.what(), Json::Value());
    }
}

void ReturnService::listSalesReturnItems(int64_t returnId, ReturnCallback &&cb)
{
    auto clientPtr = app().getDbClient("default");
    try
    {
        auto rows = clientPtr->execSqlSync(
            "SELECT ri.id, ri.direction, ri.product_id, ri.quantity, "
            "ri.price, ri.amount, "
            "p.name AS product_name, p.sku "
            "FROM sales_return_items ri LEFT JOIN products p ON ri.product_id "
            "= p.id WHERE ri.return_id = ?",
            returnId);
        Json::Value arr(Json::arrayValue);
        for (const auto &r : rows)
        {
            Json::Value o;
            o["id"] = (Json::Int64)r["id"].as<int64_t>();
            o["direction"] = r["direction"].isNull()
                                 ? "return"
                                 : r["direction"].as<std::string>();
            o["product_id"] = (Json::Int64)r["product_id"].as<int64_t>();
            o["quantity"] = r["quantity"].as<int>();
            o["price"] = r["price"].as<std::string>();
            o["amount"] = r["amount"].as<std::string>();
            o["product_name"] = r["product_name"].as<std::string>();
            o["sku"] = r["sku"].as<std::string>();
            arr.append(o);
        }
        cb(ErrorCode::OK, "success", arr);
    }
    catch (const std::exception &e)
    {
        cb(ErrorCode::INTERNAL_ERROR,
           std::string("查询销售退换货明细失败: ") + e.what(), Json::Value());
    }
}

}  // namespace erp
