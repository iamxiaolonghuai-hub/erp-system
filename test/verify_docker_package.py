"""Smoke-test release images against an isolated disposable MySQL container."""
import base64, hashlib, hmac, json, os, secrets, subprocess, tempfile, time, uuid
from pathlib import Path
from urllib.request import Request, build_opener, ProxyHandler
from urllib.error import HTTPError
root=Path(__file__).resolve().parents[1]
suffix=uuid.uuid4().hex[:10]; network='erp-package-test-'+suffix
mysql=network+'-mysql'; backend=network+'-backend'; frontend=network+'-frontend'
password=secrets.token_hex(24); secret=secrets.token_hex(32)
def docker(*args,**kwargs):return subprocess.check_output(['docker',*args],stderr=subprocess.STDOUT,**kwargs)
opener=build_opener(ProxyHandler({}))
def http(path,data=None,authorization=''):
 request=Request(base+path,data=None if data is None else json.dumps(data).encode(),headers={'Authorization':authorization,'Content-Type':'application/json'})
 with opener.open(request,timeout=30) as response:return response.status,response.read()
try:
 docker('network','create',network)
 with tempfile.TemporaryDirectory() as temp:
  env=Path(temp)/'mysql.env';env.write_text('MYSQL_ROOT_PASSWORD='+password+'\nMYSQL_DATABASE=erp_system\n')
  docker('run','-d','--name',mysql,'--network',network,'--env-file',str(env),'mysql:8.0')
  for attempt in range(90):
   try:docker('exec','-e','MYSQL_PWD='+password,mysql,'mysql','-uroot','-e','SELECT 1');break
   except subprocess.CalledProcessError:time.sleep(1)
  else:raise RuntimeError('Test MySQL did not become ready')
  docker('exec','-i','-e','MYSQL_PWD='+password,mysql,'mysql','-uroot','erp_system',input=(root/'db/schema.sql').read_bytes())
  env.write_text('\n'.join(['DB_HOST='+mysql,'DB_PORT=3306','DB_NAME=erp_system','DB_USER=root','DB_PASSWORD='+password,'JWT_SECRET='+secret])+'\n')
  docker('run','-d','--name',backend,'--network',network,'--network-alias','backend','--env-file',str(env),'erp-backend:2026-11-01-r3')
  for name in ['migrate_approvers.py','migrate_warehouse_approver.py','migrate_order_warehouse.py','migrate_reliability.py','migrate_master_import.py','migrate_operations.py','migrate_batches.py','migrate_lots.py','migrate_costing.py','migrate_backup_settings.py','migrate_departments.py','migrate_normalized_details.py','migrate_finance_periods.py','migrate_serial_inventory.py']:
   docker('exec',backend,'python3','db/'+name)
  docker('run','-d','--name',frontend,'--network',network,'-p','127.0.0.1::80','erp-frontend:2026-11-01-r3')
  address=docker('port',frontend,'80/tcp').decode().strip();base='http://'+address
  encode=lambda value:base64.urlsafe_b64encode(json.dumps(value,separators=(',',':')).encode()).decode().rstrip('=')
  body=encode({'alg':'HS256','typ':'JWT'})+'.'+encode({'user_id':'1','username':'admin','department':'','roles':'admin','exp':int(time.time())+300})
  auth='Bearer '+body+'.'+base64.urlsafe_b64encode(hmac.new(secret.encode(),body.encode(),hashlib.sha256).digest()).decode().rstrip('=')
  for attempt in range(30):
   try:status,html=http('/');break
   except Exception:time.sleep(1)
  assert status==200 and b'<div id="app">' in html
  for table in ['suppliers','customers']:
   payload={'csv':'名称,联系人,电话\n容器验证,测试,001234\n 容器验证 ,重复,999\n'}
   _,raw=http('/api/import/'+table,payload,auth);result=json.loads(raw)
   assert result.get('code')==0 and result['data']['success']==1 and result['data']['skipped']==1,result
   _,raw=http('/api/export/'+table,authorization=auth);assert '容器验证' in raw.decode('utf-8-sig')
  _,raw=http('/api/inventory/summary',authorization=auth);assert json.loads(raw).get('code')==0,raw
  _,raw=http('/api/operations/backup',authorization=auth);state=json.loads(raw);assert state['code']==0 and not state['data']['agent_online'],state
  settings=dict(enabled=True,host='backup.invalid',port=22,username='ubuntu',remote_dir='/srv/erp-backups',local_dir='/opt/erp/backups',ssh_key_path='/opt/erp/keys/backup',time='03:00',keep_count=14)
  _,raw=http('/api/operations/backup',settings,auth);assert json.loads(raw)['code']==0,raw
  request={'request_id':uuid.uuid4().hex}
  _,raw=http('/api/operations/backup/request',request,auth);queued=json.loads(raw);assert queued['code']==0,queued
  _,raw=http('/api/operations/backup/request',request,auth);assert json.loads(raw)['data']['job_id']==queued['data']['job_id'],raw
  _,raw=http('/api/operations/backup',authorization=auth);state=json.loads(raw);assert state['data']['jobs'][0]['status']=='queued',state
  _,raw=http('/api/operations/backup',{**settings,'enabled':False},auth);assert json.loads(raw)['code']==0,raw
  _,raw=http('/api/operations/backup',authorization=auth);state=json.loads(raw);assert state['data']['jobs'][0]['status']=='failed' and not state['data']['settings']['enabled'],state
  print('PASS isolated backup settings save, queue request replay, string status display, policy-disable cancellation; no agent or remote transmission started')

  print('PASS Docker frontend, API proxy, backend runtime, MySQL connection, all migrations, supplier/customer import/export and deduplication')
except Exception:
 for container in [backend,frontend]:
  try:print(docker('logs','--tail','25',container).decode(errors='replace'))
  except Exception:pass
 raise
finally:
 for container in [frontend,backend,mysql]:
  subprocess.run(['docker','rm','-f','-v',container],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 subprocess.run(['docker','network','rm',network],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 print('Isolated Docker test containers, volumes and network removed')
