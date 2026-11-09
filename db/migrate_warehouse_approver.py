import json, os, subprocess
from pathlib import Path
config=json.loads((Path(__file__).resolve().parents[1]/'config.json').read_text())['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=config['passwd'])
cmd=['mysql','-h'+config['host'],'-P'+str(config['port']),'-u'+config['user'],config['dbname'],'-N','-B']
query="SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='sales_orders' AND column_name='warehouse_approved_by'"
if subprocess.check_output(cmd+['-e',query],env=env,text=True).strip()=='0':
 subprocess.run(cmd+['-e','ALTER TABLE sales_orders ADD COLUMN warehouse_approved_by BIGINT UNSIGNED NULL'],env=env,check=True)
print('sales_orders warehouse approver column ready')
