import json
from pathlib import Path
import tempfile
import unittest
from builder.local_package import load
from builder import update as u
from test_update import fixture,source_zip,rom_bytes
import test_update as fixtures

class LocalPackageContracts(unittest.TestCase):
    def test_local_packet_hash_checked_without_network(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);raw=source_zip();(root/'initial-source.zip').write_bytes(raw)
            _,contract,_,_=fixture();contract.update(schema=1,scope='private-reviewed-owned-build',source_sha256=u.digest(raw))
            path=root/'initial-contract.json';path.write_text(json.dumps(contract))
            metadata,reader=load(path)
            self.assertEqual(metadata['channel'],'local')
            self.assertEqual(reader('local-reviewed-source',u.MAX_PACKAGE,None),raw)
            (root/'initial-source.zip').write_bytes(b'changed')
            with self.assertRaisesRegex(u.UpdateError,'integrity'):load(path)

    def test_internal_bundle_resource_symlinks_are_hash_checked(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);resources=root/'Resources';frameworks=root/'Frameworks';resources.mkdir();frameworks.mkdir()
            raw=source_zip();(resources/'initial-source.zip').write_bytes(raw)
            _,contract,_,_=fixture();contract.update(schema=1,scope='private-reviewed-owned-build',source_sha256=u.digest(raw))
            (resources/'initial-contract.json').write_text(json.dumps(contract))
            for name in ('initial-contract.json','initial-source.zip'):
                (frameworks/name).symlink_to(resources/name)
            metadata,_=load(frameworks/'initial-contract.json')
            self.assertEqual(metadata['channel'],'local')
            outside=root/'outside';outside.mkdir();(outside/'initial-source.zip').write_bytes(raw)
            (frameworks/'initial-source.zip').unlink();(frameworks/'initial-source.zip').symlink_to(outside/'initial-source.zip')
            with self.assertRaisesRegex(u.UpdateError,'integrity'):
                load(frameworks/'initial-contract.json')

class InitialSaveContracts(unittest.TestCase):
    setUp=fixtures.UpdateContracts.setUp
    tearDown=fixtures.UpdateContracts.tearDown
    build=fixtures.UpdateContracts.build
    def test_original_rom_provenance_preserves_first_install_save(self):
        _,metadata,reader,_=fixture()
        metadata={**metadata,'source':{'size':len(source_zip()),'digest':'sha256:'+u.digest(source_zip()),'browser_download_url':'synthetic'}}
        metadata['save'].update(compatible_rom_sha256=[u.digest(rom_bytes())],export_adapters=['pyboy'])
        original=self.root/'original.gbc';original.write_bytes(rom_bytes())
        save=self.root/'original.gbc.ram';save.write_bytes(b'SAVE')
        self.library.apply(metadata,approved=True,game_path=self.game,fingerprint=lambda p:metadata['source_install'],build=self.build,
                           reader=lambda *args:source_zip(),save_path=save,previous_rom=original)
        target=self.root/'new.gbc';self.library.export(target,save_adapter='pyboy')
        self.assertEqual(Path(str(target)+'.ram').read_bytes(),b'SAVE')
        self.assertEqual(save.read_bytes(),b'SAVE');self.assertEqual(original.read_bytes(),rom_bytes())
