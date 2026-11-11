#pragma once
#include "BusinessSupport.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace erp::business {
inline std::vector<std::string> serialList(const Json::Value &input, int64_t quantity) {
    require(input.isString() && input.asString().size() <= 256000 && quantity <= 500,
            "请填写本批序列号，每批最多500件");
    std::vector<std::string> result;
    std::set<std::string> unique;
    std::istringstream stream(input.asString());
    std::string raw;
    while (std::getline(stream,raw,',')) {
        const auto first = raw.find_first_not_of(" \t\r\n");
        require(first != std::string::npos,"序列号不能为空");
        const auto value = raw.substr(first,raw.find_last_not_of(" \t\r\n")-first+1);
        const auto characters = std::count_if(value.begin(),value.end(),
            [](unsigned char byte) { return (byte & 0xc0) != 0x80; });
        require(characters <= 128 && value.size() <= 512 && unique.insert(value).second,
                "序列号过长或重复");
        result.push_back(value);
    }
    require(static_cast<int64_t>(result.size()) == quantity,"序列号数量必须与处理数量一致");
    std::sort(result.begin(),result.end());
    return result;
}

inline void serialEvent(const Tx &tx, int64_t unit, int64_t warehouse,
                        const std::string &action, const std::string &no, int64_t user) {
    tx->execSqlSync("INSERT INTO inventory_serial_events(unit_id,warehouse_id,action,biz_no,operator_id) "
                    "VALUES(?,?,?,?,?)",unit,warehouse,action,no,user);
}

inline void changeSerialStock(const Tx &tx, int64_t pid, int64_t wid, int64_t delta, int64_t book,
                              const std::string &type, const std::string &no, int64_t user,
                              bool batched, Json::Value &details) {
    const auto serials = serialList(details["serial_nos"],delta>0?delta:-delta);
    auto count = tx->execSqlSync(
        "SELECT id FROM inventory_serial_units WHERE product_id=? AND warehouse_id=? AND unit_state='in_stock' FOR UPDATE",
        pid,wid);
    require(static_cast<int64_t>(count.size()) == book,"库存序列号未配齐，请先登记期初序列号",ErrorCode::CONFLICT);
    std::map<std::string,Json::Value> lots;
    for (const auto &serial : serials) {
        const auto rows = tx->execSqlSync(
            "SELECT * FROM inventory_serial_units WHERE product_id=? AND serial_no=? FOR UPDATE",pid,serial);
        if (delta > 0 && type != "sales_return" && type != "transfer_in") {
            require(rows.empty(),"序列号已登记："+serial,ErrorCode::CONFLICT);
            const auto lot = details.get("lot_no","").asString();
            const auto made = details.get("manufactured_on","").asString();
            const auto expires = details.get("expires_on","").asString();
            auto unit = tx->execSqlSync(
                "INSERT INTO inventory_serial_units(product_id,serial_no,warehouse_id,unit_state,lot_no,"
                "manufactured_on,expires_on,purchase_no) VALUES(?,?,?,'in_stock',?,NULLIF(?,''),NULLIF(?,''),?)",
                pid,serial,wid,lot,made,expires,std::string(type=="purchase_in"?no:""));
            serialEvent(tx,unit.insertId(),wid,type,no,user);
            continue;
        }

        require(!rows.empty(),"未登记序列号："+serial);
        const auto row = rows[0];
        const auto state = row["unit_state"].as<std::string>();
        const auto unit = row["id"].as<int64_t>();
        if (delta < 0) {
            require(state=="in_stock" && !row["warehouse_id"].isNull() && row["warehouse_id"].as<int64_t>()==wid,
                    "序列号不在当前仓库："+serial,ErrorCode::CONFLICT);
            if (type=="purchase_return") require(row["purchase_no"].as<std::string>()==details["original_order"].asString(),
                    "序列号不属于原采购单："+serial);
            const bool sale = type=="sale_out" || type=="sales_exchange_out";
            const auto saleNo = type=="sale_out"?no:sale?details["original_order"].asString():"";
            tx->execSqlSync(
                "UPDATE inventory_serial_units SET unit_state=?,warehouse_id=NULL,sale_no=?,transfer_no=? WHERE id=?",
                sale?"sold":type=="transfer_out"?"in_transit":"removed",saleNo,std::string(type=="transfer_out"?no:""),unit);
        } else {
            const bool eligible = type=="sales_return"
                ? state=="sold" && row["sale_no"].as<std::string>()==details["original_order"].asString()
                : state=="in_transit" && row["transfer_no"].as<std::string>()==no;
            require(eligible,"序列号与原业务单据不一致："+serial,ErrorCode::CONFLICT);
            tx->execSqlSync("UPDATE inventory_serial_units SET unit_state='in_stock',warehouse_id=?,sale_no='',transfer_no='' WHERE id=?",wid,unit);
        }
        if (batched) {
            const auto name = row["lot_no"].as<std::string>();
            require(!name.empty(),"序列号缺少批次归属："+serial);
            auto &lot = lots[name];
            lot["lot_no"] = name;
            lot["manufactured_on"] = row["manufactured_on"].isNull()?"":row["manufactured_on"].as<std::string>();
            lot["expires_on"] = row["expires_on"].isNull()?"":row["expires_on"].as<std::string>();
            lot["quantity"] = lot.get("quantity",0).asInt()+1;
        }
        serialEvent(tx,unit,wid,type,no,user);
    }
    if (!lots.empty()) {
        details["lot_allocations"] = Json::Value(Json::arrayValue);
        for (const auto &[name, lot] : lots) details["lot_allocations"].append(lot);
    }
}
}
