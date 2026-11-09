#pragma once

#include <string>
#include <map>
#include <cstdint>

namespace erp
{
// 轻量级 JWT 实现（HS256），基于 OpenSSL HMAC-SHA256
// 仅依赖 OpenSSL，无需额外第三方库
class Jwt
{
  public:
    // 生成 token
    // payload 为业务负载(如 user_id, username)，自动加入 iat/exp
    static std::string generate(const std::map<std::string, std::string> &payload,
                                int64_t expireSeconds);

    // 验证并解析 token，返回 payload；失败返回空 map
    static std::map<std::string, std::string> verify(const std::string &token);

    // 设置签名密钥
    static void setSecret(const std::string &secret);

  private:
    static std::string base64UrlEncode(const unsigned char *data, size_t len);
    static std::string base64UrlDecode(const std::string &input);
    static std::string hmacSha256(const std::string &data);
    static std::string &secret();
};

}  // namespace erp
