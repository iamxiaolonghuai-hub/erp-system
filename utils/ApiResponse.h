#pragma once

#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>
#include <json/json.h>

namespace erp
{
// 统一 JSON 响应格式
// 成功: { "code": 0, "message": "success", "data": {...} }
// 失败: { "code": 非0, "message": "...", "data": null }
class ApiResponse
{
  public:
    static drogon::HttpResponsePtr ok(const Json::Value &data = Json::Value());
    static drogon::HttpResponsePtr ok(Json::Value &&data);
    static drogon::HttpResponsePtr ok(const std::string &message,
                                      const Json::Value &data = Json::Value());
    static drogon::HttpResponsePtr error(int code,
                                         const std::string &message,
                                         drogon::HttpStatusCode httpStatus =
                                             drogon::k400BadRequest);

  private:
    static drogon::HttpResponsePtr make(int code,
                                        const std::string &message,
                                        const Json::Value &data,
                                        drogon::HttpStatusCode httpStatus);
};

// 通用错误码
namespace ErrorCode
{
constexpr int OK = 0;
constexpr int BAD_REQUEST = 40000;       // 参数错误
constexpr int UNAUTHORIZED = 40100;      // 未认证
constexpr int FORBIDDEN = 40300;         // 无权限
constexpr int NOT_FOUND = 40400;         // 资源不存在
constexpr int CONFLICT = 40900;          // 冲突(如唯一键重复)
constexpr int INTERNAL_ERROR = 50000;    // 服务器内部错误
constexpr int STOCK_NOT_ENOUGH = 41001;  // 库存不足
}  // namespace ErrorCode

}  // namespace erp
