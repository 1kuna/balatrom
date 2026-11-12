"""Load the source bundle shipped inside the app. No network access."""
import json
from pathlib import Path
from .update import UpdateError, digest, SHA, MAX_PACKAGE, version_key, verified_bytes


def load(contract_path: Path):
    value=json.loads(contract_path.read_text())
    if value.get('scope')!='private-reviewed-owned-build' or value.get('schema')!=1:
        raise UpdateError('Initial source package is not recognized')
    version_key(value['version'])
    # PyInstaller Mac bundles expose data via internal Frameworks→Resources symlinks.
    resolved_contract=contract_path.resolve(strict=True)
    package=(contract_path.parent/'initial-source.zip').resolve(strict=True)
    if package.parent!=resolved_contract.parent or not package.is_file() or package.stat().st_size>MAX_PACKAGE or digest(package.read_bytes())!=value['source_sha256']:
        raise UpdateError('Initial source package failed integrity verification')
    for field in ('archive_sha256','manifest_sha256'):
        if not SHA.fullmatch(value['source_install'][field]): raise UpdateError('Invalid owned-install fingerprint')
    if not SHA.fullmatch(value['expected_rom_sha256']): raise UpdateError('Invalid ROM contract')
    metadata={k:value[k] for k in ('version','source_install','expected_rom_sha256','save')}
    metadata.update(channel='local',source={'size':package.stat().st_size,'digest':'sha256:'+value['source_sha256'],'browser_download_url':'local-reviewed-source'})
    def reader(url,limit,cancel):
        if url!='local-reviewed-source': raise UpdateError('No network source permitted in local build')
        raw=package.read_bytes()
        if len(raw)>limit: raise UpdateError('Initial source package exceeds limit')
        return raw
    return metadata,reader
