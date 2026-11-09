from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
tag='QA_LOTCOST_'+uuid.uuid4().hex;products=[];orders=[];operations=[];returns=[]
buyer=next(u for u in users if u['department']=='采购部' and 'staff' in u['roles'])
seller=next(u for u in users if u['department']=='销售部' and 'staff' in u['roles'])
warehouse=next(u for u in users if u['department']=='仓库部' and 'staff' in u['roles'])
def ok(path,user,data=None):
 _,r=api(path,user,data);assert r.get('code')==0,r;return r['data']
def make_product():
 _,r=api('/products',admin,dict(sku=tag+str(len(products)),name=tag,supplier_id=supplier,unit='件',price=30,has_serial=0,batch_enabled=1,expiry_enabled=1));assert 'id' in r,r;products.append(r['id']);return r['id']
def order(kind,qty,price):
 partner=dict(supplier_id=supplier) if kind=='purchase' else dict(customer_id=customer)
 r=ok('/'+kind+'/orders',buyer if kind=='purchase' else seller,dict(**partner,warehouse_id=1,items=[dict(product_id=product,quantity=qty,price=price)]));orders.append((kind,r));ok('/'+kind+'/approve',admin,dict(order_id=r['order_id'],approve=True));return r
def batch(kind,step,r,qty,user=admin,**fields):
 return api('/orders/batch',user,dict(type=kind,step=step,order_id=r['order_id'],warehouse_id=1,items=[dict(product_id=product,quantity=qty,**fields)],request_id=uuid.uuid4().hex))
def snapshot():return sql(f'SELECT CONCAT(quantity,":",reserved_qty,":",cost_initialized,":",cost_value) FROM inventory WHERE product_id={product} AND warehouse_id=1')
try:
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'));customer=int(sql('SELECT id FROM customers WHERE status=1 LIMIT 1'));product=make_product()
 first=order('purchase',5,10)
 _,bad=batch('purchase','receive',first,5);assert bad.get('code')==40000,bad
 _,bad=batch('purchase','receive',first,5,lot_no='BAD',manufactured_on='2026-02-31',expires_on='2099-01-01');assert bad.get('code')==40000,bad
 assert batch('purchase','receive',first,5,lot_no='EXPIRED',manufactured_on='2000-01-01',expires_on='2001-01-01')[1]['code']==0
 second=order('purchase',5,20)
 assert batch('purchase','receive',second,5,lot_no='VALID',manufactured_on='2026-01-01',expires_on='2099-01-01')[1]['code']==0
 assert snapshot()=='10:0:1:150.000000',snapshot()
 sale=order('sales',6,30);_,bad=batch('sales','prepare',sale,6,warehouse);assert bad.get('code')==41001,bad
 assert snapshot()=='10:0:1:150.000000'
 sale2=order('sales',2,30);assert batch('sales','prepare',sale2,2,warehouse)[1]['code']==0
 sql(f"UPDATE inventory_lots SET expires_on='2026-10-30' WHERE product_id={product} AND lot_no='VALID'")
 _,bad=batch('sales','release',sale2,2);assert bad.get('code')==41001,bad
 assert snapshot()=='10:2:1:150.000000'
 sql(f"UPDATE inventory_lots SET expires_on='2099-01-01' WHERE product_id={product} AND lot_no='VALID'")
 assert batch('sales','release',sale2,2)[1]['code']==0
 assert snapshot()=='8:0:1:120.000000',snapshot()
 assert sql(f"SELECT quantity FROM inventory_lots WHERE product_id={product} AND lot_no='EXPIRED'")=='5'
 assert sql(f"SELECT cost_value FROM stock_records WHERE product_id={product} AND biz_type='sale_out'")=='30.000000'
 op=ok('/stock/operation',warehouse,dict(type='out',product_id=product,from_warehouse_id=1,qty=5,biz_type='scrap',remark=tag));operations.append(op['operation_id']);ok('/stock/operation/approve',admin,dict(operation_id=op['operation_id'],approve=True))
 assert snapshot()=='3:0:1:45.000000'
 transfer=ok('/stock/operation',warehouse,dict(type='transfer',product_id=product,from_warehouse_id=1,to_warehouse_id=2,qty=2,remark=tag));operations.append(transfer['operation_id']);ok('/stock/operation/approve',admin,dict(operation_id=transfer['operation_id'],approve=True))
 assert snapshot()=='1:0:1:15.000000'
 assert sql(f'SELECT CONCAT(quantity,":",cost_initialized,":",cost_value) FROM inventory WHERE product_id={product} AND warehouse_id=2')=='2:1:30.000000'
 returned=ok('/sales/return',seller,dict(order_id=sale2['order_id'],warehouse_id=1,items=[dict(product_id=product,quantity=1)]));returns.append(returned['return_id'])
 ok('/sales/return/approve',admin,dict(return_id=returned['return_id'],approve=True));ok('/sales/return/warehouse',admin,dict(return_id=returned['return_id'],approve=True))
 assert snapshot()=='2:0:1:30.000000'
 assert sql(f"SELECT quantity FROM inventory_lots WHERE product_id={product} AND warehouse_id=1 AND lot_no='VALID'")=='2'

 # Historical allocation does not invent inventory or opening cost.
 product=make_product();sql(f'INSERT INTO inventory(product_id,warehouse_id,quantity,min_quantity) VALUES({product},1,4,0)')
 inventory=int(sql(f'SELECT id FROM inventory WHERE product_id={product} AND warehouse_id=1'))
 ok('/lots/opening',admin,dict(product_id=product,warehouse_id=1,lot_no='OPEN',quantity=4,manufactured_on='2026-01-01',expires_on='2099-01-01',reason=tag))
 assert sql(f'SELECT cost_initialized FROM inventory WHERE id={inventory}')=='0'
 ok('/inventory/cost/opening',admin,dict(inventory_id=inventory,book_qty=4,unit_cost='0.008',reason=tag))
 assert snapshot()=='4:0:1:0.032000'
 _,bad=api('/inventory/cost/opening',admin,dict(inventory_id=inventory,book_qty=4,unit_cost='10',reason=tag));assert bad.get('code')!=0,bad
 _,bad=api('/lots/opening',admin,dict(product_id=product,warehouse_id=1,lot_no='OVER',quantity=1,manufactured_on='2026-01-01',expires_on='2099-01-01',reason=tag));assert bad.get('code')!=0,bad
 print('PASS batch/date requirements, expired-sale rejection and rollback, FEFO, weighted average cost snapshots, approved expiry scrap, historical lot allocation, 6-decimal opening cost and overwrite protection')
