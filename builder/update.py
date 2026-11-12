"""Opt-in GitHub updates. Staged local builds; never download a playable ROM.

Trust rests on the GitHub account over HTTPS. Asset digests check integrity;
they are not a separate signature. No tokens, telemetry or automatic installs.
"""
from __future__ import annotations

import contextlib
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import tempfile
import threading
from datetime import datetime, timedelta, timezone
from typing import Callable, Any
from urllib.parse import quote, urlsplit
from urllib.request import Request, build_opener, HTTPRedirectHandler
from urllib.error import URLError, HTTPError
import zipfile

REPOSITORY = "1kuna/balatrom"
REPOSITORY_API = f"https://api.github.com/repos/{REPOSITORY}"
API = REPOSITORY_API + "/releases"
# Pin the numeric release-repository identity before enabling update discovery.
REPOSITORY_ID: int | None = None
UPDATER_VERSION = "1.0.0"
MAX_JSON = 1024 * 1024
MAX_PACKAGE = 64 * 1024 * 1024
MAX_EXPANDED = 256 * 1024 * 1024
SHA = re.compile(r"[0-9a-f]{64}\Z")
VERSION = re.compile(r"v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-rc\.(0|[1-9]\d*))?\Z")


class UpdateError(ValueError):
    """An update cannot safely proceed; the existing version is retained."""


class NoRelease(UpdateError):
    pass


class Cancelled(UpdateError):
    pass


def version_key(value: str) -> tuple[int, int, int, int, int]:
    match = VERSION.fullmatch(value)
    if not match:
        raise UpdateError("Use a version such as 1.2.3 or 1.2.3-rc.1")
    a, b, c, rc = match.groups()
    return int(a), int(b), int(c), int(rc is None), int(rc or 0)


