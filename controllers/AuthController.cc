#include "AuthController.h"

#include "../services/AuthService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

void AuthController::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    std::string username = (*json).get("username", "").asString();
    std::string password = (*json).get("password", "").asString();
    if (username.empty() || password.empty())
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "用户名和密码不能为空"));
        return;
    }

    AuthService::login(
        username, password,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg, k401Unauthorized));
        });
}

void AuthController::me(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    if (userIdStr.empty())
    {
        callback(ApiResponse::error(ErrorCode::UNAUTHORIZED, "未登录", k401Unauthorized));
        return;
    }
    int64_t userId = std::stoll(userIdStr);
    AuthService::getUserInfo(
        userId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

}  // namespace erp
