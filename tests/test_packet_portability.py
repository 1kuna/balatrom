import hashlib,json,tempfile,unittest
from pathlib import Path,PureWindowsPath
from builder.owned_recipes import packet

class WinLeaf:
 def __init__(self,path,root):self.path=path;self.root=root
 def is_file(self):return self.path.is_file()
 def is_symlink(self):return self.path.is_symlink()
 def relative_to(self,root):return PureWindowsPath(self.path.relative_to(self.root))
class WinPacket:
 def __init__(self,p):self.p=Path(p)
 def is_symlink(self):return self.p.is_symlink()
 def is_dir(self):return self.p.is_dir()
 def __truediv__(self,v):return self.p/v
 def rglob(self,pattern):return (WinLeaf(p,self.p) for p in self.p.rglob(pattern))

class PacketPortability(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name);p=self.root/'source/src/a.c';p.parent.mkdir(parents=True);p.write_bytes(b'int a;\n');(self.root/'.gitignore').write_text('*\n')
  self.manifest={'rows':[dict(path='src/a.c',packet_path='source/src/a.c',packet_sha256=hashlib.sha256(p.read_bytes()).hexdigest())]};(self.root/'manifest.json').write_text(json.dumps(self.manifest))
 def tearDown(self):self.tmp.cleanup()
 def test_same_strict_packet_accepts_posix_and_windows_relative_paths(self):
  self.assertEqual(packet.load(self.root),self.manifest);self.assertEqual(packet.load(WinPacket(self.root)),self.manifest)
 def test_windows_and_posix_still_refuse_every_unrecorded_payload(self):
  for name in ['cache.rel','game.gbc','__pycache__/unexpected.pyc']:
   p=self.root/name;p.parent.mkdir(exist_ok=True);p.write_bytes(b'declared forbidden payload')
   for shape in [self.root,WinPacket(self.root)]:
    with self.assertRaisesRegex(ValueError,'unrecorded/linked'):packet.load(shape)
   p.unlink()
 def test_modified_input_hash_still_refused_on_both_shapes(self):
  (self.root/'source/src/a.c').write_bytes(b'int changed;')
  for shape in [self.root,WinPacket(self.root)]:
   with self.assertRaisesRegex(ValueError,'Private source input changed'):packet.load(shape)
 def test_linked_input_still_refused(self):
  p=self.root/'source/src/a.c';p.rename(p.with_suffix('.original'));p.symlink_to(p.with_suffix('.original').name)
  for shape in [self.root,WinPacket(self.root)]:
   with self.assertRaisesRegex(ValueError,'unrecorded/linked'):packet.load(shape)
