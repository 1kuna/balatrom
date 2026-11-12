from pathlib import Path
import contextlib,io,subprocess,sys,unittest
from unittest.mock import patch
from builder.__main__ import main
class OneConsumerUI(unittest.TestCase):
 def test_old_app_command_cannot_import_or_launch_qt(self):
  with patch.dict(sys.modules,{'builder.update_gui':None}),contextlib.redirect_stderr(io.StringIO()) as text:
   self.assertEqual(main(['app']),2)
  self.assertIn('Electron',text.getvalue())
 def test_old_qt_packager_refuses_before_input_or_output_actions(self):
  script=Path(__file__).resolve().parents[1]/'tools/package_app.py'
  result=subprocess.run([sys.executable,str(script),'--admission','nonexistent','--output','nonexistent'],capture_output=True,text=True)
  self.assertNotEqual(result.returncode,0);self.assertIn('Legacy Qt packaging is retired',result.stderr)
