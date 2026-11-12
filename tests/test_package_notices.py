import importlib.util, tempfile, unittest, json
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('package_notices',Path(__file__).resolve().parents[1]/'tools/package_app.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class NoticeBinding(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.r=Path(self.tmp.name);(self.r/'notices').mkdir();(self.r/'notices/LICENSE.txt').write_text('upstream notice')
  for n in ('README.md','INSTALL.md','THIRD-PARTY-NOTICES.md'):(self.r/n).write_text(n)
  self.record={'data_hashes':m.data_hashes(self.r)}
 def tearDown(self):self.tmp.cleanup()
 def test_notice_tamper_refused(self):
  m.verify_data_files(self.r,self.record);(self.r/'notices/LICENSE.txt').write_text('changed')
  with self.assertRaisesRegex(ValueError,'reviewed data hashes'):m.verify_data_files(self.r,self.record)
 def test_unlisted_notice_refused(self):
  (self.r/'notices/unreviewed.txt').write_text('extra')
  with self.assertRaises(ValueError):m.verify_data_files(self.r,self.record)
 def test_linked_notice_refused(self):
  (self.r/'notices/link').symlink_to(self.r/'README.md')
  with self.assertRaisesRegex(ValueError,'Linked'):m.verify_data_files(self.r,self.record)
 def test_underdeclared_binary_refused(self):
  (self.r/'native').write_bytes(bytes.fromhex('cffaedfe')+b'fixture')
  with patch.object(m.subprocess,'check_output',return_value='Load command 0\n cmd LC_BUILD_VERSION\n platform MACOS\n minos 27.0\n'):
   with self.assertRaisesRegex(ValueError,'requires newer'):m.validate_macos_deployment(self.r,'14.4')
   self.assertEqual(len(m.validate_macos_deployment(self.r,'27.0')['binaries']),1)
 def test_missing_explicit_minimum_refused(self):
  with self.assertRaisesRegex(ValueError,'minimum'):m.mac_metadata({'candidate':{'version':'0.3.2-rc.3'}})
