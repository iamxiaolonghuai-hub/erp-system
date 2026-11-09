"""Exercise rollback, concurrency, replay and access boundaries using unique disposable fixtures."""
from concurrent.futures import ThreadPoolExecutor
from urllib.request import Request
from urllib.error import HTTPError
from pathlib import Path
import json,uuid
helper=Path(__file__).with_name('verify_product_approvals.py').read_text()
exec(helper[:helper.index('pair=next')])
suffix=uuid.uuid4().hex
def employee(dept):return next(u for u in users if u['department']==dept and 'staff' in u['roles'].split(','))
buyer,seller,warehouse_user=employee('采购部'),employee('销售部'),employee('仓库部')
products=[];docs=[];account_id=None
def key():return 'qa_'+suffix+'_'+uuid.uuid4().hex
def ok(path,user,data):
 status,result=api(path,user,data)
 assert status==200 and result.get('code',0)==0,(path,status,result)
 return result.get('data',result)
def failed(path,user,data,code):
 status,result=api(path,user,data);assert result.get('code')==code,(path,status,result);return result
def read(query):return json.loads(sql(query))
def qty(pid,wid=1):return int(sql(f'SELECT COALESCE((SELECT quantity FROM inventory WHERE product_id={pid} AND warehouse_id={wid}),0)'))
def count(table,where):return int(sql(f'SELECT COUNT(*) FROM {table} WHERE {where}'))
def create_order(kind,items):
 who=buyer if kind=='purchase' else seller
 payload=dict(warehouse_id=1,remark='QA_'+suffix,items=items,request_id=key())
 payload['supplier_id' if kind=='purchase' else 'customer_id']=supplier if kind=='purchase' else customer
 result=ok('/'+kind+'/orders',who,payload);docs.append((kind+'_orders',result['order_id']))
 return result,payload
