#pragma once
#include <drogon/drogon.h>
#include <stdexcept>
#include <set>
namespace erp {
inline bool approvalAdmin(int64_t id) {
    return !drogon::app().getDbClient()->execSqlSync("SELECT u.id FROM users u JOIN user_roles ur ON ur.user_id=u.id JOIN roles r ON r.id=ur.role_id WHERE u.id=? AND u.status=1 AND r.code='admin'",id).empty();
}
inline int64_t selectedApprover(const drogon::HttpRequestPtr &req) {
    auto id=std::stoll(req->getAttributes()->get<std::string>("user_id"));
    auto j=req->getJsonObject();
    auto selected=j ? j->get("assigned_approver_id",0).asInt64() : 0;
    if (!selected) return 0; // 不指定时继续使用部门公共审批队列。
    auto rows=drogon::app().getDbClient()->execSqlSync("SELECT leader.id FROM users leader JOIN user_roles ur ON ur.user_id=leader.id JOIN roles r ON r.id=ur.role_id JOIN users applicant ON applicant.id=? WHERE leader.id=? AND leader.status=1 AND r.code='leader' AND leader.department_id=applicant.department_id AND leader.id<>applicant.id",id,selected);
    if(rows.empty()) throw std::runtime_error("请选择本部门在职审批领导，不能选择自己");
    return selected;
}
inline void checkAssignedApprover(const std::string &table,int64_t documentId,int64_t userId) {
    static const std::set<std::string> allowed={"purchase_orders","sales_orders","applications","stock_operations","purchase_returns","sales_returns"};
    if(!allowed.count(table)) throw std::runtime_error("单据类型错误");
    auto rows=drogon::app().getDbClient()->execSqlSync("SELECT assigned_approver_id FROM "+table+" WHERE id=?",documentId);
    if(rows.empty()) throw std::runtime_error("单据不存在");
    if (approvalAdmin(userId)) return;
    std::string department;
    if(table=="purchase_orders" || table=="purchase_returns") department="采购部";
    else if(table=="sales_orders" || table=="sales_returns") department="销售部";
    else if(table=="stock_operations") department="仓库部";
    else {
        auto application=drogon::app().getDbClient()->execSqlSync("SELECT type FROM applications WHERE id=?",documentId);
        if(application.empty()) throw std::runtime_error("申报不存在");
        department=application[0]["type"].as<std::string>()=="customer"?"销售部":"采购部";
    }
    auto eligible=drogon::app().getDbClient()->execSqlSync("SELECT u.id FROM user_directory u JOIN user_roles ur ON ur.user_id=u.id JOIN roles r ON r.id=ur.role_id WHERE u.id=? AND u.status=1 AND u.department=? AND r.code='leader'",userId,department);
    if(eligible.empty()) throw std::runtime_error("当前账号无该部门审批资格");
    if(!rows[0]["assigned_approver_id"].isNull()) {
        auto assigned=rows[0]["assigned_approver_id"].as<int64_t>();
        if(assigned && assigned!=userId && !approvalAdmin(userId)) throw std::runtime_error("该单据已指定其他领导审批");
    }
}
}
