#include "../services/BusinessSupport.h"
#include "FinanceController.h"

#include "../services/FinanceService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

static int64_t getOperatorId(const HttpRequestPtr &req)
{
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    if (userIdStr.empty())
        return 0;
    try
    {
        return std::stoll(userIdStr);
    }
    catch (...)
    {
        return 0;
    }
}

void FinanceController::addPayment(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t accountId = (*json).get("account_id", 0).asInt64();
    double amount = (*json).get("amount", 0.0).asDouble();
    std::string payMethod = (*json).get("pay_method", "").asString();
    std::string payDate = (*json).get("pay_date", "").asString();
    std::string remark = (*json).get("remark", "").asString();

    int64_t operatorId = getOperatorId(req);

    FinanceService::addPayment(
        accountId, amount, payMethod, payDate, remark, operatorId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req));
}

}  // namespace erp
