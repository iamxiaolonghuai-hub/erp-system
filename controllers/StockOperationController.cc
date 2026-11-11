#include "BusinessSupport.h"
#include "../services/SerialInventory.h"
#include "../services/DocumentAccess.h"
#include "StockOperationController.h"
#include "../services/ApprovalAssignment.h"

#include "../services/StockOperationService.h"
#include "../utils/ApiResponse.h"

using namespace drogon;

namespace erp
{

static int64_t getOperatorId(const HttpRequestPtr &req)
{
    std::string userIdStr = req->getAttributes()->get<std::string>("user_id");
    if (userIdStr.empty())
        return 0;
    try
    {
        return std::stoll(userIdStr);
    }
    catch (...)
    {
        return 0;
    }
}

void StockOperationController::create(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    std::string type = (*json).get("type", "").asString();
    int64_t productId = (*json).get("product_id", 0).asInt64();
    int64_t fromWh = (*json).get("from_warehouse_id", 0).asInt64();
    int64_t toWh = (*json).get("to_warehouse_id", 0).asInt64();
    int qty = (*json).get("qty", 0).asInt();
    std::string bizType = (*json).get("biz_type", "").asString();
    std::string bizNo = (*json).get("biz_no", "").asString();
    std::string logisticsCompany = (*json).get("logistics_company", "").asString();
    std::string logisticsNo = (*json).get("logistics_no", "").asString();
    std::string serialNos = (*json).get("serial_nos", "").asString();
    std::string remark = (*json).get("remark", "").asString();
    int64_t applicantId = getOperatorId(req);

    int64_t assignedApproverId=0;
    try { assignedApproverId=selectedApprover(req); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::BAD_REQUEST,e.what()));return; }
    StockOperationService::create(
        type, productId, fromWh, toWh, qty, bizType, bizNo, logisticsCompany,
        logisticsNo, serialNos, remark, applicantId,
        assignedApproverId,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        }, business::requestIdentity(req),*json);
}

void StockOperationController::approve(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请求体不是合法 JSON"));
        return;
    }
    int64_t operationId = (*json).get("operation_id", 0).asInt64();
    bool approve = (*json).get("approve", true).asBool();
    std::string remark = (*json).get("remark", "").asString();
    int64_t approverId = getOperatorId(req);
    try { checkAssignedApprover("stock_operations",(*json).get("operation_id",0).asInt64(),approverId); } catch(const std::exception &e) { callback(ApiResponse::error(ErrorCode::FORBIDDEN,e.what()));return; }


    StockOperationService::approve(
        operationId, approve, approverId, remark,
        [callback](int code, const std::string &msg, const Json::Value &data) {
            if (code == ErrorCode::OK)
                callback(ApiResponse::ok(msg, data));
            else
                callback(ApiResponse::error(code, msg));
        });
}