finally:
 for rid in returns:
  sql(f"DELETE FROM accounts WHERE biz_no=(SELECT return_no FROM sales_returns WHERE id={rid})")
  sql(f"DELETE FROM sales_returns WHERE id={rid}");sql(f"DELETE FROM business_events WHERE document_type='sales_returns' AND document_id={rid}")
 for operation in operations:
  sql(f'DELETE FROM stock_operation_logs WHERE operation_id={operation}');sql(f'DELETE FROM stock_operations WHERE id={operation}');sql(f"DELETE FROM business_events WHERE document_type='stock_operations' AND document_id={operation}")
 for kind,r in orders:
  sql(f"DELETE FROM accounts WHERE biz_no='{r['order_no']}'");sql(f"DELETE FROM order_deliveries WHERE order_type='{kind}' AND order_id={r['order_id']}");sql(f"DELETE FROM {kind}_orders WHERE id={r['order_id']}");sql(f"DELETE FROM business_events WHERE document_type='{kind}_orders' AND document_id={r['order_id']}")
 for product in products:
  sql(f"DELETE FROM business_events WHERE document_type='inventory_cost' AND document_id IN (SELECT id FROM inventory WHERE product_id={product})")
  sql(f"DELETE FROM business_events WHERE document_type='inventory_lots' AND document_id IN (SELECT id FROM inventory_lots WHERE product_id={product})")
  sql(f'DELETE FROM inventory_cost_openings WHERE inventory_id IN (SELECT id FROM inventory WHERE product_id={product})')
  sql(f'DELETE FROM lot_movements WHERE lot_id IN (SELECT id FROM inventory_lots WHERE product_id={product})');sql(f'DELETE FROM inventory_lots WHERE product_id={product}');sql(f'DELETE FROM stock_records WHERE product_id={product}');sql(f'DELETE FROM inventory WHERE product_id={product}');sql(f'DELETE FROM products WHERE id={product}')
