"""Download the official GBDK once the user approves, and set up pinned compiler copies.

No game download, global install, PATH edits, installer scripts or security overrides.
"""
from pathlib import Path, PurePosixPath
import hashlib
import json
import os
import platform
import shutil
import sys
import tarfile
import tempfile
import time
import urllib.request
import urllib.parse
import uuid
import zipfile
from contextlib import contextmanager
from .update import UpdateError, check_cancel

MAX_DOWNLOAD = 32 * 1024**2
MAX_EXPANDED = 128 * 1024**2
HOSTS = {"github.com", "release-assets.githubusercontent.com", "objects.githubusercontent.com"}


def profile():
    machine = platform.machine().lower()
    if sys.platform == "darwin" and machine in ("arm64", "aarch64"):
        name = "GBDK-macos-arm64.json"
    elif sys.platform == "win32" and machine in ("amd64", "x86_64"):
        name = "GBDK-windows-amd64.json"
    else:
        raise UpdateError("There is no supported build-tools package for this computer yet.")
    root = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parents[1]))
    return json.loads((root / "notices" / name).read_text())


def _relative(name):
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts or "\\" in name or ":" in name:
        raise UpdateError("Build-tools archive contains an unsafe path.")
    return path


def verify(root, record):
    root = Path(root)
    if root.is_symlink() or not root.is_dir():
        return False
    expected = {row["path"]: row for row in record["files"]}
    actual = set()
    for path in root.rglob("*"):
        if path.is_symlink():
            return False
        if path.is_file():
            actual.add(path.relative_to(root).as_posix())
    if actual != set(expected):
        return False
    for name, row in expected.items():
        path = root.joinpath(*_relative(name).parts)
        if path.stat().st_size != row["bytes"] or hashlib.sha256(path.read_bytes()).hexdigest() != row["sha256"]:
            return False
    return True


def cached(cache, record=None):
    record = record or profile()
    return verify(Path(cache) / record["cache_name"], record)


class _Redirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        target = urllib.parse.urlsplit(newurl)
        if target.scheme != "https" or target.hostname not in HOSTS or target.username or target.password:
            raise UpdateError("Build-tools download redirected outside the official release hosts.")
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def download(url):
    target = urllib.parse.urlsplit(url)
    if target.scheme != "https" or target.hostname != "github.com" or not target.path.startswith("/gbdk-2020/gbdk-2020/releases/download/4.5.0/"):
        raise UpdateError("Build-tools URL is not the pinned official release.")
    return urllib.request.build_opener(_Redirect()).open(
        urllib.request.Request(url, headers={"User-Agent": "BalatROM-build-tools/1.0.0"}), timeout=15)


def _unpack(archive, destination, record, cancel):
    expected = {"gbdk/" + row["path"]: row for row in record["files"]}
    seen, total = set(), 0

    def save(name, size, stream):
        nonlocal total
        check_cancel(cancel)
        _relative(name)
        if name not in expected or name in seen or size != expected[name]["bytes"]:
            raise UpdateError("Build-tools archive contents differ from the pinned release.")
        total += size
        if total > MAX_EXPANDED:
            raise UpdateError("Build-tools archive exceeds the extraction limit.")
        path = destination.joinpath(*_relative(name).parts)
        path.parent.mkdir(parents=True, exist_ok=True)
        data = stream.read(size + 1)
        if len(data) != size or hashlib.sha256(data).hexdigest() != expected[name]["sha256"]:
            raise UpdateError("Build-tools file failed its integrity check.")
        with path.open("xb") as output:
            output.write(data)
        if os.name != "nt":
            path.chmod(expected[name].get("mode", 0o644) & 0o755)
        seen.add(name)

    if record["format"] == "tar.gz":
        with tarfile.open(archive, "r:gz") as package:
            for member in package:
                _relative(member.name)
                if member.isdir():
                    continue
                if not member.isfile():
                    raise UpdateError("Linked or special build-tools archive member refused.")
                with package.extractfile(member) as stream:
                    save(member.name, member.size, stream)
    elif record["format"] == "zip":
        with zipfile.ZipFile(archive) as package:
            for member in package.infolist():
                _relative(member.filename)
                if member.is_dir():
                    continue
                kind = (member.external_attr >> 16) & 0o170000
                if kind not in (0, 0o100000):
                    raise UpdateError("Linked or special build-tools archive member refused.")
                with package.open(member) as stream:
                    save(member.filename, member.file_size, stream)
    else:
        raise UpdateError("Unknown build-tools archive format.")
    if seen != set(expected):
        raise UpdateError("Build-tools archive is incomplete.")


