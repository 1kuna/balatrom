"""Native-host packaging proposal: no dependency install/sign/upload/publication."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import subprocess
import sys
import re


def mac_metadata(record):
    candidate = record.get("candidate", {})
    version = candidate.get("version", "")
    if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:-rc\.[0-9]+)?", version):
        raise ValueError("Exact candidate version is required")
    minimum = candidate.get("minimum_macos", "")
    if not re.fullmatch(r"[0-9]+\.[0-9]+(?:\.[0-9]+)?", minimum):
        raise ValueError("Exact dependency-qualified macOS minimum is required")
    base = version.split("-")[0]
    return {"CFBundleShortVersionString": base, "CFBundleVersion": base,
            "LSMinimumSystemVersion": minimum, "BalatROMCandidateVersion": version}



def data_hashes(root):
    """Bind every notice and user-facing package document; refuse linked inputs."""
    root = Path(root)
    notices = root / 'notices'
    if notices.is_symlink() or not notices.is_dir():
        raise ValueError('Regular notice directory required')
    entries = list(notices.rglob('*'))
    if any(p.is_symlink() for p in entries):
        raise ValueError('Linked notice input refused')
    files = [p for p in entries if p.is_file()]
    files += [root / n for n in ('README.md', 'INSTALL.md', 'THIRD-PARTY-NOTICES.md')]
    if any(p.is_symlink() or not p.is_file() for p in files):
        raise ValueError('Regular package documents required')
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(files)}


def verify_data_files(root, record):
    actual = data_hashes(root)
    if not actual or actual != record.get('data_hashes'):
        raise ValueError('Package notices/documents differ from reviewed data hashes')
    return actual


def validate_macos_deployment(bundle, declared):
    """Check actual Mach-O minimums instead of trusting a wheel filename tag."""
    versions = lambda value: tuple(int(x) for x in value.split('.')) + (0,) * (3 - len(value.split('.')))
    rows = []
    magic = {bytes.fromhex(x) for x in ('cffaedfe', 'cefaedfe', 'cafebabe', 'cafebabf', 'bebafeca', 'bfbafeca')}
    for path in sorted(Path(bundle).rglob('*')):
        if path.is_symlink() or not path.is_file():
            continue
        with path.open('rb') as stream:
            if stream.read(4) not in magic:
                continue
        output = subprocess.check_output(['/usr/bin/otool', '-l', str(path)], text=True)
        minimums = []
        for block in output.split('Load command '):
            if 'LC_BUILD_VERSION' in block:
                platform = re.search(r'^\s*platform\s+(\S+)', block, re.M)
                if platform is None or platform[1] not in ('MACOS', '1'):
                    raise ValueError('Non-macOS or unknown binary platform')
                minimums.extend(re.findall(r'^\s*minos\s+(\S+)', block, re.M))
            elif 'LC_VERSION_MIN_MACOSX' in block:
                minimums.extend(re.findall(r'^\s*version\s+(\S+)', block, re.M))
        if not minimums:
            raise ValueError('Binary deployment target is unknown')
        if any(versions(v) > versions(declared) for v in minimums):
            raise ValueError('Bundled binary requires newer macOS than declared: ' + path.relative_to(bundle).as_posix())
        rows.append({'path': path.relative_to(bundle).as_posix(), 'minimums': minimums})
    if not rows:
        raise ValueError('No native binaries verified')
    return {'declared_minimum': declared, 'binaries': rows, 'older_OS_acceptance': False}


def main():
    raise SystemExit("Legacy Qt packaging is retired. Package the headless worker with tools/package_desktop_worker.py, then use the single desktop/package-app.cjs for macOS or Windows.")

if __name__ == '__main__': main()