def check_cancel(cancel: threading.Event) -> None:
    if cancel.is_set():
        raise Cancelled("Cancelled. Your current ROM is unchanged.")


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_json(raw: bytes) -> Any:
    def unique(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise UpdateError("Duplicate metadata field")
            result[key] = value
        return result
    try:
        return json.loads(raw, object_pairs_hook=unique)
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise UpdateError("Invalid update metadata") from exc


def trusted_url(url: str, *, redirect: bool = False) -> None:
    part = urlsplit(url)
    hosts = {"github.com", "api.github.com"}
    if redirect:
        hosts |= {"release-assets.githubusercontent.com", "objects.githubusercontent.com"}
    if (part.scheme != "https" or part.hostname not in hosts or
            part.username or part.password or part.port not in (None, 443)):
        raise UpdateError("Update URL is outside the trusted HTTPS source")
    if part.hostname == "github.com":
        if not part.path.startswith(f"/{REPOSITORY}/releases/download/"):
            raise UpdateError("Asset is outside the official release repository")
    if part.hostname == "api.github.com":
        if part.path != f"/repos/{REPOSITORY}" and not part.path.startswith(f"/repos/{REPOSITORY}/releases"):
            raise UpdateError("Metadata is outside the official release repository")


class TrustedRedirect(HTTPRedirectHandler):
    def redirect_request(self, req: Request, fp: Any, code: int,
                         msg: str, headers: Any, newurl: str) -> Request | None:
        trusted_url(newurl, redirect=True)
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def fetch(url: str, limit: int, cancel: threading.Event) -> bytes:
    trusted_url(url)
    request = Request(url, headers={"User-Agent": "BalatROM-Updater/0.1",
                                   "Accept": "application/vnd.github+json"})
    try:
        with build_opener(TrustedRedirect()).open(request, timeout=15) as response:
            trusted_url(response.url, redirect=True)
            result = bytearray()
            while True:
                check_cancel(cancel)
                block = response.read(min(64 * 1024, limit + 1 - len(result)))
                if not block:
                    return bytes(result)
                result.extend(block)
                if len(result) > limit:
                    raise UpdateError("Download exceeds its allowed size")
    except HTTPError as exc:
        if exc.code == 404 and url == API + "/latest":
            raise NoRelease("No stable release is available") from exc
        raise UpdateError("GitHub refused the request; the repository may be private or unavailable") from exc
    except (URLError, TimeoutError, OSError) as exc:
        raise UpdateError("Cannot reach GitHub. Your current version is unchanged; try later.") from exc


def asset_row(release: dict[str, Any], name: str) -> dict[str, Any]:
    rows = [row for row in release.get("assets", []) if row.get("name") == name]
    if len(rows) != 1:
        raise UpdateError("Release needs exactly one " + name)
    row = rows[0]
    expected = row.get("digest", "")
    if (not isinstance(expected, str) or not expected.startswith("sha256:") or
            not SHA.fullmatch(expected[7:])):
        raise UpdateError("GitHub asset SHA-256 is missing")
    url = row.get("browser_download_url", "")
    tag = release["tag_name"]
    if url != f"https://github.com/{REPOSITORY}/releases/download/{quote(tag, safe='')}/{quote(name, safe='')}":
        raise UpdateError("Asset URL does not match its release tag and filename")
    if type(row.get("size")) is not int or not 0 < row["size"] <= MAX_PACKAGE:
        raise UpdateError("Invalid release asset size")
    return row


def verified_bytes(row: dict[str, Any], reader: Callable[..., bytes],
                   cancel: threading.Event, limit: int) -> bytes:
    if row["size"] > limit:
        raise UpdateError("Release asset exceeds its size limit")
    data = reader(row["browser_download_url"], limit, cancel)
    if len(data) != row["size"] or digest(data) != row["digest"][7:]:
        raise UpdateError("Downloaded asset failed its size or SHA-256 check")
    return data


def validate_metadata(value: Any, release: dict[str, Any], channel: str,
                      now: datetime) -> dict[str, Any]:
    try:
        if type(value["schema"]) is not int or value["schema"] != 1 or value["version"] != release["tag_name"].removeprefix("v"):
            raise UpdateError("Release tag and metadata version disagree")
        key = version_key(value["version"])
        if value["channel"] != channel or (channel == "stable" and key[3] != 1):
            raise UpdateError("Release channel mismatch")
        if version_key(value["minimum_updater"]) > version_key(UPDATER_VERSION):
            raise UpdateError("A newer builder app is required; this app cannot apply that update")
        for field in ["archive_sha256", "manifest_sha256"]:
            if not SHA.fullmatch(value["source_install"][field]):
                raise UpdateError("Invalid supported-install fingerprint")
        if not SHA.fullmatch(value["expected_rom_sha256"]):
            raise UpdateError("Invalid expected ROM hash")
        if not isinstance(value["changelog"], str) or len(value["changelog"]) > 20000:
            raise UpdateError("Invalid changelog")
        save = value["save"]
        if (not re.fullmatch(r"[A-Za-z0-9._-]{1,80}", save["format_id"]) or
                type(save["bytes"]) is not int or not 0 <= save["bytes"] <= 1024 * 1024):
            raise UpdateError("Invalid save contract")
        if not isinstance(save["compatible_versions"], list):
            raise UpdateError("Invalid save compatibility list")
        for previous in save["compatible_versions"]:
            version_key(previous)
        if channel == "preview":
            published = datetime.fromisoformat(release["published_at"].replace("Z", "+00:00"))
            expiry = datetime.fromisoformat(value["expires_at"].replace("Z", "+00:00"))
            if not published <= now < expiry <= published + timedelta(days=14):
                raise UpdateError("Preview is stale or has an invalid expiry")
        source = asset_row(release, value["source_asset"])
        if not value["source_asset"].endswith(".zip"):
            raise UpdateError("Source package must be a ZIP")
        return {**value, "source": source}
    except (KeyError, TypeError, AttributeError, ValueError) as exc:
        if isinstance(exc, UpdateError):
            raise
        raise UpdateError("Incomplete or invalid update metadata") from exc


def discover(current: str, channel: str = "stable", *, reader: Callable[..., bytes] = fetch,
             cancel: threading.Event | None = None, now: datetime | None = None,
             repository_id: int | None = REPOSITORY_ID) -> dict[str, Any] | None:
    if channel not in ("stable", "preview"):
        raise UpdateError("Choose stable or preview")
    cancel = cancel or threading.Event()
    now = now or datetime.now(timezone.utc)
    if type(repository_id) is not int or repository_id <= 0:
        raise UpdateError("Release repository identity is not pinned for this app")
    identity = parse_json(reader(REPOSITORY_API, MAX_JSON, cancel))
    if (not isinstance(identity, dict) or identity.get("id") != repository_id or
            identity.get("full_name", "").casefold() != REPOSITORY.casefold()):
        raise UpdateError("Release repository identity changed; updates are blocked")
    endpoint = API + ("/latest" if channel == "stable" else "?per_page=100")
    try:
        raw = parse_json(reader(endpoint, MAX_JSON, cancel))
    except NoRelease:
        return None
    releases = [raw] if channel == "stable" else raw
    if not isinstance(releases, list):
        raise UpdateError("Invalid GitHub release response")
    eligible = []
    for row in releases:
        if not isinstance(row, dict) or row.get("draft") is not False:
            continue
        if row.get("prerelease") is not (channel == "preview"):
            continue
        try:
            if version_key(row["tag_name"]) > version_key(current):
                eligible.append(row)
        except (KeyError, TypeError, UpdateError):
            continue
    eligible.sort(key=lambda row: version_key(row["tag_name"]), reverse=True)
    for release in eligible:
        check_cancel(cancel)
        row = asset_row(release, "balatrom-update.json")
        data = parse_json(verified_bytes(row, reader, cancel, MAX_JSON))
        try:
            return validate_metadata(data, release, channel, now)
        except UpdateError as exc:
            if channel == "preview" and str(exc).startswith("Preview is stale"):
                continue
            raise
    return None


def extract_source(data: bytes, destination: Path, cancel: threading.Event) -> None:
    """Reject traversal, ambiguous Windows paths, symlinks and ZIP expansion abuse."""
    try:
        with zipfile.ZipFile(io.BytesIO(data)) as archive:
            members = archive.infolist()
            if len(members) > 5000 or sum(m.file_size for m in members) > MAX_EXPANDED:
                raise UpdateError("Source package expands beyond allowed limits")
            seen: set[str] = set()
            for member in members:
                check_cancel(cancel)
                # ZipInfo.filename is normalized on Windows and truncated at NUL.
                # Validate the original archive spelling before any filesystem use.
                name = member.orig_filename
                if name != member.filename or '\x00' in name:
                    raise UpdateError("Unsafe or ambiguous source-package member")
                path = PurePosixPath(name)
                parts = path.parts
                mode = member.external_attr >> 16
                if (not parts or path.is_absolute() or "\\" in name or
                        any(p in (".", "..") or p.endswith((".", " ")) or ":" in p
                            or p.split(".")[0].upper() in {"CON", "PRN", "AUX", "NUL", *[f"COM{i}" for i in range(1, 10)], *[f"LPT{i}" for i in range(1, 10)]}
                            for p in name.rstrip("/").split("/")) or
                        stat.S_ISLNK(mode) or (stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR)) or
                        member.flag_bits & 1 or name.rstrip("/").casefold() in seen or
                        member.file_size > max(1024 * 1024, member.compress_size * 200)):
                    raise UpdateError("Unsafe or ambiguous source-package member")
                seen.add(name.rstrip("/").casefold())
                target = destination.joinpath(*parts)
                if member.is_dir():
                    target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    with archive.open(member) as source, target.open("xb") as output:
                        shutil.copyfileobj(source, output, 64 * 1024)
            for required in ("builder/__main__.py", "engine/manifest.json", "compatibility/owned_profiles.json"):
                if not (destination / required).is_file():
                    raise UpdateError("Release package is missing " + required)
    except (zipfile.BadZipFile, RuntimeError, OSError) as exc:
        raise UpdateError("Invalid source package; current version retained") from exc


