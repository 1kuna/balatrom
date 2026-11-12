"""Exact owned-content identity; allow only the Windows executable wrapper to vary.

The entire sorted archive-member manifest (every name, size and SHA256) must
remain identical to the registered cohort. No game content or version is relaxed.
The real raw archive fingerprint is always recorded; never substitute the Mac hash.
Windows runtime/input compatibility remains untested until used on Windows.
"""
import re
import sys

def matches(actual, required, *, platform=None):
    fields=('archive_sha256','manifest_sha256')
    if any(not isinstance(actual.get(k),str) or not actual[k] or not isinstance(required.get(k),str) or not required[k] for k in fields):
        return False
    if all(actual[k]==required[k] for k in fields):
        return True
    for field in fields:
        for value in (actual[field],required[field]):
            if not re.fullmatch(r'[0-9a-f]{64}',value):
                return False
    return ((platform or sys.platform) == 'win32' and actual['manifest_sha256']==required['manifest_sha256'])
