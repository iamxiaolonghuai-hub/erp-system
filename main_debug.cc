#include <drogon/drogon.h>
#include "utils/Jwt.h"
#include <cstdio>

using namespace drogon;

int main()
{
    fprintf(stderr, "[STEP1] main start\n");
    app().loadConfigFile("config.json");
    fprintf(stderr, "[STEP2] config loaded\n");
    std::string jwtSecret = app().getCustomConfig().get("jwt_secret", "erp-system-jwt-secret-change-me").asString();
    erp::Jwt::setSecret(jwtSecret);
    fprintf(stderr, "[STEP3] jwt secret set\n");
    app().registerPostHandlingAdvice([](const HttpRequestPtr &req, const HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
    });
    fprintf(stderr, "[STEP4] advice registered\n");
    app().run();
    fprintf(stderr, "[STEP5] run returned\n");
    return 0;
}
