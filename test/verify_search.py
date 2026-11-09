"""Exercise real search APIs with unique disposable fixtures and literal wildcard input."""
from pathlib import Path
from urllib.parse import urlencode
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
prefix='QAS'+uuid.uuid4().hex[:14];created=[];ids={}
def insert(table,values):
 columns=','.join(values);literals=','.join("'"+str(value)+"'" for value in values.values())
 sql(f'INSERT INTO {table}({columns}) VALUES({literals})')
 key='username' if table=='users' else 'name'
 ident=int(sql(f"SELECT id FROM {table} WHERE {key}='{values[key]}'"));created.append((table,ident));return ident
try:
 for table in ['suppliers','customers','employees','warehouses','categories','users','products']:
  field='username' if table=='users' else 'name';ids[table]=[]
  for index,tail in enumerate(['100%','100x']):
   values={field:prefix+tail}
   if table=='users':values.update(password='unused',real_name=prefix+'alias')
   if table=='employees':values['emp_no']=prefix+str(index)
   if table=='warehouses':values['code']=prefix+str(index)
   if table=='products':values.update(sku=prefix+str(index),supplier_id=ids['suppliers'][0],price=1)
   ids[table].append(insert(table,values))
  for term,expected in [(prefix+'100',set(ids[table])),('  '+prefix+'100  ',set(ids[table])),(prefix+'100%',{ids[table][0]}),(prefix+'100_',set()),(prefix+"'",set()),(prefix+'NO_MATCH',set())]:
   status,rows=api('/'+table+'?'+urlencode({field:term}),admin)
   assert status==200 and isinstance(rows,list),(table,status,rows)
   assert {int(row['id']) for row in rows}==expected,(table,term,rows)
  status,rows=api('/'+table,admin);assert status==200 and set(ids[table])<={int(row['id']) for row in rows}
  alternate={'employees':(prefix+'0',{ids[table][0]}),'products':(prefix+'0',{ids[table][0]}),'users':(prefix+'alias',set(ids[table]))}.get(table)
  if alternate:
   status,rows=api('/'+table+'?'+urlencode({field:alternate[0]}),admin);assert status==200 and {int(row['id']) for row in rows}==alternate[1],(table,rows)
  print('PASS',table,': matching, whitespace, literal %, no match, clearing search')
 print('PASS all seven master-data search APIs')
 unprivileged=dict(id=ids['users'][0],username=prefix+'100%',department='',roles='')
 assert api('/users?username=admin',unprivileged)[0]==403
 print('PASS searching users does not bypass administrator authorization')
finally:
 for table,ident in reversed(created):sql(f'DELETE FROM {table} WHERE id={ident}')
