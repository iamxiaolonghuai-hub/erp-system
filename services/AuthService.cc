#include "AuthService.h"
#include "../utils/Password.h"
#include "../utils/Jwt.h"
#include "../utils/ApiResponse.h"
#include <drogon/drogon.h>
#include <memory>
#include <sstream>

namespace erp {
namespace {
const std::string userQuery =
    "SELECT u.*, COALESCE((SELECT GROUP_CONCAT(r.code ORDER BY r.id SEPARATOR ',') "
    "FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id),'') AS role_codes FROM user_directory u WHERE ";

Json::Value publicUser(const drogon::orm::Row &row) {
    Json::Value user;
    user["id"]=Json::UInt64(row["id"].as<uint64_t>());
    user["status"]=row["status"].as<int>();
    for (auto field : {"username","real_name","email","phone","department","last_login_at","created_at","updated_at"})
        user[field]=row[field].isNull()?Json::Value():Json::Value(row[field].as<std::string>());
    user["warehouse_id"]=row["warehouse_id"].isNull()?Json::Value():Json::Value(Json::UInt64(row["warehouse_id"].as<uint64_t>()));
    user["roles"]=Json::Value(Json::arrayValue);
    std::istringstream roles(row["role_codes"].as<std::string>());
    std::string role;
    while(std::getline(roles,role,',')) if(!role.empty()) user["roles"].append(role);
    return user;
}
}

void AuthService::login(const std::string &username,const std::string &password,AuthCallback &&cb) {
    auto callback=std::make_shared<AuthCallback>(std::move(cb));
    // Database callbacks run on the database event loop: never issue a blocking query here.
    drogon::app().getDbClient()->execSqlAsync(userQuery+"u.username=?",
        [callback,password](const drogon::orm::Result &rows) {
            if(rows.empty() || !Password::verify(password,rows[0]["password"].as<std::string>())) {
                (*callback)(ErrorCode::UNAUTHORIZED,"用户名或密码错误",Json::Value());return;
            }
            if(rows[0]["status"].as<int>()!=1) {
                (*callback)(ErrorCode::FORBIDDEN,"账号已被禁用",Json::Value());return;
            }
            auto user=publicUser(rows[0]);
            std::map<std::string,std::string> payload{
                {"user_id",std::to_string(rows[0]["id"].as<uint64_t>())},
                {"username",user["username"].asString()},
                {"department",user["department"].asString()},
                {"roles",rows[0]["role_codes"].as<std::string>()}};
            Json::Value data;data["user"]=user;data["token"]=Jwt::generate(payload,7*24*3600);
            (*callback)(ErrorCode::OK,"success",data);
        },[callback](const drogon::orm::DrogonDbException &) {
            (*callback)(ErrorCode::INTERNAL_ERROR,"登录查询失败",Json::Value());
        },username);
}

void AuthService::getUserInfo(int64_t userId,AuthCallback &&cb) {
    auto callback=std::make_shared<AuthCallback>(std::move(cb));
    drogon::app().getDbClient()->execSqlAsync(userQuery+"u.id=?",
        [callback](const drogon::orm::Result &rows) {
            if(rows.empty()){(*callback)(ErrorCode::NOT_FOUND,"用户不存在",Json::Value());return;}
            (*callback)(ErrorCode::OK,"success",publicUser(rows[0]));
        },[callback](const drogon::orm::DrogonDbException &) {
            (*callback)(ErrorCode::INTERNAL_ERROR,"用户信息查询失败",Json::Value());
        },userId);
}
}
