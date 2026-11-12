"""Bounded Steam-manifest discovery; never execute Steam or inspect user profiles."""
import os
from pathlib import Path
import re
import sys

from .archive import Archive, InputError

APP_ID = "2379780"
MAX_CONFIG = 1024 * 1024
MAX_LIBRARIES = 64


def keyvalues(path):
    path = Path(path)
    if not path.is_file() or path.is_symlink() or path.stat().st_size > MAX_CONFIG:
        raise InputError("Steam configuration must be a bounded regular file")
    with path.open("rb") as file:
        raw = file.read(MAX_CONFIG + 1)
    if len(raw) > MAX_CONFIG:
        raise InputError("Steam configuration exceeds limit")
    text = raw.decode("utf-8-sig")
    tokens = []
    token = re.compile(r'\s+|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]|[^\s{}"]+')
    offset = 0
    for match in token.finditer(text):
        if match.start() != offset:
            raise InputError("Malformed Steam configuration")
        offset = match.end()
        value = match[0]
        if value.isspace() or value.startswith("//"):
            continue
        if value.startswith('"'):
            value = re.sub(r'\\([\\"])', r'\1', value[1:-1])
        tokens.append(value)
        if len(tokens) > 20000:
            raise InputError("Steam configuration token limit exceeded")
    if offset != len(text):
        raise InputError("Malformed Steam configuration")
    position = 0

    def object_(depth=0):
        nonlocal position
        if depth > 8:
            raise InputError("Steam configuration nesting limit exceeded")
        result = {}
        while position < len(tokens):
            key = tokens[position]
            position += 1
            if key == "}":
                if not depth:
                    raise InputError("Unexpected Steam configuration brace")
                return result
            if key == "{" or position == len(tokens):
                raise InputError("Malformed Steam configuration pair")
            value = tokens[position]
            position += 1
            key = key.casefold()
            if key in result or value == "}":
                raise InputError("Duplicate or invalid Steam configuration key")
            result[key] = object_(depth + 1) if value == "{" else value
        if depth:
            raise InputError("Unclosed Steam configuration object")
        return result

    return object_()


def conventional_roots(platform=None, home=None):
    platform = platform or sys.platform
    home = Path(home or Path.home())
    if platform == "darwin":
        return [home / "Library/Application Support/Steam"]
    if platform.startswith("linux"):
        return [home / ".local/share/Steam", home / ".steam/steam", home / ".steam/root"]
    if platform == "win32":
        roots = []
        try:
            import winreg
            for hive, name, value in [(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam", "SteamPath"),
                                      (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath")]:
                try:
                    with winreg.OpenKey(hive, name) as key:
                        roots.append(Path(winreg.QueryValueEx(key, value)[0]))
                except OSError:
                    pass
        except ImportError:
            pass
        for name in ["ProgramFiles(x86)", "ProgramFiles"]:
            if os.environ.get(name):
                roots.append(Path(os.environ[name]) / "Steam")
        return roots
    return []


def discover(steam_roots=None, platform=None, home=None):
    roots = list(steam_roots if steam_roots is not None else conventional_roots(platform, home))
    if len(roots) > MAX_LIBRARIES:
        raise InputError("Too many explicit Steam roots")
    libraries, warnings = {}, []
    for root in roots:
        root = Path(root).expanduser().resolve()
        libraries[str(root)] = root
        config = root / "steamapps/libraryfolders.vdf"
        if not config.exists():
            config = root / "config/libraryfolders.vdf"
        if not config.exists():
            continue
        try:
            entries = keyvalues(config).get("libraryfolders", {})
            if not isinstance(entries, dict):
                raise InputError("Malformed Steam library list")
            for key, entry in entries.items():
                if not key.isdecimal():
                    continue
                name = entry.get("path") if isinstance(entry, dict) else entry
                if not isinstance(name, str) or not Path(name).is_absolute():
                    raise InputError("Steam library path must be absolute")
                path = Path(name).resolve()
                libraries[str(path)] = path
                if len(libraries) > MAX_LIBRARIES:
                    raise InputError("Steam library limit exceeded")
        except (InputError, OSError, UnicodeError) as exc:
            warnings.append({"manifest": str(config), "reason": str(exc)})
    if len(libraries) > MAX_LIBRARIES:
        raise InputError("Steam library limit exceeded")
    candidates = {}
    for library in libraries.values():
        manifest = library / "steamapps" / ("appmanifest_" + APP_ID + ".acf")
        if not manifest.exists():
            continue
        try:
            state = keyvalues(manifest).get("appstate", {})
            if not isinstance(state, dict) or state.get("appid") != APP_ID:
                raise InputError("Steam app manifest does not identify Balatro")
            name = state.get("installdir", "")
            if not isinstance(name, str) or not name or name in (".", "..") or any(c in name for c in "/\\:\x00\n\r"):
                raise InputError("Unsafe Steam installation directory")
            if not int(state.get("stateflags", "0")) & 4:
                raise InputError("Steam installation is incomplete; use --game-path for an explicit archive")
            install = library / "steamapps/common" / name
            for filename in ["Balatro.app", "Balatro.love", "balatro.love", "Balatro.exe", "balatro.exe"]:
                path = install / filename
                if path.is_symlink() or not path.exists():
                    continue
                if path.suffix.lower() == ".app" and not path.is_dir():
                    continue
                if path.suffix.lower() != ".app" and not path.is_file():
                    continue
                candidates[str(path)] = {"game_path": str(path), "steam_buildid": state.get("buildid"),
                                         "source_manifest": str(manifest)}
        except (InputError, OSError, UnicodeError, ValueError) as exc:
            warnings.append({"manifest": str(manifest), "reason": str(exc)})
    return {"candidates": list(candidates.values()), "warnings": warnings,
            "fallback": "Use --game-path /path/to/Balatro.app or a regular .love/.exe archive",
            "game_launched": False, "network_access": False}


def select_supported(report, archive_sha, manifest_sha):
    matches = {}
    for candidate in report["candidates"]:
        try:
            with Archive(candidate["game_path"]) as archive:
                fingerprint = archive.fingerprint()
                archive.assert_unchanged()
                if fingerprint["archive_sha256"] == archive_sha and fingerprint["manifest_sha256"] == manifest_sha:
                    matches[str(archive.path)] = candidate["game_path"]
        except (InputError, OSError):
            continue
    if len(matches) == 1:
        return next(iter(matches.values()))
    if matches:
        raise InputError("Multiple matching installations; choose one with --game-path")
    if report["candidates"]:
        raise InputError("Discovered installation does not match the tested Steam1.0.1o-FULL/build17459173 hashes; use --game-path to select another")
    raise InputError("No readable Steam Balatro installation found; use --game-path for a non-Steam or custom installation")
