#include "../services/BusinessSupport.h"
#include "PurchaseController.h"
#include "../services/ApprovalAssignment.h"

#include "../services/PurchaseService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

// 从 JWT 中间件注入的 attributes 提取操作人 ID
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

void PurchaseController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t supplierId = (*json).get("supplier_id", 0).asInt64();
    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();
    std::string remark = (*json).get("remark", "").asString();
    Json::Value items = (*json).get("items", Json::arrayValue);

    int64_t operatorId = getOperatorId(req);

    int64_t assignedApproverId=0;
    try { assignedApproverId=selectedApprover(req); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));return; }
    PurchaseService::createPurchaseOrder(
        supplierId, warehouseId, remark, operatorId, items,
        assignedApproverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req));
}

void PurchaseController::approve(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t orderId = (*json).get("order_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();

    int64_t approverId = getOperatorId(req);
    try { checkAssignedApprover("purchase_orders",(*json).get("order_id",0).asInt64(),approverId); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::FORBIDDEN,e.what()));return; }


    PurchaseService::approvePurchaseOrder(
        orderId, approve, approverId, warehouseId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void PurchaseController::warehouseInbound(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t orderId = (*json).get("order_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();
    std::string remark = (*json).get("remark", "").asString();

    int64_t operatorId = getOperatorId(req);

    PurchaseService::warehouseInbound(
        orderId, approve, operatorId, warehouseId, remark,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void PurchaseController::resubmit(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t orderId = (*json).get("order_id", 0).asInt64();
    Json::Value items = (*json).get("items", Json::arrayValue);

    int64_t operatorId = getOperatorId(req);

    PurchaseService::resubmitPurchaseOrder(
        orderId, items, operatorId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

}  // namespace erp
