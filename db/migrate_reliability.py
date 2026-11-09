import json,os,subprocess
from pathlib import Path
cfg=json.loads((Path(__file__).resolve().parents[1]/'config.json').read_text())['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=cfg['passwd'])
cmd=['mysql','-h'+cfg['host'],'-P'+str(cfg['port']),'-u'+cfg['user'],cfg['dbname'],'-N','-B']
sql='''CREATE TABLE IF NOT EXISTS business_events (
 id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
 document_type VARCHAR(40) NOT NULL,document_id BIGINT UNSIGNED NOT NULL,
 action VARCHAR(40) NOT NULL,operator_id BIGINT UNSIGNED NOT NULL,
 before_status INT NOT NULL,after_status INT NOT NULL,remark VARCHAR(1000) NOT NULL DEFAULT '',
 created_at DATETIME(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6),
 KEY idx_document (document_type,document_id,id),KEY idx_operator (operator_id,created_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;'''
subprocess.run(cmd+['-e',sql],env=env,check=True)
print('business_events ready')
q="SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='stock_checks' AND column_name='reviewed_by'"
if subprocess.check_output(cmd+['-e',q],env=env,text=True).strip()=='0':
 subprocess.run(cmd+['-e','ALTER TABLE stock_checks ADD COLUMN reviewed_by BIGINT UNSIGNED NULL'],env=env,check=True)
print('stock_checks reviewer ready')
q="SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='stock_checks' AND column_name='counted_by'"
if subprocess.check_output(cmd+['-e',q],env=env,text=True).strip()=='0':
 subprocess.run(cmd+['-e','ALTER TABLE stock_checks ADD COLUMN counted_by BIGINT UNSIGNED NULL'],env=env,check=True)
print('stock_checks counter ready')
subprocess.run(cmd+['-e',"""CREATE TABLE IF NOT EXISTS business_requests (
 user_id BIGINT UNSIGNED NOT NULL, request_key VARCHAR(128) NOT NULL,
 fingerprint CHAR(64) NOT NULL, response MEDIUMTEXT NULL,
 created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
 PRIMARY KEY(user_id,request_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"""],env=env,check=True)
print('business_requests ready')
