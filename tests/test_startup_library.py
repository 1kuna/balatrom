import unittest
from unittest.mock import patch
from builder import update_gui as gui
from builder.update import UpdateError

class StartupLibrary(unittest.TestCase):
    def check_failure(self, error):
        with patch.object(gui, 'QApplication') as app, patch.object(gui, 'Window', side_effect=error), patch.object(gui.QMessageBox, 'critical') as message:
            self.assertEqual(gui.main(), 2)
            app.return_value.exec.assert_not_called()
            self.assertIn('preserved', message.call_args.args[2])
            self.assertIn(str(error), message.call_args.args[2])
    def test_unwritable_library_reports_error(self):
        self.check_failure(OSError('test library permission denied'))
    def test_symlink_library_reports_error(self):
        self.check_failure(UpdateError('Update library must not be a symlink'))
