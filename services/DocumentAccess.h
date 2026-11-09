#pragma once
#include "BusinessSupport.h"
#include <set>
namespace erp::business {
std::string documentScope(const std::string &table, const std::string &alias, int64_t userId, drogon::orm::DbClient &db);
std::string documentScope(const std::string &table,const std::string &alias,const drogon::HttpRequestPtr &req);
void readDocuments(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> callback,const std::string &table,int64_t id=0);
Json::Value rowJson(const drogon::orm::Result &result,const drogon::orm::Row &row);
}
