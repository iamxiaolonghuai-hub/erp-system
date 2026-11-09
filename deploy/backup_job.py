"""Linux host agent. Run from ERP compose directory via systemd every five minutes.
DB_HOST (MySQL container), DB_NAME, DB_USER and DB_PASSWORD are host environment values.
The web application only writes validated configuration and queue rows; it cannot execute shell.
"""
import argparse, fcntl, hashlib, json, os, re, shlex, signal, subprocess, sys
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path
from types import SimpleNamespace
from zoneinfo import ZoneInfo
import data_backup

def literal(value):return "CONVERT(0x"+str(value).encode().hex()+" USING utf8mb4)"
def validate(settings):
 if not isinstance(settings,dict) or not isinstance(settings.get('enabled'),bool):raise ValueError('备份配置无效')
 if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9.-]{0,252}',str(settings.get('host',''))):raise ValueError('备份服务器地址无效')
 if not re.fullmatch(r'[a-z_][a-z0-9_-]{0,31}',str(settings.get('username',''))):raise ValueError('备份登录用户名无效')
 for key in ['remote_dir','local_dir','ssh_key_path']:
  value=settings.get(key,'')
  if not isinstance(value,str) or len(value)>240 or not re.fullmatch(r'/[A-Za-z0-9_./-]+',value) or '..' in value or value=='/':raise ValueError('备份路径无效')
 for key,maximum in [('port',65535),('keep_count',365)]:
  if type(settings.get(key)) is not int or not 1<=settings[key]<=maximum:raise ValueError('端口或保留份数无效')
 if not re.fullmatch(r'([01][0-9]|2[0-3]):[0-5][0-9]',str(settings.get('time',''))):raise ValueError('备份时间无效')
 return settings
def query(args,statement):return data_backup.mysql(args,statement).decode().strip()
def write_details(args,id,status,details):
 query(args,f"UPDATE backup_requests SET status={literal(status)},details={literal(json.dumps(details,ensure_ascii=False))},completed_at=NOW() WHERE id={int(id)}")
def run_command(command,timeout=1800):
 environment={key:value for key,value in os.environ.items() if key not in {"DB_PASSWORD","ERP_DB_PASSWORD","JWT_SECRET"}}
 result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=timeout,env=environment)
 if result.returncode:raise RuntimeError('异机传输失败，请检查网络、主机指纹、密钥和目录权限；原始备份保留在本地')
 return result.stdout.decode()
def upload(path,settings):
 key=Path(settings['ssh_key_path'])
 if not key.is_file() or key.stat().st_mode & 0o077:raise RuntimeError('SSH私钥不存在或权限不安全，应由宿主机用户妥善保管并设置为600')
 target=settings['username']+'@'+settings['host'];directory=settings['remote_dir'].rstrip('/')
 options=['-i',str(key),'-o','BatchMode=yes','-o','StrictHostKeyChecking=yes','-o','ConnectTimeout=15']
 run_command(['ssh','-p',str(settings['port']),*options,target,'mkdir -p -- '+shlex.quote(directory)])
 run_command(['scp','-P',str(settings['port']),*options,str(path),target+':'+directory+'/'+path.name])
 remote_hash=run_command(['ssh','-p',str(settings['port']),*options,target,'chmod 600 -- '+shlex.quote(directory+'/'+path.name)+' && sha256sum -- '+shlex.quote(directory+'/'+path.name)]).split()
 digest_builder=hashlib.sha256()
 with path.open('rb') as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):digest_builder.update(block)
 digest=digest_builder.hexdigest()
 if not remote_hash or remote_hash[0]!=digest:raise RuntimeError('远端备份校验和不一致，本地备份已保留')
 return digest
def retain(settings):
 directory=Path(settings['local_dir']).resolve()
 files=sorted((p for p in directory.glob('erp-*.tar.gz') if not p.is_symlink() and p.resolve().parent==directory and re.fullmatch(r'erp-\d{8}-\d{6}-\d{6}\.tar\.gz',p.name)),key=lambda p:p.name,reverse=True)
 for path in files[settings['keep_count']:]:path.unlink()
