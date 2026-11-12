"""Conservative compiler concurrency from local CPU/RAM, without a process census.

Available memory is a snapshot, not a guarantee. Failed/unknown measurements
fall back to one worker. These thresholds reserve headroom; they are not minimum
hardware requirements or claims about the speed of an untested computer.
"""
import ctypes
import os
import sys

GIB = 1024 ** 3


class WindowsMemoryStatus(ctypes.Structure):
    _fields_=[('length',ctypes.c_uint32),('load',ctypes.c_uint32)]+[(name,ctypes.c_uint64) for name in ('total_physical','available_physical','total_pagefile','available_pagefile','total_virtual','available_virtual','available_extended')]


def windows_memory_snapshot(kernel=None):
    kernel=kernel or ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.GlobalMemoryStatusEx.argtypes=[ctypes.POINTER(WindowsMemoryStatus)]
    kernel.GlobalMemoryStatusEx.restype=ctypes.c_int
    status=WindowsMemoryStatus();status.length=ctypes.sizeof(status)
    if not kernel.GlobalMemoryStatusEx(ctypes.pointer(status)):
        raise OSError('GlobalMemoryStatusEx failed')
    return status.total_physical,status.available_physical


def memory_snapshot():
    if sys.platform=='win32':
        return windows_memory_snapshot()
    physical = os.sysconf('SC_PHYS_PAGES') * os.sysconf('SC_PAGE_SIZE')
    if sys.platform != 'darwin':
        return physical, None
    library = ctypes.CDLL('/usr/lib/libSystem.B.dylib')
    library.mach_host_self.restype = ctypes.c_uint
    library.host_statistics64.argtypes = [ctypes.c_uint, ctypes.c_int,
                                         ctypes.POINTER(ctypes.c_uint32),
                                         ctypes.POINTER(ctypes.c_uint32)]
    words = (ctypes.c_uint32 * 64)()
    count = ctypes.c_uint32(64)
    host = library.mach_host_self()
    try:
        # HOST_VM_INFO64 starts free_count, active_count, inactive_count,
        # wire_count. Free + inactive is a bounded reclaimability estimate.
        if library.host_statistics64(host, 4, words, ctypes.byref(count)) != 0 or count.value < 4:
            return physical, None
        return physical, (words[0] + words[2]) * os.sysconf('SC_PAGE_SIZE')
    finally:
        library.mach_port_deallocate.argtypes = [ctypes.c_uint, ctypes.c_uint]
        library.mach_port_deallocate(ctypes.c_uint.in_dll(library, 'mach_task_self_').value, host)


def choose_workers(cpu_count, physical, available):
    if physical is None or available is None:
        return 1
    if cpu_count >= 8 and physical >= 16 * GIB and available >= 8 * GIB:
        return 4
    if cpu_count >= 4 and physical >= 8 * GIB and available >= 4 * GIB:
        return 2
    return 1


def snapshot():
    cpu_count = os.cpu_count() or 1
    try:
        physical, available = memory_snapshot()
    except (OSError, ValueError, AttributeError):
        physical, available = None, None
    return {'workers': choose_workers(cpu_count, physical, available),
            'cpu_count': cpu_count, 'physical_bytes': physical,
            'free_plus_inactive_estimate_bytes': available,
            'measurement_source': 'GlobalMemoryStatusEx' if sys.platform=='win32' else 'host memory snapshot',
            'scope': 'Local snapshot and conservative scheduling policy; not a hardware requirement'}
