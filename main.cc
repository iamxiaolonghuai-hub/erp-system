#include <drogon/drogon.h>
#include "utils/Jwt.h"

using namespace drogon;

int main()
{
    // 加载配置文件（数据库连接、监听端口等）
    app().loadConfigFile("config.json");

    // 从配置读取 JWT 密钥
    std::string jwtSecret =
        app().getCustomConfig().get("jwt_secret",
                                    "erp-system-jwt-secret-change-me").asString();
    erp::Jwt::setSecret(jwtSecret);

    // 全局 CORS 支持（前端 Vue dev server 跨域访问）
    app().registerPostHandlingAdvice([](const HttpRequestPtr &req,
                                        const HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods",
                        "GET, POST, PUT, DELETE, OPTIONS");
        resp->addHeader("Access-Control-Allow-Headers",
                        "Content-Type, Authorization");
        resp->addHeader("Access-Control-Max-Age", "3600");
    });

    app().run();
    return 0;
}
