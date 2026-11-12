"""Prepare release metadata locally. Never publishes anything."""
from __future__ import annotations
from datetime import datetime, timedelta, timezone
import hashlib
import json
from pathlib import Path, PurePosixPath
import zipfile
from .update import UpdateError, version_key, SHA


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def manifest_hash(manifest):
    return hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def prepare(source: Path, manifest: dict, contract: dict, qc: dict,
            artifacts: dict[str, Path], destination: Path):
    """qc is the maintainer's sign-off record, not a signature. CI can't produce it."""
    version = contract['version']
    key = version_key(version)
    if contract['channel'] not in ('stable', 'preview') or (contract['channel'] == 'stable' and not key[3]):
        raise UpdateError('Version and release channel disagree')
    if type(contract.get('repository_id')) is not int or contract['repository_id'] <= 0:
        raise UpdateError('Canonical numeric repository identity is required')
    if (qc.get('version') != version or qc.get('decision') != 'approved' or
            qc.get('approver') != 'Zach' or qc.get('source_manifest_sha256') != manifest_hash(manifest)):
        raise UpdateError('Release approval does not match this exact source')
    for gate in ('rights', 'license', 'sanitation', 'source_closure', 'save_preservation', 'hosting', 'gui_dependencies'):
        if qc.get('gates', {}).get(gate) is not True:
            raise UpdateError('Release check not done: ' + gate)
    rows = []
    for platform in ('mac', 'windows'):
        file = artifacts[platform]
        record = qc.get('personal_tests', {}).get(platform, {})
        if (file.is_symlink() or record.get('tester') != 'Zach' or record.get('result') != 'passed' or
                not record.get('evidence') or record.get('artifact_sha256') != sha(file)):
            raise UpdateError('The exact ' + platform + ' download must be personally tested')
        rows.append({'platform': platform, 'name': file.name, 'sha256': sha(file), 'bytes': file.stat().st_size})
    for field in ('archive_sha256', 'manifest_sha256'):
        if not SHA.fullmatch(contract['source_install'][field]):
            raise UpdateError('Invalid supported-install fingerprint')
    if not SHA.fullmatch(contract['expected_rom_sha256']):
        raise UpdateError('Expected locally built ROM hash is required')
    selected, seen = [], set()
    for row in manifest['files']:
        name = row['path']
        path = PurePosixPath(name)
        if (path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name or
                not path.parts or name.casefold() in seen or
                any(p.casefold() in ('private', '.git', '__pycache__', 'internal') for p in path.parts) or
                path.suffix.lower() in ('.gbc', '.gb', '.sav', '.srm', '.noi', '.o', '.rel', '.pyc')):
            raise UpdateError('Forbidden or ambiguous source manifest path')
        seen.add(name.casefold())
        file = source.joinpath(*path.parts)
        if (any(parent.is_symlink() for parent in [file, *file.parents] if parent.is_relative_to(source)) or
                not file.is_file() or file.stat().st_size != row['bytes'] or sha(file) != row['sha256']):
            raise UpdateError('Source file differs from the manifest: ' + name)
        selected.append((name, file))
    if not {'builder/__main__.py', 'engine/manifest.json', 'compatibility/owned_profiles.json'} <= seen:
        raise UpdateError('Selected manifest lacks required runnable source closure')
    metadata = {k: contract[k] for k in ('version', 'channel', 'source_install', 'save', 'expected_rom_sha256', 'changelog')}
    metadata.update(schema=1, minimum_updater='0.1.0', source_asset='balatrom-source.zip')
    if contract['channel'] == 'preview':
        expiry = datetime.fromisoformat(contract['expires_at'].replace('Z', '+00:00'))
        now = datetime.now(timezone.utc)
        if not now < expiry <= now + timedelta(days=14):
            raise UpdateError('Preview expiry must be within 14 days')
        metadata['expires_at'] = contract['expires_at']
    destination.mkdir(mode=0o700, exist_ok=False)
    archive = destination / 'balatrom-source.zip'
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as output:
        for name, file in sorted(selected):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(info, file.read_bytes())
    (destination/'balatrom-update.json').write_text(json.dumps(metadata, indent=2)+'\n')
    receipt = {'version': version, 'repository_id': contract['repository_id'], 'published': False,
               'source_sha256': sha(archive), 'artifacts': rows, 'approval': qc}
    (destination/'RELEASE-RECEIPT.json').write_text(json.dumps(receipt, indent=2)+'\n')
    return receipt
