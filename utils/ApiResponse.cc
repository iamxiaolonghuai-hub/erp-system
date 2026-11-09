#include "ApiResponse.h"

using namespace drogon;

namespace erp
{

HttpResponsePtr ApiResponse::make(int code,
                                  const std::string &message,
                                  const Json::Value &data,
                                  HttpStatusCode httpStatus)
{
    Json::Value root;
    root["code"] = code;
    root["message"] = message;
    root["data"] = data;
    auto resp = HttpResponse::newHttpJsonResponse(root);
    resp->setStatusCode(httpStatus);
    return resp;
}

HttpResponsePtr ApiResponse::ok(const Json::Value &data)
{
    return make(ErrorCode::OK, "success", data, k200OK);
}

HttpResponsePtr ApiResponse::ok(Json::Value &&data)
{
    return make(ErrorCode::OK, "success", std::move(data), k200OK);
}

HttpResponsePtr ApiResponse::ok(const std::string &message, const Json::Value &data)
{
    return make(ErrorCode::OK, message, data, k200OK);
}

HttpResponsePtr ApiResponse::error(int code,
                                   const std::string &message,
                                   HttpStatusCode httpStatus)
{
    return make(code, message, Json::Value(Json::nullValue), httpStatus);
}

}  // namespace erp
