#include "BusinessSupport.h"
#include "../services/DocumentAccess.h"
#include "ApplicationController.h"
#include "../services/ApprovalAssignment.h"

#include "../services/ApplicationService.h"
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

void ApplicationController::create(
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
    std::string title = (*json).get("title", "").asString();
    std::string remark = (*json).get("remark", "").asString();
    Json::Value payload = (*json).get("payload", Json::objectValue);

    int64_t applicantId = getOperatorId(req);

    int64_t assignedApproverId=0;
    try { assignedApproverId=selectedApprover(req); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));return; }
    ApplicationService::createApplication(
        type, title, payload, applicantId, remark,
        assignedApproverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req));
}

void ApplicationController::approve(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t applicationId = (*json).get("application_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    std::string remark = (*json).get("remark", "").asString();

    int64_t approverId = getOperatorId(req);
    try { checkAssignedApprover("applications",(*json).get("application_id",0).asInt64(),approverId); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::FORBIDDEN,e.what()));return; }


    ApplicationService::approveApplication(
        applicationId, approve, approverId, remark,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void ApplicationController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"applications"); }

void ApplicationController::resubmit(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t applicationId = (*json).get("application_id", 0).asInt64();
    std::string title = (*json).get("title", "").asString();
    Json::Value payload = (*json).get("payload", Json::objectValue);

    int64_t applicantId = getOperatorId(req);

    ApplicationService::resubmitApplication(
        applicationId, title, payload, applicantId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

}  // namespace erp
