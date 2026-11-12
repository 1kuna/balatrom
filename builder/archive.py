"""Read a user-selected ordinary desktop archive without executing it."""
from contextlib import AbstractContextManager
from pathlib import Path, PurePosixPath
import hashlib
import json
import os
import stat
import zipfile
import zlib


class InputError(ValueError):
    pass


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical_json(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode()


def game_archive(path):
    path = Path(path).expanduser()
    if path.is_symlink():
        raise InputError("Select a regular game file or app, not a symlink")
    if path.is_dir() and path.suffix.lower() == ".app":
        resources = path / "Contents" / "Resources"
        if not resources.is_dir() or resources.is_symlink():
            raise InputError("App has no regular Contents/Resources directory")
        # Only discover ordinary .love files, never launch/execute the app.
        candidates = []
        examined = 0
        import os
        for base, dirs, files in os.walk(resources, followlinks=False):
            if any((Path(base) / d).is_symlink() for d in dirs):
                raise InputError("App resource symlinks are unsupported")
            examined += len(dirs) + len(files)
            if examined > 10000:
                raise InputError("App resource discovery exceeds limit")
            for name in files:
                if name.lower().endswith(".love"):
                    candidate = Path(base) / name
                    if candidate.is_symlink():
                        raise InputError("App archive symlinks are unsupported")
                    candidates.append(candidate)
        if len(candidates) != 1:
            raise InputError("Select one ordinary .love resource; app discovery is ambiguous or empty")
        path = candidates[0]
    if not path.is_file() or path.suffix.lower() not in (".love", ".exe"):
        raise InputError("Select an installed readable .love or fused .exe, or an app containing one .love")
    if path.is_symlink():
        raise InputError("Archive symlinks are unsupported")
    return path.resolve()


class Archive(AbstractContextManager):
    MAX_ARCHIVE = 512 * 1024 * 1024
    MAX_MEMBER = 64 * 1024 * 1024
    MAX_TOTAL = 1024 * 1024 * 1024
    MAX_ENTRIES = 10000
    MAX_RATIO = 200

    def __init__(self, selected):
        self.path = game_archive(selected)
        self.file = None
        self.zip = None

    def __enter__(self):
        try:
            self.file = self.path.open("rb")
            self.before = os.fstat(self.file.fileno())
            self.assert_unchanged()
            if self.before.st_size > self.MAX_ARCHIVE:
                raise InputError("Archive exceeds size limit")
            h = hashlib.sha256()
            for chunk in iter(lambda: self.file.read(1024 * 1024), b""):
                h.update(chunk)
            self.archive_sha256 = h.hexdigest()
            self.file.seek(0)
            self.zip = zipfile.ZipFile(self.file)
            infos = self.zip.infolist()
            if not infos or len(infos) > self.MAX_ENTRIES:
                raise InputError("Archive entry count is unsupported")
            self.members = {}
            total = 0
            for info in infos:
                name = info.filename
                parts = name.rstrip("/").split("/")
                if (not name or "\\" in name or any(p in ("", ".", "..") for p in parts)
                        or PurePosixPath(name).is_absolute() or ":" in name
                        or any(ord(c) < 32 for c in name) or info.orig_filename != name):
                    raise InputError("Archive contains an unsafe member name")
                mode = info.external_attr >> 16
                if stat.S_ISLNK(mode) or stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR):
                    raise InputError("Archive links and special files are unsupported")
                if info.flag_bits & 1:
                    raise InputError("Encrypted archives are unsupported")
                if info.compress_type not in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED):
                    raise InputError("Archive compression method is unsupported")
                if name in self.members:
                    raise InputError("Archive contains duplicate members")
                if info.file_size > self.MAX_MEMBER:
                    raise InputError("Archive member exceeds size limit")
                if info.file_size > max(1, info.compress_size) * self.MAX_RATIO:
                    raise InputError("Archive expansion ratio exceeds limit")
                total += info.file_size
                if total > self.MAX_TOTAL:
                    raise InputError("Archive expansion exceeds total limit")
                self.members[name] = info
            return self
        except (OSError, zipfile.BadZipFile, NotImplementedError) as exc:
            self.close()
            raise InputError("Cannot read an ordinary unencrypted game archive") from exc
        except Exception:
            self.close()
            raise

    def read(self, name):
        info = self.members.get(name)
        if info is None or info.is_dir():
            raise InputError("Required archive member is missing: " + name)
        try:
            with self.zip.open(info) as stream:
                data = stream.read(self.MAX_MEMBER + 1)
            if len(data) != info.file_size or len(data) > self.MAX_MEMBER:
                raise InputError("Archive member size mismatch")
            return data
        except (OSError, zipfile.BadZipFile, RuntimeError, EOFError, zlib.error) as exc:
            raise InputError("Cannot validate archive member: " + name) from exc

    def fingerprint(self):
        rows = [{"name": name, "bytes": info.file_size, "sha256": digest(self.read(name))}
                for name, info in sorted(self.members.items()) if not info.is_dir()]
        self.assert_unchanged()
        return {"archive_sha256": self.archive_sha256,
                "manifest_sha256": digest(canonical_json(rows)), "members": rows}

    def assert_unchanged(self):
        after = self.path.stat()
        if (after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns) != (
                self.before.st_dev, self.before.st_ino, self.before.st_size, self.before.st_mtime_ns):
            raise InputError("Game input changed during conversion; retry with stable input")

    def close(self):
        if self.zip is not None:
            self.zip.close()
        if self.file is not None:
            self.file.close()

    def __exit__(self, *args):
        self.close()
