#include "DepartmentController.h"
#include "../services/DepartmentService.h"
#include "../utils/ApiResponse.h"
#include "../services/DocumentAccess.h"
#include "../services/ApprovalHandover.h"
#include <regex>
#include <set>

using namespace drogon;

namespace erp
{


namespace {
void maintainDepartment(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> cb,int64_t id,bool remove=false) {
 try {
  auto roles=req->getAttributes()->get<std::string>("roles");business::require((","+roles+",").find(",admin,")!=std::string::npos,"仅管理员可管理部门",ErrorCode::FORBIDDEN);
  auto actor=std::stoll(req->getAttributes()->get<std::string>("user_id"));auto input=req->getJsonObject();Json::Value body=input?*input:Json::Value();
  business::transaction([cb](int code,const std::string &message,const Json::Value &data){cb(code==ErrorCode::OK?ApiResponse::ok(message,data):ApiResponse::error(code,message));},[=](const business::Tx &tx){
   Json::Value old;
   if(id){auto rows=tx->execSqlSync("SELECT * FROM departments WHERE id=? FOR UPDATE",id);business::require(!rows.empty(),"部门不存在",ErrorCode::NOT_FOUND);old=business::rowJson(rows,rows[0]);}
   bool system=id&&(old["code"]=="sales"||old["code"]=="purchase"||old["code"]=="warehouse"||old["code"]=="finance");
   if(remove){
    business::require(!system,"系统业务部门不可删除");
    business::require(tx->execSqlSync("SELECT id FROM user_directory WHERE department_id=? LIMIT 1",id).empty()&&tx->execSqlSync("SELECT id FROM employee_directory WHERE department_id=? LIMIT 1",id).empty(),"部门下仍有成员，请先完成调度");
    tx->execSqlSync("DELETE FROM departments WHERE id=?",id);business::event(tx,"departments",id,"delete",actor,old["status"].asInt(),-1,old["name"].asString());business::Outcome out;out.message="部门已删除";return out;
   }
   business::require(body.isObject(),"请输入部门资料");
   auto name=body.get("name",old.get("name","")).asString(),code=body.get("code",old.get("code","")).asString();auto description=body.get("description",old.get("description","")).asString();int status=body.get("status",old.get("status",1)).asInt();
   auto trim=[](std::string value){auto first=value.find_first_not_of(" \t\r\n");if(first==std::string::npos)return std::string();auto last=value.find_last_not_of(" \t\r\n");return value.substr(first,last-first+1);};name=trim(name);code=trim(code);
   business::require(!name.empty()&&name.size()<=192&&std::regex_match(code,std::regex("[A-Za-z][A-Za-z0-9_]{0,31}"))&&(status==0||status==1),"部门名称、编码或状态不合法");
   if(system)business::require(name==old["name"].asString()&&code==old["code"].asString()&&status==1,"系统业务部门不可修改名称、编码或停用，可修改说明");
   business::require(tx->execSqlSync("SELECT id FROM departments WHERE (name=? OR code=?) AND id<>? LIMIT 1",name,code,id).empty(),"部门名称或编码已存在",ErrorCode::CONFLICT);
   if(id&&status==0)business::require(tx->execSqlSync("SELECT id FROM user_directory WHERE department_id=? LIMIT 1",id).empty()&&tx->execSqlSync("SELECT id FROM employee_directory WHERE department_id=? LIMIT 1",id).empty(),"部门下仍有成员，请先调度后再停用");
   auto target=id;
   if(id){tx->execSqlSync("UPDATE departments SET name=?,code=?,description=?,status=? WHERE id=?",name,code,description,status,id);}
   else target=tx->execSqlSync("INSERT INTO departments(name,code,description,status) VALUES(?,?,?,?)",name,code,description,status).insertId();
   business::event(tx,"departments",target,id?"update":"create",actor,id?old["status"].asInt():-1,status,name);
   business::Outcome out;out.message="部门已保存";out.data["id"]=Json::Int64(target);return out;
  });
 }catch(const business::Error &e){cb(ApiResponse::error(e.code,e.what()));}catch(const std::exception &){cb(ApiResponse::error(ErrorCode::BAD_REQUEST,"部门保存失败"));}
}
}
void DepartmentController::saveDepartment(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb){maintainDepartment(req,std::move(cb),0);}
void DepartmentController::editDepartment(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb,int64_t id){maintainDepartment(req,std::move(cb),id);}
void DepartmentController::removeDepartment(const HttpRequestPtr &req,std::function<void(const HttpResponsePtr &)> &&cb,int64_t id){maintainDepartment(req,std::move(cb),id,true);}

void DepartmentController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    DepartmentService::listDepartments(
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void DepartmentController::employees(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    // 领导只能看本部门；管理员可指定 department 参数看任意部门
    std::string roles = req->getAttributes()->get<std::string>("roles");
    std::string myDept = req->getAttributes()->get<std::string>("department");
    bool isAdmin = roles.find("admin") != std::string::npos;

    std::string department = req->getParameter("department");
    if (department.empty())
        department = myDept;

    // 非管理员且指定了非本部门 → 拦截
    if (!isAdmin && department != myDept)
    {
        callback(ApiResponse::error(ErrorCode::FORBIDDEN,
                                    "无权限查看其他部门员工", k403Forbidden));
        return;
    }

    DepartmentService::listDepartmentEmployees(
        department,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void DepartmentController::allUsers(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    DepartmentService::listAllUsers(
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void DepartmentController::dashboard(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string roles = req->getAttributes()->get<std::string>("roles");
    std::string myDept = req->getAttributes()->get<std::string>("department");
    bool isAdmin = roles.find("admin") != std::string::npos;
    bool isLeader = roles.find("leader") != std::string::npos;

    std::string department = req->getParameter("department");
    if (department.empty())
        department = myDept;

    if (!isAdmin && department != myDept)
    {
        callback(ApiResponse::error(ErrorCode::FORBIDDEN,
                                    "无权限查看其他部门仪表盘", k403Forbidden));
        return;
    }

    // 销售部/仓库部员工只看自己的数据（仓库部按所属仓库过滤）
    int64_t userId = 0;
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    try { userId = std::stoll(userIdStr); } catch (...) {}
    bool onlySelf = (!isAdmin && !isLeader) &&
                    (department == "销售部" || department == "仓库部");

    DepartmentService::departmentDashboard(
        department, userId, onlySelf,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void DepartmentController::trend(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    std::string roles = req->getAttributes()->get<std::string>("roles");
    std::string myDept = req->getAttributes()->get<std::string>("department");
    bool isAdmin = roles.find("admin") != std::string::npos;
    bool isLeader = roles.find("leader") != std::string::npos;

    std::string department = req->getParameter("department");
    if (department.empty())
        department = myDept;

    if (!isAdmin && department != myDept)
    {
        callback(ApiResponse::error(ErrorCode::FORBIDDEN,
                                    "无权限查看其他部门趋势", k403Forbidden));
        return;
    }

    std::string period = req->getParameter("period");
    if (period.empty())
        period = "day";

    // 销售部员工只看自己的数据
    int64_t userId = 0;
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    try { userId = std::stoll(userIdStr); } catch (...) {}
    bool onlySelf = (department == "销售部" && !isAdmin && !isLeader);

    DepartmentService::departmentTrend(
        department, period, userId, onlySelf,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void DepartmentController::assignWarehouse(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "缺少请求参数"));
        return;
    }

    int64_t userId = (*json).get("user_id", 0).asInt64();
    int64_t warehouseId = (*json).get("warehouse_id", 0).asInt64();

    if (userId <= 0)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST,
                                    "user_id 不能为空"));
        return;
    }

    DepartmentService::assignWarehouse(
        userId, warehouseId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void DepartmentController::handoverPreview(
    const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    using namespace business;
    try {
        const auto roles = req->getAttributes()->get<std::string>("roles");
        require(("," + roles + ",").find(",admin,") != std::string::npos,
                "仅管理员可查看移交待办", ErrorCode::FORBIDDEN);
        const auto userId = std::stoll(req->getParameter("user_id"));
        require(userId > 0, "请选择员工账号");
        auto db = app().getDbClient();
        Json::Value groups(Json::arrayValue);
        for (const std::string department : {"采购部", "销售部", "仓库部", "财务部"}) {
            Json::Value group;
            group["department"] = department;
            int count = 0;
            std::set<int64_t> creators;
            for (const auto &source : handoverSources) {
                if (department != source.department) continue;
                const auto rows = db->execSqlSync("SELECT " + std::string(source.creator) +
                    " AS creator_id FROM " + source.table + pendingAssigned + source.condition, userId);
                count += rows.size();
                for (const auto &row : rows) {
                    if (!row["creator_id"].isNull()) creators.insert(row["creator_id"].as<int64_t>());
                }
            }
            if (!count) continue;
            group["count"] = count;
            group["candidates"] = Json::Value(Json::arrayValue);
            const auto users = db->execSqlSync(
                "SELECT u.id,COALESCE(NULLIF(u.real_name,''),u.username) AS name FROM user_directory u "
                "WHERE u.status=1 AND u.id<>? AND " + handoverEligibility(department) + " ORDER BY u.id",
                userId, department);
            for (const auto &row : users) {
                if (!creators.count(row["id"].as<int64_t>())) {
                    group["candidates"].append(rowJson(users, row));
                }
            }
            groups.append(group);
        }
        callback(ApiResponse::ok(groups));
    } catch (const Error &e) {
        callback(ApiResponse::error(e.code, e.what()));
    } catch (const std::logic_error &) {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "员工账号参数错误"));
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        callback(ApiResponse::error(ErrorCode::INTERNAL_ERROR, "待办查询失败"));
    }
}

void DepartmentController::assign(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "缺少请求参数"));
        return;
    }

    try {
        auto userId=(*json).get("user_id",0).asInt64();business::require(userId>0,"请选择员工账号");
        auto actor=std::stoll(req->getAttributes()->get<std::string>("user_id"));auto payload=*json;
        business::transaction([callback](int code,const std::string &msg,const Json::Value &data){callback(code==ErrorCode::OK?ApiResponse::ok(msg,data):ApiResponse::error(code,msg));},[=](const business::Tx &tx){
            using namespace business;
            const bool handoverOnly = payload.get("handover_only", false).asBool();
            auto user=tx->execSqlSync("SELECT id,department_id,department,warehouse_id FROM user_directory WHERE id=? FOR UPDATE",userId);
            require(!user.empty(),"员工账号不存在",ErrorCode::NOT_FOUND);
            const auto oldDepartment = user[0]["department_id"].isNull() ? 0 : user[0]["department_id"].as<int64_t>();
            auto did=handoverOnly ? oldDepartment : payload.get("department_id",0).asInt64();
            std::string name=handoverOnly ? user[0]["department"].as<std::string>() : "";
            if(!handoverOnly && (did || !payload.isMember("department_id"))){
                auto departments=did?tx->execSqlSync("SELECT id,name,status FROM departments WHERE id=? FOR UPDATE",did):tx->execSqlSync("SELECT id,name,status FROM departments WHERE name=? FOR UPDATE",payload.get("department","").asString());
                business::require(!departments.empty()&&departments[0]["status"].as<int>()==1,"请选择有效的启用部门");did=departments[0]["id"].as<int64_t>();name=departments[0]["name"].as<std::string>();
            }
            const auto handover = payload.get("handover", Json::Value(Json::objectValue));
            require(handover.isObject(), "待办接收人格式错误");
            int transferred = 0;
            for (const auto &source : handoverSources) {
                const auto receiver = handover.get(source.department, 0).asInt64();
                if (did == oldDepartment && !handoverOnly && !receiver) continue;
                const auto pending = tx->execSqlSync("SELECT id," + std::string(source.creator) +
                    " AS creator_id FROM " + source.table + pendingAssigned + source.condition + " FOR UPDATE", userId);
                if (pending.empty()) continue;
                require(receiver > 0 && receiver != userId, "仍有" + std::string(source.department) + "待办，请选择有效接收人", ErrorCode::CONFLICT);
                require(!tx->execSqlSync(
                    "SELECT u.id FROM user_directory u WHERE u.id=? AND u.status=1 AND " +
                    handoverEligibility(source.department) + " FOR UPDATE", receiver, source.department).empty(),
                    "接收人已停用或不具备该业务审批权限", ErrorCode::CONFLICT);
                for (const auto &row : pending) {
                    require(row["creator_id"].isNull() || row["creator_id"].as<int64_t>() != receiver,
                            "接收人不能审批自己发起的单据", ErrorCode::CONFLICT);
                    const auto id = row["id"].as<int64_t>();
                    tx->execSqlSync("UPDATE " + std::string(source.table) +
                        " SET assigned_approver_id=? WHERE id=?", receiver, id);
                    event(tx,source.table,id,"approval_handover",actor,0,0,
                          "原审批人账号 #" + std::to_string(userId) + " → 接收人账号 #" + std::to_string(receiver));
                    ++transferred;
                }
            }
            if (!handoverOnly) {
                int64_t warehouseId = 0;
                if (name == "仓库部") {
                    if (payload.isMember("warehouse_id")) {
                        warehouseId = payload["warehouse_id"].asInt64();
                        warehouse(tx,warehouseId);
                    } else if (!user[0]["warehouse_id"].isNull()) {
                        warehouseId = user[0]["warehouse_id"].as<int64_t>();
                    }
                }
                tx->execSqlSync("UPDATE users SET department_id=NULLIF(?,0),warehouse_id=NULLIF(?,0) WHERE id=?",
                                did,warehouseId,userId);
                tx->execSqlSync("UPDATE employees SET department_id=NULLIF(?,0) WHERE user_id=?",did,userId);
                event(tx,"users",userId,"department_assign",actor,-1,1,name);
            }
            Outcome out;
            out.message=handoverOnly ? "待办移交完成" : "员工调度已保存";
            out.data["transferred"]=transferred;
            out.data["user_id"]=Json::Int64(userId);
            out.data["department_id"]=did?Json::Value(Json::Int64(did)):Json::Value();
            out.data["department"]=name;
            return out;
        });
    }catch(const business::Error &e){callback(ApiResponse::error(e.code,e.what()));}catch(const std::exception &){callback(ApiResponse::error(ErrorCode::BAD_REQUEST,"部门调度失败"));}
}

}  // namespace erp
