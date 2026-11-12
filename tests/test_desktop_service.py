import json
from pathlib import Path
import tempfile
import threading
import unittest
from unittest.mock import patch, Mock
from builder.desktop_service import Service
from builder import update
from builder.archive import InputError

class DesktopServiceTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.events=[];self.service=Service(Path(self.tmp.name)/'library',Path(self.tmp.name)/'contract.json',self.events.append)
    def call(self, command, **args):
        self.service.accept({'id':'test','command':command,'args':args})
        self.service.thread.join(3)
        self.assertFalse(self.service.thread.is_alive())
        return self.events[-1]
    def test_missing_install_explains_no_build_without_content(self):
        with patch('builder.desktop_service.local_package.load',return_value=({'source_install':{'archive_sha256':'a','manifest_sha256':'b'}},None)),patch('builder.desktop_service.discovery.discover',return_value={'candidates':[]}),patch('builder.desktop_service.discovery.select_supported',side_effect=InputError('No supported install')):
            result=self.call('discover')
        self.assertEqual(result['type'],'discovery');self.assertEqual(result['status'],'not_found')
        self.assertEqual(self.call('build')['type'],'error')
    def test_unsupported_manual_location_cannot_set_game(self):
        with patch('builder.desktop_service.local_package.load',return_value=({'source_install':{'x':'a'}},None)),patch('builder.desktop_service.fingerprint',return_value={'x':'b'}):
            result=self.call('locate',path=self.tmp.name)
        self.assertEqual(result['type'],'error');self.assertIsNone(self.service.game)
    def test_busy_command_rejected_and_cancel_immediate(self):
        release=threading.Event()
        self.service.operation=lambda message: release.wait(2)
        self.service.accept({'id':1,'command':'discover'})
        self.service.accept({'id':2,'command':'build'})
        self.assertEqual(self.events[-1]['type'],'error')
        self.service.accept({'id':3,'command':'cancel'})
        self.assertTrue(self.service.cancel.is_set());release.set();self.service.thread.join()
    def test_export_does_not_overwrite_existing_destination(self):
        target=Path(self.tmp.name)/'existing.gbc';target.write_bytes(b'preserve')
        result=self.call('export',destination=str(target))
        self.assertEqual(result['type'],'error');self.assertEqual(target.read_bytes(),b'preserve')
    def test_verified_same_version_reuses_only_matching_rom(self):
        self.service.game=Path(self.tmp.name)
        rom=Path(self.tmp.name)/'existing.gbc';rom.write_bytes(b'rom')
        metadata={'source_install':{'archive_sha256':'a'*64,'manifest_sha256':'b'*64},'version':'0.3.4-rc.1','expected_rom_sha256':update.digest(b'rom')}
        with patch('builder.desktop_service.local_package.load',return_value=(metadata,None)),patch('builder.desktop_service.fingerprint',return_value=metadata['source_install']),patch.object(self.service.library,'state',return_value={'active':metadata['version']}),patch.object(self.service.library,'active_rom',return_value=rom):
            result=self.call('build')
            self.assertTrue(result['reused'])
            rom.write_bytes(b'tampered');self.assertEqual(self.call('build')['type'],'error')

if __name__=='__main__':unittest.main()
