#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 导入导出控制器：CSV 导出/导入
class ExportImportController
    : public drogon::HttpController<ExportImportController>
{
  public:
    METHOD_LIST_BEGIN
    // 导出 CSV（返回文本）
    ADD_METHOD_TO(ExportImportController::doExport, "/api/export/{1}",
                  drogon::Get, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 导入 CSV
    ADD_METHOD_TO(ExportImportController::doImport, "/api/import/{1}",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    METHOD_LIST_END

    void doExport(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  const std::string &table);
    void doImport(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback,
                  const std::string &table);
};

}  // namespace erp
