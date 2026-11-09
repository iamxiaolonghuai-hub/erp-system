"""Real password-login regression, including concurrent callbacks and live account changes."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import secrets
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
name='qa_login_'+uuid.uuid4().hex[:18];password=secrets.token_urlsafe(24);salt=secrets.token_hex(16)
hashed=salt+'$'+hashlib.sha256((salt+password).encode()).hexdigest();uid=None
def call(path,data=None,bearer=''):
 request=Request('http://127.0.0.1:5555/api'+path,data=None if data is None else json.dumps(data).encode(),headers={'Content-Type':'application/json','Authorization':'Bearer '+bearer})
 try:
  with opener.open(request,timeout=5) as response:return response.status,json.load(response)
 except HTTPError as error:return error.code,json.load(error)
def login():
 status,result=call('/auth/login',dict(username=name,password=password))
 assert status==200 and result['code']==0,result
 assert 'password' not in result['data']['user']
 return result['data']['token']
try:
 sql(f"INSERT INTO users(username,password,real_name,department_id,status) VALUES('{name}','{hashed}','登录验证',(SELECT id FROM departments WHERE code='sales'),1)")
 uid=int(sql(f"SELECT id FROM users WHERE username='{name}'"))
 sql(f"INSERT INTO user_roles(user_id,role_id) SELECT {uid},id FROM roles WHERE code='staff'")
 assert call('/auth/login',dict(username=name,password='wrong'))[1]['code']==40100
 assert call('/auth/login',dict(username=name+'_missing',password=password))[1]['code']==40100
 with ThreadPoolExecutor(max_workers=4) as pool:tokens=list(pool.map(lambda _:login(),range(12)))
 bearer=tokens[0]
 status,profile=call('/auth/me',bearer=bearer);assert status==200 and profile['code']==0
 assert profile['data']['roles']==['staff'] and 'password' not in profile['data']
 sql(f'UPDATE users SET status=0 WHERE id={uid}')
 assert call('/auth/login',dict(username=name,password=password))[1]['code']==40300
 assert call('/auth/me',bearer=bearer)[1]['code']==40100
 sql(f'UPDATE users SET status=1 WHERE id={uid}')
 sql(f"DELETE FROM user_roles WHERE user_id={uid};INSERT INTO user_roles(user_id,role_id) SELECT {uid},id FROM roles WHERE code='leader'")
 assert call('/auth/me',bearer=bearer)[1]['data']['roles']==['leader']
 print('PASS real password login, concurrent login, profile without password hash, invalid credentials, disabled accounts and current roles')
finally:
 if uid:
  time.sleep(.2)
  sql(f'DELETE FROM operation_logs WHERE user_id={uid};DELETE FROM user_roles WHERE user_id={uid};DELETE FROM users WHERE id={uid}')
