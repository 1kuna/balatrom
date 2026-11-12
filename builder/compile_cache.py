"""Optional local cache of compiler output. It never replaces input verification.

A context binds every reconstructed source/header, every SDK file, flags, bank
assignment and host ABI. Cache bytes are checked on every read. The caller still
links in descriptor order and accepts only the expected ROM hash.
"""
from pathlib import Path
import hashlib
import json
import os
import platform
import tempfile


def tree_digest(root):
    root = Path(root)
    rows = []
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            raise ValueError('Compiler cache context cannot contain symlinks')
        if path.is_file():
            rows.append([path.relative_to(root).as_posix(), hashlib.sha256(path.read_bytes()).hexdigest()])
    return hashlib.sha256(json.dumps(rows, separators=(',', ':')).encode()).hexdigest()


def context_key(view, sdk, descriptor):
    data = {'schema': 1, 'view': tree_digest(view), 'sdk': tree_digest(sdk),
            'descriptor': descriptor, 'host': [os.name, platform.machine(), platform.system()]}
    return hashlib.sha256(json.dumps(data, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


class ObjectCache:
    def __init__(self, root, context):
        self.root = Path(root)
        if self.root.is_symlink():
            raise ValueError('Compiler cache must be a regular private directory')
        self.root.mkdir(mode=0o700, parents=True, exist_ok=True)
        self.context = context

    def path(self, name):
        key = hashlib.sha256((self.context + ':' + name).encode()).hexdigest()
        return self.root / (key + '.json')

    def get(self, name):
        path = self.path(name)
        try:
            if path.is_symlink() or path.stat().st_size > 8 * 1024 * 1024:
                return None
            record = json.loads(path.read_text())
            raw = bytes.fromhex(record['object_hex'])
            if (record['context'] != self.context or record['name'] != name or
                    hashlib.sha256(raw).hexdigest() != record['sha256']):
                return None
            return raw
        except (OSError, ValueError, KeyError, TypeError):
            return None

    def put(self, name, raw):
        record = {'context': self.context, 'name': name,
                  'sha256': hashlib.sha256(raw).hexdigest(), 'object_hex': raw.hex()}
        fd, temporary = tempfile.mkstemp(prefix='.object-', dir=self.root)
        try:
            with os.fdopen(fd, 'w') as stream:
                json.dump(record, stream, separators=(',', ':'))
            os.replace(temporary, self.path(name))
        finally:
            Path(temporary).unlink(missing_ok=True)
