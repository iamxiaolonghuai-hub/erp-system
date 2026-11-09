"""Host-side Docker backup and restore. Restore only into an empty database.
Password is read from ERP_DB_PASSWORD, never from command arguments or the archive.
"""
import argparse, hashlib, io, json, os, re, signal, subprocess, sys, tarfile, tempfile
from datetime import datetime, timezone
from pathlib import Path

def docker(*args, input=None):
 result=subprocess.run(['docker',*args],input=b'' if input is None else input,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=1800)
 if result.returncode:raise RuntimeError('Docker operation failed: '+result.stderr.decode(errors='replace').replace(os.environ.get('ERP_DB_PASSWORD','!NO_PASSWORD!'),'[redacted]'))
 return result.stdout
def mysql(args,query=None,dump=False,ignore_views=()):
 return docker('exec','-i','-e','MYSQL_PWD='+os.environ['ERP_DB_PASSWORD'],args.mysql_container,
  *(['mysqldump','--single-transaction','--skip-lock-tables','--no-tablespaces','--hex-blob',*['--ignore-table='+args.database+'.'+name for name in ignore_views]] if dump else ['mysql','-N','-B','--raw']),
  '--default-character-set=utf8mb4',
  '-u'+args.db_user,args.database,input=None if query is None else query.encode())
def image_for(args):return getattr(args,'backend_image',None) or docker('inspect','--format','{{.Image}}',args.backend_container).decode().strip()
def recover_backend(path):
 marker=Path(path)
 if not marker.exists():return
 value=json.loads(marker.read_text())
 name=value.get('backend_container','')
 if value.get('was_running') is not True or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]{0,127}',name):raise RuntimeError('备份恢复标记无效，请人工核对后端状态')
 docker('start',name);marker.unlink()
 print('Recovered backend after interrupted backup: '+name,flush=True)
def backup(args):
 image=image_for(args)
 running=docker('inspect','--format','{{.State.Running}}',args.backend_container).decode().strip()=='true'
 marker=Path(args.recovery_file) if getattr(args,'recovery_file',None) else None
 if marker and not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]{0,127}',args.backend_container):raise RuntimeError('启用恢复标记时须使用有效的容器名或ID')
 if marker and marker.exists():raise RuntimeError('存在待恢复的备份标记，请先核对并恢复后端')
 if running and marker:
  marker.parent.mkdir(parents=True,exist_ok=True)
  with marker.open('x') as stream:
   os.chmod(marker,0o600);json.dump({'was_running':True,'backend_container':args.backend_container},stream);stream.flush();os.fsync(stream.fileno())
 try:
  if running:docker('stop',args.backend_container)
  view_rows=mysql(args,"SELECT JSON_OBJECT('name',TABLE_NAME,'sql',VIEW_DEFINITION) FROM information_schema.VIEWS WHERE TABLE_SCHEMA=DATABASE();").decode().splitlines()
  views=[json.loads(row) for row in view_rows]
  database=mysql(args,dump=True,ignore_views=[view['name'] for view in views])
  if not database:raise RuntimeError('Empty database dump refused')
  uploads=docker('run','--rm','--volumes-from',args.backend_container,'--entrypoint','python3',image,'-c',"import sys,tarfile; t=tarfile.open(fileobj=sys.stdout.buffer,mode='w|'); t.add('/app/uploads',arcname='uploads'); t.close()")
  metadata={'version':2,'database':args.database,'views':views,'created_at':datetime.now(timezone.utc).isoformat(),'sha256':{name:hashlib.sha256(value).hexdigest() for name,value in [('database.sql',database),('uploads.tar',uploads)]}}
  output=Path(args.output);output.mkdir(parents=True,exist_ok=True)
  target=output/('erp-'+datetime.now().strftime('%Y%m%d-%H%M%S-%f')+'.tar.gz')
  with target.open('xb') as stream:
   os.chmod(target,0o600)
   with tarfile.open(fileobj=stream,mode='w:gz') as archive:
    for name,value in [('database.sql',database),('uploads.tar',uploads),('manifest.json',json.dumps(metadata).encode())]:
     item=tarfile.TarInfo(name);item.size=len(value);item.mode=0o600;archive.addfile(item,io.BytesIO(value))
  print(target)
  return target
 finally:
  if running:
   docker('start',args.backend_container)
   if marker and marker.exists():marker.unlink()
