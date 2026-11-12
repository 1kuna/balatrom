"""Mocked cancellation: no compiler or native process is launched."""
from pathlib import Path
from types import SimpleNamespace
import tempfile
import threading
import subprocess
import sys
import unittest
from unittest.mock import patch, Mock
from builder.owned_install import compile_fresh
from builder.update_runtime import build_adapter
from builder.update import UpdateError

class RuntimeContracts(unittest.TestCase):
    def test_cancel_stops_mocked_compiler_group(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root); (path/'view').mkdir()
            process=Mock(pid=123); checks=0
            def cancel():
                nonlocal checks
                checks+=1
                if checks==2: raise RuntimeError('synthetic cancellation')
            with patch('builder.owned_install.subprocess.Popen',return_value=process), patch('builder.owned_install.os.killpg') as kill:
                with self.assertRaisesRegex(RuntimeError,'synthetic cancellation'):
                    compile_fresh(path,{'compile_flags':[],'objects':[{'name':'unit','bank':None,'source':'unit.c'}]},Path('fake-lcc'),cancel_check=cancel)
                kill.assert_called_once(); process.communicate.assert_called_once_with()
    def test_cancel_race_preserves_original_error_and_drains(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root); (path/'view').mkdir()
            process=Mock(pid=123); checks=0
            def cancel():
                nonlocal checks
                checks+=1
                if checks==2: raise RuntimeError('original cancellation')
            with patch('builder.owned_install.subprocess.Popen',return_value=process), patch('builder.owned_install.os.killpg',side_effect=ProcessLookupError):
                with self.assertRaisesRegex(RuntimeError,'original cancellation'):
                    compile_fresh(path,{'compile_flags':[],'objects':[{'name':'unit','bank':None,'source':'unit.c'}]},Path('fake-lcc'),cancel_check=cancel)
                process.communicate.assert_called_once_with()

    def test_isolated_worker_uses_explicit_synthetic_release_source(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root); source=path/'source'; module=source/'builder'
            module.mkdir(parents=True)
            (module/'__init__.py').write_text('')
            (module/'owned_install.py').write_text("from pathlib import Path\ndef build_owned(game, packet, output, lcc, cancel_check, *, compile_workers, object_cache):\n assert compile_workers == 1 and object_cache is None\n cancel_check()\n Path(output).write_text('SYNTHETIC RELEASE SOURCE')\n")
            worker=Path(__file__).resolve().parents[1]/'builder/update_worker.py'
            result=subprocess.run([sys.executable,'-I',str(worker),'--source',str(source),'--game','fake',
                '--output',str(path/'receipt'),'--lcc','fake','--cancel-file',str(path/'cancel')],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual((path/'receipt').read_text(),'SYNTHETIC RELEASE SOURCE')

    def test_unknown_platform_refused_and_windows_test_lane_passes_command(self):
        with tempfile.TemporaryDirectory() as td:
            path=Path(td);source=path/'source with spaces';game=path/'game.exe';output=path/'new output';compiler=path/'lcc.exe'
            with patch('builder.update_runtime.os',SimpleNamespace(name='unsupported')):
                with self.assertRaisesRegex(UpdateError,'unsupported'):
                    build_adapter(compiler)(source,game,output,threading.Event())
            process=Mock();process.wait.return_value=0
            with patch('builder.update_runtime.os',SimpleNamespace(name='nt')),patch('builder.update_runtime.subprocess.Popen',return_value=process) as spawn:
                result=build_adapter(compiler)(source,game,output,threading.Event())
            command=spawn.call_args.args[0]
            self.assertIn(str(source),command);self.assertIn(str(game),command);self.assertNotIn('shell',spawn.call_args.kwargs)
            self.assertEqual(result,output/'linked/balatrom.gbc')
            # Declared mock only, never a Windows runtime/cancellation claim.
