"""Scripted end-to-end test of the legacy Qt GUI; not part of the released app.

Uses the real window, controller and worker. File dialogs and confirmations are
answered from a pinned plan file.
"""
import json,os,sys,time,hashlib
from pathlib import Path
from unittest.mock import patch
from builder.__main__ import main

if sys.argv[1:2]==['--update-worker']:
    raise SystemExit(main(sys.argv[1:]))

from PySide6.QtCore import QTimer,Qt
from PySide6.QtTest import QTest
from PySide6.QtWidgets import QApplication,QFileDialog,QMessageBox
from builder.update_gui import Window
plan=json.loads(Path(sys.argv[1]).read_text())
root=Path(plan['output']).resolve();root.mkdir(exist_ok=False)
for key in ('game','lcc','original_rom','save','contract'):
    path=Path(plan[key]);assert path.exists()
for key in ('original_rom','save'):
    assert hashlib.sha256(Path(plan[key]).read_bytes()).hexdigest()==plan[key+'_sha256']
app=QApplication([])
contract=Path(getattr(sys,'_MEIPASS',Path(plan['contract']).parent))/'initial-contract.json'
assert hashlib.sha256(contract.read_bytes()).hexdigest()==plan['contract_sha256']
window=Window(root/'library',contract);window.show();app.processEvents()
started=time.monotonic()
file_results=iter([(plan['lcc'],''),(plan['save'],''),(plan['original_rom'],'')])
def choose(*args,**kwargs):return next(file_results)
with patch.object(QMessageBox,'question',return_value=QMessageBox.StandardButton.Yes),patch.object(QFileDialog,'getExistingDirectory',return_value=plan['game']),patch.object(QFileDialog,'getOpenFileName',side_effect=choose):
    QTest.mouseClick(window.first_build,Qt.MouseButton.LeftButton)
    while window.job is not None:
        app.processEvents();time.sleep(0.02)
    window.grab().save(str(root/'BUILD-RESULT.png'))
    if window.library.state()['active'] is None:
        (root/'ERROR.txt').write_text(window.status.text());raise SystemExit(2)
    target=root/'exported.gbc'
    with patch.object(QFileDialog,'getSaveFileName',return_value=(str(target),'')):
        QTest.mouseClick(window.export,Qt.MouseButton.LeftButton);app.processEvents()
    assert target.exists() and Path(str(target)+'.ram').exists(),window.status.text()
    assert hashlib.sha256(target.read_bytes()).hexdigest()==plan['expected_rom_sha256']
    assert Path(str(target)+'.ram').read_bytes()==Path(plan['save']).read_bytes()
    assert hashlib.sha256(Path(plan['original_rom']).read_bytes()).hexdigest()==plan['original_rom_sha256']
    assert hashlib.sha256(Path(plan['save']).read_bytes()).hexdigest()==plan['save_sha256']
    receipt={'status':'PASS_PACKAGED_GUI_OWNED_BUILD_EXPORT_SAVE_COPY','wall_seconds':time.monotonic()-started,
        'ROM_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'save_sha256':plan['save_sha256'],
        'owned_receipt':str(next((root/'library').rglob('OWNED_INSTALL_RECEIPT.json'))),
        'real_GUI_clicks':True,'dialog_inputs_and_consent':'scripted',
        'originals_unchanged':True,'emulator_continue_verified_in_this_stage':False,'user_saves_accessed':False}
    (root/'GUI-FLOW-RESULT.json').write_text(json.dumps(receipt,indent=2)+'\n')
window.close();app.processEvents()
