#include "ReportController.h"

#include "../services/ReportService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

void ReportController::dashboard(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    ReportService::dashboard(
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void ReportController::purchaseSummary(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    ReportService::purchaseSummary(
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void ReportController::salesSummary(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    ReportService::salesSummary(
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void ReportController::inventorySummary(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    ReportService::inventorySummary(
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

}  // namespace erp
