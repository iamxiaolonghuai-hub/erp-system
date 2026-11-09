"""Department CRUD, account/profile consistency and normalized storage regression."""
from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
tag='QAD'+uuid.uuid4().hex[:12];departments=[];uid=eid=pid=op=supplier=warehouse=leader_id=application_id=None
def ok(path,data=None,method=None):
 status,result=api(path,admin,data,method);assert status==200 and result.get('code')==0,(path,status,result);return result.get('data')
def rejected(path,data=None,method=None):
 _,result=api(path,admin,data,method);assert result.get('code',0)!=0,(path,result)
try:
 sql(f"INSERT INTO users(username,password,status) VALUES('{tag}','unused',1)");uid=int(sql(f"SELECT id FROM users WHERE username='{tag}'"))
 for index in range(2):
  row=ok('/departments',dict(name=tag+str(index),code=tag+str(index),status=1));departments.append(int(row['id']))
 listed=ok('/departments');assert set(departments)<={row['id'] for row in listed} and all('status' in row for row in listed)
 rejected('/departments',dict(name=tag+'0',code=tag+'0',status=1))
 unprivileged=dict(id=uid,username=tag,department='',roles='')
 assert api('/departments',unprivileged,dict(name=tag+'X',code=tag+'X',status=1))[1]['code']==40300
 sales=int(sql("SELECT id FROM departments WHERE code='sales'"));rejected('/departments/'+str(sales),dict(name=tag+'rename'),'PUT')
 row=ok('/employees',dict(emp_no=tag,name=tag,user_id=uid,department_id=departments[0],phone='123',email='',status=1));eid=int(row['id'])
 assert sql(f'SELECT department_id FROM users WHERE id={uid}')==str(departments[0])
 assert sql(f'SELECT department_id FROM employees WHERE id={eid}')==str(departments[0])
 rejected('/employees',dict(emp_no=tag+'dup',name=tag,user_id=uid,department_id=departments[0],status=1))
 for _ in range(2):ok('/departments/assign',dict(user_id=uid,department_id=departments[1]))
 assert sql(f'SELECT department_id FROM employees WHERE id={eid}')==str(departments[1])
 ok('/departments/'+str(departments[1]),dict(name=tag+'renamed'),'PUT')
 assert sql(f'SELECT department FROM user_directory WHERE id={uid}')==tag+'renamed'
 assert sql(f'SELECT department FROM employee_directory WHERE id={eid}')==tag+'renamed'
 rejected('/departments/'+str(departments[1]),dict(status=0),'PUT');rejected('/departments/'+str(departments[1]),method='DELETE')
 ok('/users/'+str(uid),dict(real_name=tag+'profile',phone='456',email=''),'PUT')
 assert sql(f'SELECT name FROM employees WHERE id={eid}')==tag+'profile'
 ok('/employees/'+str(eid),dict(name=tag+'employee'),'PUT')
 assert sql(f'SELECT real_name FROM users WHERE id={uid}')==tag+'employee'
 ok('/departments/assign',dict(user_id=uid,department_id=0));assert sql(f'SELECT COUNT(*) FROM employees WHERE id={eid} AND department_id IS NULL')=='1'
 ok('/departments/'+str(departments[1]),dict(status=0),'PUT');rejected('/departments/assign',dict(user_id=uid,department_id=departments[1]))
 # Moving the author must not move an existing business application to a different approval department.
 sql(f"INSERT INTO users(username,password,status,department_id) VALUES('{tag}L','unused',1,{sales})");leader_id=int(sql(f"SELECT id FROM users WHERE username='{tag}L'"))
 sql(f"INSERT INTO user_roles(user_id,role_id) SELECT {uid},id FROM roles WHERE code='staff';INSERT INTO user_roles(user_id,role_id) SELECT {leader_id},id FROM roles WHERE code='leader'")
 sql(f"INSERT INTO user_roles(user_id,role_id) SELECT {uid},id FROM roles WHERE code='leader'")
 assert sum(row['id']==uid for row in ok('/departments/users'))==1
 ok('/departments/assign',dict(user_id=uid,department_id=sales))
 author=dict(id=uid,username=tag,department='销售部',roles='staff');reviewer=dict(id=leader_id,username=tag+'L',department='销售部',roles='leader')
 _,submitted=api('/application/create',author,dict(type='customer',title=tag,payload={},assigned_approver_id=leader_id));assert submitted['code']==0,submitted
 application_id=int(sql(f"SELECT id FROM applications WHERE title='{tag}'"))
 ok('/departments/assign',dict(user_id=uid,department_id=departments[0]))
 _,reviewed=api('/application/approve',reviewer,dict(application_id=application_id,approve=False));assert reviewed['code']==0,reviewed
 for table,column in [('users','department'),('employees','department'),('stock_operations','serial_nos'),('order_deliveries','items'),('purchase_returns','supplier_id'),('sales_returns','customer_id')]:
  assert sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='{column}'")=='0',(table,column)
 sql(f"INSERT INTO suppliers(name,status) VALUES('{tag}',1)");supplier=int(sql(f"SELECT id FROM suppliers WHERE name='{tag}'"))
 status,row=api('/products',admin,dict(sku=tag,name=tag,unit='unit',price=1,supplier_id=supplier,has_serial=1));assert status==200;pid=int(row['id'])
 sql(f"INSERT INTO warehouses(name,code,status) VALUES('{tag}','{tag}',1)");warehouse=int(sql(f"SELECT id FROM warehouses WHERE code='{tag}'"))
 row=ok('/stock/operation',dict(type='in',product_id=pid,to_warehouse_id=warehouse,qty=2,serial_nos='SERIAL_A,SERIAL_B'));op=int(row['operation_id'])
 assert sql(f'SELECT COUNT(*) FROM stock_operation_serials WHERE operation_id={op}')=='2'
 status,result=api('/stock/operations',admin);assert status==200
 displayed=next(item for item in result if item['id']==op);assert displayed['serial_nos']=='SERIAL_A,SERIAL_B'
 print('PASS department management, ID selection, atomic account/profile transfer, rename projection, reference protection, normalized columns, serial rows and compatible display')
finally:
 if application_id:sql(f"DELETE FROM applications WHERE id={application_id};DELETE FROM business_events WHERE document_type='applications' AND document_id={application_id}")
 if op:sql(f"DELETE FROM stock_operation_logs WHERE operation_id={op};DELETE FROM stock_operations WHERE id={op};DELETE FROM business_events WHERE document_type='stock_operations' AND document_id={op}")
 if pid:sql(f'DELETE FROM products WHERE id={pid}')
 if supplier:sql(f'DELETE FROM suppliers WHERE id={supplier}')
 if warehouse:sql(f'DELETE FROM warehouses WHERE id={warehouse}')
 if eid:sql(f"DELETE FROM employees WHERE id={eid};DELETE FROM business_events WHERE document_type='employees' AND document_id={eid}")
 if uid:sql(f"DELETE FROM users WHERE id={uid};DELETE FROM business_events WHERE document_type='users' AND document_id={uid}")
 if leader_id:sql(f'DELETE FROM users WHERE id={leader_id}')
 for did in departments:sql(f"DELETE FROM departments WHERE id={did};DELETE FROM business_events WHERE document_type='departments' AND document_id={did}")