def process(args,settings,job):
 id=int(job['id']);details=job.get('details') or {}
 if isinstance(details,str):details=json.loads(details)
 claimed=query(args,f"START TRANSACTION;SELECT setting_value FROM operational_settings WHERE setting_key='backup' FOR UPDATE;UPDATE backup_requests SET status='running',started_at=NOW() WHERE id={id} AND status='queued';SELECT ROW_COUNT();COMMIT;").splitlines()
 if not claimed or claimed[-1]!='1':return
 try:
  settings=json.loads(claimed[0]);validate(settings)
  if not settings['enabled']:raise RuntimeError('备份已禁用，任务未执行')
  details.update(target_host=settings['host'],remote_dir=settings['remote_dir'],started_at=datetime.now(ZoneInfo('Asia/Shanghai')).isoformat())
  query(args,f"UPDATE backup_requests SET details={literal(json.dumps(details,ensure_ascii=False))} WHERE id={id}")
  backend=os.environ.get('ERP_BACKEND_CONTAINER')
  if not backend:backend=data_backup.docker('compose','ps','-aq','backend').decode().strip()
  if not backend or '\n' in backend:raise RuntimeError('无法确定唯一ERP后端容器，请配置ERP_BACKEND_CONTAINER')
  request=SimpleNamespace(**vars(args));request.backend_container=backend;request.backend_image=os.environ.get('ERP_BACKEND_IMAGE');request.output=settings['local_dir'];request.recovery_file=os.environ.get('ERP_BACKUP_RECOVERY_FILE','/opt/erp/state/backup-recovery.json')
  archive=data_backup.backup(request);details['archive']=archive.name
  details['sha256']=upload(archive,settings);details['message']='数据库和附件已备份并完成远端SHA256校验'
  retain(settings);write_details(args,id,'success',details);print('Backup succeeded: job '+str(id),flush=True)
 except BaseException as error:
  text='任务收到停止信号，已尝试恢复后端，请检查归档后重试' if isinstance(error,(KeyboardInterrupt,SystemExit)) else str(error)
  message=text.replace(os.environ.get('ERP_DB_PASSWORD','!NO_PASSWORD!'),'[redacted]')[:500]
  details['message']=message;write_details(args,id,'failed',details);print('Backup failed: job '+str(id)+' '+message,file=sys.stderr,flush=True);raise
def tick(args,force=False):
 query(args,'INSERT INTO backup_agent(id,last_seen_at) VALUES(1,NOW()) ON DUPLICATE KEY UPDATE last_seen_at=NOW();')
 raw=query(args,"SELECT setting_value FROM operational_settings WHERE setting_key='backup';")
 if not raw:return
 settings=json.loads(raw)
 if not settings.get('enabled'):return
 validate(settings);now=datetime.now(ZoneInfo('Asia/Shanghai'));today=now.date().isoformat()
 if force or now.strftime('%H:%M')>=settings['time']:
  details={'scheduled_date':today,'trigger':'scheduled'}
  query(args,"START TRANSACTION;SELECT setting_key FROM operational_settings WHERE setting_key='backup' FOR UPDATE;INSERT INTO backup_requests(details) SELECT "+literal(json.dumps(details))+" WHERE NOT EXISTS(SELECT 1 FROM backup_requests WHERE status IN ('queued','running')) AND NOT EXISTS(SELECT 1 FROM backup_requests WHERE JSON_UNQUOTE(JSON_EXTRACT(details,'$.scheduled_date'))="+literal(today)+");COMMIT;")
 jobs=query(args,"SELECT JSON_OBJECT('id',id,'details',details) FROM backup_requests WHERE status='queued' ORDER BY id LIMIT 1;")
 if jobs:process(args,settings,json.loads(jobs))
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--lock-file',default='/opt/erp/state/backup-job.lock');parser.add_argument('--force',action='store_true');args=parser.parse_args()
 password=os.environ.get('DB_PASSWORD') or os.environ.get('ERP_DB_PASSWORD')
 if not password:parser.error('DB_PASSWORD or ERP_DB_PASSWORD is required')
 os.environ['ERP_DB_PASSWORD']=password
 database=os.environ.get('DB_NAME','erp_system')
 if not re.fullmatch(r'[A-Za-z0-9_]+',database):parser.error('Invalid database name')
 lock=Path(args.lock_file);lock.parent.mkdir(parents=True,exist_ok=True)
 with lock.open('a') as stream:
  try:fcntl.flock(stream,fcntl.LOCK_EX|fcntl.LOCK_NB)
  except BlockingIOError:return
  connection=SimpleNamespace(mysql_container=os.environ.get('BACKUP_MYSQL_CONTAINER',os.environ.get('DB_HOST','db-mysql')),database=database,db_user=os.environ.get('DB_USER','erp'))
  signal.signal(signal.SIGTERM,lambda *_:sys.exit(143))
  data_backup.recover_backend(os.environ.get('ERP_BACKUP_RECOVERY_FILE','/opt/erp/state/backup-recovery.json'))
  query(connection,"UPDATE backup_requests SET status='failed',completed_at=NOW(),details=JSON_SET(COALESCE(details,JSON_OBJECT()),'$.message','上一任务中断，已核对后端恢复标记，请检查归档后重试') WHERE status='running';")
  tick(connection,args.force)
if __name__=='__main__':main()
