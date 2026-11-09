import importlib.util, os, secrets, subprocess, tempfile, time, uuid
from pathlib import Path
from types import SimpleNamespace
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('backup_tool',root/'deploy/data_backup.py');tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)
tag='erp-restore-test-'+uuid.uuid4().hex[:10];mysql=tag+'-mysql';backend=tag+'-backend';volume=tag+'-uploads'
password=secrets.token_hex(24);os.environ['ERP_DB_PASSWORD']=password
args=SimpleNamespace(mysql_container=mysql,database='erp_source',db_user='root',backend_container=backend,backend_image='erp-backend:2026-11-01-r3')
try:
 with tempfile.TemporaryDirectory() as temporary:
  env=Path(temporary)/'db.env';env.write_text('MYSQL_ROOT_PASSWORD='+password+'\nMYSQL_DATABASE=erp_source\n')
  tool.docker('run','-d','--name',mysql,'--env-file',str(env),'mysql:8.0')
  for attempt in range(90):
   try:tool.mysql(args,'SELECT 1;');break
   except RuntimeError:time.sleep(1)
  else:raise RuntimeError('Test database not ready')
  tool.mysql(args,"CREATE TABLE proof(id INT PRIMARY KEY,note VARCHAR(100)); INSERT INTO proof VALUES(1,'restore verified'); CREATE TABLE accounts(id INT,note VARCHAR(100)); INSERT INTO accounts VALUES(1,'restore verified'); CREATE VIEW posted_sales_activity AS SELECT note FROM accounts; CREATE TABLE operational_settings(setting_key VARCHAR(64) PRIMARY KEY,setting_value JSON); INSERT INTO operational_settings VALUES('backup',JSON_OBJECT('enabled',true)); CREATE TABLE backup_requests(id INT PRIMARY KEY,status VARCHAR(16),completed_at DATETIME NULL,details JSON NULL); INSERT INTO backup_requests(id,status) VALUES(1,'queued'),(2,'running');")
  tool.mysql(args,"CREATE TABLE departments(id INT PRIMARY KEY,name VARCHAR(100));INSERT INTO departments VALUES(1,'restored department');CREATE TABLE users(id INT PRIMARY KEY,department_id INT);INSERT INTO users VALUES(1,1);CREATE TABLE employees(id INT PRIMARY KEY,department_id INT);INSERT INTO employees VALUES(1,1);CREATE VIEW user_directory AS SELECT u.*,d.name AS department FROM users u LEFT JOIN departments d ON d.id=u.department_id;CREATE VIEW employee_directory AS SELECT e.*,d.name AS department FROM employees e LEFT JOIN departments d ON d.id=e.department_id;")
  tool.docker('create','--name',backend,'-v',volume+':/app/uploads','--entrypoint','python3',args.backend_image,'-c','import time; time.sleep(600)')
  tool.docker('start',backend)
  tool.docker('run','--rm','--volumes-from',backend,'--entrypoint','python3',args.backend_image,'-c',"from pathlib import Path; Path('/app/uploads/proof.txt').write_text('attachment verified')")
  args.output=temporary;args.recovery_file=str(Path(temporary)/'recovery.json');tool.backup(args);assert not Path(args.recovery_file).exists();assert tool.docker('inspect','--format','{{.State.Running}}',backend).decode().strip()=='true';args.archive=str(next(Path(temporary).glob('erp-*.tar.gz')))
  try:tool.restore(args);raise AssertionError('Non-empty database restore must fail')
  except RuntimeError as error:assert 'not empty' in str(error)
  tool.mysql(args,'CREATE DATABASE erp_target;');args.database='erp_target'
  target=tag+'-target';tool.docker('create','--name',target,'-v',tag+'-target-volume:/app/uploads','--entrypoint','python3',args.backend_image,'-c','pass');args.backend_container=target
  tool.restore(args)
  assert 'restore verified' in tool.mysql(args,'SELECT note FROM proof WHERE id=1;').decode()
  source_args=SimpleNamespace(**{**vars(args),'database':'erp_source'})
  tool.mysql(source_args,"UPDATE accounts SET note='source changed after restore';")
  assert 'restore verified' in tool.mysql(args,'SELECT note FROM posted_sales_activity;').decode()
  tool.mysql(source_args,"UPDATE departments SET name='source changed after restore';")
  for view in ['user_directory','employee_directory']:assert 'restored department' in tool.mysql(args,'SELECT department FROM '+view+';').decode()

  assert tool.mysql(args,"SELECT JSON_EXTRACT(setting_value,'$.enabled') FROM operational_settings WHERE setting_key='backup';").decode().strip()=='false'
  assert tool.mysql(args,"SELECT COUNT(*) FROM backup_requests WHERE status='failed';").decode().strip()=='2'
  output=tool.docker('run','--rm','--volumes-from',target,'--entrypoint','python3',args.backend_image,'-c',"from pathlib import Path; print(Path('/app/uploads/proof.txt').read_text())")
  assert b'attachment verified' in output
  print('PASS backup archive checksums, refusal to overwrite existing data, restore into isolated empty database, attachment content restored')
finally:
 for name in [tag+'-target',backend,mysql]:subprocess.run(['docker','rm','-f','-v',name],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
 for name in [volume,tag+'-target-volume']:subprocess.run(['docker','volume','rm',name],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
