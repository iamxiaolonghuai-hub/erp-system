"""Read-only and invalid-input backup API checks; never saves a live destination or starts a job."""
from pathlib import Path
exec(Path(__file__).with_name('verify_product_approvals.py').read_text().split('pair=next')[0])
_,result=api('/operations/backup',admin);assert result.get('code')==0,result
assert {'settings','agent_online','jobs'}<=set(result['data']),result
staff=next(u for u in users if 'admin' not in u['roles'].split(','))
_,result=api('/operations/backup',staff);assert result.get('code')==40300,result
_,result=api('/operations/backup/request',staff,{});assert result.get('code')==40300,result
invalid=dict(enabled=True,host='host;command',username='ubuntu',port=22,remote_dir='/srv/erp-backups',local_dir='/opt/erp/backups',ssh_key_path='/opt/erp/keys/backup',time='03:00',keep_count=14)
_,result=api('/operations/backup',admin,invalid);assert result.get('code')==40000,result
_,result=api('/operations/backup',admin,{**invalid,'host':'backup.example.com','remote_dir':'/tmp/../etc'});assert result.get('code')==40000,result
print('PASS backup settings read, administrator authorization and unsafe input rejection; no live destination saved or job executed')
