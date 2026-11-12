"""Compiler ownership: POSIX process groups, Windows kill-on-close Job Objects.

The Windows branch requires native Windows validation; portable tests do not qualify it.
"""
from __future__ import annotations
import ctypes
import os
import signal
import subprocess
from pathlib import Path
from typing import Any


class WindowsJob:
    """Assign a suspended compiler before it can create descendants, then resume it."""
    def __init__(self) -> None:
        if os.name != "nt":
            raise RuntimeError("Windows Job Objects require a native Windows host")
        from ctypes import wintypes as w
        k = self.k = ctypes.WinDLL("kernel32", use_last_error=True)
        class Basic(ctypes.Structure):
            _fields_ = [("process_time",ctypes.c_longlong),("job_time",ctypes.c_longlong),("flags",w.DWORD),
                        ("min_working",ctypes.c_size_t),("max_working",ctypes.c_size_t),("active",w.DWORD),
                        ("affinity",ctypes.c_size_t),("priority",w.DWORD),("scheduling",w.DWORD)]
        class IO(ctypes.Structure):
            _fields_ = [(name,ctypes.c_ulonglong) for name in ("read_ops","write_ops","other_ops","read_bytes","write_bytes","other_bytes")]
        class Extended(ctypes.Structure):
            _fields_ = [("basic",Basic),("io",IO),("process_memory",ctypes.c_size_t),("job_memory",ctypes.c_size_t),
                        ("peak_process",ctypes.c_size_t),("peak_job",ctypes.c_size_t)]
        k.CreateJobObjectW.argtypes=[ctypes.c_void_p,w.LPCWSTR];k.CreateJobObjectW.restype=w.HANDLE
        k.SetInformationJobObject.argtypes=[w.HANDLE,ctypes.c_int,ctypes.c_void_p,w.DWORD];k.SetInformationJobObject.restype=w.BOOL
        k.AssignProcessToJobObject.argtypes=[w.HANDLE,w.HANDLE];k.AssignProcessToJobObject.restype=w.BOOL
        k.TerminateJobObject.argtypes=[w.HANDLE,w.UINT];k.TerminateJobObject.restype=w.BOOL
        k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle.restype=w.BOOL
        self.handle=k.CreateJobObjectW(None,None)
        if not self.handle:raise ctypes.WinError(ctypes.get_last_error())
        info=Extended();info.basic.flags=0x2000  # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        if not k.SetInformationJobObject(self.handle,9,ctypes.byref(info),ctypes.sizeof(info)):
            error=ctypes.WinError(ctypes.get_last_error());self.close();raise error

    def attach_and_resume(self, process: subprocess.Popen[str]) -> None:
        from ctypes import wintypes as w
        k=self.k
        if not k.AssignProcessToJobObject(self.handle,int(process._handle)):
            raise ctypes.WinError(ctypes.get_last_error())
        class Thread(ctypes.Structure):
            _fields_=[("size",w.DWORD),("usage",w.DWORD),("tid",w.DWORD),("pid",w.DWORD),
                      ("base",w.LONG),("delta",w.LONG),("flags",w.DWORD)]
        k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE
        for name in ("Thread32First","Thread32Next"):
            fn=getattr(k,name);fn.argtypes=[w.HANDLE,ctypes.POINTER(Thread)];fn.restype=w.BOOL
        k.OpenThread.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenThread.restype=w.HANDLE
        k.ResumeThread.argtypes=[w.HANDLE];k.ResumeThread.restype=w.DWORD
        snapshot=k.CreateToolhelp32Snapshot(4,0)
        if snapshot==ctypes.c_void_p(-1).value:raise ctypes.WinError(ctypes.get_last_error())
        try:
            row=Thread();row.size=ctypes.sizeof(row);more=k.Thread32First(snapshot,ctypes.byref(row))
            while more:
                if row.pid==process.pid:
                    thread=k.OpenThread(2,False,row.tid)
                    if not thread:raise ctypes.WinError(ctypes.get_last_error())
                    try:
                        if k.ResumeThread(thread)==0xffffffff:raise ctypes.WinError(ctypes.get_last_error())
                    finally:k.CloseHandle(thread)
                    return
                more=k.Thread32Next(snapshot,ctypes.byref(row))
            raise RuntimeError("Suspended compiler thread not found; refusing unowned launch")
        finally:k.CloseHandle(snapshot)

    def terminate(self) -> None:
        if self.handle and not self.k.TerminateJobObject(self.handle,1):
            raise ctypes.WinError(ctypes.get_last_error())

    def close(self) -> None:
        if self.handle:
            self.k.CloseHandle(self.handle);self.handle=None


class ManagedCompiler:
    def __init__(self, command: list[str], cwd: Path, *, env: dict[str,str] | None = None) -> None:
        self.job: WindowsJob | None = WindowsJob() if os.name=="nt" else None
        kwargs: dict[str, Any] = {"creationflags":0x4|0x200|0x08000000} if self.job else {"start_new_session":True}
        try:
            self.process=subprocess.Popen(command,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,**kwargs)
            if self.job:self.job.attach_and_resume(self.process)
        except BaseException as original:
            try:
                if hasattr(self,"process"):
                    try:self.process.kill()
                    except Exception as cleanup:original.add_note("Failed-start kill cleanup: " + repr(cleanup))
            finally:
                if self.job:
                    try:self.job.close()
                    except Exception as cleanup:original.add_note("Failed-start Job close: " + repr(cleanup))
            if hasattr(self,"process"):
                try:self.process.communicate(timeout=5)
                except Exception as cleanup:original.add_note("Failed-start pipe cleanup: " + repr(cleanup))
            raise

    @property
    def returncode(self) -> int | None:return self.process.returncode

    def communicate(self, timeout: float | None = None) -> tuple[str,str]:
        return self.process.communicate() if timeout is None else self.process.communicate(timeout=timeout)

    def terminate_tree(self) -> None:
        if self.job:self.job.terminate()
        else:
            try:os.killpg(self.process.pid,signal.SIGKILL)
            except ProcessLookupError:pass

    def close(self) -> None:
        if self.job:self.job.close()

    def __enter__(self) -> ManagedCompiler:return self
    def __exit__(self, *_: object) -> None:self.close()
