#pragma once
#include <drogon/HttpController.h>
namespace erp {class BackupController:public drogon::HttpController<BackupController>{public:
 METHOD_LIST_BEGIN
 ADD_METHOD_TO(BackupController::get,"/api/operations/backup",drogon::Get,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(BackupController::save,"/api/operations/backup",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 ADD_METHOD_TO(BackupController::request,"/api/operations/backup/request",drogon::Post,"erp::JwtAuthMiddleware","erp::RoleAuthMiddleware");
 METHOD_LIST_END
 void get(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void save(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
 void request(const drogon::HttpRequestPtr &,std::function<void(const drogon::HttpResponsePtr &)> &&);
};}
