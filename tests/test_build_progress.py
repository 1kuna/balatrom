import contextlib,ctypes,io,json,tempfile,time,unittest
from pathlib import Path
from unittest.mock import patch
from builder import build_progress,worker_budget
from builder.update_runtime import latest_status

class ProgressContracts(unittest.TestCase):
 def test_slow_first_unit_has_immediate_zero_and_factual_heartbeat(self):
  with tempfile.TemporaryDirectory() as td,contextlib.redirect_stdout(io.StringIO()) as log:
   with build_progress.BuildProgress(Path(td)/'trace.json') as p:
    p.phase('compile','Compiling');p.unit_started('actual-child');time.sleep(2.15)
    lines=[json.loads(x.split(' ',1)[1]) for x in log.getvalue().splitlines() if x.startswith('BUILD_STATUS ')]
    compiling=[x for x in lines if x['stage']=='compile'];self.assertGreaterEqual(len(compiling),3)
    self.assertTrue(all(x['completed']==0 for x in compiling));self.assertGreater(compiling[-1]['elapsedSeconds'],1.9)
    self.assertEqual(compiling[-1]['activeUnits'][0]['name'],'actual-child');p.unit_finished('actual-child');p.units_completed(1)
   self.assertFalse(p.thread.is_alive());self.assertIsNone(build_progress.current())
   trace=json.loads((Path(td)/'trace.json').read_text());self.assertEqual(trace['completed'],1)
 def test_parser_uses_zero_unit_stage_and_rejects_invalid_counter(self):
  event={'schema':1,'stage':'compile','message':'Compiling','completed':0,'total':198,'elapsedSeconds':2.0,'activeUnits':[]}
  raw=b'older log\nBUILD_STATUS '+json.dumps(event).encode()+b'\n';self.assertEqual(latest_status(raw),event)
  event['completed']=199;self.assertIsNone(latest_status(b'BUILD_STATUS '+json.dumps(event).encode()))
 def test_windows_memory_policy_uses_actual_api_layout_not_unix_sysconf(self):
  class Function:
   def __call__(self,p):
    self.seen=p.contents.length;p.contents.total_physical=32*worker_budget.GIB;p.contents.available_physical=20*worker_budget.GIB;return 1
  class Kernel:GlobalMemoryStatusEx=Function()
  k=Kernel();self.assertEqual(ctypes.sizeof(worker_budget.WindowsMemoryStatus),64)
  self.assertEqual(worker_budget.windows_memory_snapshot(k),(32*worker_budget.GIB,20*worker_budget.GIB));self.assertEqual(k.GlobalMemoryStatusEx.seen,64)
  with patch.object(worker_budget.sys,'platform','win32'),patch.object(worker_budget,'windows_memory_snapshot',return_value=(32*worker_budget.GIB,20*worker_budget.GIB)),patch.object(worker_budget.os,'sysconf',side_effect=AssertionError('Unix memory API on Windows')),patch.object(worker_budget.os,'cpu_count',return_value=8):
   self.assertEqual(worker_budget.snapshot()['workers'],4)
 def test_windows_memory_api_failure_remains_one_worker(self):
  with patch.object(worker_budget.sys,'platform','win32'),patch.object(worker_budget,'windows_memory_snapshot',side_effect=OSError('declared failure')):
   self.assertEqual(worker_budget.snapshot()['workers'],1)
