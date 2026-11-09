from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
from concurrent.futures import ThreadPoolExecutor
suffix=uuid.uuid4().hex;tag='QA_BATCH_'+suffix;orders=[];operations=[];product=None
buyer=next(u for u in users if u['department']=='采购部' and 'staff' in u['roles'])
seller=next(u for u in users if u['department']=='销售部' and 'staff' in u['roles'])
warehouse=next(u for u in users if u['department']=='仓库部' and 'staff' in u['roles'])
reviewer=next(u for u in users if u['department']=='财务部')
def ok(path,user,data=None):
 _,r=api(path,user,data);assert r.get('code')==0,r;return r['data']
def batch(kind,step,id,qty,user=warehouse,key=None):return api('/orders/batch',user,dict(type=kind,step=step,order_id=id,warehouse_id=1,items=[dict(product_id=product,quantity=qty)],request_id=key or uuid.uuid4().hex))
def order(kind,qty):
 partner=dict(supplier_id=supplier) if kind=='purchase' else dict(customer_id=customer)
 r=ok('/'+kind+'/orders',buyer if kind=='purchase' else seller,dict(**partner,warehouse_id=1,remark=tag,items=[dict(product_id=product,quantity=qty,price=10)],request_id=uuid.uuid4().hex));orders.append((kind,r['order_id'],r['order_no']));ok('/'+kind+'/approve',admin,dict(order_id=r['order_id'],approve=True));return r['order_id']
def stock():return sql(f'SELECT CONCAT(quantity,":",reserved_qty) FROM inventory WHERE product_id={product} AND warehouse_id=1')
try:
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'));customer=int(sql('SELECT id FROM customers WHERE status=1 LIMIT 1'))
 _,r=api('/products',admin,dict(sku=tag,name=tag,supplier_id=supplier,price=10,unit='件',has_serial=0));product=int(r['id'])
 purchase=order('purchase',10)
 assert batch('purchase','receive',purchase,3)[1]['code']==0;assert stock()=='3:0'
 assert sql(f'SELECT status FROM purchase_orders WHERE id={purchase}')=='1'
 replay=uuid.uuid4().hex
 with ThreadPoolExecutor(2) as pool:results=list(pool.map(lambda _:batch('purchase','receive',purchase,7,key=replay),range(2)))
 assert all(r[1]['code']==0 for r in results),results
 assert stock()=='10:0'
 assert sql(f'SELECT COUNT(*) FROM order_deliveries WHERE order_type="purchase" AND order_id={purchase}')=='2'
 sales=[order('sales',8),order('sales',8)]
 with ThreadPoolExecutor(2) as pool:results=list(pool.map(lambda id:batch('sales','prepare',id,6),sales))
 assert sorted(r[1]['code'] for r in results)==[0,41001],results
 winner=sales[next(i for i,r in enumerate(results) if r[1]['code']==0)];assert stock()=='10:6'
 operation=ok('/stock/operation',warehouse,dict(type='out',product_id=product,qty=5,from_warehouse_id=1,request_id=uuid.uuid4().hex));operations.append(operation['operation_id'])
 _,blocked=api('/stock/operation/approve',admin,dict(operation_id=operation['operation_id'],approve=True));assert blocked['code']==41001,blocked
 assert stock()=='10:6'

 assert batch('sales','release',winner,6,admin)[1]['code']==0;assert stock()=='4:0'
 assert sql(f'SELECT status FROM sales_orders WHERE id={winner}')=='1'
 assert batch('sales','prepare',winner,2)[1]['code']==0
 releaseKey=uuid.uuid4().hex
 with ThreadPoolExecutor(2) as pool:results=list(pool.map(lambda _:batch('sales','release',winner,2,admin,releaseKey),range(2)))
 assert all(r[1]['code']==0 for r in results),results
 assert stock()=='2:0'
 no=next(no for kind,id,no in orders if id==winner and kind=='sales')
 assert sql(f"SELECT amount FROM accounts WHERE biz_type='sale' AND biz_no='{no}'")=='80.00'
 assert batch('sales','release',winner,2,admin)[1]['code']!=0
 # Cancellation releases a prepared, unposted batch.
 remaining=next(id for id in sales if id!=winner)
 assert batch('sales','prepare',remaining,2)[1]['code']==0;assert stock()=='2:2'
 ok('/orders/lifecycle',admin,dict(type='sales',order_id=remaining,action='void',reason=tag));assert stock()=='2:0'
 # Financial review races are serialized at the account/partner boundary.
 account=int(sql(f"SELECT id FROM accounts WHERE biz_type='sale' AND biz_no='{no}'"))
 actions=[ok('/finance/actions',admin,dict(kind='payment',account_id=account,amount=60,pay_date='2026-10-31',remark=tag,request_id=uuid.uuid4().hex))['action_id'] for _ in range(2)]
 with ThreadPoolExecutor(2) as pool:results=list(pool.map(lambda id:api('/finance/actions/review',reviewer,dict(action_id=id,approve=True)),actions))
 assert sorted(r[1]['code'] for r in results)==[0,40900],results
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={account}')=='60.00'
 stats=ok('/report/dashboard',admin)
 assert float(stats['sales_total'])==float(sql("SELECT COALESCE(SUM(amount),0) FROM accounts WHERE biz_type='sale'"))
 if stats['gross_profit'] is not None:assert float(stats['gross_profit'])==float(sql("SELECT COALESCE(SUM(CASE WHEN biz_type IN ('sale','sales_return') THEN amount ELSE 0 END),0) FROM accounts"))-float(sql("SELECT COALESCE(SUM(CASE WHEN biz_type IN ('sale_out','sales_exchange_out') THEN cost_value WHEN biz_type='sales_return' THEN -cost_value ELSE 0 END),0) FROM stock_records"))
 print('PASS partial receipt, concurrent request replay, concurrent reservations, partial shipment, release replay, cancellation release, concurrent financial review, posted-report totals')
finally:
 for id in operations:
  sql(f"DELETE FROM stock_operation_logs WHERE operation_id={id}")
  sql(f"DELETE FROM business_events WHERE document_type='stock_operations' AND document_id={id}")
  sql(f"DELETE FROM stock_operations WHERE id={id}")
 for kind,id,no in orders:
  for table in ['finance_actions','payments']:
   sql(f"DELETE FROM business_events WHERE document_type='{table}' AND document_id IN (SELECT id FROM {table} WHERE account_id IN (SELECT id FROM accounts WHERE biz_no='{no}'))")
  sql(f"DELETE FROM finance_actions WHERE account_id IN (SELECT id FROM accounts WHERE biz_no='{no}')")
  sql(f"DELETE FROM accounts WHERE biz_no='{no}'")
  sql(f"DELETE FROM order_deliveries WHERE order_type='{kind}' AND order_id={id}")
  sql(f"DELETE FROM business_events WHERE document_type='{kind}_orders' AND document_id={id}")
  sql(f'DELETE FROM {kind}_orders WHERE id={id}')
 if product:
  sql(f'DELETE FROM stock_records WHERE product_id={product}');sql(f'DELETE FROM inventory WHERE product_id={product}');sql(f'DELETE FROM products WHERE id={product}')
