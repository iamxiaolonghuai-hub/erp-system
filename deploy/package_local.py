"""WSL fallback: package explicitly selected local runtime files, never host config/secrets.
Run after cmake build and frontend npm run build; requires cached nginx:1.27-alpine.
"""
import os, re, shutil, subprocess, sysconfig
from pathlib import Path
root=Path(__file__).resolve().parents[1]
work=root/'release'/'offline-context'
if work.exists():
 if work.is_symlink() or work.resolve().parent!=(root/'release').absolute():raise RuntimeError('Refusing to clean build context outside release directory')
 shutil.rmtree(work)
work.mkdir(parents=True,exist_ok=True)
runtime=work/'runtime'
runtime.mkdir(exist_ok=True)
copied=set()
def copy_file(source,dest=None):
 source=Path(source); target=runtime/str(dest or source).lstrip('/')
 target.parent.mkdir(parents=True,exist_ok=True)
 shutil.copy2(source.resolve(),target)
def dependencies(source):
 source=Path(source).resolve()
 if str(source) in copied:return
 copied.add(str(source))
 result=subprocess.run(['ldd',str(source)],capture_output=True,text=True)
 for line in result.stdout.splitlines():
  if 'not found' in line:raise RuntimeError(line)
  match=re.search(r'(?:=>\s+)?(/[^\s]+)',line)
  if match:
   dep=match.group(1);copy_file(dep);dependencies(dep)
binary=root/'build_wsl'/'erp_system'
copy_file(binary,'/app/erp_system');dependencies(binary)
for executable in ['/usr/bin/python3','/usr/bin/mysql','/bin/sh']:
 copy_file(executable);dependencies(executable)
stdlib=Path(sysconfig.get_path('stdlib'))
shutil.copytree(stdlib,runtime/str(stdlib).lstrip('/'),dirs_exist_ok=True,
 ignore=shutil.ignore_patterns('__pycache__','site-packages','dist-packages','test','tests','idlelib','tkinter','ensurepip'))
for module in stdlib.rglob('*.so'):
 if (runtime/str(module).lstrip('/')).exists():dependencies(module)
plugins=Path('/usr/lib/x86_64-linux-gnu/libmariadb3/plugin')
if plugins.exists():
 for plugin in plugins.glob('*.so'):copy_file(plugin);dependencies(plugin)
for source in ['/etc/ssl/certs/ca-certificates.crt','/etc/nsswitch.conf','/usr/share/zoneinfo/Asia/Shanghai']:
 if Path(source).exists():copy_file(source)
for directory in ['app/uploads','app/public','tmp','etc']:(runtime/directory).mkdir(parents=True,exist_ok=True)
os.chmod(runtime/'tmp',0o1777)
for directory in ['app','app/uploads','app/public']:os.chown(runtime/directory,10001,10001)
(runtime/'etc/passwd').write_text('erp:x:10001:10001:ERP:/app:/bin/sh\n')
(runtime/'etc/group').write_text('erp:x:10001:\n')
for name in ['entrypoint.py','config.template.json']:copy_file(root/'deploy'/name,'/app/'+name)
shutil.copytree(root/'db',runtime/'app/db',dirs_exist_ok=True,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
(work/'Dockerfile.backend').write_text('FROM scratch\nCOPY runtime/ /\nWORKDIR /app\nENV PATH=/usr/bin:/bin TZ=Asia/Shanghai\nUSER 10001:10001\nEXPOSE 5555\nENTRYPOINT ["/usr/bin/python3","/app/entrypoint.py"]\nCMD ["./erp_system"]\n')
shutil.copytree(root/'frontend/dist',work/'dist',dirs_exist_ok=True)
shutil.copy2(root/'deploy/nginx.conf',work/'nginx.conf')
(work/'Dockerfile.frontend').write_text('FROM nginx:1.27-alpine\nCOPY dist/ /usr/share/nginx/html/\nCOPY nginx.conf /etc/nginx/conf.d/default.conf\nEXPOSE 80\n')
environment=dict(os.environ,DOCKER_BUILDKIT='0')
for target in ['backend','frontend']:
 subprocess.run(['docker','build','--pull=false','-f',str(work/('Dockerfile.'+target)),'-t','erp-'+target+':2026-11-01-r3',str(work)],env=environment,check=True)
print('Offline Docker images built from current WSL binaries and cached Nginx (linux/amd64).')
