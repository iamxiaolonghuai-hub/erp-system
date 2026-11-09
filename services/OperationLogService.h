#pragma once

#include <string>

namespace erp
{
// 操作日志服务：记录用户操作日志（在鉴权中间件中调用）
class OperationLogService
{
  public:
    // 记录一条操作日志
    // userId 用户ID(0表示未知), username 用户名, method HTTP方法, path 路径, ip 来源IP, statusCode 状态码, costMs 耗时
    static void record(int64_t userId,
                       const std::string &username,
                       const std::string &method,
                       const std::string &path,
                       const std::string &ip,
                       int statusCode,
                       int64_t costMs);
};

}  // namespace erp
