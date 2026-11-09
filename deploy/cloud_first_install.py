"""One-time empty installation on the authorized cloud host; refuses existing ERP data."""
import hashlib,json,os,secrets,subprocess
from pathlib import Path

base=Path('/opt/erp')
assert os.geteuid()==0 and base.is_dir(),'Run as root from the prepared ERP deployment'
assert not (base/'.env').exists(),'Existing ERP configuration found; use an upgrade procedure'
def docker(*args,input=None):
 result=subprocess.run(['docker',*args],input=input,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=300)
 if result.returncode:raise RuntimeError('Docker operation failed; inspect server logs')
 return result.stdout
metadata=json.loads(docker('inspect','db-mysql'))[0]
environment=dict(value.split('=',1) for value in metadata['Config']['Env'] if '=' in value)
root_password=environment.get('MYSQL_ROOT_PASSWORD')
assert root_password,'MySQL root credential unavailable'
def sql(statement,database=None):
 return docker('exec','-i','-e','MYSQL_PWD='+root_password,'db-mysql','mysql','--default-character-set=utf8mb4','-N','-B','-uroot',*([database] if database else []),input=statement.encode()).decode().strip()
assert sql("SELECT COUNT(*) FROM information_schema.SCHEMATA WHERE SCHEMA_NAME='erp_system'")=='0','ERP database already exists'
assert sql("SELECT COUNT(*) FROM mysql.user WHERE User='erp_app'")=='0','ERP database user already exists'
password=secrets.token_hex(32)
sql("CREATE DATABASE erp_system CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;CREATE USER 'erp_app'@'%' IDENTIFIED BY '"+password+"';GRANT ALL PRIVILEGES ON erp_system.* TO 'erp_app'@'%';")
sql((base/'db/schema.sql').read_text(),'erp_system')
# Only this newly created database is cleaned; the schema contains demonstration master data.
sql("SET FOREIGN_KEY_CHECKS=0;DELETE FROM products;DELETE FROM categories;DELETE FROM warehouses;DELETE FROM employees WHERE user_id IN (SELECT id FROM users WHERE username IN ('leader','staff'));DELETE FROM user_roles WHERE user_id IN (SELECT id FROM users WHERE username IN ('leader','staff'));DELETE FROM users WHERE username IN ('leader','staff');SET FOREIGN_KEY_CHECKS=1;",'erp_system')
accounts=[]
for username in sql('SELECT username FROM users ORDER BY id','erp_system').splitlines():
 initial=secrets.token_urlsafe(18);salt=secrets.token_hex(16);hashed=salt+'$'+hashlib.sha256((salt+initial).encode()).hexdigest()
 sql("UPDATE users SET password='"+hashed+"' WHERE username='"+username+"'",'erp_system')
 accounts.append((username,initial))
sql("UPDATE users SET real_name=CASE username WHEN 'admin' THEN '系统管理员' WHEN 'sale_leader' THEN '销售主管' WHEN 'sale_staff' THEN '销售员工' WHEN 'warehouse_leader' THEN '仓库主管' WHEN 'warehouse_staff' THEN '仓库员工' WHEN 'purchase_leader' THEN '采购主管' WHEN 'purchase_staff' THEN '采购员工' WHEN 'finance_leader' THEN '财务主管' WHEN 'finance_staff' THEN '财务员工' ELSE real_name END;UPDATE employees e JOIN users u ON u.id=e.user_id SET e.name=u.real_name;",'erp_system')
def protected(path,text):
 with path.open('x') as stream:os.chmod(path,0o600);stream.write(text)
protected(base/'.env','\n'.join(['DB_HOST=db-mysql','DB_PORT=3306','DB_NAME=erp_system','DB_USER=erp_app','DB_PASSWORD='+password,'JWT_SECRET='+secrets.token_hex(48),'MYSQL_NETWORK=erp-shared','ERP_HTTP_PORT=8080'])+'\n')
protected(base/'initial-accounts.txt','ERP HTTP port: 8080\nInitial accounts: username / password\n'+''.join(name+' / '+value+'\n' for name,value in accounts))
(base/'compose.override.yaml').write_text('services:\n  backend:\n    mem_limit: 768m\n    cpus: 1.25\n    pids_limit: 256\n    logging:\n      driver: json-file\n      options: {max-size: "10m", max-file: "3"}\n  frontend:\n    mem_limit: 128m\n    cpus: 0.50\n    pids_limit: 128\n    logging:\n      driver: json-file\n      options: {max-size: "10m", max-file: "3"}\n')
networks=docker('network','ls','--format','{{.Name}}').decode().splitlines()
if 'erp-shared' not in networks:docker('network','create','erp-shared')
if 'erp-shared' not in metadata['NetworkSettings']['Networks']:docker('network','connect','erp-shared','db-mysql')
print('Created empty ERP database, dedicated database account, nine randomized role accounts and isolated deployment configuration; existing blog unchanged')
