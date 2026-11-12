from pathlib import Path
import unittest
from builder.install_discovery import classify
class SharedDiscovery(unittest.TestCase):
 def test_same_states_and_strict_fingerprint_for_both_platforms(self):
  class Fake:
   def __init__(self,path):self.path=Path(path)
   def __enter__(self):return self
   def __exit__(self,*args):pass
   def assert_unchanged(self):pass
   def fingerprint(self):return {'archive_sha256':'a','manifest_sha256':'wrong' if 'bad' in str(self.path) else 'm'}
  for platform in ('darwin','win32'):
   for paths,status in [([], 'not_found'),(['/supported'],'supported'),(['/bad'],'found_unqualified'),(['/one','/two'],'multiple_supported')]:
    with self.subTest(platform=platform,paths=paths):
     result=classify({'candidates':[{'game_path':p} for p in paths]},'a','m',case_insensitive=platform=='win32',archive_factory=Fake)
     self.assertEqual(result['status'],status)
     self.assertFalse(result['game_launched']);self.assertFalse(result['network_access'])
 def test_incomplete_discovery_is_not_reported_as_missing(self):
  self.assertEqual(classify({'candidates':[],'warnings':[{'reason':'unreadable Steam config'}]},'a','m')['status'],'discovery_incomplete')
 def test_shared_bound_never_partially_selects(self):
  r=classify({'candidates':[{'game_path':'x'}]*17},'a','m')
  self.assertEqual(r['status'],'discovery_incomplete');self.assertIsNone(r['selected_game'])

class SharedProviderFailure(unittest.TestCase):
 def test_provider_failure_is_incomplete_not_missing_or_runtime_crash(self):
  from builder.install_discovery import scan_and_classify
  def bad():raise OSError('unreadable library')
  result=scan_and_classify(bad,'a','m')
  self.assertEqual(result['status'],'discovery_incomplete');self.assertIsNone(result['selected_game'])
