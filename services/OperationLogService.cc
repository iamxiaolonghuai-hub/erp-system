#include "OperationLogService.h"

#include "../models/OperationLogs.h"

#include <drogon/orm/Mapper.h>
#include <drogon/orm/DbClient.h>
#include <drogon/drogon.h>

using namespace drogon;
using namespace drogon::orm;
using namespace drogon_model::erp_system;

namespace erp
{

void OperationLogService::record(int64_t userId,
                                 const std::string &username,
                                 const std::string &method,
                                 const std::string &path,
                                 const std::string &ip,
                                 int statusCode,
                                 int64_t costMs)
{
    // 只记录 /api/ 开头的业务接口
    if (path.compare(0, 4, "/api") != 0)
        return;

    OperationLogs log;
    if (userId > 0)
        log.setUserId(userId);
    log.setUsername(username);
    log.setMethod(method);
    log.setPath(path);
    log.setIp(ip);
    log.setStatusCode(statusCode);
    log.setCostMs(costMs);

    try
    {
        auto clientPtr = app().getDbClient("default");
        Mapper<OperationLogs> mapper(clientPtr);
        mapper.insert(
            log,
            [](const OperationLogs &) {
                // 成功，静默
            },
            [](const DrogonDbException &e) {
                LOG_DEBUG << "记录操作日志失败: " << e.base().what();
            });
    }
    catch (const std::exception &e)
    {
        LOG_DEBUG << "记录操作日志异常: " << e.what();
    }
}

}  // namespace erp
