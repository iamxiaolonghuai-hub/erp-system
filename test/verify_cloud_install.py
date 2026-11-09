"""Read-only deployment acceptance with the separately protected initial account file."""
import argparse,json
from pathlib import Path
from urllib.request import Request,build_opener,ProxyHandler
parser=argparse.ArgumentParser();parser.add_argument('--base-url',required=True);parser.add_argument('--accounts-file',required=True);args=parser.parse_args()
opener=build_opener(ProxyHandler({}));base=args.base_url.rstrip('/')
def api(path,data=None,bearer=''):
 request=Request(base+'/api'+path,data=None if data is None else json.dumps(data).encode(),headers={'Content-Type':'application/json','Authorization':'Bearer '+bearer})
 with opener.open(request,timeout=12) as response:return json.load(response)
with opener.open(base,timeout=12) as response:assert response.status==200
accounts=[row.split(' / ',1) for row in Path(args.accounts_file).read_text(encoding='utf-8-sig').splitlines() if ' / ' in row and not row.startswith('Initial')]
assert len(accounts)==9
for username,password in accounts:
 login=api('/auth/login',dict(username=username,password=password));assert login['code']==0,username
 assert 'password' not in login['data']['user']
 token=login['data']['token'];profile=api('/auth/me',bearer=token)
 assert profile['code']==0 and profile['data']['username']==username and 'password' not in profile['data']
 if username=='admin':
  for path in ['/report/dashboard','/inventory/summary','/operations/backup']:
   result=api(path,bearer=token);assert result['code']==0,path
   if path=='/operations/backup':assert result['data']['agent_online'] and not result['data']['settings']['enabled']
 print('PASS public login and profile:',username)
print('PASS public frontend, nine real accounts, dashboard, stock summary and disabled backup policy with live agent')
