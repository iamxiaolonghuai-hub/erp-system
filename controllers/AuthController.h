#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 认证控制器：登录 / 登出 / 当前用户信息
class AuthController : public drogon::HttpController<AuthController>
{
  public:
    METHOD_LIST_BEGIN
    // 登录
    ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
    // 当前用户信息（需登录）
    ADD_METHOD_TO(AuthController::me, "/api/auth/me", drogon::Get,
                  "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void me(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
