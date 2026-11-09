#pragma once

#include <string>
#include <functional>
#include <json/json.h>
#include <drogon/HttpTypes.h>

namespace erp
{
// 认证服务：登录、用户查询
// 结果通过回调返回，回调参数 (code, message, data)
using AuthCallback = std::function<void(int, const std::string &, const Json::Value &)>;

class AuthService
{
  public:
    // 登录：校验用户名密码，成功返回 JWT token 与用户信息
    static void login(const std::string &username,
                      const std::string &password,
                      AuthCallback &&cb);

    // 根据 user_id 查询用户基本信息（含角色）
    static void getUserInfo(int64_t userId, AuthCallback &&cb);
};

}  // namespace erp
