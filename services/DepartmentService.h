#pragma once

#include <drogon/HttpTypes.h>
#include <json/json.h>
#include <functional>
#include <string>

namespace erp
{

// 部门服务：部门字典、部门员工、部门仪表盘、员工调度
class DepartmentService
{
  public:
    using Callback =
        std::function<void(int code, const std::string &msg,
                           const Json::Value &data)>;

    // 部门列表（从 departments 表）
    static void listDepartments(Callback &&cb);

    // 部门员工列表（users 表中 department 匹配，含角色信息）
    static void listDepartmentEmployees(const std::string &department,
                                        Callback &&cb);

    // 全部用户列表（含部门，供管理员调度）
    static void listAllUsers(Callback &&cb);

    // 部门仪表盘（员工数 + 部门相关业务统计）
    // userId: 当前用户ID（销售部员工只看自己的数据），0 表示不过滤
    // onlySelf: 是否只看自己（销售部员工为 true）
    static void departmentDashboard(const std::string &department,
                                    int64_t userId,
                                    bool onlySelf,
                                    Callback &&cb);

    // 部门趋势统计（日/周/月/年，用于折线图）
    // period: day / week / month / year
    // userId: 当前用户ID，onlySelf: 是否只看自己（销售部员工为 true）
    static void departmentTrend(const std::string &department,
                                const std::string &period,
                                int64_t userId,
                                bool onlySelf,
                                Callback &&cb);


    // 分配仓库（仓库部员工所属仓库；管理员或仓库部领导操作）
    static void assignWarehouse(int64_t userId, int64_t warehouseId,
                               Callback &&cb);
};

}  // namespace erp
