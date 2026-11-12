"""Windows Steam discovery/report adapter; reuses the bounded shared parser/scanner.

No global disk search, process execution, registry write or game-profile/save read.
"""
from __future__ import annotations
from collections.abc import Callable, Iterable
import os
import ntpath
from pathlib import Path
import sys

from .archive import Archive, InputError
from .discovery import conventional_roots, discover, APP_ID, MAX_LIBRARIES
from .owned_install import ARCHIVE_SHA, MANIFEST_SHA

from .install_discovery import MAX_FINGERPRINT_CANDIDATES, scan_and_classify


def windows_steam_roots(home: Path | None = None) -> list[Path]:
    """Read existing Steam registry hints and bounded conventional per-user paths.

    Registry handling remains in discovery.conventional_roots: HKCU SteamPath and
    HKLM WOW6432Node InstallPath, with read-only OpenKey. No profile files are read.
    """
    home = Path(home or Path.home())
    roots = list(conventional_roots(platform="win32", home=home))
    local = Path(os.environ.get("LOCALAPPDATA", home / "AppData" / "Local"))
    roots.extend((local / "Steam", local / "Programs" / "Steam"))
    result: dict[str, Path] = {}
    for root in roots:
        root = Path(ntpath.expandvars(str(root))).expanduser()
        if not root.is_absolute():
            continue  # Never resolve an untrusted registry hint relative to cwd.
        result.setdefault(str(root).casefold(), root)
    return list(result.values())


def discover_windows(
    *,
    archive_sha: str = ARCHIVE_SHA,
    manifest_sha: str = MANIFEST_SHA,
    roots_provider: Callable[[], Iterable[Path]] | None = None,
    archive_factory: Callable = Archive,
) -> dict:
    """Return a UI-ready classification, selecting only one exact known fingerprint.

    roots_provider/archive_factory exist for declared portable fake fixtures; their
    tests do not establish Windows registry/filesystem/runtime qualification.
    """
    if roots_provider is None and sys.platform != "win32":
        raise InputError("Windows discovery requires Windows or explicit fake roots for a portable test")
    def scan():
        roots=list((roots_provider or windows_steam_roots)())
        if len(roots)>MAX_LIBRARIES:
            raise InputError("Too many Windows Steam roots")
        return discover(steam_roots=roots,platform="win32")
    return scan_and_classify(scan,archive_sha,manifest_sha,case_insensitive=True,archive_factory=archive_factory)
