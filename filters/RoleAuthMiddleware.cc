#include "RoleAuthMiddleware.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

namespace
{
// 判断 roles 字符串（逗号分隔）是否包含指定角色
bool hasRole(const std::string &roles, const std::string &role)
{
    if (roles.empty())
        return false;
    size_t pos = 0;
    while (pos <= roles.size())
    {
        size_t comma = roles.find(',', pos);
        std::string part = roles.substr(pos, comma == std::string::npos
                                                  ? std::string::npos
                                                  : comma - pos);
        if (part == role)
            return true;
        if (comma == std::string::npos)
            break;
        pos = comma + 1;
    }
    return false;
}

// 判断 path 是否以 prefix 开头
bool startsWith(const std::string &path, const std::string &prefix)
{
    return path.compare(0, prefix.size(), prefix) == 0;
}
}  // namespace

void RoleAuthMiddleware::invoke(const HttpRequestPtr &req,
                                MiddlewareNextCallback &&nextCb,
                                MiddlewareCallback &&mcb)
{
    std::string roles = req->getAttributes()->get<std::string>("roles");
    std::string path = req->getPath();
    std::string department =
        req->getAttributes()->get<std::string>("department");

    bool isAdmin = hasRole(roles, "admin");
    bool isLeader = hasRole(roles, "leader");
    bool isFinance = (department == "财务部");
    auto method=req->getMethod();
    bool writing=(method==Post || method==Put || method==Delete || method==Patch);
    if(startsWith(path,"/api/export") && !isAdmin) {
        mcb(ApiResponse::error(ErrorCode::FORBIDDEN,"当前批量导出仅开放给管理员",k403Forbidden));return;
    }
    // 基础资料的员工变更走申报审批，禁止直接调用通用 CRUD 绕过流程。
    if(writing && !isAdmin && (startsWith(path,"/api/customers") || startsWith(path,"/api/suppliers") || startsWith(path,"/api/categories") || startsWith(path,"/api/employees"))) {
        mcb(ApiResponse::error(ErrorCode::FORBIDDEN,"基础资料请通过申报审批维护",k403Forbidden));return;
    }
    if((startsWith(path,"/api/accounts") || startsWith(path,"/api/payments") || startsWith(path,"/api/finance") || startsWith(path,"/api/report")) && !isAdmin && !isFinance) {
        mcb(ApiResponse::error(ErrorCode::FORBIDDEN,"仅财务人员可访问财务明细，请在部门仪表盘查看部门统计",k403Forbidden));return;
    }

    if (startsWith(path, "/api/products") && req->getMethod()!=drogon::Get && req->getMethod()!=drogon::Options && !isAdmin) {
        mcb(ApiResponse::error(ErrorCode::FORBIDDEN,"仅管理员可维护商品",k403Forbidden));return;
    }
    // 手工出入库与安全库存设置只允许仓库负责人操作。
    if (path == "/api/stock/in" || path == "/api/stock/out" ||
        (startsWith(path, "/api/inventory/") &&
         path.size() >= 7 && path.rfind("/safety") == path.size() - 7))
    {
        if (!isAdmin && !(department == "仓库部" && isLeader))
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限调整库存",
                                   k403Forbidden));
            return;
        }
    }

    // 修改用户状态：admin 或 leader（领导仅限本部门，由控制器二次校验）
    if (path == "/api/users/status")
    {
        if (!isAdmin && !isLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限访问该接口",
                                   k403Forbidden));
            return;
        }
        nextCb(std::move(mcb));
        return;
    }

    // 导入（写入）：仅 admin
    if (startsWith(path, "/api/import"))
    {
        if (!isAdmin)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限导入数据",
                                   k403Forbidden));
            return;
        }
    }

    // 系统管理 + 员工调度：仅 admin
    if (startsWith(path, "/api/users") || startsWith(path, "/api/roles") ||
        startsWith(path, "/api/permissions") ||
        startsWith(path, "/api/operationlogs") ||
        startsWith(path, "/api/userroles") ||
        startsWith(path, "/api/rolepermissions") ||
        path == "/api/departments/assign")
    {
        if (!isAdmin)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限访问该接口",
                                   k403Forbidden));
            return;
        }
    }

    // 用户列表（含所属仓库）：admin 或仓库部领导（员工管理里分配仓库用）
    if (path == "/api/departments/users")
    {
        bool isWarehouseLeader = (department == "仓库部" && isLeader);
        if (!isAdmin && !isWarehouseLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限访问该接口",
                                   k403Forbidden));
            return;
        }
    }

    // 分配仓库：admin 或仓库部领导
    if (path == "/api/departments/assign-warehouse")
    {
        bool isWarehouseLeader = (department == "仓库部" && isLeader);
        if (!isAdmin && !isWarehouseLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限分配仓库",
                                   k403Forbidden));
            return;
        }
    }

    // 财务报表：admin 或 leader 或财务部成员（财务部不分上下级）
    if (startsWith(path, "/api/report") || startsWith(path, "/api/finance"))
    {
        if (!isAdmin && !isLeader && !isFinance)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限访问该接口",
                                   k403Forbidden));
            return;
        }
    }

    // 单据审批接口：按部门隔离（admin 可审批全部，leader 仅本部门单据）
    // 采购单/采购退货 → admin 或采购部领导；销售单/销售退货 → admin 或销售部领导
    // 库存操作 → admin 或仓库部领导；通用申请 → admin 或任意 leader
    if (path == "/api/purchase/approve" || path == "/api/purchase/return/approve")
    {
        bool isPurchaseLeader = (department == "采购部" && isLeader);
        if (!isAdmin && !isPurchaseLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN,
                                   "采购单由采购部领导审批",
                                   k403Forbidden));
            return;
        }
    }
    if (path == "/api/sales/approve" || path == "/api/sales/return/approve")
    {
        bool isSalesLeader = (department == "销售部" && isLeader);
        if (!isAdmin && !isSalesLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN,
                                   "销售单由销售部领导审批",
                                   k403Forbidden));
            return;
        }
    }
    if (path == "/api/stock/operation/approve")
    {
        bool isWarehouseLeader = (department == "仓库部" && isLeader);
        if (!isAdmin && !isWarehouseLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN,
                                   "库存操作由仓库部领导审批",
                                   k403Forbidden));
            return;
        }
    }
    if (path == "/api/application/approve")
    {
        if (!isAdmin && !isLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限审批单据",
                                   k403Forbidden));
            return;
        }
    }

    // 仓管员入库确认（采购单 1→3）：仅 admin 或仓库部成员
    if (path == "/api/purchase/inbound")
    {
        bool isWarehouseDept = (department == "仓库部");
        if (!isAdmin && !isWarehouseDept)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限确认入库",
                                   k403Forbidden));
            return;
        }
    }

    // 退货实物确认还须通过服务层的仓库范围校验。
    if (path == "/api/purchase/return/warehouse" ||
        path == "/api/sales/return/warehouse")
    {
        if (!isAdmin && department != "仓库部")
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN,
                                   "无权限操作退货出入库",
                                   k403Forbidden));
            return;
        }
    }

    // 仓管员出库确认（销售单 1→5，填物流）：仅 admin 或仓库部成员
    if (path == "/api/sales/confirm")
    {
        bool isWarehouseDept = (department == "仓库部");
        if (!isAdmin && !isWarehouseDept)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限确认出库",
                                   k403Forbidden));
            return;
        }
    }

    // 仓库领导批准出库（销售单 5→3，扣库存）：仅 admin 或仓库部领导
    if (path == "/api/sales/outbound")
    {
        bool isWarehouseLeader = (department == "仓库部" && isLeader);
        if (!isAdmin && !isWarehouseLeader)
        {
            mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限批准出库",
                                   k403Forbidden));
            return;
        }
    }

    // 仓库管理：读操作所有登录用户，写操作仅 admin 或仓库部领导
    if (startsWith(path, "/api/warehouses"))
    {
        auto method = req->getMethod();
        bool isWrite = (method == drogon::Post || method == drogon::Put ||
                        method == drogon::Delete);
        if (isWrite)
        {
            bool isWarehouseLeader = (department == "仓库部" && isLeader);
            if (!isAdmin && !isWarehouseLeader)
            {
                mcb(ApiResponse::error(ErrorCode::FORBIDDEN, "无权限管理仓库",
                                       k403Forbidden));
                return;
            }
        }
    }

    // 通过
    nextCb(std::move(mcb));
}

}  // namespace erp
