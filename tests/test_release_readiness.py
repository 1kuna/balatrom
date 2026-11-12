"""Release boundary regressions, using declared synthetic data only."""
import json
import tempfile
import unittest
from pathlib import Path
from builder import update as u
from test_update import fixture, NOW, rom_bytes

class ReleaseReadiness(unittest.TestCase):
    def test_missing_save_metadata_fails_before_export_files(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); lib = u.Updater(root / 'library')
            version = lib.root / 'v1.0.0'; version.mkdir()
            (version / 'balatrom.gbc').write_bytes(rom_bytes())
            lib.state_path.write_text(json.dumps({'active':'1.0.0','previous':None,'highest':'1.0.0','versions':{'1.0.0':{'rom_sha256':u.digest(rom_bytes()),'carried_save':True}}}))
            with self.assertRaisesRegex(u.UpdateError, 'save metadata'):
                lib.export(root / 'export.gbc', save_adapter='pyboy')
            self.assertFalse((root / 'export.gbc').exists())
            self.assertFalse((root / 'export.gbc.ram').exists())

    def test_foreign_version_race_does_not_receive_failure_receipt(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);lib=u.Updater(root/'library'); game=root/'owned/game';game.parent.mkdir();game.write_bytes(b'fixture')
            _,_,reader,_=fixture();metadata=u.discover('0.0.0',reader=reader,now=NOW,repository_id=123)
            foreign=lib.root/('v'+metadata['version'])
            def build(source, game, output, cancel):
                output.mkdir();rom=output/'balatrom.gbc';rom.write_bytes(rom_bytes())
                foreign.mkdir();(foreign/'keep').write_bytes(b'other-owner')
                return rom
            with self.assertRaisesRegex(u.UpdateError,'Version folder already exists'):
                lib.apply(metadata, approved=True,game_path=game,fingerprint=lambda p:metadata['source_install'],build=build,reader=reader)
            self.assertEqual((foreign/'keep').read_bytes(),b'other-owner')
            self.assertFalse((foreign/'FAILURE.json').exists())
            self.assertEqual(len(list(lib.root.glob('pending-*/FAILURE.json'))),1)
            self.assertIsNone(lib.state()['active'])
