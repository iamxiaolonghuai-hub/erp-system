from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import os
root=Path(__file__).resolve().parents[1]
out=root/'release'/'erp-source.zip'
out.parent.mkdir(exist_ok=True)
files=['README.md','.gitignore','Dockerfile','.dockerignore','compose.yaml','.env.example','CMakeLists.txt','main.cc']
folders=['controllers','filters','plugins','models','services','utils','views','test','db','deploy','frontend','docs']
with ZipFile(out,'w',ZIP_DEFLATED) as archive:
 for name in files:archive.write(root/name,name)
 for folder in folders:
  for base,dirs,names in os.walk(root/folder):
   dirs[:]=[d for d in dirs if d not in ('node_modules','dist','__pycache__')]
   for name in names:
    path=Path(base)/name
    rel=path.relative_to(root)
    if path.name in ('model.json','.env') or path.suffix in ('.log','.pyc','.tsbuildinfo'):continue
    archive.write(path,rel.as_posix())
print(out)
