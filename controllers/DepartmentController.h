#pragma once

#include <drogon/HttpController.h>

namespace erp
{
// 部门控制器：部门列表、部门员工、部门仪表盘、员工调度
class DepartmentController
    : public drogon::HttpController<DepartmentController>
{
  public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(DepartmentController::saveDepartment,"/api/departments",drogon::Post,"erp::JwtAuthMiddleware");
    ADD_METHOD_TO(DepartmentController::editDepartment,"/api/departments/{1}",drogon::Put,"erp::JwtAuthMiddleware");
    ADD_METHOD_TO(DepartmentController::removeDepartment,"/api/departments/{1}",drogon::Delete,"erp::JwtAuthMiddleware");
    // 部门列表（所有登录用户可见）
    ADD_METHOD_TO(DepartmentController::list, "/api/departments",
                  drogon::Get, "erp::JwtAuthMiddleware");
    // 部门员工列表（领导看本部门、管理员看全部）
    ADD_METHOD_TO(DepartmentController::employees, "/api/departments/employees",
                  drogon::Get, "erp::JwtAuthMiddleware");
    // 全部用户列表（仅管理员，含部门，用于调度）
    ADD_METHOD_TO(DepartmentController::allUsers, "/api/departments/users",
                  drogon::Get, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 部门仪表盘（领导看本部门）
    ADD_METHOD_TO(DepartmentController::dashboard, "/api/departments/dashboard",
                  drogon::Get, "erp::JwtAuthMiddleware");
    // 部门趋势统计（折线图数据）
    ADD_METHOD_TO(DepartmentController::trend, "/api/departments/trend",
                  drogon::Get, "erp::JwtAuthMiddleware");
    ADD_METHOD_TO(DepartmentController::handoverPreview, "/api/departments/handover",
                  drogon::Get, "erp::JwtAuthMiddleware");
    // 员工调度（仅管理员）
    ADD_METHOD_TO(DepartmentController::assign, "/api/departments/assign",
                  drogon::Post, "erp::JwtAuthMiddleware",
                  "erp::RoleAuthMiddleware");
    // 分配仓库（管理员或仓库部领导，给仓库部员工设置所属仓库）
    ADD_METHOD_TO(DepartmentController::assignWarehouse,
                  "/api/departments/assign-warehouse", drogon::Post,
                  "erp::JwtAuthMiddleware", "erp::RoleAuthMiddleware");
    METHOD_LIST_END
    void saveDepartment(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
    void editDepartment(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&,int64_t);
    void removeDepartment(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&,int64_t);

    void list(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void employees(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void allUsers(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void dashboard(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void trend(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void handoverPreview(const drogon::HttpRequestPtr &,
                         std::function<void(const drogon::HttpResponsePtr &)> &&);
    void assign(const drogon::HttpRequestPtr &req,
                std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void assignWarehouse(
        const drogon::HttpRequestPtr &req,
        std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};

}  // namespace erp