def approve_order(kind,id):return ok('/'+kind+'/approve',admin,dict(order_id=id,approve=True))
def prepare(id):return ok('/sales/confirm',warehouse_user,dict(order_id=id,approve=True,warehouse_id=1,logistics_company='QA',logistics_no=suffix))
try:
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 ORDER BY id LIMIT 1'))
 customer=int(sql('SELECT id FROM customers WHERE status=1 ORDER BY id LIMIT 1'))
 for n in range(2):
  result=ok('/products',admin,dict(sku='QA_'+suffix[:20]+str(n),name='可靠性验证临时商品',unit='件',price=12.5,supplier_id=supplier,has_serial=0))
  products.append(result['id'])
 p1,p2=products
 # Creation and its lines share a transaction; the nonexistent second line must leave nothing.
 bad=dict(warehouse_id=1,supplier_id=supplier,remark='QA_BAD_'+suffix,items=[dict(product_id=p1,quantity=1,price=12.5),dict(product_id=9223372036854775000,quantity=1,price=1)],request_id=key())
 failed('/purchase/orders',buyer,bad,40000)
 assert count('purchase_orders',f"remark='QA_BAD_{suffix}'")==0
 print('PASS invalid second line rolls back the order and first line')
 order,payload=create_order('purchase',[dict(product_id=p1,quantity=20,price=12.5),dict(product_id=p2,quantity=20,price=12.5)])
 replay=ok('/purchase/orders',buyer,payload);assert replay['order_id']==order['order_id']
 assert count('purchase_orders',f"order_no='{order['order_no']}'")==1
 approve_order('purchase',order['order_id']);ok('/purchase/inbound',admin,dict(order_id=order['order_id'],approve=True))
 failed('/purchase/inbound',admin,dict(order_id=order['order_id'],approve=True),40900)
 assert qty(p1)==20 and qty(p2)==20
 assert count('stock_records',f"biz_no='{order['order_no']}'")==2
 assert count('accounts',f"biz_no='{order['order_no']}'")==1
 print('PASS request replay and repeated receipt do not duplicate inventory or accounts')
 sale,_=create_order('sales',[dict(product_id=p1,quantity=6,price=12.5),dict(product_id=p2,quantity=99,price=12.5)])
 approve_order('sales',sale['order_id'])
 failed('/sales/confirm',warehouse_user,dict(order_id=sale['order_id'],approve=True,warehouse_id=1),41001)
 assert qty(p1)==20 and qty(p2)==20
 assert count('stock_records',f"biz_no='{sale['order_no']}'")==0
 assert count('accounts',f"biz_no='{sale['order_no']}'")==0
 assert sql(f"SELECT status FROM sales_orders WHERE id={sale['order_id']}")=='1'
 print('PASS second-line stock shortage rolls back the first-line deduction and every posting')
 # Repair only the disposable fixture, then retry the same failed document.
 sql(f'UPDATE inventory SET quantity=100 WHERE product_id={p2} AND warehouse_id=1')
 prepare(sale['order_id']);ok('/sales/outbound',admin,dict(order_id=sale['order_id'],approve=True));assert qty(p1)==14 and qty(p2)==1
 failed('/sales/outbound',admin,dict(order_id=sale['order_id'],approve=True),40900)
 assert count('stock_records',f"biz_no='{sale['order_no']}'")==2
 code,history=api('/approval/history?document_type=sales_orders&document_id='+str(sale['order_id']),seller)
 assert code==200 and [row['action'] for row in history]==['submit','approve','prepare','ship'],history
 print('PASS retry after a rollback commits exactly once')
 competing=[]
 for _ in range(2):
  entry,_=create_order('sales',[dict(product_id=p1,quantity=10,price=12.5)])
  approve_order('sales',entry['order_id']);competing.append(entry)
 with ThreadPoolExecutor(max_workers=2) as pool:
  results=list(pool.map(lambda entry:api('/sales/confirm',warehouse_user,dict(order_id=entry['order_id'],approve=True,warehouse_id=1)),competing))
 codes=sorted(r[1].get('code',0) for r in results);assert codes==[0,41001],results
 winner=competing[next(i for i,r in enumerate(results) if r[1].get('code')==0)]
 ok('/sales/outbound',admin,dict(order_id=winner['order_id'],approve=True))
 assert qty(p1)==4
 print('PASS concurrent sales cannot oversell the same product')
 # The destination overflow happens after the source stock update; both must roll back.
 sql(f'INSERT INTO inventory (product_id,warehouse_id,quantity,min_quantity) VALUES ({p1},2,2147483647,0) ON DUPLICATE KEY UPDATE quantity=2147483647')
 operation=ok('/stock/operation',warehouse_user,dict(type='transfer',product_id=p1,qty=1,from_warehouse_id=1,to_warehouse_id=2,request_id=key()))
 docs.append(('stock_operations',operation['operation_id']))
 failed('/stock/operation/approve',admin,dict(operation_id=operation['operation_id'],approve=True),40000)
 assert qty(p1)==4 and qty(p1,2)==2147483647
 assert count('stock_records',f"biz_no='{operation['operation_no']}'")==0
 print('PASS failed destination movement rolls back the source warehouse')
 # Return quantity and price are controlled by original order records.
 failed('/sales/return',seller,dict(order_id=sale['order_id'],warehouse_id=1,items=[dict(product_id=p1,quantity=7,price=0)],request_id=key()),40000)
 returned=ok('/sales/return',seller,dict(order_id=sale['order_id'],warehouse_id=1,items=[dict(product_id=p1,quantity=1,price=0)],request_id=key()))
 docs.append(('sales_returns',returned['return_id']))
 assert sql(f"SELECT total_amount FROM sales_returns WHERE id={returned['return_id']}")=='12.50'
 print('PASS excess returns rejected and refund amount ignores a tampered client price')
 # Stock-check snapshot must still match the ledger at review time.
 check=ok('/stockcheck/create',warehouse_user,dict(warehouse_id=1,counted_by=admin['id'],items=[dict(product_id=p1,book_qty=4,actual_qty=5)],request_id=key()))
 docs.append(('stock_checks',check['check_id']))
 assert int(sql(f"SELECT counted_by FROM stock_checks WHERE id={check['check_id']}"))==warehouse_user['id']
 code,visible_checks=api('/stockchecks',admin)
 counted=next(r for r in visible_checks if r['id']==check['check_id'])
 assert counted['counter_name'] and counted['counted_by']==warehouse_user['id'],counted
 failed('/stockcheck/complete',warehouse_user,dict(check_id=check['check_id']),40300)
 sql(f'UPDATE inventory SET quantity=6 WHERE product_id={p1} AND warehouse_id=1')
 failed('/stockcheck/complete',admin,dict(check_id=check['check_id']),40900);assert qty(p1)==6
 sql(f'UPDATE inventory SET quantity=4 WHERE product_id={p1} AND warehouse_id=1')
 ok('/stockcheck/complete',admin,dict(check_id=check['check_id']));assert qty(p1)==5
 failed('/stockcheck/complete',admin,dict(check_id=check['check_id']),40900)
 print('PASS stock checks require independent review and detect stale inventory snapshots')
 # Payment registration, balance changes and request replay are atomic.
 no='QA_PAY_'+suffix
 sql(f"INSERT INTO accounts (account_type,biz_type,biz_no,partner_id,partner_name,amount,paid_amount,status) VALUES ('receivable','qa','{no}',{customer},'QA',100,0,0)")
 account_id=int(sql(f"SELECT id FROM accounts WHERE biz_type='qa' AND biz_no='{no}'"))
 payment=dict(account_id=account_id,amount=80,pay_date='2026-10-31',pay_method='QA',request_id=key())
 registered=ok('/finance/payments',admin,payment);replayed=ok('/finance/payments',admin,payment)
 assert registered['action_id']==replayed['action_id']
 reviewer=employee('财务部')
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={account_id}')=='0.00'
 ok('/finance/actions/review',reviewer,dict(action_id=registered['action_id'],approve=True))
 failed('/finance/payments',admin,{**payment,'request_id':key()},40000)
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={account_id}')=='80.00'
 failed('/finance/payments',admin,{**payment,'request_id':key(),'amount':1,'pay_date':'2026-02-31'},40000)
 actions=[ok('/finance/payments',admin,{**payment,'request_id':key(),'amount':15})['action_id'] for _ in range(2)]
 with ThreadPoolExecutor(max_workers=2) as pool:
  attempts=list(pool.map(lambda id:api('/finance/actions/review',reviewer,dict(action_id=id,approve=True)),actions))
 assert sorted(r[1].get('code',0) for r in attempts)==[0,40900],attempts
 assert sql(f'SELECT paid_amount FROM accounts WHERE id={account_id}')=='95.00'
 print('PASS reviewed payment replay and concurrent overpayment protection')
 # Back-end data scope and live-role lookup cannot be bypassed by query or stale claims.
 code,data=api('/salesorders',buyer);assert isinstance(data,list) and sale['order_id'] not in [r['id'] for r in data],data
 code,data=api('/salesorders/'+str(sale['order_id']),buyer);assert code==400 or code==404 or data.get('code')==40400,data
 fake=dict(buyer,roles='admin');failed('/customers',fake,dict(name='QA_FORBIDDEN_'+suffix),40300)
 own=ok('/purchase/orders',admin,dict(supplier_id=supplier,warehouse_id=1,items=[dict(product_id=p1,quantity=1,price=12.5)],request_id=key()))
 docs.append(('purchase_orders',own['order_id']))
 failed('/purchase/approve',admin,dict(order_id=own['order_id'],approve=True),40300)
 try:
  opener.open(Request('http://127.0.0.1:5555/api/customers'),timeout=15);raise AssertionError('unauthenticated customer list allowed')
 except HTTPError as e:assert e.code==401
 print('PASS unauthorized lists, details, direct writes and forged role claims rejected')
 stats=ok('/inventory/summary',admin,None)
 inventory=api('/inventory',admin)[1]
 assert stats['low_stock']==sum(r['quantity']<=r['min_quantity'] for r in inventory)
 assert stats['record_count']==count('stock_records','1=1')
 for route in ['/permissions','/rolepermissions']:
  try:
   opener.open(Request('http://127.0.0.1:5555/api'+route,headers={'Authorization':'Bearer '+token(admin)}),timeout=15);raise AssertionError('removed module still exposed')
  except HTTPError as e:assert e.code==404
 print('PASS safety-stock summary matches visible warning records; removed permission APIs are unavailable')
 print('ALL RELIABILITY CHECKS PASSED')
