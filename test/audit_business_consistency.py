"""Read-only checks of stock reservations, batch quantities and account balances."""
from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
checks={
 'employee_department_disagrees_with_account':"SELECT COUNT(*) FROM employees e JOIN users u ON u.id=e.user_id WHERE NOT(e.department_id<=>u.department_id)",
 'delivery_header_disagrees_with_lines':"SELECT COUNT(*) FROM order_deliveries d LEFT JOIN (SELECT delivery_id,SUM(quantity*unit_price) amount FROM order_delivery_items GROUP BY delivery_id) i ON i.delivery_id=d.id WHERE i.delivery_id IS NULL OR d.amount<>i.amount",
 'known_inventory_value_disagrees_with_cost_ledger':"SELECT COUNT(*) FROM inventory i LEFT JOIN inventory_cost_openings o ON o.id=(SELECT MAX(o2.id) FROM inventory_cost_openings o2 WHERE o2.inventory_id=i.id) WHERE i.cost_initialized=1 AND NOT EXISTS(SELECT 1 FROM stock_records s WHERE s.product_id=i.product_id AND s.warehouse_id=i.warehouse_id AND s.id>COALESCE(o.baseline_record_id,0) AND s.cost_value IS NULL) AND i.cost_value<>COALESCE(o.total_value,0)+COALESCE((SELECT SUM(IF(s.change_qty>0,s.cost_value,-s.cost_value)) FROM stock_records s WHERE s.product_id=i.product_id AND s.warehouse_id=i.warehouse_id AND s.id>COALESCE(o.baseline_record_id,0)),0)",
 'negative_inventory_value':"SELECT COUNT(*) FROM inventory WHERE cost_initialized=1 AND cost_value<0",
 'empty_inventory_with_value':"SELECT COUNT(*) FROM inventory WHERE quantity=0 AND cost_value<>0",
 'lot_quantity_disagrees_with_stock':"SELECT COUNT(*) FROM inventory i JOIN products p ON p.id=i.product_id LEFT JOIN (SELECT product_id,warehouse_id,SUM(quantity) AS q FROM inventory_lots GROUP BY product_id,warehouse_id) l ON l.product_id=i.product_id AND l.warehouse_id=i.warehouse_id WHERE p.batch_enabled=1 AND i.quantity<>COALESCE(l.q,0)",
 'invalid_lot_quantities_or_dates':"SELECT COUNT(*) FROM inventory_lots l JOIN products p ON p.id=l.product_id WHERE l.quantity<0 OR (l.quantity>0 AND p.expiry_enabled=1 AND (l.manufactured_on IS NULL OR l.expires_on IS NULL OR l.manufactured_on>l.expires_on))",
 'negative_or_over_reserved_inventory':"SELECT COUNT(*) FROM inventory WHERE quantity<0 OR reserved_qty<0 OR reserved_qty>quantity",
 'reservation_differs_from_pending_batches':"SELECT COUNT(*) FROM inventory i LEFT JOIN (SELECT s.product_id,o.warehouse_id,SUM(s.pending_qty) AS q FROM sales_order_items s JOIN sales_orders o ON o.id=s.order_id WHERE o.status=5 GROUP BY s.product_id,o.warehouse_id) p ON p.product_id=i.product_id AND p.warehouse_id=i.warehouse_id WHERE i.reserved_qty<>COALESCE(p.q,0)",
 'invalid_sales_completion':"SELECT COUNT(*) FROM sales_order_items WHERE shipped_qty<0 OR shipped_qty>quantity OR pending_qty<0 OR pending_qty>quantity-shipped_qty",
 'invalid_purchase_completion':"SELECT COUNT(*) FROM purchase_order_items WHERE received_qty<0 OR received_qty>quantity",
 'pending_quantities_on_non_pending_sales':"SELECT COUNT(*) FROM sales_order_items s JOIN sales_orders o ON o.id=s.order_id WHERE s.pending_qty>0 AND o.status<>5",
 'invalid_positive_account_settlement':"SELECT COUNT(*) FROM accounts WHERE amount>=0 AND (paid_amount<0 OR offset_amount<0 OR paid_amount+offset_amount>amount)",
 'invalid_credit_settlement':"SELECT COUNT(*) FROM accounts WHERE amount<0 AND (paid_amount>0 OR offset_amount>0 OR paid_amount+offset_amount<amount)",
 'account_status_disagrees_with_balance':"SELECT COUNT(*) FROM accounts WHERE (status=1 AND amount<>paid_amount+offset_amount) OR (status=0 AND amount=paid_amount+offset_amount AND amount<>0)",
 'self_reviewed_financial_actions':"SELECT COUNT(*) FROM finance_actions WHERE status=1 AND created_by=approved_by",
 'batch_total_disagrees_with_account':"SELECT COUNT(*) FROM (SELECT d.order_type,o.order_no,SUM(d.amount) AS total FROM order_deliveries d JOIN purchase_orders o ON o.id=d.order_id WHERE d.order_type='purchase' GROUP BY d.order_type,o.order_no UNION ALL SELECT d.order_type,o.order_no,SUM(d.amount) FROM order_deliveries d JOIN sales_orders o ON o.id=d.order_id WHERE d.order_type='sales' GROUP BY d.order_type,o.order_no) b LEFT JOIN accounts a ON a.biz_no=b.order_no AND a.biz_type=IF(b.order_type='purchase','purchase','sale') WHERE a.id IS NULL OR a.amount<>b.total",
}
failed=[]
for name,query in checks.items():
 count=int(sql(query));print(('PASS' if count==0 else 'FINDING'),name,count)
 if count:failed.append((name,count))
if failed:raise SystemExit('Existing-data inconsistencies found; no records changed: '+str(failed))
print('PASS all read-only business consistency checks; no data modified')