def ensure(cache, *, approved, cancel, progress, record=None, reader=download):
    record = record or profile()
    cache = Path(cache)
    if cache.is_symlink():
        raise UpdateError("Linked build-tools cache refused; existing files preserved.")
    if verify(cache / record["cache_name"], record):
        progress("Verified build tools are ready.")
        return cache / record["cache_name"] / record["compiler"]
    if not approved:
        raise UpdateError("Build tools download needs your Build approval.")
    check_cancel(cancel)
    cache.mkdir(parents=True, exist_ok=True, mode=0o700)
    if shutil.disk_usage(cache).free < 2 * 1024**3:
        raise UpdateError("Not enough free space for build setup; at least 2 GiB is needed.")
    target = cache / record["cache_name"]
    if target.exists() or target.is_symlink():
        if target.is_symlink():
            raise UpdateError("Linked build-tools directory refused; existing files preserved.")
        target.rename(cache / ("preserved-invalid-" + uuid.uuid4().hex))
        progress("Preserved damaged tool cache; setting up a verified copy.")
    stage = Path(tempfile.mkdtemp(prefix="pending-tools-", dir=cache))
    try:
        progress("Getting free build tools from the official GBDK release…")
        archive = stage / "download"
        count = 0
        deadline = time.monotonic() + 120
        digest = hashlib.sha256()
        with reader(record["url"]) as response, archive.open("xb") as output:
            while True:
                check_cancel(cancel)
                if time.monotonic() > deadline:
                    raise UpdateError("Build-tools download timed out. Retry Build when the connection is ready.")
                data = response.read(65536)
                if not data:
                    break
                count += len(data)
                if count > min(MAX_DOWNLOAD, record["archive_bytes"]):
                    raise UpdateError("Build-tools download is larger than the pinned release.")
                digest.update(data)
                output.write(data)
                progress(f"Getting build tools: {count // 1024} / {record['archive_bytes'] // 1024} KiB")
        if count != record["archive_bytes"] or digest.hexdigest() != record["archive_sha256"]:
            raise UpdateError("Build-tools download failed verification. Retry Build; existing files are unchanged.")
        progress("Verifying and preparing build tools…")
        _unpack(archive, stage, record, cancel)
        check_cancel(cancel)
        if not verify(stage / "gbdk", record):
            raise UpdateError("Build tools failed final verification.")
        (stage / "gbdk").rename(target)
        progress("Build tools ready. Creating your ROM…")
        return target / record["compiler"]
    except OSError as exc:
        raise UpdateError("Build-tools setup could not finish. Retry Build when online; existing game and saves were preserved. " + str(exc)) from exc
    finally:
        shutil.rmtree(stage)


def _apply_working_copy_overlay(work, record):
    """Replace only a disposable compiler after verifying upstream and overlay pins."""
    overlay = record.get('working_copy_overlay')
    if overlay is None:
        return
    if (set(overlay) != {'source', 'destination', 'bytes', 'sha256'} or
            overlay['destination'] != 'bin/sdcc.exe' or
            type(overlay['bytes']) is not int or not 0 < overlay['bytes'] < 16 * 1024**2):
        raise UpdateError('Invalid reproducible compiler record.')
    source_name = _relative(overlay['source'])
    root = Path(__file__).resolve().parents[1]
    source = root.joinpath(*source_name.parts)
    if any(path.is_symlink() for path in [source, *list(source.parents)[:len(source_name.parts)-1]]):
        raise UpdateError('Linked reproducible compiler refused.')
    if not source.is_file() or source.stat().st_size != overlay['bytes']:
        raise UpdateError('Reproducible compiler is missing or incomplete.')
    data = source.read_bytes()
    if hashlib.sha256(data).hexdigest() != overlay['sha256']:
        raise UpdateError('Reproducible compiler failed integrity verification.')
    if not verify(work, record):
        raise UpdateError('Original build-tools working copy failed verification.')
    rows = [dict(row) for row in record['files']]
    matches = [row for row in rows if row['path'] == overlay['destination']]
    if len(matches) != 1:
        raise UpdateError('Reproducible compiler target is absent from the pinned SDK.')
    matches[0].update(bytes=overlay['bytes'], sha256=overlay['sha256'])
    target = Path(work).joinpath(*_relative(overlay['destination']).parts)
    target.write_bytes(data)
    if not verify(work, dict(record, files=rows)):
        raise UpdateError('Reproducible build-tools working copy failed verification.')


@contextmanager
def compiler_session(cache, *, approved, cancel, progress, record=None, reader=download):
    """Keep upstream lcc's 128-byte expansion buffer away from long user paths.

    Uses the verified upstream tools, plus the separately pinned compiler if there is one.
    The private cache stays the reference copy; the working copy is disposable.
    """
    record = record or profile()
    bundle = Path(__file__).resolve().parents[1] / 'bundled-tools' / 'gbdk'
    if bundle.is_dir():
        if not verify(bundle, record):
            raise UpdateError('Bundled build tools failed integrity verification. Re-extract the original ZIP.')
        compiler = bundle / record['compiler']
        progress('Verified bundled build tools are ready.')
    else:
        compiler = ensure(cache, approved=approved, cancel=cancel, progress=progress, record=record, reader=reader)
    if sum(row['bytes'] for row in record['files']) > MAX_EXPANDED:
        raise UpdateError('Build-tools working copy exceeds its size limit.')
    # macOS /tmp is a short, OS-provided temporary location.
    # mkdtemp creates a unique mode0700 directory; no pre-existing path is reused.
    parent = '/tmp' if sys.platform == 'darwin' else tempfile.gettempdir()
    with tempfile.TemporaryDirectory(prefix='brt-', dir=parent) as name:
        work = Path(name) / 'g'
        if len(str(work)) > 80:
            raise UpdateError('The system temporary location is too long for these build tools; existing files were preserved.')
        check_cancel(cancel)
        shutil.copytree(compiler.parent.parent, work, symlinks=False)
        if not verify(work, record):
            raise UpdateError('Temporary build-tools verification failed.')
        _apply_working_copy_overlay(work, record)
        progress('Build tools ready. Creating your ROM…')
        check_cancel(cancel)
        yield work / record['compiler']
