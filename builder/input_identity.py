"""Identify a supported install; only the Windows executable wrapper may differ.

The sorted archive-member manifest (every name, size and SHA-256) must match
the registered one exactly. The raw archive fingerprint is always recorded;
never substitute the Mac hash. Not yet tested on a real Windows install.
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