def atomic_json(path: Path, value: Any) -> None:
    fd, temporary = tempfile.mkstemp(prefix=".state-", dir=path.parent)
    try:
        with os.fdopen(fd, "w") as output:
            json.dump(value, output, indent=2)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


class Updater:
    """Version library. Call apply only after explicit user approval."""

    def __init__(self, root: Path):
        self.root = root.expanduser().absolute()
        if self.root.is_symlink():
            raise UpdateError("Update library must not be a symlink")
        self.root.mkdir(parents=True, exist_ok=True, mode=0o700)
        self.root = self.root.resolve()
        self.state_path = self.root / "state.json"
        if self.state_path.is_symlink():
            raise UpdateError("Update state must not be a symlink")

    def state(self) -> dict[str, Any]:
        if not self.state_path.exists():
            return {"active": None, "previous": None, "highest": "0.0.0", "versions": {}}
        value = parse_json(self.state_path.read_bytes())
        if (not isinstance(value, dict) or not all(k in value for k in ("active", "highest", "versions")) or
                not isinstance(value["highest"], str) or not isinstance(value["versions"], dict)):
            raise UpdateError("Retained library state is invalid; restore a verified record without deleting saves")
        version_key(value["highest"])
        for key in ("active", "previous"):
            version = value.get(key)
            if version is None:
                continue
            if not isinstance(version, str):
                raise UpdateError("Retained version identity is invalid")
            version_key(version)
            record = value["versions"].get(version)
            if not isinstance(record, dict) or not isinstance(record.get("rom_sha256"), str) or not SHA.fullmatch(record["rom_sha256"]):
                raise UpdateError("Retained version integrity record is invalid; original files retained")
        return dict(value, previous=value.get("previous"))

    @contextlib.contextmanager
    def lock(self):
        # Persistent inode plus OS advisory lock: a crash releases ownership.
        # Never unlink a held lock; old empty O_EXCL files are safely reusable.
        path = self.root / ".update.lock"
        if path.is_symlink():
            raise UpdateError("Update lock must not be a symlink")
        fd = os.open(path, os.O_CREAT | os.O_RDWR | getattr(os, "O_NOFOLLOW", 0), 0o600)
        try:
            if not stat.S_ISREG(os.fstat(fd).st_mode):
                raise UpdateError("Update lock must be a regular file")
            if os.name == "posix":
                import fcntl
                try:
                    fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                except BlockingIOError as exc:
                    raise UpdateError("An update is already running") from exc
            else:
                import msvcrt
                if os.fstat(fd).st_size == 0:
                    os.write(fd, b" ")
                os.lseek(fd, 0, os.SEEK_SET)
                try:
                    msvcrt.locking(fd, msvcrt.LK_NBLCK, 1)
                except OSError as exc:
                    raise UpdateError("An update is already running") from exc
            os.lseek(fd, 1, os.SEEK_SET)
            os.write(fd, json.dumps({"pid": os.getpid()}).encode())
            os.ftruncate(fd, os.lseek(fd, 0, os.SEEK_CUR))
            yield
        finally:
            os.close(fd)

    def apply(self, metadata: dict[str, Any], *, approved: bool, game_path: Path,
              fingerprint: Callable[[Path], dict[str, Any]], build: Callable[..., Path],
              reader: Callable[..., bytes] = fetch, cancel: threading.Event | None = None,
              save_path: Path | None = None, new_save: bool = False, previous_rom: Path | None = None,
              progress: Callable[[str], None] = lambda _: None) -> Path:
        if not approved:
            raise UpdateError("Download and local build require your approval")
        cancel = cancel or threading.Event()
        version_key(metadata["version"])
        if self.root.is_relative_to(game_path.resolve().parent):
            raise UpdateError("Keep the update library outside your game installation")
        with self.lock():
            state = self.state()
            if version_key(metadata["version"]) <= version_key(state["highest"]):
                raise UpdateError("This release is not newer; use rollback for a retained version")
            final = self.root / ("v" + metadata["version"])
            if final.exists() or final.is_symlink():
                if final.is_symlink() or not final.is_dir() or metadata["version"] in state["versions"]:
                    raise UpdateError("Existing version output requires manual verification; files retained")
                try:
                    receipt = final / "receipt.json"
                    rom = final / "balatrom.gbc"
                    if receipt.is_symlink() or rom.is_symlink():
                        raise ValueError("Linked interrupted output")
                    record = parse_json(receipt.read_bytes())
                    if (record["version"] != metadata["version"] or
                        record["source_sha256"] != metadata["source"]["digest"][7:] or
                        record["rom_sha256"] != metadata["expected_rom_sha256"] or
                        digest(rom.read_bytes()) != record["rom_sha256"]):
                        raise ValueError("Unverified interrupted output")
                except (OSError, ValueError, KeyError, TypeError) as exc:
                    raise UpdateError("Interrupted version has no matching verified receipt; files retained for manual recovery") from exc
                quarantine = Path(tempfile.mkdtemp(prefix="interrupted-" + metadata["version"] + "-", dir=self.root))
                os.replace(final, quarantine / "retained-version")
                progress("Verified interrupted output retained separately; retrying approved build")
            check_cancel(cancel)
            actual = fingerprint(game_path)
            required = metadata["source_install"]
            if any(actual.get(k) != required[k] for k in ("archive_sha256", "manifest_sha256")):
                raise UpdateError("Your Balatro install is not supported by this update")
            old = state["versions"].get(state["active"])
            compatible = bool(old and old["save"]["format_id"] == metadata["save"]["format_id"] and
                              old["version"] in metadata["save"]["compatible_versions"])
            if save_path and not new_save:
                # The chosen save is not implicitly associated with the active ROM.
                if previous_rom is None:
                    raise UpdateError("Save compatibility requires the original ROM used by this save")
                if previous_rom.is_symlink() or not previous_rom.is_file():
                    raise UpdateError("Choose a regular original ROM for save provenance")
                compatible = digest(previous_rom.read_bytes()) in metadata["save"].get("compatible_rom_sha256", [])
            if save_path and not compatible and not new_save:
                raise UpdateError("Save compatibility is not verified. Keep your original ROM and save.")
            if shutil.disk_usage(self.root).free < 2 * 1024**3:
                raise UpdateError("At least 2GiB free space is required before staging and compiling an owned build")
            stage = Path(tempfile.mkdtemp(prefix="pending-", dir=self.root))
            promoted = False
            try:
                if save_path:
                    if save_path.is_symlink() or not save_path.is_file() or save_path.stat().st_size > 1024 * 1024:
                        raise UpdateError("Choose a regular save file (close the emulator first)")
                    save = save_path.read_bytes()
                    if save != save_path.read_bytes():
                        raise UpdateError("Save changed while reading; close the emulator and retry")
                    if not new_save and metadata["save"].get("header_versions"):
                        if len(save) < 8 or save[:3] != b"NBG" or save[3] not in metadata["save"]["header_versions"]:
                            raise UpdateError("Save header version is not supported")
                        size = int.from_bytes(save[4:6], "little")
                        if size > len(save) - 8:
                            raise UpdateError("Save payload is truncated")
                        checksum = 0xb471
                        for byte in save[8:8+size]:
                            checksum = (((checksum << 1) | (checksum >> 15)) & 0xffff) ^ byte
                        if checksum != int.from_bytes(save[6:8], "little"):
                            raise UpdateError("Save checksum failed")
                    (stage / "save-backup.bin").write_bytes(save)
                    if not new_save and len(save) != metadata["save"]["bytes"]:
                        raise UpdateError("Save size differs from the verified compatibility contract")
                progress("Downloading verified source package")
                package = verified_bytes(metadata["source"], reader, cancel, MAX_PACKAGE)
                check_cancel(cancel)
                source = stage / "source"
                source.mkdir()
                extract_source(package, source, cancel)
                progress("Building privately from your supported installation")
                rom = build(source, game_path, stage / "build", cancel)
                check_cancel(cancel)
                if rom.is_symlink():
                    raise UpdateError("Build returned a symlink instead of a ROM")
                rom = rom.resolve(strict=True)
                if not rom.is_relative_to(stage):
                    raise UpdateError("Build returned a ROM outside its staged output")
                data = rom.read_bytes()
                if len(data) < 0x150 or data[0x143] not in (0x80, 0xC0) or digest(data) != metadata["expected_rom_sha256"]:
                    raise UpdateError("Built ROM failed its expected hash or GBC header check")
                shutil.copyfile(rom, stage / "balatrom.gbc")
                final = self.root / ("v" + metadata["version"])
                if final.exists() or final.is_symlink():
                    raise UpdateError("Version folder already exists; inspect interrupted output before retrying")
                check_cancel(cancel)
                record = {"version": metadata["version"], "rom_sha256": digest(data),
                          "save": metadata["save"], "carried_save": bool(save_path and not new_save),
                          "save_backup": bool(save_path), "save_sha256": digest(save) if save_path else None, "channel": metadata["channel"],
                          "source_sha256": metadata["source"]["digest"][7:],
                          "source_install": required}
                atomic_json(stage / "receipt.json", record)
                os.replace(stage, final)
                promoted = True
                state["previous"] = state["active"]
                state["active"] = metadata["version"]
                state["highest"] = metadata["version"]
                state["versions"][metadata["version"]] = record
                atomic_json(self.state_path, state)
                progress("ROM ready to export; no device has been flashed")
                return final / "balatrom.gbc"
            except BaseException as exc:
                # Preserve interrupted source/logs/save backup; no automatic deletion.
                retained = final if promoted else stage
                try:
                    atomic_json(retained / "FAILURE.json", {"status": "interrupted-owned-build", "active_pointer_changed": None, "state_must_be_verified": True,
                        "save_backup_retained": (retained / "save-backup.bin").exists(), "original_save_untouched": True,
                        "cleanup": "Inspect this retained folder; preserve any save-backup.bin before separately approved removal"})
                except OSError:
                    pass
                message = f" Interrupted files retained at {retained}; preserve save-backup.bin before cleanup."
                if isinstance(exc, UpdateError):
                    wrapped=type(exc)(str(exc))
                    wrapped.detail=getattr(exc,"detail","")+message
                    raise wrapped from exc
                exc.add_note(message)
                raise

    def active_rom(self, state=None) -> Path:
        state = self.state() if state is None else state
        version = state.get("active")
        if not version:
            raise UpdateError("Build a ROM first")
        version_key(version)
        root = self.root / ("v" + version)
        rom = root / "balatrom.gbc"
        if root.is_symlink() or rom.is_symlink() or digest(rom.read_bytes()) != state["versions"][version]["rom_sha256"]:
            raise UpdateError("Retained ROM failed its integrity check")
        return rom

    def export(self, target: Path, *, save_adapter: str | None = None) -> None:
        with self.lock():
            self.export_locked(target, save_adapter=save_adapter)
    def export_locked(self, target: Path, *, save_adapter: str | None = None) -> None:
        if target.suffix.lower() != ".gbc":
            raise UpdateError("Export a .gbc file")
        state = self.state()
        rom = self.active_rom(state)
        save_neighbors = [target.with_suffix(s) for s in (".sav", ".srm", ".ram")]
        save_neighbors += [Path(str(target) + s) for s in (".ram", ".rtc")]
        for neighbor in save_neighbors:
            if neighbor.exists() or neighbor.is_symlink():
                raise UpdateError("Choose a new ROM basename; an existing save uses this name")
        target.parent.resolve(strict=True)
        record = state["versions"][state["active"]]
        if save_adapter and save_adapter != "pyboy":
            raise UpdateError("No supported save export adapter for this loader")
        if save_adapter and not isinstance(record.get("save"), dict):
            raise UpdateError("Retained save metadata is missing or invalid; keep the original ROM/save and restore a verified receipt")
        if save_adapter and save_adapter not in record["save"].get("export_adapters", []):
            raise UpdateError("This release does not support that save export adapter")
        if not isinstance(record.get("carried_save"), bool):
            raise UpdateError("Retained save provenance is missing; keep the original ROM/save and restore a verified receipt before export")
        companion = (Path(str(target) + ".ram") if save_adapter == "pyboy" else target.with_suffix(".save-backup")) if record["carried_save"] else None
        if record["carried_save"]:
            expected_save = record.get("save_sha256")
            if not isinstance(expected_save, str) or not SHA.fullmatch(expected_save):
                raise UpdateError("Retained save backup has no verified integrity record; keep the original save")
            if digest((rom.parent / "save-backup.bin").read_bytes()) != expected_save:
                raise UpdateError("Private save backup failed integrity check")
        # Reserve all outputs exclusively before writing. Clean up only files we created.
        created: list[Path] = []
        try:
            with contextlib.ExitStack() as stack:
                output = stack.enter_context(target.open("xb"))
                created.append(target)
                save_output = None
                if companion:
                    save_output = stack.enter_context(companion.open("xb"))
                    created.append(companion)
                output.write(rom.read_bytes())
                if save_output:
                    save_output.write((rom.parent / "save-backup.bin").read_bytes())
        except BaseException:
            for path in created:
                path.unlink(missing_ok=True)
            raise

    def rollback(self, *, approved: bool) -> str:
        if not approved:
            raise UpdateError("Rollback requires your approval")
        with self.lock():
            state = self.state()
            previous = state["previous"]
            if not previous:
                raise UpdateError("No previous version is retained")
            version_key(previous)
            rom = self.root / ("v" + previous) / "balatrom.gbc"
            if rom.parent.is_symlink() or rom.is_symlink() or digest(rom.read_bytes()) != state["versions"][previous]["rom_sha256"]:
                raise UpdateError("Previous ROM failed its integrity check")
            state["active"], state["previous"] = previous, state["active"]
            atomic_json(self.state_path, state)
            return previous
