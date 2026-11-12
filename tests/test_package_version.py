import importlib.util,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('package_app',Path(__file__).resolve().parents[1]/'tools/package_app.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class PackageVersion(unittest.TestCase):
 def test_exact_candidate_metadata(self):
  value=module.mac_metadata({'candidate':{'version':'0.3.2-rc.3','minimum_macos':'27.0'}})
  self.assertEqual(value['CFBundleVersion'],'0.3.2');self.assertEqual(value['LSMinimumSystemVersion'],'27.0');self.assertEqual(value['BalatROMCandidateVersion'],'0.3.2-rc.3')
 def test_missing_candidate_refused(self):
  with self.assertRaises(ValueError):module.mac_metadata({})