finally:
 # Every cleanup is restricted to this test's generated IDs or unique marker.
 for table,id in docs:
  if table in ('purchase_orders','sales_orders'):
   no=sql(f'SELECT order_no FROM {table} WHERE id={id}')
   sql(f"DELETE FROM accounts WHERE biz_no='{no}' AND biz_type IN ('purchase','sale')")
  if table=='stock_operations':sql(f'DELETE FROM stock_operation_logs WHERE operation_id={id}')
 for table,id in reversed(docs):
  if table=='stock_checks':sql(f'DELETE FROM stock_check_items WHERE check_id={id}')
  
  if table in ('purchase_orders','sales_orders'):sql(f"DELETE FROM order_deliveries WHERE order_type='{ 'purchase' if table=='purchase_orders' else 'sales'}' AND order_id={id}")
  sql(f'DELETE FROM {table} WHERE id={id}')
  sql(f"DELETE FROM business_events WHERE document_type='{table}' AND document_id={id}")
 if account_id:
  ids=sql(f'SELECT id FROM payments WHERE account_id={account_id}').splitlines()
  for id in ids:sql(f"DELETE FROM business_events WHERE document_type='payments' AND document_id={int(id)}")
  sql(f"DELETE FROM business_events WHERE document_type='finance_actions' AND document_id IN (SELECT id FROM finance_actions WHERE account_id={account_id})")
  sql(f'DELETE FROM finance_actions WHERE account_id={account_id}')
  sql(f'DELETE FROM accounts WHERE id={account_id}')
 if products:
  ids=','.join(map(str,products));sql(f'DELETE FROM stock_records WHERE product_id IN ({ids})');sql(f'DELETE FROM inventory WHERE product_id IN ({ids})');sql(f'DELETE FROM products WHERE id IN ({ids})')
 sql(f"DELETE FROM business_requests WHERE request_key LIKE 'qa_{suffix}_%'")
 print('Unique test fixtures removed; existing business records retained')
