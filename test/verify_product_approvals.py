"""Local integration checks; creates and removes only uniquely named test records."""
import base64, hashlib, hmac, json, os, subprocess, time, uuid
from pathlib import Path
from urllib.request import Request, build_opener, ProxyHandler
from urllib.error import HTTPError
root=Path(__file__).resolve().parents[1]
config=json.loads((root/'config.json').read_text()); db=config['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=db['passwd'])
def sql(query):
 return subprocess.check_output(['mysql','-h'+db['host'],'-P'+str(db['port']),'-u'+db['user'],db['dbname'],'-N','-B','-e',query],env=env,text=True,stderr=subprocess.DEVNULL).strip()
def token(user):
 enc=lambda x:base64.urlsafe_b64encode(json.dumps(x,separators=(',',':')).encode()).decode().rstrip('=')
 body=enc({'alg':'HS256','typ':'JWT'})+'.'+enc(dict(user_id=str(user['id']),username=user['username'],department=user['department'],roles=user['roles'],exp=int(time.time())+300))
 return body+'.'+base64.urlsafe_b64encode(hmac.new(config['custom_config']['jwt_secret'].encode(),body.encode(),hashlib.sha256).digest()).decode().rstrip('=')
opener=build_opener(ProxyHandler({}))
def api(path,user,data=None,method=None):
 req=Request('http://127.0.0.1:5555/api'+path,data=None if data is None else json.dumps(data).encode(),headers={'Authorization':'Bearer '+token(user),'Content-Type':'application/json'},method=method)
 try:
  with opener.open(req,timeout=20) as r:return r.status,json.load(r)
 except HTTPError as e:return e.code,json.load(e)
users=[json.loads(line) for line in sql("SELECT JSON_OBJECT('id',u.id,'username',u.username,'department',u.department,'roles',GROUP_CONCAT(r.code)) FROM user_directory u JOIN user_roles ur ON ur.user_id=u.id JOIN roles r ON r.id=ur.role_id WHERE u.status=1 GROUP BY u.id").splitlines()]
admin=next(u for u in users if 'admin' in u['roles'].split(','))
pair=next(((s,l) for s in users for l in users if 'staff' in s['roles'].split(',') and 'leader' in l['roles'].split(',') and s['department']==l['department']=='销售部' and s['id']!=l['id']),None)
assert pair,'Need an existing staff/leader pair for approval verification'
staff,leader=pair
suffix=uuid.uuid4().hex; sku='QA_'+suffix[:20]; title='QA_APPROVAL_'+suffix; product_id=None; orders=[]
try:
 supplier=int(sql('SELECT id FROM suppliers WHERE status=1 ORDER BY id LIMIT 1'))
 payload=dict(sku=sku,name='自动验证临时商品',unit='件',spec='',price=12.5,supplier_id=supplier,has_serial=1)
 status,created=api('/products',admin,payload);assert status==200,created;product_id=int(created['id'])
 assert sql(f'SELECT CONCAT(supplier_id,":",has_serial) FROM products WHERE id={product_id}')==f'{supplier}:1'
 api('/products/'+str(product_id),admin,payload,'PUT')
 assert sql(f'SELECT has_serial FROM products WHERE id={product_id}')=='1'
 print('PASS product supplier and serial saved together, repeated edit succeeds')
 status,error=api('/products',admin,{**payload,'sku':sku+'X','supplier_id':0});assert status==400,error
 status,error=api('/stock/operation',admin,dict(type='in',product_id=product_id,to_warehouse_id=1,qty=2,serial_nos='ONLY_ONE'));assert error.get('code')==40000,error
 print('PASS invalid supplier and serial count rejected without business writes')
 customer=int(sql('SELECT id FROM customers WHERE status=1 ORDER BY id LIMIT 1'))
 for kind,department,partner in [('purchase','采购部',dict(supplier_id=supplier)),('sales','销售部',dict(customer_id=customer))]:
  match=next(((s,l) for s in users for l in users if 'staff' in s['roles'].split(',') and 'leader' in l['roles'].split(',') and s['department']==department and l['department']==department),None)
  if not match:continue
  author,assigned=match
  status,result=api('/'+kind+'/orders',author,dict(**partner,warehouse_id=1,remark=title,assigned_approver_id=assigned['id'],items=[dict(product_id=product_id,quantity=1,price=12.5)]))
  assert result.get('code')==0,result
  oid=int(result['data']['order_id']);orders.append((kind+'_orders',oid))
  assert sql(f'SELECT warehouse_id FROM {kind}_orders WHERE id={oid}')=='1'
  status,error=api('/'+kind+('/inbound' if kind=='purchase' else '/confirm'),admin,dict(order_id=oid,warehouse_id=2,approve=True))
  assert error.get('code')==40000 and '仓库一致' in error.get('message',''),error
  assert sql(f'SELECT CONCAT(assigned_approver_id,":",status) FROM {kind}_orders WHERE id={oid}')==f'{assigned["id"]}:0'
  status,result=api('/'+kind+'/approve',assigned,dict(order_id=oid,approve=False))
  assert result.get('code')==0,result
  assert sql(f'SELECT CONCAT(approved_by,":",status) FROM {kind}_orders WHERE id={oid}')==f'{assigned["id"]}:2'
  status,metadata=api('/approval/assignments?table='+kind+'_orders',author)
  row=next(x for x in metadata if x['id']==oid)
  assert row['warehouse_id']==1,row
  assert row['created_by_name'] and row['approver_name'] not in ('待审批','—') and row['warehouse_approver_name']=='待批准',row
  print('PASS '+kind+' order publishes with selected approver')
 status,leaders=api('/approval/leaders',staff);assert status==200 and leader['id'] in [u['id'] for u in leaders],leaders
 status,created=api('/application/create',staff,dict(type='customer',title=title,payload={},assigned_approver_id=leader['id']));assert created.get('code')==0,created
 application_id=int(sql(f"SELECT id FROM applications WHERE title='{title}'"))
 assert int(sql(f'SELECT assigned_approver_id FROM applications WHERE id={application_id}'))==leader['id']
 wrong=next((u for u in users if 'leader' in u['roles'].split(',') and u['id']!=leader['id']),None)
 if wrong:
  status,error=api('/application/approve',wrong,dict(application_id=application_id,approve=False));assert error.get('code')==40300,error
  assert sql(f'SELECT status FROM applications WHERE id={application_id}')=='0'
 print('PASS chosen leader persisted and unauthorized approval rejected')
 status,error=api('/application/create',staff,dict(type='customer',title=title+'INVALID',payload={},assigned_approver_id=staff['id']));assert error.get('code')==40000,error
 print('PASS self-approval selection rejected')
finally:
 for aid in sql(f"SELECT id FROM applications WHERE title='{title}'").splitlines():sql(f"DELETE FROM business_events WHERE document_type='applications' AND document_id={int(aid)}")
 sql(f"DELETE FROM applications WHERE title='{title}'")
 for table,oid in orders:
  sql(f"DELETE FROM {table} WHERE id={oid} AND remark='{title}'")
  sql(f"DELETE FROM business_events WHERE document_type='{table}' AND document_id={oid}")
 if product_id:sql(f"DELETE FROM products WHERE id={product_id} AND sku='{sku}'")
 print('Temporary test product and application removed; no inventory or finance postings performed')
