#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>

namespace erp
{
// JWT 认证中间件：校验 Authorization: Bearer <token>
// 校验通过后将 user_id / username 注入 request attributes
// 业务控制器可通过 req->getAttributes()->get<std::string>("user_id") 获取当前登录用户
class JwtAuthMiddleware : public drogon::HttpMiddleware<JwtAuthMiddleware>
{
  public:
    JwtAuthMiddleware() = default;

    void invoke(const drogon::HttpRequestPtr &req,
                drogon::MiddlewareNextCallback &&nextCb,
                drogon::MiddlewareCallback &&mcb) override;
};

}  // namespace erp
