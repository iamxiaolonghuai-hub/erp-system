#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>

namespace erp
{
// 角色权限中间件：在 JwtAuthMiddleware 之后执行，
// 根据请求路径前缀判断所需角色，拦截无权限的访问。
//
// 角色等级：admin（管理员） > leader（领导，只读） > staff（员工，日常操作）
// 规则：
//   /api/users* /api/roles* /api/permissions* /api/operationlogs*
//   /api/export* /api/import*          -> 仅 admin
//   /api/report* /api/finance*         -> admin 或 leader
//   其余业务接口                          -> 需登录即可（由 JwtAuthMiddleware 保证）
class RoleAuthMiddleware : public drogon::HttpMiddleware<RoleAuthMiddleware>
{
  public:
    RoleAuthMiddleware() = default;

    void invoke(const drogon::HttpRequestPtr &req,
                drogon::MiddlewareNextCallback &&nextCb,
                drogon::MiddlewareCallback &&mcb) override;
};

}  // namespace erp
