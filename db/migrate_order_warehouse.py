import json,os,subprocess
from pathlib import Path
cfg=json.loads((Path(__file__).resolve().parents[1]/'config.json').read_text())['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=cfg['passwd'])
cmd=['mysql','-h'+cfg['host'],'-P'+str(cfg['port']),'-u'+cfg['user'],cfg['dbname'],'-N','-B']
for table in ['purchase_orders','sales_orders']:
 q=f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='warehouse_id'"
 if subprocess.check_output(cmd+['-e',q],env=env,text=True).strip()=='0':
  subprocess.run(cmd+['-e',f'ALTER TABLE {table} ADD COLUMN warehouse_id BIGINT UNSIGNED NULL, ADD INDEX idx_warehouse (warehouse_id)'],env=env,check=True)
 print(table+': warehouse ready')
