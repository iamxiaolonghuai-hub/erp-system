#include "../services/BusinessSupport.h"
#include "StockCheckController.h"

#include "../services/StockCheckService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

static int64_t getOperatorId(const HttpRequestPtr &req)
{
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    if (userIdStr.empty())
        return 0;
    try
    {
        return std::stoll(userIdStr);
    }
    catch (...)
    {
        return 0;
    }
}

void StockCheckController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();
    std::string remark = (*json).get("remark", "").asString();
    Json::Value items = (*json).get("items", Json::arrayValue);
    int64_t operatorId = getOperatorId(req);

    StockCheckService::createStockCheck(
        warehouseId, remark, operatorId, items,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req));
}

void StockCheckController::complete(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t checkId = (*json).get("check_id", 0).asInt64();
    int64_t operatorId = getOperatorId(req);

    StockCheckService::completeStockCheck(
        checkId, operatorId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

}  // namespace erp
