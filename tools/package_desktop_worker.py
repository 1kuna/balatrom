"""Package only the headless worker; no Qt UI is included."""
from pathlib import Path
import argparse,json,subprocess,sys,hashlib,os
p=argparse.ArgumentParser();p.add_argument('--packet',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];packet=a.packet.resolve(strict=True);output=a.output.absolute();output.mkdir(exist_ok=False)
manifest=json.loads((packet/'SOURCE-MANIFEST.json').read_text())
for row in manifest['files']:
    if hashlib.sha256((root/row['path']).read_bytes()).hexdigest()!=row['sha256']:raise ValueError('Source changed after packet preparation')
if hashlib.sha256((packet/'initial-source.zip').read_bytes()).hexdigest()!=json.loads((packet/'initial-contract.json').read_text())['source_sha256']:raise ValueError('Packet hash differs')
data=[(str(packet/name),'.') for name in ('initial-source.zip','initial-contract.json')]+[(str(root/'notices'),'notices')]
spec=output/'worker.spec'
spec.write_text('a=Analysis('+repr([str(root/'tools/desktop_entry.py')])+', pathex='+repr([str(root)])+', binaries=[], datas='+repr(data)+", hiddenimports=['builder.update_worker','builder.owned_install','builder.compile_cache'], excludes=['PySide6','shiboken6','tkinter'])\npyz=PYZ(a.pure)\nexe=EXE(pyz,a.scripts,[],exclude_binaries=True,name='balatrom-worker',console=True,strip=False,upx=False)\ncoll=COLLECT(exe,a.binaries,a.datas,name='balatrom-worker',strip=False,upx=False)\n")
environment=dict(os.environ, PYINSTALLER_CONFIG_DIR=str(output/'cache'), PYTHONDONTWRITEBYTECODE='1')
subprocess.run([sys.executable,'-m','PyInstaller','--noconfirm','--log-level','WARN','--distpath',str(output/'dist'),'--workpath',str(output/'work'),str(spec)],check=True,env=environment)
print(json.dumps({'worker':str(output/'dist/balatrom-worker'/('balatrom-worker.exe' if sys.platform=='win32' else 'balatrom-worker')),'source_files_verified':len(manifest['files']),'Qt_GUI':False}))
