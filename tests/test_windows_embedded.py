import hashlib,tempfile,threading,unittest,zipfile
from pathlib import Path
from unittest.mock import patch
from builder.input_identity import matches
from builder.archive import Archive
from builder import managed_sdk,update

class EmbeddedWindowsTests(unittest.TestCase):
 def test_identical_complete_members_allow_only_wrapper_variation_on_windows(self):
  with tempfile.TemporaryDirectory() as td:
   base=Path(td)/'game.love';fused=Path(td)/'game.exe'
   with zipfile.ZipFile(base,'w') as z:z.writestr('game.lua',b'declared synthetic fixture, not retail')
   fused.write_bytes(b'MZ'+b'SYNTHETIC EXECUTABLE PREFIX'+base.read_bytes())
   with Archive(base) as a,Archive(fused) as b:
    required={k:a.fingerprint()[k] for k in ('archive_sha256','manifest_sha256')};actual={k:b.fingerprint()[k] for k in required}
   self.assertNotEqual(actual['archive_sha256'],required['archive_sha256']);self.assertEqual(actual['manifest_sha256'],required['manifest_sha256'])
   self.assertTrue(matches(actual,required,platform='win32'));self.assertFalse(matches(actual,required,platform='darwin'))
   self.assertTrue(matches(required,required,platform='darwin'))
   wrong=dict(actual,manifest_sha256='0'*64);self.assertFalse(matches(wrong,required,platform='win32'))
   self.assertFalse(matches({}, {},platform='win32'))
 def test_bundled_sdk_is_hash_verified_without_network_or_install(self):
  with tempfile.TemporaryDirectory() as td:
   root=Path(td);backend=root/'backend';module=backend/'builder/managed_sdk.py';module.parent.mkdir(parents=True)
   bundle=backend/'bundled-tools/gbdk';(bundle/'bin').mkdir(parents=True);(bundle/'bin/lcc.exe').write_bytes(b'declared PE fixture not executed')
   data=(bundle/'bin/lcc.exe').read_bytes();record={'files':[{'path':'bin/lcc.exe','bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}],'compiler':'bin/lcc.exe'}
   with patch.object(managed_sdk,'__file__',str(module)),patch.object(managed_sdk,'ensure',side_effect=AssertionError('No network allowed')):
    with managed_sdk.compiler_session(root/'cache',approved=True,cancel=threading.Event(),progress=lambda _:None,record=record) as compiler:
     self.assertEqual(compiler.read_bytes(),data)
    (bundle/'bin/lcc.exe').write_bytes(b'tampered')
    with self.assertRaisesRegex(update.UpdateError,'integrity'):
     with managed_sdk.compiler_session(root/'cache',approved=True,cancel=threading.Event(),progress=lambda _:None,record=record):pass
