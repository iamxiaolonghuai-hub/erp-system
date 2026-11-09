"""Add financial review/settlement fields without rewriting historical balances."""
import json, os, subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
db=json.loads((root/'config.json').read_text())['db_clients'][0]
env=dict(os.environ,MYSQL_PWD=db['passwd'])
command=['mysql','-h'+db['host'],'-P'+str(db['port']),'-u'+db['user'],db['dbname'],'-N','-B']
def sql(value):return subprocess.check_output(command+['-e',value],env=env,text=True)
for table,column,definition in [
 ('accounts','offset_amount','DECIMAL(14,2) NOT NULL DEFAULT 0.00'),
 ('payments','action_id','BIGINT UNSIGNED NULL'),
 ('payments','approved_by','BIGINT UNSIGNED NULL'),
 ('payments','reversal_of','BIGINT UNSIGNED NULL'),
 ('payments','reversed_by','BIGINT UNSIGNED NULL'),
 ('payments','settlement_amount','DECIMAL(14,2) NULL')]:
 if sql(f"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='{table}' AND column_name='{column}'").strip()=='0':
  sql(f'ALTER TABLE {table} ADD COLUMN {column} {definition}')
sql((root/'db/operations_upgrade.sql').read_text())
print('Financial review, refund, offset and reversal schema ready')
