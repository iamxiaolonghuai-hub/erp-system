#include "UserController.h"
#include "../services/ApprovalHandover.h"

#include "../models/Users.h"
#include "../models/UserRoles.h"
#include "../utils/ApiResponse.h"
#include "../utils/Password.h"

#include <drogon/orm/Mapper.h>
#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>

#include <memory>

using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::erp_system;

namespace erp
{

void UserController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    std::string username = (*json).get("username", "").asString();
    std::string password = (*json).get("password", "").asString();
    std::string realName = (*json).get("real_name", "").asString();
    std::string email = (*json).get("email", "").asString();
    std::string phone = (*json).get("phone", "").asString();
    int64_t roleId = (*json).get("role_id", 0).asInt64();

    if (username.empty() || password.empty())
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "用户名和密码不能为空"));
        return;
    }

    auto clientPtr = app().getDbClient("default");

    // 检查用户名是否已存在
    Mapper<Users> mapper(clientPtr);
    mapper.findOne(
        Criteria(Users::Cols::_username, CompareOperator::EQ, username),
        [callback](const Users &) {
            callback(ApiResponse::error(ErrorCode::CONFLICT, "用户名已存在"));
        },
        [callback, clientPtr, username, password, realName, email, phone,
         roleId](const DrogonDbException &) {
            // 用户名不存在，创建
            Users user;
            user.setUsername(username);
            user.setPassword(Password::hash(password));
            user.setRealName(realName);
            user.setEmail(email);
            user.setPhone(phone);
            user.setStatus(1);

            Mapper<Users> mapper(clientPtr);
            mapper.insert(
                user,
                [callback, clientPtr, roleId](const Users &newUser) {
                    int64_t userId = (int64_t)newUser.getValueOfId();

                    // 分配角色
                    if (roleId > 0)
                    {
                        UserRoles ur;
                        ur.setUserId(userId);
                        ur.setRoleId(roleId);
                        Mapper<UserRoles> urMapper(clientPtr);
                        urMapper.insert(
                            ur,
                            [callback, newUser](const UserRoles &) {
                                Json::Value data = newUser.toJson();
                                callback(ApiResponse::ok(
                                    std::string("用户创建成功"), data));
                            },
                            [callback](const DrogonDbException &e) {
                                callback(ApiResponse::error(
                                    ErrorCode::INTERNAL_ERROR,
                                    "分配角色失败: " +
                                        std::string(e.base().what())));
                            });
                    }
                    else
                    {
                        Json::Value data = newUser.toJson();
                        callback(ApiResponse::ok(std::string("用户创建成功"),
                                                 data));
                    }
                },
                [callback](const DrogonDbException &e) {
                    callback(ApiResponse::error(
                        ErrorCode::INTERNAL_ERROR,
                        "创建用户失败: " + std::string(e.base().what())));
                });
        });
}

void UserController::resetPassword(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t userId = (*json).get("user_id", 0).asInt64();
    std::string newPassword = (*json).get("password", "").asString();

    if (userId <= 0 || newPassword.empty())
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "用户和密码不能为空"));
        return;
    }

    auto clientPtr = app().getDbClient("default");
    Mapper<Users> mapper(clientPtr);

    mapper.findByPrimaryKey(
        userId,
        [callback, clientPtr, newPassword](const Users &user) {
            Users updated = user;
            updated.setPassword(Password::hash(newPassword));
            Mapper<Users> m(clientPtr);
            m.update(
                updated,
                [callback](const size_t count) {
                    if (count == 1)
                        callback(ApiResponse::ok(
                            std::string("密码重置成功")));
                    else
                        callback(ApiResponse::error(ErrorCode::INTERNAL_ERROR,
                                                    "更新失败"));
                },
                [callback](const DrogonDbException &e) {
                    callback(ApiResponse::error(
                        ErrorCode::INTERNAL_ERROR,
                        "重置密码失败: " + std::string(e.base().what())));
                });
        },
        [callback](const DrogonDbException &) {
            callback(ApiResponse::error(ErrorCode::NOT_FOUND, "用户不存在"));
        });
}

void UserController::updateStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }

    int64_t userId = (*json).get("user_id", 0).asInt64();
    int status = (*json).get("status", 1).asInt();

    if (userId <= 0 || (status != 0 && status != 1))
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST,
                                    "user_id 非法或 status 只能为 0/1"));
        return;
    }

    // 权限：领导只能改本部门员工，且不能改自己/其他领导；管理员不受限
    std::string roles = req->getAttributes()->get<std::string>("roles");
    std::string myDept = req->getAttributes()->get<std::string>("department");
    std::string myIdStr = req->getAttributes()->get<std::string>("user_id");
    bool isAdmin = roles.find("admin") != std::string::npos;
    bool isLeader = roles.find("leader") != std::string::npos;

    int64_t myId = 0;
    try { myId = std::stoll(myIdStr); } catch (...) {}

    business::transaction(
        [callback](int code, const std::string &message, const Json::Value &data) {
            callback(code == ErrorCode::OK ? ApiResponse::ok(message, data) : ApiResponse::error(code, message));
        },
        [=](const business::Tx &tx) {
            using namespace business;
            const auto rows = tx->execSqlSync(
                "SELECT id,department,status FROM user_directory WHERE id=? FOR UPDATE", userId);
            require(!rows.empty(), "用户不存在", ErrorCode::NOT_FOUND);
            if (!isAdmin) {
                require(isLeader, "无权限修改用户状态", ErrorCode::FORBIDDEN);
                require(rows[0]["department"].as<std::string>() == myDept,
                        "只能修改本部门员工状态", ErrorCode::FORBIDDEN);
                require(userId != myId, "不能修改自己的状态", ErrorCode::FORBIDDEN);
                const auto roles = tx->execSqlSync(
                    "SELECT r.code FROM user_roles ur JOIN roles r ON r.id=ur.role_id "
                    "WHERE ur.user_id=? AND r.code IN ('admin','leader')", userId);
                require(roles.empty(), "不能修改管理员或其他领导的状态", ErrorCode::FORBIDDEN);
            }
            if (status == 0 && rows[0]["status"].as<int>() == 1) requirePendingHandover(tx, userId);
            tx->execSqlSync("UPDATE users SET status=? WHERE id=?", status, userId);
            Outcome out;
            out.message = "状态更新成功";
            out.data["user_id"] = Json::Int64(userId);
            out.data["status"] = status;
            return out;
        });
}

}  // namespace erp
