"""Source packages must exclude Git metadata, local inputs and output folders."""
from pathlib import Path
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile


class SourcePackage(unittest.TestCase):
    def test_only_tracked_files_enter_source_package(self):
        script = Path(__file__).resolve().parents[1] / 'tools/prepare_desktop_packet.py'
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / 'source'
            (source / 'tools').mkdir(parents=True)
            (source / 'desktop').mkdir()
            shutil.copyfile(script, source / 'tools/prepare_desktop_packet.py')
            (source / 'desktop/package.json').write_text('{"version":"1.0.0"}')
            (source / 'builder.py').write_text('source = True\n')
            subprocess.run(['git', 'init', '-q', str(source)], check=True)
            subprocess.run(['git', '-C', str(source), 'add', 'tools', 'desktop', 'builder.py'], check=True)
            (source / '.git/local-only').write_text('GIT METADATA MUST STAY LOCAL')
            (source / 'game.love').write_bytes(b'LOCAL INPUT MUST STAY LOCAL')
            (source / 'build').mkdir()
            (source / 'build/output.gbc').write_bytes(b'BUILD OUTPUT MUST STAY LOCAL')
            contract = root / 'contract.json'
            contract.write_text('{"schema":1}')
            output = root / 'package'
            subprocess.run([sys.executable, str(source / 'tools/prepare_desktop_packet.py'),
                            '--base-contract', str(contract), '--output', str(output)],
                           check=True, capture_output=True)
            expected = {'tools/prepare_desktop_packet.py', 'desktop/package.json', 'builder.py'}
            with zipfile.ZipFile(output / 'initial-source.zip') as archive:
                self.assertEqual(set(archive.namelist()), expected)
            manifest = json.loads((output / 'SOURCE-MANIFEST.json').read_text())
            self.assertEqual({row['path'] for row in manifest['files']}, expected)
