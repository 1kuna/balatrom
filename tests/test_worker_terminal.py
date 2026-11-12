import tempfile,threading,time,unittest
from pathlib import Path
from unittest.mock import patch
import subprocess
from builder.update_runtime import build_adapter
from builder.update import UpdateError,Cancelled
from builder.desktop_service import Service
BODY='''import json,time\nfrom pathlib import Path\ndef build_owned(game,engine,output,lcc,**kwargs):\n mode=str(game)\n if mode!='fail-silent':print('BUILD_STATUS '+json.dumps(dict(schema=1,stage='verify',message='Build stopped' if mode=='fail-status' else 'Verifying',elapsedSeconds=.01,completed=None,total=None,activeUnits=[])),flush=True)\n if mode=='reported-failure-hang':\n  print('BUILD_STATUS '+json.dumps(dict(schema=1,stage='verify',message='Build stopped',outcome='failed',elapsedSeconds=.01,completed=None,total=None,activeUnits=[])),flush=True);time.sleep(60)\n if mode.startswith('fail'):raise ValueError('Source packet has unrecorded/linked inputs; cached objects and ROMs are forbidden')\n if mode=='cancel':\n  while True:kwargs['cancel_check']();time.sleep(.02)\n p=Path(output)/'linked';p.mkdir(parents=True);(p/'balatrom.gbc').write_bytes(b'DECLARED NON-ROM FIXTURE')\n'''
class WorkerTerminal(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.p=Path(self.tmp.name);self.source=self.p/'source';m=self.source/'builder';m.mkdir(parents=True);(m/'__init__.py').write_text('');(m/'owned_install.py').write_text(BODY)
 def tearDown(self):self.tmp.cleanup()
 def run_case(self,mode,cancel=None):
  self.events=[];return build_adapter(Path('/declared-not-executed-compiler'),event_progress=self.events.append)(self.source,Path(mode),self.p/'out',cancel or threading.Event())
 def test_fast_failure_with_status_drains_and_raises_short_error_with_trace(self):
  began=time.monotonic()
  with self.assertRaises(UpdateError) as cm:self.run_case('fail-status')
  self.assertLess(time.monotonic()-began,3);self.assertEqual(self.events[-1]['message'],'Build stopped');self.assertIn('Source packet has unrecorded/linked inputs',cm.exception.detail);self.assertNotIn(str(self.p),str(cm.exception))
 def test_fast_failure_without_status_still_finishes(self):
  with self.assertRaises(UpdateError) as cm:self.run_case('fail-silent')
  self.assertEqual(self.events,[]);self.assertIn('ValueError',cm.exception.detail)
 def test_success_is_not_misclassified_as_failure(self):
  self.assertEqual(self.run_case('success').read_bytes(),b'DECLARED NON-ROM FIXTURE');self.assertTrue(self.events)
 def test_reported_failure_with_lingering_process_is_reaped_before_error(self):
  actual=subprocess.Popen;children=[]
  def launch(*a,**kw):
   child=actual(*a,**kw);children.append(child);return child
  with patch('builder.update_runtime.FAILED_SHUTDOWN_GRACE_SECONDS',.25),patch('builder.update_runtime.subprocess.Popen',side_effect=launch):
   with self.assertRaises(UpdateError):self.run_case('reported-failure-hang')
  self.assertEqual(len(children),1);self.assertIsNotNone(children[0].poll())
 def test_cancel_finishes_and_does_not_publish(self):
  stop=threading.Event();timer=threading.Timer(.25,stop.set);timer.start()
  try:
   with self.assertRaises(Cancelled):self.run_case('cancel',stop)
  finally:timer.join()
  self.assertFalse((self.p/'out/linked/balatrom.gbc').exists())
 def test_status_recovers_terminal_without_starting_or_repeating_work(self):
  events=[];s=Service(self.p/'versions',self.p/'unused-contract',events.append);s.operation=lambda m:s.send(m['id'],'error',message='Declared failure',detail='Details')
  s.accept(dict(id=1,command='build'));s.thread.join(3);thread=s.thread;self.assertFalse(s.busy)
  s.accept(dict(id=2,command='status'));self.assertIs(s.thread,thread);self.assertEqual(events[-1]['terminal']['type'],'error');self.assertFalse(events[-1]['busy'])
  s.accept(dict(id=3,command='build'));s.thread.join(3);self.assertEqual(events[-1]['id'],3);self.assertFalse(s.busy)
 def test_unexpected_thread_exit_still_has_terminal_result(self):
  events=[];s=Service(self.p/'versions',self.p/'unused-contract',events.append)
  def exit_thread(m):raise SystemExit('Declared unexpected worker thread exit')
  s.operation=exit_thread;s.accept(dict(id=1,command='build'));s.thread.join(3);self.assertFalse(s.busy);self.assertEqual(events[-1]['type'],'error');self.assertIn('Declared unexpected',events[-1]['detail'])
