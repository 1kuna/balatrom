"""Portable Windows-preparation contracts. Real Job APIs run only on Windows."""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import Mock, patch
from builder.platform_process import ManagedCompiler, WindowsJob


class PortableWindowsPreparation(unittest.TestCase):
    def test_pinned_windows_sdk_is_explicit_and_unqualified(self):
        record=json.loads((Path(__file__).parents[1]/'builder/windows_sdk.json').read_text())
        self.assertEqual(record['architecture'],'AMD64')
        self.assertEqual(record['archive_sha256'],'266854ce92e3064871c5b28cd3436cc2a6cb136af9e7cf617140108f8c1c5890')
        self.assertFalse(record['runtime_qualified'])
        rows={r['path']:r for r in record['prerequisites']}
        self.assertIn('bin/lcc.exe',rows)
        self.assertTrue(any(n.endswith('.dll') for n in rows))
        self.assertIn('include/gb/gb.h',rows)

    def test_mocked_windows_suspends_before_job_attachment(self):
        process=Mock(pid=42);job=Mock()
        with patch('builder.platform_process.os.name','nt'), patch('builder.platform_process.WindowsJob',return_value=job), patch('builder.platform_process.subprocess.Popen',return_value=process) as spawn:
            compiler=ManagedCompiler(['fake'],Path('.'))
            self.assertTrue(spawn.call_args.kwargs['creationflags'] & 4)
            job.attach_and_resume.assert_called_once_with(process)
            compiler.terminate_tree();job.terminate.assert_called_once()
            compiler.close();job.close.assert_called_once()

    def test_mocked_assignment_failure_never_leaves_child_running(self):
        process=Mock();job=Mock();job.attach_and_resume.side_effect=OSError('assignment refused')
        with patch('builder.platform_process.os.name','nt'), patch('builder.platform_process.WindowsJob',return_value=job), patch('builder.platform_process.subprocess.Popen',return_value=process):
            with self.assertRaisesRegex(OSError,'assignment refused'):ManagedCompiler(['fake'],Path('.'))
        process.kill.assert_called_once();process.communicate.assert_called_once_with(timeout=5);job.close.assert_called_once()

    def test_mocked_failed_start_kill_race_preserves_original_and_closes_job(self):
        process=Mock();process.kill.side_effect=ProcessLookupError('already exited');process.communicate.side_effect=OSError('pipe race')
        job=Mock();job.attach_and_resume.side_effect=OSError('original assignment error')
        with patch('builder.platform_process.os.name','nt'), patch('builder.platform_process.WindowsJob',return_value=job), patch('builder.platform_process.subprocess.Popen',return_value=process):
            with self.assertRaisesRegex(OSError,'original assignment error') as caught:ManagedCompiler(['fake'],Path('.'))
        job.close.assert_called_once();self.assertEqual(len(caught.exception.__notes__),2)

    @unittest.skipIf(os.name=='nt','POSIX only')
    def test_real_posix_child_capture_still_works(self):
        with ManagedCompiler([sys.executable,'-c','print("canary")'],Path('.')) as process:
            out,err=process.communicate(timeout=5)
            self.assertEqual((process.returncode,out.strip(),err),(0,'canary',''))


@unittest.skipUnless(os.name=='nt','Needs Windows; mocks cannot test Job Objects')
class NativeWindowsOwnership(unittest.TestCase):
    def test_real_windows_success(self):
        with ManagedCompiler([sys.executable,'-c','print("native Windows")'],Path('.')) as p:
            out,err=p.communicate(timeout=10)
            self.assertEqual((p.returncode,out.strip(),err),(0,'native Windows',''))

    def test_real_windows_job_terminates_descendant(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);pidfile=root/'child.pid'
            code='import subprocess,sys,time; from pathlib import Path; p=subprocess.Popen([sys.executable,"-c","import time; time.sleep(60)"]); Path(sys.argv[1]).write_text(str(p.pid)); time.sleep(60)'
            with ManagedCompiler([sys.executable,'-c',code,str(pidfile)],root) as p:
                deadline=time.monotonic()+10
                while not pidfile.exists() and time.monotonic()<deadline:time.sleep(.02)
                self.assertTrue(pidfile.exists())
                from ctypes import wintypes as w
                import ctypes
                k=ctypes.WinDLL('kernel32',use_last_error=True)
                k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
                k.WaitForSingleObject.argtypes=[w.HANDLE,w.DWORD];k.WaitForSingleObject.restype=w.DWORD
                k.CloseHandle.argtypes=[w.HANDLE]
                child=k.OpenProcess(0x100000,False,int(pidfile.read_text()))
                self.assertTrue(child)
                try:
                    p.terminate_tree();p.communicate(timeout=10)
                    self.assertEqual(k.WaitForSingleObject(child,5000),0)
                finally:k.CloseHandle(child)


if __name__=='__main__':unittest.main()
