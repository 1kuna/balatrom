"""Legacy Qt frontend, retained for compatibility tests.

The desktop application uses the Electron frontend in desktop/.
"""
from __future__ import annotations
import os
from pathlib import Path
import sys
import threading
from PySide6.QtCore import QThread, Signal, Qt, QTimer
from PySide6.QtWidgets import (QApplication, QComboBox, QFileDialog, QLabel,
                              QMainWindow, QMessageBox, QPushButton, QTextEdit,
                              QVBoxLayout, QWidget)
from . import update
from .update_runtime import fingerprint, build_adapter


class Job(QThread):
    done = Signal(object)
    failed = Signal(str)
    progress = Signal(str)
    def __init__(self, operation):
        super().__init__()
        self.operation = operation
        self.cancel = threading.Event()
    def run(self):
        try:
            self.done.emit(self.operation(self.cancel, self.progress.emit))
        except Exception as exc:
            self.failed.emit(str(exc))


class Window(QMainWindow):
    def __init__(self, root: Path, initial_contract: Path | None = None, *, autodiscover=True):
        super().__init__()
        self.setWindowTitle("BalatROM")
        self.resize(540, 460)
        self.library = update.Updater(root)
        self.offer = None
        self.initial_contract = initial_contract
        self.local_reader = None
        self.job = None
        self.choosing = False
        self.game_path = None
        self.import_existing_save = False
        self.export_after_build = False
        self.advanced = False
        widget = QWidget()
        layout = QVBoxLayout(widget)
        self.status = QLabel("Looking for your Balatro installation…")
        self.status.setWordWrap(True)
        self.status.setMinimumHeight(90)
        self.status.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse | Qt.TextInteractionFlag.TextSelectableByKeyboard)
        self.identity = QLabel()
        self.identity.setWordWrap(True)
        self.channel = QComboBox()
        self.channel.addItems(["stable"])
        self.channel.hide()
        self.channel.setAccessibleName("Update channel")
        self.channel.currentTextChanged.connect(self.reset_offer)
        self.notes = QTextEdit()
        self.notes.setReadOnly(True)
        self.notes.setMinimumHeight(54)
        self.notes.setMaximumHeight(90)
        self.notes.setAcceptRichText(False)
        self.notes.setAccessibleName("Release notes")
        self.first_build = QPushButton("Build ROM")
        self.first_build.setEnabled(initial_contract is not None)
        self.first_build.clicked.connect(self.build_initial)
        self.locate = QPushButton("Locate Balatro…")
        self.locate.clicked.connect(self.choose_game)
        self.locate.hide()
        self.rescan = QPushButton("Find Balatro again")
        self.rescan.clicked.connect(self.find_installation)
        self.rescan.hide()
        self.import_run = QPushButton("Build and keep an existing run…")
        self.import_run.clicked.connect(lambda: self.build_initial(import_save=True))
        self.import_run.setEnabled(False)
        self.import_run.setToolTip("Optional. First builds can start without importing any save.")
        self.more = QPushButton("Existing run options…")
        self.more.setFlat(True)
        self.more.clicked.connect(self.toggle_advanced)
        self.check = QPushButton("Update")
        self.check.clicked.connect(self.check_update)
        self.apply = QPushButton("Download and build")
        self.apply.setEnabled(False)
        self.apply.clicked.connect(self.apply_update)
        self.export = QPushButton("Save .gbc file…")
        self.export.clicked.connect(self.export_rom)
        self.export.setEnabled(False)
        self.back = QPushButton("Go back to the previous version")
        self.back.clicked.connect(self.rollback)
        self.cancel = QPushButton("Cancel")
        self.cancel.setEnabled(False)
        self.cancel.clicked.connect(self.cancel_job)
        for control in (self.identity, self.status, self.notes, self.first_build, self.check, self.apply,
                        self.export, self.back, self.more, self.import_run, self.locate, self.rescan, self.cancel):
            layout.addWidget(control)
        self.setCentralWidget(widget)
        self.refresh_identity()
        if autodiscover:
            QTimer.singleShot(0, self.find_installation)
    def toggle_advanced(self):
        self.advanced = not self.advanced
        self.refresh_identity()

    def show_error(self, message):
        self.notes.setPlainText(message)
        self.notes.setMaximumHeight(140)
        self.status.setText("The operation stopped. Existing game files and saves were preserved. Details are below; you can retry Build after a connection/setup error.")

    def find_installation(self):
        if self.job is not None:
            return
        self.status.setText("Looking for Balatro in your Steam libraries…")
        self.start(self.discover_installation, self.show_installation)

    def discover_installation(self, cancel, progress):
        import json
        from . import discovery
        from .update import check_cancel
        from .archive import InputError
        check_cancel(cancel)
        if not self.initial_contract:
            return {"path": None, "status": "no_package"}
        pins = json.loads(self.initial_contract.read_text())["source_install"]
        if sys.platform == "win32":
            from .windows_discovery import discover_windows
            result = discover_windows(archive_sha=pins["archive_sha256"], manifest_sha=pins["manifest_sha256"])
            return {"path": result["selected_game"], "status": result["status"]}
        report = discovery.discover()
        check_cancel(cancel)
        try:
            path = discovery.select_supported(report, pins["archive_sha256"], pins["manifest_sha256"])
            return {"path": path, "status": "supported"}
        except InputError:
            return {"path": None, "status": "found_unqualified" if report["candidates"] else "not_found"}

    def show_installation(self, result):
        self.game_path = Path(result["path"]) if result.get("path") else None
        found = self.game_path is not None
        self.locate.setVisible(not found)
        self.rescan.setVisible(not found)
        if found:
            self.status.setText("Balatro found. Build ROM prepares verified free build tools automatically (about 6–9 MB from the official project). No game files are uploaded. Choose where to save the ROM when it finishes.")
            self.notes.setPlainText(str(self.game_path))
        elif result.get("status") == "no_package":
            self.status.setText("The app's bundled build package is missing. Existing ROMs and saves are preserved; report this app installation problem.")
        elif result.get("status") == "found_unqualified":
            self.status.setText("Balatro was found, but this installed edition is not supported yet. Existing files are unchanged. Locate another owned install or report your version.")
        elif result.get("status") in ("discovery_incomplete", "multiple_supported"):
            self.status.setText("Balatro's location needs checking. Choose Locate Balatro, or try Find Balatro again. Existing files are unchanged.")
        else:
            self.status.setText("Install Balatro in Steam, then choose Find Balatro again. If it is already installed elsewhere, choose Locate Balatro.")
        self.refresh_identity()

    def choose_game(self):
        if self.job is not None:
            return
        game = QFileDialog.getExistingDirectory(self, "Locate your Balatro.app", options=QFileDialog.Option.DontUseNativeDialog | QFileDialog.Option.ShowDirsOnly) if sys.platform == "darwin" else ""
        if not game:
            game, _ = QFileDialog.getOpenFileName(self, "Locate your owned Balatro installation")
        if not game:
            return
        try:
            import json
            pins = json.loads(self.initial_contract.read_text())["source_install"]
            if fingerprint(Path(game)) != pins:
                raise update.UpdateError("This Balatro edition is not supported yet. No game files were changed.")
            self.show_installation({"path": game, "status": "supported"})
        except (OSError, ValueError) as exc:
            self.status.setText(str(exc))

    def refresh_identity(self):
        try:
            state = self.library.state()
            active = state.get("active")
            update.version_key(state["highest"])
            if active:
                update.version_key(active)
                record = state["versions"][active]
                rom_hash = record["rom_sha256"]
                if not isinstance(rom_hash, str) or not update.SHA.fullmatch(rom_hash):
                    raise ValueError("Invalid retained ROM hash")
                self.identity.setText(f"Active version {active} · ROM {rom_hash[:12]}")
            else:
                self.identity.setText("No ROM built yet")
        except (OSError, ValueError, KeyError, TypeError, update.UpdateError):
            self.identity.setText("Retained library state is invalid; existing files are preserved.")
            self.status.setText("Restore a verified library record before building or exporting.")
            for button in (self.first_build, self.check, self.apply, self.export, self.back):
                button.setEnabled(False)
            return
        idle = self.job is None and not self.choosing
        self.check.setVisible(update.REPOSITORY_ID is not None)
        self.apply.setVisible(update.REPOSITORY_ID is not None and self.offer is not None)
        self.export.setVisible(bool(active))
        self.back.setVisible(self.advanced and bool(state.get("previous")))
        self.import_run.setVisible(self.advanced)
        self.cancel.setVisible(not idle)
        self.check.setEnabled(update.REPOSITORY_ID is not None and idle)
        self.check.setText("Update" if update.REPOSITORY_ID is not None else "Updates unavailable in this build")
        self.export.setEnabled(bool(active) and idle)
        self.back.setEnabled(bool(state.get("previous")) and idle)
        if self.initial_contract:
            try:
                import json
                data = json.loads(self.initial_contract.read_text())
                self.identity.setText(self.identity.text() + f"\nBundled version {data['version']} · ROM {data['expected_rom_sha256'][:12]}")
                self.first_build.setEnabled(idle and self.game_path is not None and update.version_key(data['version']) > update.version_key(state['highest']))
                self.import_run.setEnabled(self.first_build.isEnabled())
            except (OSError, ValueError, KeyError, TypeError):
                self.first_build.setEnabled(False)
    def build_complete(self, rom):
        self.reset_offer()
        self.refresh_identity()
        self.status.setText("Your ROM is ready. Choose where to save it.")
        self.export_after_build = True
    def reset_offer(self, *_):
        self.offer = None
        self.apply.setEnabled(False)
        self.notes.clear()
    def start(self, operation, complete):
        if self.job is not None:
            return
        self.first_build.setEnabled(False)
        self.check.setEnabled(False)
        self.apply.setEnabled(False)
        self.channel.setEnabled(False)
        self.export.setEnabled(False)
        self.back.setEnabled(False)
        self.cancel.setEnabled(True)
        self.cancel.show()
        for button in (self.locate, self.rescan, self.import_run):
            button.setEnabled(False)
        self.job = Job(operation)
        self.job.progress.connect(self.status.setText)
        self.job.done.connect(complete)
        self.job.failed.connect(self.show_error)
        self.job.finished.connect(self.finished)
        self.job.start()
    def build_initial(self, _checked=False, *, import_save=False):
        if self.job is not None or self.choosing:
            return
        self.import_existing_save = import_save
        if self.game_path is None:
            self.status.setText("Install Balatro in Steam, or use Locate Balatro if it is already installed elsewhere.")
            self.locate.show()
            return
        from .local_package import load
        try:
            self.offer, self.local_reader = load(self.initial_contract)
            self.apply_update()
        except (OSError, ValueError) as exc:
            self.status.setText(str(exc))
    def finished(self):
        self.first_build.setEnabled(self.initial_contract is not None)
        for button in (self.check, self.channel, self.export, self.back):
            button.setEnabled(True)
        self.cancel.setEnabled(False)
        self.apply.setEnabled(self.offer is not None)
        self.job.deleteLater()
        self.job = None
        self.refresh_identity()
        for button in (self.locate, self.rescan):
            button.setEnabled(True)
        if self.export_after_build:
            self.export_after_build = False
            QTimer.singleShot(0, self.export_rom)
    def cancel_job(self):
        if self.job:
            self.job.cancel.set()
            self.status.setText("Canceling. Waiting for the current operation to stop…")
    def check_update(self):
        self.reset_offer()
        self.local_reader = None
        self.status.setText("Checking for updates…")
        try:
            current = self.library.state()["highest"]
        except (OSError, update.UpdateError) as exc:
            self.status.setText(str(exc))
            self.refresh_identity()
            return
        channel = self.channel.currentText()
        self.start(lambda cancel, progress: update.discover(current, channel, cancel=cancel), self.show_offer)
    def show_offer(self, offer):
        self.offer = offer
        if offer:
            self.status.setText(f"Version {offer['version']} is available")
            self.notes.setPlainText(offer["changelog"])
        else:
            self.status.setText("No update is available yet.")
    def apply_update(self):
        if self.job is not None or self.choosing or not self.offer:
            return
        self.choosing = True
        for button in (self.first_build, self.check, self.apply):
            button.setEnabled(False)
        try:
            self.choose_and_apply()
        finally:
            self.choosing = False
            self.refresh_identity()
            self.apply.setEnabled(self.job is None and self.offer is not None)
    def choose_and_apply(self):
        if self.job is not None or not self.offer:
            return
        game = self.game_path
        if game is None:
            self.status.setText("Install Balatro or choose its existing location first.")
            self.locate.show()
            return
        metadata = self.offer
        selected_save = None
        previous_rom = None
        has_save = QMessageBox.StandardButton.Yes if self.import_existing_save else QMessageBox.StandardButton.No
        if has_save == QMessageBox.StandardButton.Yes:
            name, _ = QFileDialog.getOpenFileName(self, "Choose your existing save for backup and compatibility checks")
            if not name:
                return
            selected_save = Path(name)
            original, _ = QFileDialog.getOpenFileName(self, "Choose the original ROM used by this save", filter="Game Boy Color (*.gbc)")
            if not original:
                return
            previous_rom = Path(original)
        # Existing saves need the exact old ROM and a known-compatible version.
        # No migration, emulator save installation, or hardware write is performed here.
        reader = self.local_reader or update.fetch
        def operation(cancel, progress):
            from .managed_sdk import compiler_session
            with compiler_session(self.library.root.parent / "build-tools", approved=True, cancel=cancel, progress=progress) as lcc:
                return self.library.apply(metadata, approved=True,
                    game_path=Path(game), fingerprint=fingerprint, build=build_adapter(lcc, progress=progress),
                    cancel=cancel, progress=progress, save_path=selected_save, new_save=False, reader=reader, previous_rom=previous_rom)
        self.start(operation, self.build_complete)
    def export_rom(self):
        try:
            self.library.active_rom()
            state = self.library.state()
            version = state["active"]
            record = state["versions"][version]
            if not isinstance(record.get("carried_save"), bool):
                raise update.UpdateError("Retained save provenance is missing; preserve original files and restore a verified receipt before export")
            path, _ = QFileDialog.getSaveFileName(self, "Save ROM", f"balatrom-{version}.gbc", "Game Boy Color (*.gbc)")
            if not path:
                return
            adapter = None
            if record["carried_save"]:
                if QMessageBox.question(self, "Keep your run", "Export a separate save copy for PyBoy alongside the new ROM? Your original is retained.") == QMessageBox.StandardButton.Yes:
                    adapter = "pyboy"
            self.library.export(Path(path), save_adapter=adapter)
            self.status.setText("Saved. Open the .gbc in a Game Boy Color emulator.")
        except (OSError, update.UpdateError) as exc:
            self.status.setText(str(exc))
    def rollback(self):
        if QMessageBox.question(self, "Go back", "Use the retained previous ROM? Your live save will not be changed.") != QMessageBox.StandardButton.Yes:
            return
        try:
            version = self.library.rollback(approved=True)
            self.refresh_identity()
            self.status.setText(f"Version {version} selected. Export it to a new .gbc file.")
        except (OSError, update.UpdateError) as exc:
            self.status.setText(str(exc))
    def closeEvent(self, event):
        if self.job:
            self.cancel_job()
            event.ignore()
            return
        super().closeEvent(event)


def main():
    app = QApplication(sys.argv)
    base = Path(os.environ.get("LOCALAPPDATA", Path.home() / "Library" / "Application Support"))
    package_root = Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parents[1]))
    initial = package_root / "initial-contract.json"
    try:
        window = Window(base / "BalatROM" / "versions", initial if initial.exists() else None)
    except (OSError, update.UpdateError) as exc:
        QMessageBox.critical(None, "Cannot open local library",
                             "The local version library could not be opened. "
                             "Existing ROMs and saves were preserved.\n\n" + str(exc))
        return 2
    window.show()
    return app.exec()
