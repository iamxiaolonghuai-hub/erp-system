#include "BusinessSupport.h"
#include "../services/DocumentAccess.h"
#include "ReturnController.h"
#include "../services/ApprovalAssignment.h"

#include "../services/ReturnService.h"
#include "../services/ReturnWorkflow.h"
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

void ReturnController::createPurchaseReturn(
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
    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();
    std::string remark = (*json).get("remark", "").asString();
    std::string logisticsCompany = (*json).get("logistics_company", "").asString();
    std::string logisticsNo = (*json).get("logistics_no", "").asString();
    Json::Value items = (*json).get("items", Json::arrayValue);
    int64_t operatorId = getOperatorId(req);

    int64_t assignedApproverId=0;
    try { assignedApproverId=selectedApprover(req); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));return; }
    ReturnService::createPurchaseReturn(
        orderId, warehouseId, remark, logisticsCompany, logisticsNo,
        operatorId, items,
        assignedApproverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req));
}

void ReturnController::approvePurchaseReturn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    int64_t returnId = (*json).get("return_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    int64_t approverId = getOperatorId(req);
    try { checkAssignedApprover("purchase_returns",(*json).get("return_id",0).asInt64(),approverId); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::FORBIDDEN,e.what()));return; }


    ReturnService::approvePurchaseReturn(
        returnId, approve, approverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void ReturnController::warehousePurchaseReturn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    int64_t returnId = (*json).get("return_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    std::string remark = (*json).get("remark", "").asString();
    std::string logisticsCompany = (*json).get("logistics_company", "").asString();
    std::string logisticsNo = (*json).get("logistics_no", "").asString();
    int64_t operatorId = getOperatorId(req);

    business::warehouseReturn(true,
        returnId, approve, operatorId, remark, logisticsCompany, logisticsNo,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, (*json)["serial_items"]);
}

void ReturnController::createSalesReturn(
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
    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();
    std::string remark = (*json).get("remark", "").asString();
    std::string logisticsCompany = (*json).get("logistics_company", "").asString();
    std::string logisticsNo = (*json).get("logistics_no", "").asString();
    std::string returnType = (*json).get("return_type", "return").asString();
    Json::Value items = (*json).get("items", Json::arrayValue);
    int64_t operatorId = getOperatorId(req);

    int64_t assignedApproverId=0;
    try { assignedApproverId=selectedApprover(req); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));return; }
    ReturnService::createSalesReturn(
        orderId, warehouseId, remark, logisticsCompany, logisticsNo,
        operatorId, returnType, items,
        assignedApproverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req));
}

void ReturnController::approveSalesReturn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    int64_t returnId = (*json).get("return_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    int64_t approverId = getOperatorId(req);
    try { checkAssignedApprover("sales_returns",(*json).get("return_id",0).asInt64(),approverId); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::FORBIDDEN,e.what()));return; }


    ReturnService::approveSalesReturn(
        returnId, approve, approverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void ReturnController::warehouseSalesReturn(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    int64_t returnId = (*json).get("return_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    std::string remark = (*json).get("remark", "").asString();
    std::string logisticsCompany = (*json).get("logistics_company", "").asString();
    std::string logisticsNo = (*json).get("logistics_no", "").asString();
    int64_t operatorId = getOperatorId(req);

    business::warehouseReturn(false,
        returnId, approve, operatorId, remark, logisticsCompany, logisticsNo,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, (*json)["serial_items"]);
}

void ReturnController::listPurchaseReturns(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"purchase_returns"); }

void ReturnController::listSalesReturns(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"sales_returns"); }

void ReturnController::listPurchaseReturnItems(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"purchase_return_items"); }

void ReturnController::listSalesReturnItems(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"sales_return_items"); }

}  // namespace erp
