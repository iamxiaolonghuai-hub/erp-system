from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
tag='QA_LIFE_'+uuid.uuid4().hex
orders=[]
try:
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'))
 _,record=api('/products',admin,dict(sku=tag,name=tag,supplier_id=supplier,unit='件',price=1,has_serial=0));product=int(record['id'])
 _,r=api('/purchase/orders',admin,dict(supplier_id=supplier,warehouse_id=1,remark=tag,items=[dict(product_id=product,quantity=1,price=1)]));assert r.get('code')==0,r
 oid=r['data']['order_id'];orders.append(oid)
 payload=dict(order_id=oid,type='purchase',action='withdraw',reason=tag)
 _,r=api('/orders/lifecycle',admin,payload);assert r.get('code')==0,r
 assert sql(f'SELECT status FROM purchase_orders WHERE id={oid}')=='6'
 _,r=api('/purchase/resubmit',admin,dict(order_id=oid,items=[dict(product_id=product,quantity=2,price=1)]));assert r.get('code')==0,r
 _,r=api('/orders/lifecycle',admin,dict(payload,action='void'));assert r.get('code')==0,r
 _,r=api('/purchase/approve',admin,dict(order_id=oid,approve=True));assert r.get('code')!=0,r
 assert sql(f'SELECT status FROM purchase_orders WHERE id={oid}')=='7'
 print('PASS withdraw, resubmit, void and blocked approval after void')
finally:
 for oid in orders:
  sql(f"DELETE FROM business_events WHERE document_type='purchase_orders' AND document_id={oid}")
  sql(f'DELETE FROM purchase_orders WHERE id={oid}')
 if 'product' in globals():sql(f"DELETE FROM products WHERE id={product} AND sku='{tag}'")
