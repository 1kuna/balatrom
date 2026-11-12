"""Run the build worker for the desktop app."""
from __future__ import annotations
import os
from pathlib import Path
import subprocess
import sys
import threading
import re
import json
import time

FAILED_SHUTDOWN_GRACE_SECONDS=5.0
from .archive import Archive
from .update import UpdateError, Cancelled, check_cancel


def fingerprint(path: Path):
    with Archive(str(path)) as archive:
        result = archive.fingerprint()
        return {k: result[k] for k in ("archive_sha256", "manifest_sha256")}


def latest_status(tail):
    for line in reversed(tail.splitlines()):
        if not line.startswith(b'BUILD_STATUS '):continue
        try:
            event=json.loads(line[len(b'BUILD_STATUS '):])
            if event.get('schema')!=1 or event.get('stage') not in ('prepare','verify','compile','link'):continue
            if not isinstance(event.get('message'),str) or len(event['message'])>200:continue
            if event.get('total') is not None and (event['total']!=198 or type(event.get('completed')) is not int or not 0<=event['completed']<=198):continue
            if not isinstance(event.get('elapsedSeconds'),(float,int)) or event['elapsedSeconds']<0:continue
            if not isinstance(event.get('activeUnits'),list) or len(event['activeUnits'])>4:continue
            return event
        except (ValueError,TypeError,KeyError):continue
    return None


def build_adapter(lcc: Path, progress=lambda message: None, *, workers=2, cache_dir=None, event_progress=None):
    def build(source: Path, game: Path, output: Path, cancel: threading.Event) -> Path:
        if os.name not in ("posix", "nt"):
            raise UpdateError("This operating system is unsupported")
        check_cancel(cancel)
        marker = output.parent / "cancel-build"
        args = ["--source", str(source), "--game", str(game), "--output", str(output),
                "--lcc", str(lcc), "--cancel-file", str(marker), "--workers", str(workers)]
        if cache_dir is not None:
            args += ["--object-cache", str(cache_dir)]
        if getattr(sys, "frozen", False):
            command = [sys.executable, "--update-worker", *args]
        else:
            command = [sys.executable, "-I", "-B", str(Path(__file__).with_name("update_worker.py")), *args]
        # Capture logs privately; filenames/retail data are never sent to GitHub.
        with (output.parent / "build-worker.log").open("wb") as log:
            process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=log, stderr=log)
            last_progress = None
            failed_at = None
            def drain():
                nonlocal last_progress, failed_at
                try:
                    with (output.parent / "build-worker.log").open("rb") as reader:
                        reader.seek(0, 2)
                        reader.seek(max(0, reader.tell() - 16384))
                        tail=reader.read(16384)
                except OSError:
                    return
                status=latest_status(tail)
                if status is not None and status.get('outcome')=='failed' and failed_at is None:failed_at=time.monotonic()
                if status is not None:
                    key=json.dumps(status,sort_keys=True)
                    if key!=last_progress:
                        last_progress=key
                        if event_progress:event_progress(status)
                        else:progress(status['message'])
                else:
                    matches = re.findall(rb"(?:Fresh compiled|Compiled) (\d+)/198", tail)
                    if matches and matches[-1] != last_progress:
                        last_progress=matches[-1]
                        progress("Creating your ROM: " + last_progress.decode() + "/198 parts")
            while True:
                drain()
                if cancel.is_set():
                    marker.touch(exist_ok=True)
                try:
                    code = process.wait(timeout=0.2)
                    drain()  # Early exits must not lose the final status/traceback.
                    break
                except subprocess.TimeoutExpired:
                    if failed_at is not None and time.monotonic()-failed_at>=FAILED_SHUTDOWN_GRACE_SECONDS:
                        # The worker reports failure after build cleanup unwinds.
                        # Reap only this owned supervisor before allowing a safe retry.
                        process.terminate()
                        try:code=process.wait(timeout=2)
                        except subprocess.TimeoutExpired:
                            process.kill();code=process.wait(timeout=2)
                        drain()
                        break
                    continue
        if cancel.is_set():
            raise Cancelled("Canceled. The current version is unchanged.")
        if code:
            error=UpdateError("The ROM build stopped. Open Details for the diagnostic log.")
            error.detail=(output.parent / "build-worker.log").read_text(encoding="utf8",errors="replace")[-12000:]+"\nPrivate log: "+str(output.parent / "build-worker.log")
            raise error
        return output / "linked" / "balatrom.gbc"
    return build
