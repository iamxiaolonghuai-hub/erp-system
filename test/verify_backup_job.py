"""Host agent unit checks. SSH/subprocess and DB writes are mocked; no remote data is sent."""
import hashlib, importlib.util, json, os, sys, tempfile
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'deploy'))
import backup_job as job
import data_backup
settings=dict(enabled=True,host='backup.example.com',port=22,username='ubuntu',remote_dir='/srv/erp-backups',ssh_key_path='/opt/erp/keys/backup',local_dir='/opt/erp/backups',time='03:00',keep_count=1)
assert job.validate(settings)==settings
for key,value in [('host','-oProxyCommand=bad'),('host','host;command'),('remote_dir','/tmp/../etc'),('username','root@host'),('port',0),('time','25:00')]:
 try:job.validate({**settings,key:value});raise AssertionError('unsafe setting accepted')
 except ValueError:pass
with tempfile.TemporaryDirectory() as directory:
 folder=Path(directory);archive=folder/'erp-20261031-030000-123456.tar.gz';archive.write_bytes(b'backup test data')
 key=folder/'key';key.write_text('test placeholder, not a real private key');key.chmod(0o600)
 configured={**settings,'ssh_key_path':str(key),'local_dir':str(folder)};digest=hashlib.sha256(archive.read_bytes()).hexdigest();commands=[]
 def command(args,timeout=1800):
  commands.append(args);return digest+' file' if 'sha256sum' in args[-1] else ''
 with patch.object(job,'run_command',side_effect=command):assert job.upload(archive,configured)==digest
 assert len(commands)==3 and all('StrictHostKeyChecking=yes' in args for args in commands)
 with patch.object(job,'run_command',return_value='bad checksum'):
  try:job.upload(archive,configured);raise AssertionError('incorrect remote checksum accepted')
  except RuntimeError:pass
 assert archive.exists()
 args=SimpleNamespace(mysql_container='test',database='test',db_user='test')
 with patch.dict(os.environ,{'ERP_BACKEND_CONTAINER':'test-backend','ERP_DB_PASSWORD':'not-a-real-password'}):
  with patch.object(job,'query',return_value=json.dumps(configured)+'\n1'),patch.object(data_backup,'backup',return_value=archive),patch.object(job,'upload',side_effect=RuntimeError('simulated transfer failure')),patch.object(job,'write_details') as write,patch.object(job,'retain') as retain:
   try:job.process(args,configured,{'id':1});raise AssertionError('transfer failure not propagated')
   except RuntimeError:pass
   assert write.call_args.args[2]=='failed' and not retain.called
  with patch.object(job,'query',return_value=json.dumps({**configured,'enabled':False})+'\n1'),patch.object(data_backup,'backup') as backup,patch.object(job,'write_details') as write:
   try:job.process(args,configured,{'id':2});raise AssertionError('disabled settings were executed')
   except RuntimeError:pass
   assert not backup.called and write.call_args.args[2]=='failed'
  with patch.object(job.subprocess,'run',return_value=SimpleNamespace(returncode=0,stdout=b'ok')) as run:
   assert job.run_command(['ssh','example'])=='ok'
   assert 'ERP_DB_PASSWORD' not in run.call_args.kwargs['env']
  calls=[]
  def docker(*args,**kwargs):calls.append(args);return b'true' if args[0]=='inspect' else b''
  marker=folder/'recovery.json'
  request=SimpleNamespace(backend_container='test-backend',backend_image='test-image',recovery_file=str(marker),output=str(folder))
  with patch.object(data_backup,'docker',side_effect=docker),patch.object(data_backup,'mysql',side_effect=RuntimeError('dump failed')):
   try:data_backup.backup(request);raise AssertionError('dump failure not propagated')
   except RuntimeError:pass
   assert ('stop','test-backend') in calls and ('start','test-backend') in calls and not marker.exists()
  marker.write_text(json.dumps({'was_running':True,'backend_container':'test-backend'}))
  with patch.object(data_backup,'docker',return_value=b'ok') as restart:data_backup.recover_backend(marker);restart.assert_called_with('start','test-backend')
  assert not marker.exists()
  statement=data_backup.view_sql({'name':'posted_sales_activity','sql':"SELECT * FROM `source_db`.`accounts` WHERE 'literal `source_db`.`accounts`'='x'"},'source_db','target_db')
  assert '`target_db`.`accounts`' in statement and "'literal `source_db`.`accounts`'" in statement
  for view,table in [('user_directory','users'),('employee_directory','employees')]:
   statement=data_backup.view_sql({'name':view,'sql':f'SELECT p.id FROM `source_db`.`{table}` p LEFT JOIN `source_db`.`departments` d ON d.id=p.department_id'},'source_db','target_db')
   assert f'`target_db`.`{table}`' in statement and '`target_db`.`departments`' in statement and '`source_db`.' not in statement
  try:data_backup.view_sql({'name':'custom_view','sql':'SELECT 1'},'source_db','target_db');raise AssertionError('unknown cross-database view accepted')
  except RuntimeError:pass
print('PASS backup validation, strict SSH verification, remote checksum, failed upload retention, disabled configuration race, secret environment filtering and restart after dump failure')
