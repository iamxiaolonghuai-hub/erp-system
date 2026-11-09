#include "ExportImportController.h"

#include "../services/ExportImportService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

void ExportImportController::doExport(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &table)
{
    ExportImportService::exportCsv(
        table,
        [callback, table](int code, const std::string &msg,
                          const Json::Value &data) {
            if (code != ErrorCode::OK)
            {
                callback(ApiResponse::error(code, msg));
                return;
            }
            // 返回 CSV 文本，设置下载头
            auto resp = HttpResponse::newHttpResponse();
            resp->setStatusCode(k200OK);
            resp->setContentTypeCode(CT_TEXT_PLAIN);
            resp->setContentTypeString("text/csv; charset=utf-8");
            resp->addHeader("Content-Disposition",
                            "attachment; filename=\"" + table + ".csv\"");
            // 加 UTF-8 BOM，避免 Excel 打开中文乱码
            resp->setBody(std::string("\xEF\xBB\xBF") + data["csv"].asString());
            callback(resp);
        });
}

void ExportImportController::doImport(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    const std::string &table)
{
    // 支持两种请求：JSON {csv: "..."} 或纯文本 CSV body
    std::string csv;
    auto json = req->getJsonObject();
    if (json && (*json).isMember("csv"))
    {
        csv = (*json)["csv"].asString();
    }
    else
    {
        csv = std::string(req->getBody());
    }

    if (csv.empty())
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "CSV 内容为空"));
        return;
    }

    ExportImportService::importCsv(
        table, csv,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        },std::stoll(req->getAttributes()->get<std::string>("user_id")));
}

}  // namespace erp