void StockOperationController::receiveTransfer(
    const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    using namespace business;
    const auto body = req->getJsonObject();
    if (!body || !(*body)["operation_id"].isInt64() || (*body)["operation_id"].asInt64() <= 0) {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST, "请选择调拨单"));
        return;
    }
    const auto id = (*body)["operation_id"].asInt64();
    const auto signer = getOperatorId(req);
    transaction(
        [callback](int code, const std::string &message, const Json::Value &data) {
            callback(code == ErrorCode::OK ? ApiResponse::ok(message, data) : ApiResponse::error(code, message));
        },
        [=](const Tx &tx) {
            const auto rows = tx->execSqlSync("SELECT * FROM stock_operations WHERE id=? FOR UPDATE", id);
            require(!rows.empty(), "调拨单不存在", ErrorCode::NOT_FOUND);
            const auto row = rows[0];
            require(row["type"].as<std::string>() == "transfer" && row["status"].as<int>() == 3,
                    "只有在途调拨单可以签收", ErrorCode::CONFLICT);
            const auto destination = row["to_warehouse_id"].as<int64_t>();
            warehouseAccess(tx, signer, destination);
            require(row["approved_by"].as<int64_t>() != signer, "发运与签收须由不同人员处理", ErrorCode::FORBIDDEN);
            Json::Value shipment;
            Json::Reader reader;
            require(!row["lot_details"].isNull() && reader.parse(row["lot_details"].as<std::string>(), shipment)
                    && shipment["lot_allocations"].isArray(), "调拨发运记录不完整");
            stock(tx,row["product_id"].as<int64_t>(),destination,row["qty"].as<int64_t>(),
                  "transfer_in",row["operation_no"].as<std::string>(),signer,shipment);
            tx->execSqlSync("UPDATE stock_operations SET status=1 WHERE id=?",id);
            tx->execSqlSync("INSERT INTO stock_operation_logs(operation_id,action,operator_id,remark) VALUES (?,'receive',?,'目标仓签收')",id,signer);
            event(tx,"stock_operations",id,"receive",signer,3,1,"目标仓签收");
            Outcome out;
            out.message = "目标仓签收完成，库存已入账";
            out.data["operation_id"] = Json::Int64(id);
            out.data["status"] = 1;
            return out;
        }, requestIdentity(req));
}

void StockOperationController::list(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback)
{ erp::business::readDocuments(req,std::move(callback),"stock_operations"); }

void StockOperationController::serialHistory(
    const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    using namespace business;
    try {
        const auto serial = req->getParameter("serial_no");
        require(!serial.empty() && serial.size()<=512,"请输入完整序列号");
        const auto user = getOperatorId(req);
        const auto db = app().getDbClient();
        const auto users = db->execSqlSync(
            "SELECT u.department,u.warehouse_id,"
            "EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='admin') AS is_admin,"
            "EXISTS(SELECT 1 FROM user_roles ur JOIN roles r ON r.id=ur.role_id WHERE ur.user_id=u.id AND r.code='leader') AS is_leader "
            "FROM user_directory u WHERE u.id=? AND u.status=1",user);
        require(!users.empty(),"账号已停用",ErrorCode::FORBIDDEN);
        const auto identity = users[0];
        const bool admin = identity["is_admin"].as<int>()!=0;
        require(admin || identity["department"].as<std::string>()=="仓库部","仅仓库人员可查序列号",ErrorCode::FORBIDDEN);
        const bool all = admin || identity["is_leader"].as<int>()!=0;
        const int64_t warehouseId = identity["warehouse_id"].isNull()?0:identity["warehouse_id"].as<int64_t>();
        auto rows = db->execSqlSync(
            "SELECT e.*,s.serial_no,s.lot_no,p.name AS product_name,w.name AS warehouse_name,"
            "IF(? OR s.warehouse_id=e.warehouse_id,s.unit_state,NULL) AS unit_state,"
            "IF(? OR s.warehouse_id=e.warehouse_id,s.warehouse_id,NULL) AS current_warehouse_id "
            "FROM inventory_serial_events e JOIN inventory_serial_units s ON s.id=e.unit_id "
            "JOIN products p ON p.id=s.product_id LEFT JOIN warehouses w ON w.id=e.warehouse_id "
            "WHERE s.serial_no=? AND (? OR e.warehouse_id=?) ORDER BY e.id DESC LIMIT 200",
            all,all,serial,all,warehouseId);
        Json::Value out(Json::arrayValue);
        for (const auto &row : rows) out.append(rowJson(rows,row));
        callback(ApiResponse::ok("序列号历史",out));
    } catch (const Error &e) {
        callback(ApiResponse::error(e.code,e.what()));
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
        callback(ApiResponse::error(ErrorCode::INTERNAL_ERROR,"序列号查询失败"));
    }
}

