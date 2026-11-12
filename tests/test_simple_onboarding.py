from pathlib import Path
import json
import tempfile
import threading
import unittest
from unittest.mock import Mock,patch
from PySide6.QtWidgets import QApplication,QFileDialog
from builder.update_gui import Window


class SimpleOnboarding(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.app=QApplication.instance() or QApplication([])
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name)
        self.contract=self.root/'initial-contract.json';self.contract.write_text(json.dumps({'version':'0.3.3-rc.1','expected_rom_sha256':'0'*64,'source_install':{'archive_sha256':'a'*64,'manifest_sha256':'b'*64}}))
        self.window=Window(self.root/'versions',self.contract,autodiscover=False);self.addCleanup(self.window.close)

    def test_missing_install_explains_install_and_offers_only_fallback(self):
        self.window.show_installation({'path':None,'status':'not_found'})
        self.assertIn('Install Balatro in Steam',self.window.status.text());self.assertFalse(self.window.first_build.isEnabled());self.assertFalse(self.window.locate.isHidden())

    def test_found_but_unqualified_is_not_reported_as_missing(self):
        self.window.show_installation({'path':None,'status':'found_unqualified'})
        self.assertIn('found',self.window.status.text());self.assertIn('not supported',self.window.status.text());self.assertFalse(self.window.first_build.isEnabled())

    def test_supported_install_enables_one_build_without_compiler_picker(self):
        self.window.show_installation({'path':str(self.root/'declared-Balatro.app'),'status':'supported'})
        self.assertTrue(self.window.first_build.isEnabled());self.assertTrue(self.window.locate.isHidden());self.assertIn('free build tools automatically',self.window.status.text())
        self.window.offer={'version':'0.3.3-rc.1'};self.window.local_reader=Mock();self.window.start=Mock()
        with patch('builder.local_package.load',return_value=(self.window.offer,self.window.local_reader)),patch.object(QFileDialog,'getOpenFileName',side_effect=AssertionError('Unexpected prerequisite picker')):
            self.window.first_build.click()
        operation=self.window.start.call_args.args[0]
        with patch('builder.managed_sdk.compiler_session') as tools,patch('builder.update_gui.build_adapter',return_value='declared builder') as adapter,patch.object(self.window.library,'apply',return_value='declared-ROM') as apply:
            tools.return_value.__enter__.return_value=self.root/'managed-tools/bin/lcc'
            self.assertEqual(operation(threading.Event(),lambda _:None),'declared-ROM')
        self.assertTrue(tools.call_args.kwargs['approved']);self.assertEqual(adapter.call_args.args[0],self.root/'managed-tools/bin/lcc');self.assertIsNone(apply.call_args.kwargs['save_path'])

    def test_build_failure_keeps_existing_library(self):
        original=self.window.library.state();self.window.show_installation({'path':str(self.root/'game'),'status':'supported'})
        self.window.offer={'version':'0.3.3-rc.1'};self.window.start=Mock();self.window.choose_and_apply();operation=self.window.start.call_args.args[0]
        with patch('builder.managed_sdk.compiler_session',side_effect=OSError('declared network failure')),patch.object(self.window.library,'apply') as apply:
            with self.assertRaises(OSError):operation(threading.Event(),lambda _:None)
        apply.assert_not_called();self.assertEqual(self.window.library.state(),original)

    def test_success_queues_destination_prompt(self):
        self.window.build_complete(self.root/'ROM');self.assertTrue(self.window.export_after_build)
        self.assertIn('choose where',self.window.status.text().lower())


if __name__=='__main__':unittest.main()
