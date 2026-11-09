"""Export tested image tags and a self-contained upload bundle, no local credentials/data."""
import hashlib, io, json, subprocess, tarfile
from pathlib import Path
root=Path(__file__).resolve().parents[1];release=root/'release';release.mkdir(exist_ok=True)
tags=['erp-backend:2026-11-01-r3','erp-frontend:2026-11-01-r3']
metadata=json.loads(subprocess.check_output(['docker','image','inspect',*tags]))
assert all(image['Architecture']=='amd64' and image['Os']=='linux' for image in metadata)
images=release/'erp-images.tar'
subprocess.run(['docker','save','-o',str(images),*tags],check=True)
archive=release/'erp-server-linux-amd64-2026-11-01-r3.tar.gz'
with tarfile.open(archive,'w:gz',compresslevel=6) as bundle:
 bundle.add(images,arcname='erp-images.tar')
 for path in ['compose.yaml','.env.example','deploy','db','docs/异机备份设置说明.md','docs/backup-restore-guide.md','docs/批次效期与移动平均成本操作说明.md','docs/batch-reservation-release.md','docs/ERP角色功能模块说明.md','docs/completion-status.md','docs/数据库规范化与部门管理说明.md']:
  bundle.add(root/path,arcname=path,filter=lambda info:None if '__pycache__' in info.name or info.name.endswith('.pyc') else info)
 manifest=json.dumps({'platform':'linux/amd64','images':{tags[i]:image['Id'] for i,image in enumerate(metadata)},'build':'WSL binary runtime packaging; cached nginx:1.27-alpine'},indent=2).encode()
 info=tarfile.TarInfo('BUILD-INFO.json');info.size=len(manifest);bundle.addfile(info,io.BytesIO(manifest))
checksum=hashlib.file_digest(archive.open('rb'),'sha256').hexdigest()
(release/(archive.name+'.sha256')).write_text(checksum+'  '+archive.name+'\n')
print(archive);print('Size: %.1f MB'%(archive.stat().st_size/1024/1024));print('SHA256: '+checksum)
