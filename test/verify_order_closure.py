from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
tag='QA_CLOSE_'+uuid.uuid4().hex;product=None;order=None;returned=None
buyer=next(u for u in users if u['department']=='采购部' and 'staff' in u['roles'])
def ok(path,user,data=None):
 _,r=api(path,user,data);assert r.get('code')==0,r;return r['data']
try:
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'))
 _,r=api('/products',admin,dict(sku=tag,name=tag,supplier_id=supplier,unit='件',price=10,has_serial=0));product=int(r['id'])
 order=ok('/purchase/orders',buyer,dict(supplier_id=supplier,warehouse_id=1,items=[dict(product_id=product,quantity=5,price=10)]))
 ok('/purchase/approve',admin,dict(order_id=order['order_id'],approve=True))
 ok('/orders/batch',admin,dict(type='purchase',step='receive',order_id=order['order_id'],warehouse_id=1,items=[dict(product_id=product,quantity=2)],request_id=uuid.uuid4().hex))
 _,r=api('/orders/lifecycle',buyer,dict(type='purchase',action='close',order_id=order['order_id'],reason=tag));assert r.get('code')==40300,r
 ok('/orders/lifecycle',admin,dict(type='purchase',action='close',order_id=order['order_id'],reason=tag))
 _,r=api('/orders/batch',admin,dict(type='purchase',step='receive',order_id=order['order_id'],warehouse_id=1,items=[dict(product_id=product,quantity=3)]));assert r.get('code')==40900,r
 assert sql(f"SELECT amount FROM accounts WHERE biz_no='{order['order_no']}'")=='20.00'
 _,r=api('/purchase/return',buyer,dict(order_id=order['order_id'],warehouse_id=1,items=[dict(product_id=product,quantity=3)]));assert r.get('code')==40000,r
 returned=ok('/purchase/return',buyer,dict(order_id=order['order_id'],warehouse_id=1,items=[dict(product_id=product,quantity=1)]))
 ok('/purchase/return/approve',admin,dict(return_id=returned['return_id'],approve=True))
 ok('/purchase/return/warehouse',admin,dict(return_id=returned['return_id'],approve=True))
 credit_id=int(sql(f"SELECT id FROM accounts WHERE biz_no=(SELECT return_no FROM purchase_returns WHERE id={returned['return_id']})"))
 advice=ok('/finance/settlement-advice?account_id='+str(credit_id),admin)
 assert advice['source_biz_no']==order['order_no'] and advice['suggested_offset']=='10.00' and advice['suggested_refund']=='0.00',advice
 print('PASS settlement advice links original invoice and prefers offset for unpaid purchases')
 print('PASS partial-order closure, no further receipt, posted amount unchanged, employee forbidden, returns limited to actual received quantity')
finally:
 if returned:
  rid=returned['return_id'];sql(f"DELETE FROM accounts WHERE biz_no=(SELECT return_no FROM purchase_returns WHERE id={rid})");sql(f'DELETE FROM purchase_returns WHERE id={rid}');sql(f"DELETE FROM business_events WHERE document_type='purchase_returns' AND document_id={rid}")
 if order:
  oid=order['order_id'];sql(f"DELETE FROM accounts WHERE biz_no='{order['order_no']}'");sql(f"DELETE FROM order_deliveries WHERE order_type='purchase' AND order_id={oid}");sql(f'DELETE FROM purchase_orders WHERE id={oid}');sql(f"DELETE FROM business_events WHERE document_type='purchase_orders' AND document_id={oid}")
 if product:
  sql(f'DELETE FROM stock_records WHERE product_id={product}');sql(f'DELETE FROM inventory WHERE product_id={product}');sql(f'DELETE FROM products WHERE id={product}')
