import json,os,subprocess
from pathlib import Path
cfg=json.loads((Path(__file__).resolve().parents[1]/'config.json').read_text())['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=cfg['passwd'])
subprocess.run(['mysql','-h'+cfg['host'],'-P'+str(cfg['port']),'-u'+cfg['user'],cfg['dbname'],'-e',"CREATE TABLE IF NOT EXISTS master_import_locks (kind VARCHAR(32) NOT NULL PRIMARY KEY) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"],env=env,check=True)
print('master import locking ready')
