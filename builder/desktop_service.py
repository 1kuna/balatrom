"""Private stdio service for the desktop shell; never opens a listening socket.

The shell supplies only explicit user-selected paths. Owned-install admission,
source verification, tool acquisition and atomic export remain in the backend.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import sys
import threading

from . import discovery, local_package, update
from .archive import InputError
from .input_identity import matches
from .managed_sdk import compiler_session
from .update_runtime import fingerprint, build_adapter
from .worker_budget import snapshot


class Service:
    def __init__(self, root, contract, emit):
        self.library = update.Updater(Path(root))
        self.contract = Path(contract)
        self.emit = emit
        self.game = None
        self.cancel = threading.Event()
        self.thread = None
        self.guard = threading.Lock()
        self.busy = False
        self.terminal = None

    def send(self, request, kind, **fields):
        value = dict(id=request, type=kind, **fields)
        if kind in ('discovery', 'complete', 'exported', 'cancelled', 'error'):
            self.terminal = value
        else:
            self.emit(value)

    def progress(self, request, text):
        import re
        match = re.search(r"(\d+)/198", text)
        stage = "compile" if match else "verify" if "verif" in text.lower() else "tools" if "tool" in text.lower() else "prepare"
        self.send(request, "progress", stage=stage, message=text,
                  completed=int(match[1]) if match else None, total=198 if match else None)

    def native_progress(self, request, event):
        self.send(request,'progress',stage=event['stage'],message=event['message'],completed=event.get('completed'),total=event.get('total'),elapsedSeconds=event['elapsedSeconds'],activeWorkers=len(event.get('activeUnits',[])))

    def discover(self, request):
        metadata, _ = local_package.load(self.contract)
        pins = metadata['source_install']
        if sys.platform == 'win32':
            from .windows_discovery import discover_windows
            result = discover_windows(archive_sha=pins['archive_sha256'], manifest_sha=pins['manifest_sha256'])
            path, status = result['selected_game'], result['status']
        else:
            from .install_discovery import scan_and_classify
            result=scan_and_classify(discovery.discover,pins['archive_sha256'],pins['manifest_sha256'])
            path,status=result['selected_game'],result['status']
        self.game = Path(path) if path else None
        self.send(request, 'discovery', status=status, path=str(self.game) if self.game else None)

    def operation(self, message):
        request, command = message['id'], message['command']
        args = message.get('args', {})
        try:
            if command == 'discover':
                self.discover(request)
            elif command == 'locate':
                metadata, _ = local_package.load(self.contract)
                path = Path(args['path']).expanduser().resolve(strict=True)
                if not matches(fingerprint(path), metadata['source_install']):
                    raise update.UpdateError('This Balatro edition is not supported. Existing files are unchanged.')
                self.game = path
                self.send(request, 'discovery', status='supported', path=str(path))
            elif command == 'build':
                if self.game is None:
                    raise update.UpdateError('Install Balatro in Steam, or locate your owned installation first.')
                metadata, reader = local_package.load(self.contract)
                self.progress(request,'Verifying Balatro')
                actual_input = fingerprint(self.game)
                if not matches(actual_input, metadata['source_install']):
                    raise update.UpdateError('The selected Balatro installation changed or is unsupported.')
                # Preserve the real Windows fused-file identity in the build receipt.
                metadata = dict(metadata, source_install=actual_input)
                state = self.library.state()
                if state.get('active') == metadata['version']:
                    rom = self.library.active_rom(state)
                    if update.digest(rom.read_bytes()) != metadata['expected_rom_sha256']:
                        raise update.UpdateError('The retained ROM failed integrity verification. Existing files preserved.')
                    self.send(request, 'complete', version=metadata['version'], romSha256=metadata['expected_rom_sha256'], reused=True)
                    return
                progress = lambda text: self.progress(request, text)
                policy = snapshot()
                update.atomic_json(self.library.root.parent / 'WORKER-POLICY.json', policy)
                with compiler_session(self.library.root.parent / 'build-tools', approved=True,
                                      cancel=self.cancel, progress=progress) as compiler:
                    rom = self.library.apply(metadata, approved=True, game_path=self.game,
                        fingerprint=fingerprint, build=build_adapter(compiler, progress=progress, workers=policy['workers'],
                            cache_dir=self.library.root.parent / "compiler-cache",
                            event_progress=lambda event:self.native_progress(request,event)),
                        reader=reader, cancel=self.cancel, progress=progress)
                self.send(request, 'complete', version=metadata['version'], romSha256=update.digest(rom.read_bytes()), reused=False)
            elif command == 'export':
                destination = Path(args['destination']).expanduser().absolute()
                self.library.export(destination)
                self.send(request, 'exported', path=str(destination))
            else:
                raise update.UpdateError('Unsupported desktop command.')
        except update.Cancelled as exc:
            self.send(request, 'cancelled', message=str(exc))
        except Exception as exc:
            if self.cancel.is_set():
                self.send(request, 'cancelled', message='Canceled. Existing game files and saves were preserved.')
            else:
                self.send(request, 'error', message=str(exc), detail=getattr(exc, 'detail', ''))

    def accept(self, message):
        if not isinstance(message, dict) or not isinstance(message.get('id'), (str, int)):
            raise ValueError('Invalid request')
        with self.guard:
            if message.get('command') == 'status':
                self.emit(dict(id=message['id'],type='state',busy=self.busy,terminal=self.terminal))
                return
            if message.get('command') == 'cancel':
                self.cancel.set()
                self.send(message['id'], 'progress', stage='cancel', message='Canceling safely…')
                return
            if self.busy:
                self.emit(dict(id=message['id'], type='error', message='Wait for the current operation to finish.'))
                return
            self.cancel.clear()
            self.busy = True
            self.terminal = None
            def execute():
                try:
                    self.operation(message)
                except BaseException as exc:
                    # A worker-thread failure must never leave the UI without a terminal result.
                    self.terminal=dict(id=message['id'],type='error',message='The build service stopped.',detail=str(exc))
                finally:
                    terminal = self.terminal
                    self.busy = False
                    if terminal is not None:
                        self.emit(terminal)
            self.thread = threading.Thread(target=execute, daemon=False)
            self.thread.start()

    def close(self):
        self.cancel.set()
        if self.thread:
            self.thread.join()


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--contract', type=Path)
    args = parser.parse_args(argv)
    contract = args.contract or Path(getattr(sys, '_MEIPASS', Path(__file__).resolve().parents[1])) / 'initial-contract.json'
    lock = threading.Lock()
    def emit(value):
        with lock:
            print(json.dumps(value, ensure_ascii=True), flush=True)
    service = Service(args.root, contract, emit)
    try:
        while True:
            line = sys.stdin.buffer.readline(65537)
            if not line:
                break
            if len(line) > 65536:
                emit(dict(type='error', message='Request exceeds size limit.'))
                break
            try:
                service.accept(json.loads(line))
            except (ValueError, TypeError) as exc:
                emit(dict(type='error', message=str(exc)))
    finally:
        service.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
