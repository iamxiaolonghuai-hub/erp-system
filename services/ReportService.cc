#include "ReportService.h"
#include "BusinessSupport.h"
#include "DocumentAccess.h"
namespace erp {
namespace {
void query(const std::string &sql,ReportCallback cb,bool single=false){try{auto rows=drogon::app().getDbClient()->execSqlSync(sql);Json::Value result(single?Json::objectValue:Json::arrayValue);for(auto row:rows){auto value=business::rowJson(rows,row);if(single)result=value;else result.append(value);}cb(ErrorCode::OK,"按已过账账目统计，退货单独列示，未结余额扣除现金及抵扣",result);}catch(const std::exception &e){LOG_ERROR<<e.what();cb(ErrorCode::INTERNAL_ERROR,"报表查询失败",Json::Value());}}
}
void ReportService::dashboard(ReportCallback &&cb){query(
 "SELECT (SELECT COUNT(*) FROM products) AS product_count,(SELECT COALESCE(SUM(quantity),0) FROM inventory) AS inventory_total,(SELECT COUNT(*) FROM inventory WHERE quantity<=min_quantity) AS low_stock,"
 "COALESCE(SUM(CASE WHEN biz_type='purchase' THEN amount ELSE 0 END),0) AS purchase_total,COALESCE(SUM(CASE WHEN biz_type='sale' THEN amount ELSE 0 END),0) AS sales_total,"
 "COALESCE(SUM(CASE WHEN biz_type='purchase_return' THEN -amount ELSE 0 END),0) AS purchase_returns,COALESCE(SUM(CASE WHEN biz_type='sales_return' THEN -amount ELSE 0 END),0) AS sales_returns,"
 "IF((SELECT COUNT(*) FROM stock_records WHERE biz_type IN ('sale_out','sales_return','sales_exchange_out') AND cost_value IS NULL)=0 AND (SELECT COUNT(*) FROM accounts a WHERE a.biz_type IN ('sale','sales_return') AND NOT EXISTS(SELECT 1 FROM stock_records s WHERE s.biz_no=a.biz_no AND s.biz_type IN ('sale_out','sales_return')))=0,COALESCE(SUM(CASE WHEN biz_type IN ('sale','sales_return') THEN amount ELSE 0 END),0)-(SELECT COALESCE(SUM(CASE WHEN biz_type IN ('sale_out','sales_exchange_out') THEN cost_value WHEN biz_type='sales_return' THEN -cost_value ELSE 0 END),0) FROM stock_records),NULL) AS gross_profit,COALESCE(SUM(CASE WHEN account_type='receivable' AND amount>0 THEN amount-paid_amount-offset_amount ELSE 0 END),0) AS receivable,"
 "COALESCE(SUM(CASE WHEN account_type='payable' AND amount>0 THEN amount-paid_amount-offset_amount ELSE 0 END),0) AS payable,"
 "COALESCE(SUM(CASE WHEN account_type='receivable' AND amount<0 THEN -(amount-paid_amount-offset_amount) ELSE 0 END),0) AS customer_credit,"
 "COALESCE(SUM(CASE WHEN account_type='payable' AND amount<0 THEN -(amount-paid_amount-offset_amount) ELSE 0 END),0) AS supplier_credit FROM accounts",std::move(cb),true);}
void ReportService::purchaseSummary(ReportCallback &&cb){query("SELECT partner_id AS supplier_id,SUM(CASE WHEN amount>0 THEN amount ELSE 0 END) AS total_amount,SUM(CASE WHEN amount<0 THEN -amount ELSE 0 END) AS return_amount,SUM(amount) AS net_amount,COUNT(DISTINCT CASE WHEN biz_type='purchase' THEN biz_no END) AS order_count FROM accounts WHERE biz_type IN ('purchase','purchase_return') GROUP BY partner_id",std::move(cb));}
void ReportService::salesSummary(ReportCallback &&cb){query("SELECT partner_id AS customer_id,SUM(CASE WHEN amount>0 THEN amount ELSE 0 END) AS total_amount,SUM(CASE WHEN amount<0 THEN -amount ELSE 0 END) AS return_amount,SUM(amount) AS net_amount,COUNT(DISTINCT CASE WHEN biz_type='sale' THEN biz_no END) AS order_count FROM accounts WHERE biz_type IN ('sale','sales_return') GROUP BY partner_id",std::move(cb));}
void ReportService::inventorySummary(ReportCallback &&cb){query("SELECT warehouse_id,COUNT(*) AS product_count,SUM(quantity) AS total_quantity,SUM(reserved_qty) AS reserved_qty,SUM(quantity-reserved_qty) AS available_qty FROM inventory GROUP BY warehouse_id",std::move(cb));}
}
