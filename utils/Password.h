#pragma once

#include <string>

namespace erp
{
// 密码哈希工具：加盐 SHA256（格式: hex_salt$hex_hash），基于 OpenSSL
class Password
{
  public:
    // 生成密码哈希（随机盐）
    static std::string hash(const std::string &plain);

    // 校验明文密码与哈希是否匹配
    static bool verify(const std::string &plain, const std::string &hashed);

  private:
    static std::string sha256(const std::string &data);
    static std::string randomSaltHex(size_t bytes = 16);
};
}  // namespace erp