def view_sql(view,source,target):
 name=view['name'];definition=view['sql']
 if source!=target:
  if name not in {'posted_sales_activity','posted_purchase_activity','user_directory','employee_directory'}:raise RuntimeError('跨库恢复包含额外自定义视图，需要先人工核对视图引用；未写入目标数据库')
  # Only requalify the ERP view's known base-table identifiers outside string literals.
  replacements={f'`{source}`.`{table}`':f'`{target}`.`{table}`' for table in ['accounts','sales_orders','purchase_orders','order_deliveries','users','employees','departments']}
  def replace(value):
   for original,new in replacements.items():value=value.replace(original,new)
   if re.search(re.escape("`"+source+"`")+r"\s*\.",value):raise RuntimeError("视图仍有未识别的源库引用，禁止跨库恢复，请人工核对")
   return value
  result=[];start=0;index=0
  while index<len(definition):
   if definition[index] not in "'\"":index+=1;continue
   result.append(replace(definition[start:index]));quote=definition[index];end=index+1
   while end<len(definition):
    if definition[end]=='\\':end+=2;continue
    if definition[end]==quote:
     if end+1<len(definition) and definition[end+1]==quote:end+=2;continue
     end+=1;break
    end+=1
   result.append(definition[index:end]);index=end;start=end
  result.append(replace(definition[start:]));definition=''.join(result)
 return 'CREATE SQL SECURITY INVOKER VIEW `'+name.replace('`','``')+'` AS '+definition+';'
def restore(args):
 with tarfile.open(args.archive,'r:gz') as archive:
  if set(archive.getnames())!={'database.sql','uploads.tar','manifest.json'}:raise RuntimeError('Unexpected archive members')
  payload={name:archive.extractfile(name).read() for name in archive.getnames()}
 manifest=json.loads(payload['manifest.json'])
 for name in ['database.sql','uploads.tar']:
  if hashlib.sha256(payload[name]).hexdigest()!=manifest['sha256'][name]:raise RuntimeError('Backup checksum mismatch')
 if manifest.get('version',1)<2 and manifest['database']!=args.database and b' VIEW ' in payload['database.sql']:
  raise RuntimeError('旧版备份包含视图，禁止直接跨库恢复；请在独立环境使用同名空库并核对视图')
 views=[view_sql(view,manifest['database'],args.database) for view in manifest.get('views',[])]
 # Only restore to a dedicated empty database; never overwrite existing tables.
 count=mysql(args,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE();").decode().strip()
 if count!='0':raise RuntimeError('Target database is not empty; restore refused')
 if docker('inspect','--format','{{.State.Running}}',args.backend_container).decode().strip()=='true':raise RuntimeError('Stop the target backend before restoring')
 image=image_for(args)
 check="from pathlib import Path; import sys; p=Path('/app/uploads'); sys.exit(1 if p.exists() and any(p.iterdir()) else 0)"
 docker('run','--rm','--volumes-from',args.backend_container,'--entrypoint','python3',image,'-c',check)
 # Validate nested paths before making any database changes.
 with tarfile.open(fileobj=io.BytesIO(payload['uploads.tar'])) as archive:
  for member in archive:
   path=Path(member.name)
   if path.is_absolute() or '..' in path.parts or not member.name.startswith('uploads') or member.issym() or member.islnk():raise RuntimeError('Unsafe attachment archive')
 mysql(args,payload['database.sql'].decode())
 pending=views[:]
 while pending:
  remaining=[]
  for statement in pending:
   try:mysql(args,statement)
   except RuntimeError:remaining.append(statement)
  if len(remaining)==len(pending):raise RuntimeError('视图重建失败，目标后端保持停止，请核对数据库权限及视图依赖')
  pending=remaining
 tables=mysql(args,"SELECT TABLE_NAME FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN ('operational_settings','backup_requests');").decode().splitlines()
 if 'operational_settings' in tables:mysql(args,"UPDATE operational_settings SET setting_value=JSON_SET(setting_value,'$.enabled',CAST('false' AS JSON)) WHERE setting_key='backup';")
 if 'backup_requests' in tables:mysql(args,"UPDATE backup_requests SET status='failed',completed_at=NOW(),details=JSON_OBJECT('message','恢复后取消历史排队任务，请核对备份策略后重新启用') WHERE status IN ('queued','running');")
 extract="import sys,tarfile; t=tarfile.open(fileobj=sys.stdin.buffer,mode='r|'); t.extractall('/app',filter='data')"
 docker('run','--rm','-i','--volumes-from',args.backend_container,'--entrypoint','python3',image,'-c',extract,input=payload['uploads.tar'])
 print('Database and attachments restored. Target backend remains stopped; verify before starting.')
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('operation',choices=['backup','restore'])
 parser.add_argument('--mysql-container',required=True);parser.add_argument('--database',required=True);parser.add_argument('--db-user',required=True)
 parser.add_argument('--backend-container',required=True);parser.add_argument('--backend-image')
 parser.add_argument('--output',default='./backups');parser.add_argument('--archive')
 parser.add_argument('--recovery-file')
 args=parser.parse_args()
 if not os.environ.get('ERP_DB_PASSWORD'):parser.error('Set ERP_DB_PASSWORD in the host environment')
 if not re.fullmatch('[A-Za-z0-9_]+',args.database):parser.error('Database name must contain only letters, digits and underscores')
 if args.operation=='restore' and not args.archive:parser.error('restore requires --archive')
 signal.signal(signal.SIGTERM,lambda *_:sys.exit(143))
 (backup if args.operation=='backup' else restore)(args)
if __name__=='__main__':main()
