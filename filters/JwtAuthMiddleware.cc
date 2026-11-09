#include "JwtAuthMiddleware.h"
#include "../utils/Jwt.h"
#include "../utils/ApiResponse.h"
#include "../services/OperationLogService.h"
#include <chrono>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

using namespace drogon;

namespace erp
{

void JwtAuthMiddleware::invoke(const HttpRequestPtr &req,
                              MiddlewareNextCallback &&nextCb,
                              MiddlewareCallback &&mcb)
{
    // 获取 Authorization 头
    std::string auth = req->getHeader("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.size() < prefix.size() ||
        auth.compare(0, prefix.size(), prefix) != 0)
    {
        mcb(ApiResponse::error(ErrorCode::UNAUTHORIZED, "未登录或凭证缺失",
                               k401Unauthorized));
        return;
    }

    std::string token = auth.substr(prefix.size());
    auto payload = Jwt::verify(token);
    if (payload.empty() || payload.count("user_id") == 0)
    {
        mcb(ApiResponse::error(ErrorCode::UNAUTHORIZED, "凭证无效或已过期",
                               k401Unauthorized));
        return;
    }

    std::string userIdStr = payload["user_id"];
    std::string username,roles,department;
    int64_t currentId=0;
    try {
        currentId=std::stoll(userIdStr);
        auto rows=app().getDbClient()->execSqlSync("SELECT u.username,u.department,u.status,COALESCE(GROUP_CONCAT(r.code),'') AS roles FROM user_directory u LEFT JOIN user_roles ur ON ur.user_id=u.id LEFT JOIN roles r ON r.id=ur.role_id WHERE u.id=? GROUP BY u.id",currentId);
        if(rows.empty() || rows[0]["status"].as<int>()!=1) {
            mcb(ApiResponse::error(ErrorCode::UNAUTHORIZED,"账号已停用或不存在",k401Unauthorized));return;
        }
        username=rows[0]["username"].as<std::string>();department=rows[0]["department"].as<std::string>();roles=rows[0]["roles"].as<std::string>();
    } catch(const std::exception &e) {
        LOG_ERROR<<"Authentication lookup failed: "<<e.what();
        mcb(ApiResponse::error(ErrorCode::UNAUTHORIZED,"无法验证当前账号，请重新登录",k401Unauthorized));return;
    }

    // 注入用户信息到 request attributes
    req->getAttributes()->insert("user_id", userIdStr);
    req->getAttributes()->insert("username", username);
    req->getAttributes()->insert("roles", roles);
    req->getAttributes()->insert("department", department);

    // 记录操作日志
    int64_t userId = 0;
    try
    {
        userId = std::stoll(userIdStr);
    }
    catch (...)
    {
        userId = 0;
    }
    std::string ip;
    try
    {
        ip = req->getPeerAddr().toIp();
    }
    catch (...)
    {
        ip = "";
    }
    auto started=std::chrono::steady_clock::now();
    nextCb([req,userId,username,ip,started,mcb=std::move(mcb)](const HttpResponsePtr &response) mutable {
        int ms=static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count());
        OperationLogService::record(userId,username,req->getMethodString(),req->getPath(),ip,static_cast<int>(response->getStatusCode()),ms);
        mcb(response);
    });
}

}  // namespace erp
