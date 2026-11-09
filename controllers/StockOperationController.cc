#include "BusinessSupport.h"
#include "../services/DocumentAccess.h"
#include "StockOperationController.h"
#include "../services/ApprovalAssignment.h"

#include "../services/StockOperationService.h"
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

void StockOperationController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    std::string type = (*json).get("type", "").asString();
    int64_t productId = (*json).get("product_id", 0).asInt64();
    int64_t fromWh = (*json).get("from_warehouse_id", 0).asInt64();
    int64_t toWh = (*json).get("to_warehouse_id", 0).asInt64();
    int qty = (*json).get("qty", 0).asInt();
    std::string bizType = (*json).get("biz_type", "").asString();
    std::string bizNo = (*json).get("biz_no", "").asString();
    std::string logisticsCompany = (*json).get("logistics_company", "").asString();
    std::string logisticsNo = (*json).get("logistics_no", "").asString();
    std::string serialNos = (*json).get("serial_nos", "").asString();
    std::string remark = (*json).get("remark", "").asString();
    int64_t applicantId = getOperatorId(req);

    int64_t assignedApproverId=0;
    try { assignedApproverId=selectedApprover(req); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));return; }
    StockOperationService::create(
        type, productId, fromWh, toWh, qty, bizType, bizNo, logisticsCompany,
        logisticsNo, serialNos, remark, applicantId,
        assignedApproverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req),*json);
}

void StockOperationController::approve(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    int64_t operationId = (*json).get("operation_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    std::string remark = (*json).get("remark", "").asString();
    int64_t approverId = getOperatorId(req);
    try { checkAssignedApprover("stock_operations",(*json).get("operation_id",0).asInt64(),approverId); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::FORBIDDEN,e.what()));return; }


    StockOperationService::approve(
        operationId, approve, approverId, remark,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void StockOperationController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"stock_operations"); }

}  // namespace erp
