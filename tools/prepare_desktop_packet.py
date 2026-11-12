"""Bundle tracked source files into a deterministic initial app package."""
from pathlib import Path
import hashlib,json,zipfile,argparse,re,subprocess
p=argparse.ArgumentParser();p.add_argument('--product',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--base-contract',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1]
product=a.product or root/'desktop/package.json'
if product.is_symlink() or not product.is_file() or product.stat().st_size>1024*1024:raise ValueError('Shared product metadata required')
version=json.loads(product.read_text())['version']
if not re.fullmatch(r'\d+\.\d+\.\d+(?:-rc\.\d+)?',version):raise ValueError('Invalid shared product version')
a.output.mkdir(parents=True,exist_ok=False)
files=[]
names=subprocess.check_output(['git','-C',str(root),'ls-files','--cached','-z']).decode().split('\0')
for name in sorted(n for n in names if n):
    relative=Path(name)
    if relative.is_absolute() or '..' in relative.parts or '.git' in relative.parts:
        raise ValueError('Unsafe tracked source path')
    path=root/relative
    if any(p.is_symlink() for p in (path,*[root/Path(*relative.parts[:i]) for i in range(1,len(relative.parts))])):
        raise ValueError('Source symlink is not admitted')
    if path.is_symlink():raise ValueError('Source symlink is not admitted')
    if not path.is_file():raise ValueError('Tracked source file is missing: '+name)
    if '__pycache__' in path.parts or path.suffix=='.pyc':raise ValueError('Remove generated Python cache before packaging')
    files.append({'path':name,'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
with zipfile.ZipFile(a.output/'initial-source.zip','w',compression=zipfile.ZIP_DEFLATED) as z:
    for row in files:
        name=row['path']
        if name.startswith('tests/') or name in ('tools/gui_qa_entry.py','builder/update_gui.py','tools/app_entry.py','tools/package_app.py'):continue
        info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED
        z.writestr(info,(root/name).read_bytes())
contract=json.loads(a.base_contract.read_text());contract['version']=version;contract['source_sha256']=hashlib.sha256((a.output/'initial-source.zip').read_bytes()).hexdigest()
(a.output/'initial-contract.json').write_text(json.dumps(contract,indent=2)+'\n')
(a.output/'SOURCE-MANIFEST.json').write_text(json.dumps({'schema':1,'files':files},indent=2)+'\n')
print(json.dumps({'version':contract['version'],'source_files':len(files),'source_zip_sha256':contract['source_sha256']}))
