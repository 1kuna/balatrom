import json,os,subprocess,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
from builder import update as u
from test_update import fixture,NOW,rom_bytes
from PySide6.QtWidgets import QApplication,QFileDialog
from builder.update_gui import Window
os.environ['QT_QPA_PLATFORM']='offscreen'
class QCRepairs(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.root=Path(self.tmp.name);self.lib=u.Updater(self.root/'lib')
 def tearDown(self):self.tmp.cleanup()
 def test_crashed_owner_releases_kernel_lock(self):
  code='import os;from pathlib import Path;from builder.update import Updater\nwith Updater(Path('+repr(str(self.lib.root))+')).lock(): os._exit(0)'
  subprocess.run([sys.executable,'-c',code],check=True)
  with self.lib.lock():
   with self.assertRaises(u.UpdateError):
    with self.lib.lock():pass
  with self.lib.lock():pass
 def test_legacy_empty_lock_is_recoverable_and_symlink_is_rejected(self):
  p=self.lib.root/'.update.lock';p.touch()
  with self.lib.lock():pass
  p.unlink();p.symlink_to(self.root/'arbitrary')
  with self.assertRaises(u.UpdateError):
   with self.lib.lock():pass
 def test_selected_save_not_associated_with_active_version(self):
  game=self.root/'owned/game';game.parent.mkdir();game.write_bytes(b'fixture');save=self.root/'other.sav';save.write_bytes(b'SAVE')
  self.lib.state_path.write_text(json.dumps({'active':'1.0.0','highest':'1.0.0','previous':None,'versions':{'1.0.0':{'version':'1.0.0','rom_sha256':u.digest(rom_bytes()),'save':{'format_id':'synthetic-save-rng1'}}}}))
  _,_,reader,_=fixture('1.1.0',compatible=['1.0.0']);m=u.discover('0.0.0',reader=reader,now=NOW,repository_id=123)
  with self.assertRaisesRegex(u.UpdateError,'original ROM'):
   self.lib.apply(m,approved=True,game_path=game,fingerprint=lambda p:m['source_install'],build=lambda *args:None,reader=reader,save_path=save)
  self.assertEqual(list(self.lib.root.glob('pending-*')),[]);self.assertEqual(save.read_bytes(),b'SAVE')
 def test_active_old_identity_and_success_disable_duplicate_build(self):
  app=QApplication.instance() or QApplication([]);contract=self.root/'initial.json';contract.write_text(json.dumps({'version':'0.2.0','expected_rom_sha256':'4'*64}))
  self.lib.state_path.write_text(json.dumps({'active':'0.1.0','highest':'0.1.0','previous':None,'versions':{'0.1.0':{'rom_sha256':'a'*64}}}))
  w=Window(self.lib.root,contract,autodiscover=False);w.game_path=self.root/'declared-owned-game';w.refresh_identity();self.assertIn('Active version 0.1.0',w.identity.text());self.assertIn('aaaaaaaaaaaa',w.identity.text());self.assertIn('444444444444',w.identity.text());self.assertTrue(w.first_build.isEnabled());self.assertFalse(w.check.isEnabled())
  self.lib.state_path.write_text(json.dumps({'active':'0.2.0','highest':'0.2.0','previous':None,'versions':{'0.2.0':{'rom_sha256':'4'*64}}}));w.offer={'version':'0.2.0'};w.build_complete(None);self.assertFalse(w.first_build.isEnabled());self.assertFalse(w.apply.isEnabled());self.assertIsNone(w.offer);w.close()
 def test_actual_qt_directory_picker_selects_app_bundle(self):
  app=QApplication.instance() or QApplication([]);bundle=self.root/'Balatro.app';bundle.mkdir();(bundle/'Contents').mkdir();dlg=QFileDialog();dlg.setOption(QFileDialog.Option.DontUseNativeDialog);dlg.setFileMode(QFileDialog.FileMode.Directory);dlg.setOption(QFileDialog.Option.ShowDirsOnly);dlg.setDirectory(str(self.root));dlg.selectFile(str(bundle));self.assertEqual(Path(dlg.selectedFiles()[0]),bundle);dlg.close()
 def test_malformed_old_state_does_not_crash_window(self):
  app=QApplication.instance() or QApplication([]);self.lib.state_path.write_text(json.dumps({'active':'0.1.0','highest':'0.1.0','versions':{}}));w=Window(self.lib.root,autodiscover=False);self.assertIn('invalid',w.identity.text());self.assertFalse(w.export.isEnabled());self.assertFalse(w.first_build.isEnabled());w.close()
 def test_finished_reenables_configured_update_after_job_clears(self):
  app=QApplication.instance() or QApplication([]);w=Window(self.lib.root,autodiscover=False)
  class FinishedJob:
   def deleteLater(self):pass
  w.job=FinishedJob()
  with patch.object(u,'REPOSITORY_ID',123):w.finished();self.assertIsNone(w.job);self.assertTrue(w.check.isEnabled())
  w.close()
 def test_nested_modal_apply_and_running_start_are_ignored(self):
  from PySide6.QtWidgets import QMessageBox
  app=QApplication.instance() or QApplication([]);w=Window(self.lib.root,autodiscover=False);w.offer={'version':'0.2.0'};calls=[]
  w.game_path=self.root/'declared-owned-game';w.import_existing_save=True
  def picker(*args,**kwargs):calls.append(1);w.apply_update();return ('','')
  with patch.object(QFileDialog,'getOpenFileName',side_effect=picker):w.apply_update()
  self.assertEqual(len(calls),1);self.assertFalse(w.choosing)
  marker=object();w.job=marker;w.start(lambda *args:None,lambda *args:None);self.assertIs(w.job,marker);w.job=None;w.close()
 def synthetic_apply(self):
  game=self.root/'owned/game';game.parent.mkdir(exist_ok=True);game.write_bytes(b'fixture');_,_,reader,_=fixture();m=u.discover('0.0.0',reader=reader,now=NOW,repository_id=123)
  def build(source,game,output,cancel):output.mkdir();p=output/'balatrom.gbc';p.write_bytes(rom_bytes());return p
  return self.lib.apply(m,approved=True,game_path=game,fingerprint=lambda p:m['source_install'],build=build,reader=reader)
 def test_interrupted_commit_verified_output_is_preserved_and_retry_works(self):
  real=u.atomic_json
  def fail_state(path,value):
   if path==self.lib.state_path:raise OSError('injected state commit failure')
   return real(path,value)
  with patch.object(u,'atomic_json',side_effect=fail_state):
   with self.assertRaises(OSError):self.synthetic_apply()
  self.assertTrue((self.lib.root/'v1.0.0/receipt.json').exists());self.synthetic_apply();self.assertEqual(self.lib.state()['active'],'1.0.0');self.assertEqual(len(list(self.lib.root.glob('interrupted-*/retained-version/balatrom.gbc'))),1)
 def test_unknown_interrupted_output_is_not_moved(self):
  q=self.lib.root/'v1.0.0';q.mkdir();(q/'unknown').write_bytes(b'keep')
  with self.assertRaisesRegex(u.UpdateError,'manual recovery'):self.synthetic_apply()
  self.assertEqual((q/'unknown').read_bytes(),b'keep');self.assertEqual(list(self.lib.root.glob('interrupted-*')),[])
 def test_export_holds_one_identity_lock_against_rollback(self):
  self.synthetic_apply();state=self.lib.state();state['previous']='0.0.1';state['versions']['0.0.1']={'rom_sha256':u.digest(rom_bytes())};self.lib.state_path.write_text(json.dumps(state));original=self.lib.active_rom
  def checked(snapshot):
   with self.assertRaisesRegex(u.UpdateError,'already running'):self.lib.rollback(approved=True)
   return original(snapshot)
  with patch.object(self.lib,'active_rom',side_effect=checked):self.lib.export(self.root/'guarded.gbc')
  self.assertEqual((self.root/'guarded.gbc').read_bytes(),rom_bytes());self.assertEqual(self.lib.state()['active'],'1.0.0')
 def test_actual_crash_after_version_rename_retry_preserves_orphan(self):
  game=self.root/'owned/game';game.parent.mkdir();game.write_bytes(b'fixture');_,_,reader,_=fixture();m=u.discover('0.0.0',reader=reader,now=NOW,repository_id=123)
  (self.root/'metadata.json').write_text(json.dumps(m));(self.root/'source.zip').write_bytes(reader(m['source']['browser_download_url'],u.MAX_PACKAGE,u.threading.Event()));(self.root/'reference.gbc').write_bytes(rom_bytes())
  code='''import os,json
from pathlib import Path
from builder import update as u
r=Path(ROOT);library=u.Updater(r/'lib');real=u.atomic_json
def crash(path,value):
 if path==library.state_path:os._exit(19)
 real(path,value)
u.atomic_json=crash
def build(source,game,output,cancel):
 output.mkdir();p=output/'balatrom.gbc';p.write_bytes((r/'reference.gbc').read_bytes());return p
m=json.loads((r/'metadata.json').read_text());library.apply(m,approved=True,game_path=r/'owned/game',fingerprint=lambda p:m['source_install'],build=build,reader=lambda *args:(r/'source.zip').read_bytes())
'''.replace('ROOT',repr(str(self.root)))
  result=subprocess.run([sys.executable,'-c',code]);self.assertEqual(result.returncode,19);self.assertTrue((self.lib.root/'v1.0.0/receipt.json').is_file());self.assertIsNone(self.lib.state()['active'])
  def build(source,game,output,cancel):output.mkdir();p=output/'balatrom.gbc';p.write_bytes(rom_bytes());return p
  self.lib.apply(m,approved=True,game_path=game,fingerprint=lambda p:m['source_install'],build=build,reader=lambda *args:(self.root/'source.zip').read_bytes());self.assertEqual(self.lib.state()['active'],'1.0.0');self.assertEqual(len(list(self.lib.root.glob('interrupted-*/retained-version/receipt.json'))),1)
 def test_real_qt_missing_malformed_and_unreadable_startup_and_finished(self):
  app=QApplication.instance() or QApplication([])
  for raw in (b'{}',b'{',b'null'):
   self.lib.state_path.write_bytes(raw);w=Window(self.lib.root,autodiscover=False);self.assertFalse(w.export.isEnabled());self.assertIn('invalid',w.identity.text())
   class FinishedJob:
    def deleteLater(self):pass
   w.job=FinishedJob();w.finished();self.assertIsNone(w.job);w.close()
  self.lib.state_path.unlink();w=Window(self.lib.root,autodiscover=False);self.assertFalse(w.export.isEnabled());w.close()
  self.lib.state_path.write_text('{}');self.lib.state_path.chmod(0)
  try:
   with self.assertRaises(PermissionError):self.lib.state_path.read_bytes()
   w=Window(self.lib.root,autodiscover=False);self.assertFalse(w.export.isEnabled());w.job=FinishedJob();w.finished();self.assertIsNone(w.job);w.close()
  finally:self.lib.state_path.chmod(0o600)
 def test_actual_worker_failure_keeps_private_log_in_details(self):
  import threading,shutil
  from builder.update_runtime import build_adapter
  release=self.root/'fake-release';release.mkdir();shutil.copytree(Path(u.__file__).parent,release/'builder');output=self.root/'private/build';output.parent.mkdir();game=self.root/'owned/fake-game';game.parent.mkdir();game.write_bytes(b'not-owned')
  with self.assertRaisesRegex(u.UpdateError,'Open Details') as raised:build_adapter(Path('/no/compiler'))(release,game,output,threading.Event())
  log=output.parent/'build-worker.log';self.assertTrue(log.is_file());self.assertIn('explicit regular private engine packet',log.read_text())
  self.assertIn(str(log),raised.exception.detail)
  self.assertNotIn(str(log),str(raised.exception))
  from builder.desktop_service import Service
  service=Service(self.root/'desktop-library',self.root/'contract.json',lambda event:None)
  with patch('builder.desktop_service.local_package.load',return_value=({'source_install':{}},None)),patch('builder.desktop_service.fingerprint',side_effect=raised.exception):
   service.operation({'id':1,'command':'locate','args':{'path':str(game)}})
  self.assertEqual(service.terminal['type'],'error')
  self.assertEqual(service.terminal['detail'],raised.exception.detail)
 def test_actual_a6_carried_save_record_exports_and_missing_flag_fails_closed(self):
  import shutil,hashlib
  configured=os.environ.get('BALATROM_PRIVATE_FIXTURE_LIBRARY')
  if not configured:self.skipTest('private genuine-fixture integration: set BALATROM_PRIVATE_FIXTURE_LIBRARY explicitly; no fixture is bundled')
  original=Path(configured).expanduser()
  if not original.is_absolute():self.fail('BALATROM_PRIVATE_FIXTURE_LIBRARY must be an absolute path')
  self.assertTrue((original/'state.json').is_file());state=json.loads((original/'state.json').read_text());version=state['active'];dest=self.lib.root/('v'+version);dest.mkdir()
  for name in ('balatrom.gbc','save-backup.bin','receipt.json'):shutil.copyfile(original/('v'+version)/name,dest/name)
  self.lib.state_path.write_text(json.dumps(state));self.lib.export(self.root/'old-a6.gbc',save_adapter='pyboy');self.assertEqual(hashlib.sha256((self.root/'old-a6.gbc').read_bytes()).hexdigest(),'a6a4c061a48ffa913403800e15d9fc5fd99b1ee5dcdb7c6c94360b51523faa31');self.assertEqual((self.root/'old-a6.gbc.ram').read_bytes(),(dest/'save-backup.bin').read_bytes())
  state['versions'][version].pop('carried_save');self.lib.state_path.write_text(json.dumps(state))
  with self.assertRaisesRegex(u.UpdateError,'provenance'):self.lib.export(self.root/'unknown.gbc')
  self.assertFalse((self.root/'unknown.gbc').exists());app=QApplication.instance() or QApplication([]);w=Window(self.lib.root,autodiscover=False)
  with patch.object(QFileDialog,'getSaveFileName') as dialog:w.export_rom();dialog.assert_not_called()
  self.assertIn('provenance',w.status.text());w.close()
 def test_two_gib_preflight_rejects_before_pending_source_or_build(self):
  from types import SimpleNamespace
  with patch.object(u.shutil,'disk_usage',return_value=SimpleNamespace(free=1024**3)):
   with self.assertRaisesRegex(u.UpdateError,'2GiB'):self.synthetic_apply()
  self.assertEqual(list(self.lib.root.glob('pending-*')),[])
