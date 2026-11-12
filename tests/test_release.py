"""Synthetic release gates. No actual approval, game data or installable assets."""
from pathlib import Path
import tempfile
import unittest
from builder.release import prepare, sha, manifest_hash
from builder.update import UpdateError

class ReleaseGates(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.root=Path(self.temp.name)
        self.source=self.root/'source'; self.source.mkdir()
        files=[]
        for name in ('builder/__main__.py','engine/manifest.json','compatibility/owned_profiles.json'):
            path=self.source/name; path.parent.mkdir(exist_ok=True); path.write_text('synthetic')
            files.append({'path':name,'bytes':path.stat().st_size,'sha256':sha(path)})
        self.manifest={'files':files}; self.artifacts={}; tests={}
        for platform in ('mac','windows'):
            path=self.root/(platform+'.zip'); path.write_bytes(b'SYNTHETIC')
            self.artifacts[platform]=path
            tests[platform]={'tester':'Zach','result':'passed','evidence':'SYNTHETIC TEST ONLY','artifact_sha256':sha(path)}
        self.qc={'version':'1.0.0','decision':'approved','approver':'Zach','source_manifest_sha256':manifest_hash(self.manifest),
                 'personal_tests':tests,'gates':dict.fromkeys(('rights','license','sanitation','source_closure','save_preservation','hosting','gui_dependencies'),True)}
        self.contract={'version':'1.0.0','channel':'stable','repository_id':123,'source_install':{'archive_sha256':'a'*64,'manifest_sha256':'b'*64},
                       'save':{'format_id':'synthetic','bytes':4,'compatible_versions':[]},'expected_rom_sha256':'c'*64,'changelog':'Synthetic'}
    def tearDown(self): self.temp.cleanup()
    def run_prepare(self): return prepare(self.source,self.manifest,self.contract,self.qc,self.artifacts,self.root/'candidate')
    def test_both_personal_tests_mandatory(self):
        del self.qc['personal_tests']['windows']
        with self.assertRaisesRegex(UpdateError,'personally test'): self.run_prepare()
        self.assertFalse((self.root/'candidate').exists())
    def test_changed_artifact_invalidates_approval(self):
        self.artifacts['mac'].write_bytes(b'changed')
        with self.assertRaises(UpdateError): self.run_prepare()
    def test_open_save_gate_blocks_candidate(self):
        self.qc['gates']['save_preservation']=False
        with self.assertRaisesRegex(UpdateError,'save_preservation'): self.run_prepare()
    def test_exact_fixture_creates_private_candidate_only(self):
        self.assertFalse(self.run_prepare()['published'])
        self.assertTrue((self.root/'candidate'/'balatrom-source.zip').is_file())
    def test_source_hash_must_match(self):
        (self.source/'builder/__main__.py').write_text('changed')
        with self.assertRaisesRegex(UpdateError,'differs'): self.run_prepare()
