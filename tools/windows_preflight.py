"""Native Windows preparation probe: reads selected game/SDK, never executes either."""
from pathlib import Path
import argparse
import hashlib
import importlib.metadata
import json
import os
import platform
import struct
import sys


def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game',type=Path,required=True)
    p.add_argument('--lcc',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if os.name!='nt' or struct.calcsize('P')!=8 or platform.machine().lower() not in ('amd64','x86_64'):
        p.error('Run on 64-bit Windows (AMD64) with 64-bit CPython 3.13')
    if sys.version_info[:2]!=(3,13):p.error('This preparation lock targets CPython3.13 only')
    root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root))
    from builder.archive import Archive,InputError
    from builder.owned_install import private_destination,build_owned
    output=private_destination(args.output,args.game);output.mkdir(mode=0o700)
    sha=lambda data:hashlib.sha256(data).hexdigest()
    result={'scope':'Read-only native Windows interpreter/dependency/SDK/input probe; no compiler/game execution',
            'platform':platform.platform(),'python':sys.version,'architecture':platform.machine(),
            'profiles_or_saves_accessed':False,'game_executed':False,'compiler_executed':False,
            'Windows_consumer_GUI_guard':'disabled until tested on Windows',
            'runtime_qualified':False,'errors':[]}
    versions={'PySide6-Essentials':'6.11.2','shiboken6':'6.11.2','PyInstaller':'6.22.3','Pillow':'12.3.0'}
    result['dependencies']={}
    for name,expected in versions.items():
        try:actual=importlib.metadata.version(name)
        except importlib.metadata.PackageNotFoundError:actual=None
        result['dependencies'][name]=actual
        if actual!=expected:result['errors'].append('Dependency mismatch: '+name)
    record=json.loads((root/'builder/windows_sdk.json').read_text());sdk=args.lcc.resolve().parent.parent
    if args.lcc.name.lower()!='lcc.exe':result['errors'].append('Select pinned bin/lcc.exe')
    missing=[]
    for row in record['prerequisites']:
        path=sdk/row['path']
        if path.is_symlink() or not path.is_file() or sha(path.read_bytes())!=row['sha256']:missing.append(row['path'])
    result['SDK']={'GBDK':'4.5.0','archive_sha256':record['archive_sha256'],'pins_checked':len(record['prerequisites']),'mismatches':missing}
    if missing:result['errors'].append('SDK differs from official pinned win64 archive')
    try:
        with Archive(args.game) as archive:
            fingerprint=archive.fingerprint()
            result['input_fingerprint']=fingerprint
            archive.assert_unchanged()
    except (InputError,OSError) as exc:result['errors'].append('Cannot inspect game: '+str(exc))
    if not result['errors']:
        try:result['registered_preflight']=build_owned(args.game,root/'engine',output/'unused-build-output',args.lcc,preflight=True)
        except (InputError,OSError) as exc:result['errors'].append('Missing or unsupported build prerequisites: '+str(exc))
    result['status']='FAIL_UNQUALIFIED' if result['errors'] else 'PASS_PREREQUISITES_ONLY'
    (output/'WINDOWS-PREFLIGHT.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='input_fingerprint'},indent=2))
    return int(bool(result['errors']))


if __name__=='__main__':raise SystemExit(main())
