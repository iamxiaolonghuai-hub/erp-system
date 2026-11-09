import json,subprocess,os
from pathlib import Path
root=Path(__file__).resolve().parents[1]
cfg=json.loads((root/'config.json').read_text())['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=cfg['passwd'])
args=['mysql','-h'+cfg['host'],'-P'+str(cfg['port']),'-u'+cfg['user'],cfg['dbname'],'-N','-B']
tables=['purchase_orders','sales_orders','applications','stock_operations','purchase_returns','sales_returns']
for table in tables:
 query=f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='assigned_approver_id'"
 found=subprocess.check_output(args+['-e',query],env=env,text=True).strip()
 if found=='0':subprocess.run(args+['-e',f'ALTER TABLE {table} ADD COLUMN assigned_approver_id BIGINT UNSIGNED NULL, ADD INDEX idx_assigned_approver (assigned_approver_id,status)'],env=env,check=True)
 print(table+': ready')
