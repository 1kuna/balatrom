"""Small actual Qt regressions; no owned files, compiler, network or GUI jobs."""
import os,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
os.environ['QT_QPA_PLATFORM']='offscreen'
try:
    from PySide6.QtWidgets import QApplication,QMessageBox,QFileDialog
    from builder.update_gui import Window
    HAVE_QT=True
except ImportError:
    HAVE_QT=False

@unittest.skipUnless(HAVE_QT,'Qt runtime not installed')
class GUIRegression(unittest.TestCase):
    def setUp(self):
        self.app=QApplication.instance() or QApplication([])
        self.temp=tempfile.TemporaryDirectory()
        self.window=Window(Path(self.temp.name)/'library', autodiscover=False)
    def tearDown(self):
        self.window.close();self.temp.cleanup()
    def test_missing_install_never_prompts_for_developer_prerequisites(self):
        self.window.offer={'version':'0.1.0'}
        self.window.local_reader=lambda *args:b''
        with patch.object(QFileDialog,'getOpenFileName') as picker:
            self.window.apply_update()
        picker.assert_not_called()
        self.assertIn('Install Balatro',self.window.status.text())
        self.assertIsNone(self.window.library.state()['active'])
    def test_export_before_build_is_disabled_and_handler_reports_reason(self):
        self.assertFalse(self.window.export.isEnabled())
        with patch.object(QFileDialog,'getSaveFileName') as dialog:
            self.window.export_rom()
        dialog.assert_not_called()
        self.assertEqual(self.window.status.text(),'Build a ROM first')