void StockOperationController::openingSerials(
    const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
    using namespace business;
    const auto body = req->getJsonObject();
    if (!body || !body->isObject() || !(*body)["product_id"].isInt64() || !(*body)["warehouse_id"].isInt64()
        || !(*body)["quantity"].isInt64() || !(*body)["reason"].isString()) {
        callback(ApiResponse::error(ErrorCode::BAD_REQUEST,"请填写商品、仓库、数量和期初登记依据"));
        return;
    }
    const auto user = getOperatorId(req);
    transaction(
        [callback](int code,const std::string &message,const Json::Value &data) {
            callback(code==ErrorCode::OK?ApiResponse::ok(message,data):ApiResponse::error(code,message));
        },
        [=](const Tx &tx) {
            const auto pid = (*body)["product_id"].asInt64(), wid = (*body)["warehouse_id"].asInt64();
            const auto qty = (*body)["quantity"].asInt64();
            const auto reason = (*body)["reason"].asString();
            require(qty>0 && !reason.empty() && reason.size()<=255,"请填写正确数量及登记依据");
            warehouseAccess(tx,user,wid,true);
            auto serials = serialList((*body)["serial_nos"],qty);
            const auto product = tx->execSqlSync("SELECT has_serial,batch_enabled FROM products WHERE id=? FOR SHARE",pid);
            require(!product.empty() && product[0]["has_serial"].as<int>(),"商品未启用序列号管理");
            const auto inventory = tx->execSqlSync("SELECT quantity FROM inventory WHERE product_id=? AND warehouse_id=? FOR UPDATE",pid,wid);
            require(!inventory.empty(),"没有可登记的期初库存");
            const auto existing = tx->execSqlSync(
                "SELECT id,lot_no FROM inventory_serial_units WHERE product_id=? AND warehouse_id=? "
                "AND unit_state='in_stock' FOR UPDATE",pid,wid);
            require(static_cast<int64_t>(existing.size())+qty<=inventory[0]["quantity"].as<int64_t>(),
                    "登记数超过未登记库存");
            std::string lot, made, expires;
            if (product[0]["batch_enabled"].as<int>()) {
                require((*body)["lot_no"].isString(),"请指定已登记的期初批次");
                lot=(*body)["lot_no"].asString();
                const auto batches = tx->execSqlSync(
                    "SELECT quantity,manufactured_on,expires_on FROM inventory_lots WHERE product_id=? "
                    "AND warehouse_id=? AND lot_no=? FOR UPDATE",pid,wid,lot);
                require(!batches.empty(),"请先登记期初批次");
                int64_t assigned = 0;
                for (const auto &row : existing) if (row["lot_no"].as<std::string>()==lot) ++assigned;
                const auto row = batches[0];
                require(assigned+qty<=row["quantity"].as<int64_t>(),"登记数超过该批次的未登记库存");
                if (!row["manufactured_on"].isNull()) made=row["manufactured_on"].as<std::string>();
                if (!row["expires_on"].isNull()) expires=row["expires_on"].as<std::string>();
            }
            const auto no = number("SER");
            for (const auto &serial : serials) {
                require(tx->execSqlSync("SELECT id FROM inventory_serial_units WHERE product_id=? AND serial_no=? FOR UPDATE",pid,serial).empty(),
                        "序列号已登记："+serial,ErrorCode::CONFLICT);
                auto inserted = tx->execSqlSync(
                    "INSERT INTO inventory_serial_units(product_id,serial_no,warehouse_id,unit_state,lot_no,manufactured_on,expires_on) "
                    "VALUES(?,?,?,'in_stock',?,NULLIF(?,''),NULLIF(?,''))",pid,serial,wid,lot,made,expires);
                serialEvent(tx,inserted.insertId(),wid,"opening",no,user);
                event(tx,"inventory_serial_units",inserted.insertId(),"opening",user,-1,1,reason);
            }
            Outcome out;
            out.message="期初序列号已登记";
            return out;
        },requestIdentity(req));
}

}  // namespace erp
