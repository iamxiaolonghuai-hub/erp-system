#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 申报控制器：供应商/商品申报（员工申报 + 领导审批 + 列表）
class ApplicationController
    : public drogon::HttpController<ApplicationController>
{
  public:
    METHOD_LIST_BEGIN
    // 创建申报单（员工，销售部）
    ADD_METHOD_TO(ApplicationController::create, "/api/application/create",
                  drogon::Post, "erp::JwtAuthMiddleware");
    // 审批申报单（销售部领导）
    ADD_METHOD_TO(ApplicationController::approve, "/api/application/approve",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 申报单列表（员工看自己的/全部，领导看本部门）
    ADD_METHOD_TO(ApplicationController::list, "/api/application/list",
                  drogon::Get, "erp::JwtAuthMiddleware");
    // 被驳回的申报单修改后重新提交
    ADD_METHOD_TO(ApplicationController::resubmit, "/api/application/resubmit",
                  drogon::Post, "erp::JwtAuthMiddleware");
    METHOD_LIST_END

    void create(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void approve(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void list(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void resubmit(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
