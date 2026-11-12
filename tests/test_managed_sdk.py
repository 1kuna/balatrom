from pathlib import Path
import hashlib
import io
import tarfile
import tempfile
import threading
import unittest
import zipfile
from unittest.mock import patch
from builder.managed_sdk import ensure, verify, compiler_session, _Redirect
from builder.update import UpdateError, Cancelled


class ManagedTools(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);self.cache=self.root/'tools';self.cancel=threading.Event();self.messages=[]
        scope=patch('builder.managed_sdk.__file__',str(self.root/'backend/builder/managed_sdk.py'));scope.start();self.addCleanup(scope.stop)
        self.data=b'#!/bin/false\n';self.payload,self.record=self.fixture()

    def fixture(self, format='tar.gz', extra=None):
        files={'bin/lcc':self.data,'licenses/LICENSE_test':b'Synthetic license fixture\n'}
        out=io.BytesIO()
        if format=='tar.gz':
            with tarfile.open(fileobj=out,mode='w:gz') as t:
                for name,data in files.items():
                    info=tarfile.TarInfo('gbdk/'+name);info.size=len(data);info.mode=0o755 if name=='bin/lcc' else 0o644;t.addfile(info,io.BytesIO(data))
                if extra:t.addfile(extra,io.BytesIO(b'x') if extra.size else None)
        else:
            with zipfile.ZipFile(out,'w') as z:
                for name,data in files.items():z.writestr('gbdk/'+name,data)
        raw=out.getvalue();record={'cache_name':'synthetic-gbdk','compiler':'bin/lcc','format':format,'url':'https://github.com/gbdk-2020/gbdk-2020/releases/download/4.5.0/synthetic','archive_bytes':len(raw),'archive_sha256':hashlib.sha256(raw).hexdigest(),'files':[{'path':n,'bytes':len(d),'sha256':hashlib.sha256(d).hexdigest(),'mode':0o755 if n=='bin/lcc' else 0o644} for n,d in files.items()]}
        return raw,record

    def run_setup(self, **kwargs):
        values=dict(approved=True,cancel=self.cancel,progress=self.messages.append,record=self.record,reader=lambda url:io.BytesIO(self.payload));values.update(kwargs)
        return ensure(self.cache,**values)

    def test_first_build_downloads_verifies_and_prepares_without_PATH(self):
        compiler=self.run_setup();self.assertEqual(compiler.read_bytes(),self.data);self.assertTrue(verify(compiler.parent.parent,self.record));self.assertTrue(any('Creating your ROM' in m for m in self.messages))
        self.assertTrue((compiler.parent.parent/'licenses/LICENSE_test').is_file())

    def test_cached_verified_tools_need_no_network(self):
        first=self.run_setup()
        self.assertEqual(self.run_setup(approved=False,reader=lambda _:self.fail('Network used with valid cache')),first)

    def test_no_user_approval_never_downloads(self):
        with self.assertRaisesRegex(UpdateError,'approval'):self.run_setup(approved=False,reader=lambda _:self.fail('Unapproved download'))
        self.assertFalse(self.cache.exists())

    def test_corrupt_download_preserves_game_save_and_retry_succeeds(self):
        original=self.root/'personal-save';original.write_bytes(b'untouched')
        with self.assertRaises(UpdateError):self.run_setup(reader=lambda _:io.BytesIO(b'bad'))
        self.assertFalse((self.cache/'synthetic-gbdk').exists());self.assertEqual(original.read_bytes(),b'untouched');self.run_setup()

    def test_offline_then_retry(self):
        def offline(_):raise OSError('declared offline fixture')
        with self.assertRaisesRegex(UpdateError,'Retry Build'):self.run_setup(reader=offline)
        self.assertFalse(list(self.cache.glob('pending-tools-*')));self.run_setup()

    def test_cancel_mid_download_leaves_no_partial_install(self):
        def progress(message):self.cancel.set()
        with self.assertRaises(Cancelled):self.run_setup(progress=progress)
        self.assertFalse((self.cache/'synthetic-gbdk').exists());self.assertFalse(list(self.cache.glob('pending-tools-*')))
        self.cancel.clear();self.run_setup()

    def test_damaged_cache_is_preserved_and_repaired(self):
        compiler=self.run_setup();compiler.write_bytes(b'changed')
        self.run_setup();old=list(self.cache.glob('preserved-invalid-*'));self.assertEqual(len(old),1);self.assertEqual((old[0]/'bin/lcc').read_bytes(),b'changed')

    def test_traversal_member_refused(self):
        member=tarfile.TarInfo('gbdk/../../escape');member.size=1;raw,record=self.fixture(extra=member)
        with self.assertRaisesRegex(UpdateError,'unsafe path'):self.run_setup(record=record,reader=lambda _:io.BytesIO(raw))
        self.assertFalse((self.root/'escape').exists())

    def test_symlink_member_refused(self):
        member=tarfile.TarInfo('gbdk/link');member.type=tarfile.SYMTYPE;member.linkname='/tmp/other';raw,record=self.fixture(extra=member)
        with self.assertRaisesRegex(UpdateError,'Linked or special'):self.run_setup(record=record,reader=lambda _:io.BytesIO(raw))

    def test_windows_zip_recipe_is_portable_not_native_runtime_proof(self):
        raw,record=self.fixture(format='zip');self.assertTrue(self.run_setup(record=record,reader=lambda _:io.BytesIO(raw)).is_file())

    def test_redirect_cannot_leave_official_https_hosts(self):
        import urllib.request
        request=urllib.request.Request('https://github.com/test')
        for target in ('http://github.com/test','https://example.org/sdk','https://user:pass@github.com/sdk'):
            with self.assertRaises(UpdateError):_Redirect().redirect_request(request,None,302,'redirect',{},target)

    def test_short_verified_execution_copy_is_cleaned_and_cache_remains(self):
        with compiler_session(self.cache,approved=True,cancel=self.cancel,progress=self.messages.append,record=self.record,reader=lambda _:io.BytesIO(self.payload)) as compiler:
            self.assertLess(len(str(compiler.parent.parent)),81)
            self.assertEqual(compiler.read_bytes(),self.data)
            self.assertNotEqual(compiler,self.cache/'synthetic-gbdk/bin/lcc')
            temporary=compiler.parent.parent
        self.assertFalse(temporary.exists());self.assertTrue(verify(self.cache/'synthetic-gbdk',self.record))


if __name__=='__main__':unittest.main()
