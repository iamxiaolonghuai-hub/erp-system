#include "StockController.h"

#include "../services/StockService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

static bool parseStockReq(const HttpRequestPtr &req,
                          int64_t &productId,
                          int64_t &warehouseId,
                          int &qty,
                          std::string &bizType,
                          std::string &bizNo,
                          int64_t &operatorId,
                          std::string &err)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        err = "请求体不是合法 JSON";
        return false;
    }
    productId = (*json).get("product_id", 0).asInt64();
    warehouseId = (*json).get("warehouse_id", 0).asInt64();
    qty = (*json).get("qty", 0).asInt();
    bizType = (*json).get("biz_type", "").asString();
    bizNo = (*json).get("biz_no", "").asString();

    if (productId <= 0 || warehouseId <= 0 || qty <= 0)
    {
        err = "product_id、warehouse_id、qty 为必填且必须为正数";
        return false;
    }

    // 从 JWT 过滤器注入的 attributes 获取操作人
    operatorId = 0;
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    if (!userIdStr.empty())
    {
        try
        {
            operatorId = std::stoll(userIdStr);
        }
        catch (...)
        {
            operatorId = 0;
        }
    }
    return true;
}

void StockController::stockIn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    int64_t productId, warehouseId, operatorId;
    int qty;
    std::string bizType, bizNo, err;
    if (!parseStockReq(req, productId, warehouseId, qty, bizType, bizNo,
                       operatorId, err))
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, err));
        return;
    }

    StockService::stockIn(
        productId, warehouseId, qty, bizType, bizNo, operatorId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void StockController::stockOut(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    int64_t productId, warehouseId, operatorId;
    int qty;
    std::string bizType, bizNo, err;
    if (!parseStockReq(req, productId, warehouseId, qty, bizType, bizNo,
                       operatorId, err))
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, err));
        return;
    }

    StockService::stockOut(
        productId, warehouseId, qty, bizType, bizNo, operatorId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

}  // namespace erp
