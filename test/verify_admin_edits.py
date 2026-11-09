"""Regression for id-less edits, unchanged saves and employee offboarding."""
from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
suffix=uuid.uuid4().hex[:16];wid=pid=eid=uid=None
try:
 body=dict(code='QA_'+suffix,name='QA_'+suffix,address='',manager='',status=1)
 status,row=api('/warehouses',admin,body);assert status==200,row;wid=int(row['id'])
 for _ in range(2):
  status,result=api('/warehouses/'+str(wid),admin,body,'PUT');assert status in (200,202),result
 status,result=api('/warehouses',admin,body);assert status==409,result
 status,result=api('/warehouses/999999999',admin,body,'PUT');assert status==404,result
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 LIMIT 1'))
 product=dict(sku='QA_'+suffix,name='QA_'+suffix,unit='件',price=12.5,supplier_id=supplier,has_serial=0,batch_enabled=0,expiry_enabled=0)
 status,row=api('/products',admin,product);assert status==200,row;pid=int(row['id'])
 for _ in range(2):assert api('/products/'+str(pid),admin,product,'PUT')[0]==200
 for _ in range(2):assert api('/products/'+str(pid)+'/serial',admin,dict(has_serial=0),'PUT')[0]==200
 sql(f"INSERT INTO users(username,password,status) VALUES('QA_{suffix}','unused',1)")
 uid=int(sql(f"SELECT id FROM users WHERE username='QA_{suffix}'"))
 for _ in range(2):assert api('/departments/assign',admin,dict(user_id=uid,department='销售部'))[1]['code']==0
 for warehouse_id in [wid,wid,0,0]:
  status,result=api('/departments/assign-warehouse',admin,dict(user_id=uid,warehouse_id=warehouse_id));assert result['code']==0,result
 sql(f"INSERT INTO employees(emp_no,name,user_id,status) VALUES('QA_{suffix}','QA',{uid},1)")
 eid=int(sql(f'SELECT id FROM employees WHERE user_id={uid}'))
 for _ in range(2):
  status,result=api('/employees/'+str(eid),admin,dict(status=0),'PUT');assert result['code']==0,result
 assert sql(f'SELECT status FROM users WHERE id={uid}')=='0'
 assert sql(f'SELECT status FROM employees WHERE id={eid}')=='0'
 for path in ['/employees','/warehouses','/products','/products/serial-info','/departments/users','/suppliers','/customers','/categories','/operations/backup','/inventory/summary','/report/dashboard']:
  start=time.monotonic();assert api(path,admin)[0]==200;assert time.monotonic()-start<5
 print('PASS id-less warehouse edits, repeated unchanged saves, duplicate and missing warehouses, product repeated saves, atomic employee offboarding and responsive lists')
finally:
 if eid:sql(f"DELETE FROM business_events WHERE document_type='employees' AND document_id={eid};DELETE FROM employees WHERE id={eid}")
 if uid:sql(f'DELETE FROM users WHERE id={uid}')
 if pid:sql(f'DELETE FROM products WHERE id={pid}')
 if wid:sql(f'DELETE FROM warehouses WHERE id={wid}')
