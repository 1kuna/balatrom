"""Read-only native save harness preflight. Does not import/run any emulator."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(plan_path: Path, harness: Path):
    plan=json.loads(plan_path.read_text())
    if sha(harness)!=plan['script_sha256']:
        raise ValueError('Harness hash changed; update the plan')
    pins=[]
    for row in plan['variants']+plan['fixtures']:
        path=Path(row['path'])
        if not path.is_file() or path.is_symlink() or sha(path)!=row['sha256']:
            raise ValueError('Pinned ROM or fixture missing or changed: '+path.name)
        if 'noi_sha256' in row and sha(path.with_suffix('.noi'))!=row['noi_sha256']:
            raise ValueError('Pinned symbols changed: '+path.name)
        pins.append({'name':path.name,'sha256':row['sha256'],'bytes':path.stat().st_size})
    return {'status':'SOURCE_PREFLIGHT_ONLY','pins':pins,'harness_sha256':sha(harness),
            'native_run':False,'real_save_compatibility_proven':False,'user_saves_accessed':False,
            'required_runtime_cases':['old v14 SRAM → new ROM Continue and next action',
                'new-ROM re-saved SRAM → new cold boot', 'new SRAM → old ROM rollback and next action',
                'retained genuine v10 → new ROM; explicit v12/v13 fixtures or documented unsupported disposition',
                'export filename and private backup byte integrity; originals unchanged']}


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('plan',type=Path);parser.add_argument('harness',type=Path)
    args=parser.parse_args()
    print(json.dumps(check(args.plan,args.harness),indent=2))


if __name__=='__main__': main()
