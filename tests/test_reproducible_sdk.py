from pathlib import Path
from unittest.mock import patch
import copy,hashlib,json,tempfile,threading,unittest,os
from builder import managed_sdk
from builder.update import UpdateError

class ReproducibleCompiler(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name);self.backend=self.root/'backend';(self.backend/'builder').mkdir(parents=True);(self.backend/'toolchain').mkdir();self.work=self.root/'working';(self.work/'bin').mkdir(parents=True)
  self.original=b'declared original compiler';self.new=b'declared reproducible compiler';self.source=self.backend/'toolchain/sdcc.exe';self.source.write_bytes(self.new)
  files={'bin/lcc':b'declared driver','bin/sdcc.exe':self.original}
  for n,b in files.items():(self.work/n).write_bytes(b)
  self.record={'compiler':'bin/lcc','files':[{'path':n,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()} for n,b in files.items()],'working_copy_overlay':{'source':'toolchain/sdcc.exe','destination':'bin/sdcc.exe','bytes':len(self.new),'sha256':hashlib.sha256(self.new).hexdigest()}}
  self.scope=patch.object(managed_sdk,'__file__',str(self.backend/'builder/managed_sdk.py'));self.scope.start();self.addCleanup(self.scope.stop)
 def apply(self):managed_sdk._apply_working_copy_overlay(self.work,self.record)
 def test_only_verified_working_compiler_changes(self):
  self.apply();self.assertEqual((self.work/'bin/sdcc.exe').read_bytes(),self.new);self.assertEqual((self.work/'bin/lcc').read_bytes(),b'declared driver');self.assertEqual(self.source.read_bytes(),self.new)
 def test_corrupt_overlay_fails_before_original_change(self):
  self.source.write_bytes(b'x'*len(self.new))
  with self.assertRaisesRegex(UpdateError,'integrity'):self.apply()
  self.assertEqual((self.work/'bin/sdcc.exe').read_bytes(),self.original)
 def test_corrupt_upstream_is_not_covered_by_good_overlay(self):
  (self.work/'bin/lcc').write_bytes(b'corrupt')
  with self.assertRaisesRegex(UpdateError,'Original'):self.apply()
  self.assertEqual((self.work/'bin/sdcc.exe').read_bytes(),self.original)
 def test_traversal_and_wrong_destination_refused(self):
  for field,value in [('source','../escape.exe'),('destination','../escape.exe'),('destination','bin/lcc')]:
   original=copy.deepcopy(self.record);self.record['working_copy_overlay'][field]=value
   with self.assertRaises(UpdateError):self.apply()
   self.record=original
 def test_missing_overlay_refused(self):
  self.source.unlink()
  with self.assertRaises(UpdateError):self.apply()
 def test_linked_overlay_refused(self):
  if os.name=='nt':self.skipTest('Creating Windows symlinks requires privilege; no security changes made')
  self.source.unlink();other=self.root/'other';other.write_bytes(self.new);self.source.symlink_to(other)
  with self.assertRaisesRegex(UpdateError,'Linked'):self.apply()
 def test_no_overlay_leaves_upstream_unchanged(self):
  self.record.pop('working_copy_overlay');self.apply();self.assertTrue(managed_sdk.verify(self.work,self.record))
 def test_session_preserves_bundled_original_and_cleans_working_copy(self):
  bundle=self.backend/'bundled-tools/gbdk';bundle.parent.mkdir();import shutil;shutil.copytree(self.work,bundle)
  with managed_sdk.compiler_session(self.root/'cache',approved=False,cancel=threading.Event(),progress=lambda _:None,record=self.record) as lcc:
   temporary=lcc.parent.parent;self.assertEqual((temporary/'bin/sdcc.exe').read_bytes(),self.new);self.assertEqual((bundle/'bin/sdcc.exe').read_bytes(),self.original)
  self.assertFalse(temporary.exists());self.assertTrue(managed_sdk.verify(bundle,self.record))

if __name__=='__main__':unittest.main()
