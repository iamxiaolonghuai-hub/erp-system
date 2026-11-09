"""Partner CSV import/export integration; removes only its own fixtures."""
from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
import csv, io
from concurrent.futures import ThreadPoolExecutor
prefix='QA_CSV_'+uuid.uuid4().hex
try:
 for table in ['suppliers','customers']:
  name=prefix+'_'+table
  content='\ufeff名称,联系人,电话,邮箱,地址,状态,备注\r\n'+f'{name},张三,001234,,"上海,浦东",1,"第一行\n第二行"\r\n'+f'  {name.lower()}  ,覆盖,999,,,1,\r\n'+',无名称,,,,1,\r\n'
  status,res=api('/import/'+table,admin,{'csv':content})
  assert res.get('code')==0,res
  assert (res['data']['success'],res['data']['skipped'],res['data']['failed'])==(1,1,1),res
  assert sql(f"SELECT CONCAT(contact,':',phone) FROM {table} WHERE name='{name}'")=='张三:001234'
  _,repeat=api('/import/'+table,admin,{'csv':content})
  assert (repeat['data']['success'],repeat['data']['skipped'])==(0,2),repeat
  req=Request('http://127.0.0.1:5555/api/export/'+table,headers={'Authorization':'Bearer '+token(admin)})
  with opener.open(req) as response: rows=list(csv.DictReader(io.StringIO(response.read().decode('utf-8-sig'))))
  row=next(r for r in rows if r['name']==name)
  assert row['remark']=='第一行\n第二行' and row['address']=='上海,浦东' and row['phone']=='001234',row
  concurrent_csv='name\n'+name+'_parallel\n'
  with ThreadPoolExecutor(2) as pool: results=list(pool.map(lambda _:api('/import/'+table,admin,{'csv':concurrent_csv})[1],range(2)))
  assert sum(r['data']['success'] for r in results)==1,results
  _,bad=api('/import/'+table,admin,{'csv':'name\n"unclosed'})
  assert bad.get('code')==40000,bad
  staff=next(u for u in users if 'staff' in u['roles'].split(','))
  status,bad=api('/import/'+table,staff,{'csv':content})
  assert status==403 or bad.get('code')==40300,bad
  print('PASS',table,'Chinese headers, duplicates, replay, concurrent uploads, row validation, export roundtrip, permissions')
finally:
 for table in ['suppliers','customers']:
  sql(f"DELETE FROM business_events WHERE document_type='{table}' AND document_id IN (SELECT id FROM {table} WHERE name LIKE '{prefix}%')")
  sql(f"DELETE FROM {table} WHERE name LIKE '{prefix}%'")
 print('Partner test fixtures removed')
