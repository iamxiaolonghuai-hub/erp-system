#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 用户管理控制器：创建用户（密码哈希）、重置密码
class UserController : public drogon::HttpController<UserController>
{
  public:
    METHOD_LIST_BEGIN
    // 创建用户（自动哈希密码）
    ADD_METHOD_TO(UserController::create, "/api/users/create",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 重置密码
    ADD_METHOD_TO(UserController::resetPassword, "/api/users/reset_password",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 修改用户状态（领导可修改本部门员工；管理员可修改任意用户）
    ADD_METHOD_TO(UserController::updateStatus, "/api/users/status",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void resetPassword(const drogon::HttpRequestPtr &req,
                       std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void updateStatus(const drogon::HttpRequestPtr &req,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
