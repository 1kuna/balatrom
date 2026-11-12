"""Real missing-Pillow path; no compiler, game, network or GUI."""
import os
import subprocess
import sys
import tempfile
import unittest
import venv
from pathlib import Path

INSTALL = "python -m pip install --only-binary=:all: Pillow==12.3.0"

class OwnedDependencyGuidance(unittest.TestCase):
    def test_missing_pillow_in_real_empty_environment(self):
        source = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            venv.EnvBuilder(with_pip=False, symlinks=os.name != "nt").create(root / "empty")
            exe = root / "empty" / ("Scripts/python.exe" if sys.platform == "win32" else "bin/python")
            code = "import sys; sys.path.insert(0," + repr(str(source)) + "); from builder.owned_install import build_owned; build_owned('/unread-game','/unread-packet'," + repr(str(root / "output")) + ",'/unread-sdk',preflight=True)"
            result = subprocess.run([str(exe), "-I", "-B", "-c", code], capture_output=True, text=True, timeout=30)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("InputError", result.stderr)
            self.assertIn(INSTALL, result.stderr)
            self.assertNotIn("install requirements.txt", result.stderr)
            self.assertFalse((root / "output").exists())

    def test_install_instruction_is_documented(self):
        guide = (Path(__file__).resolve().parents[1] / "BUILDING.md").read_text()
        self.assertIn(INSTALL, guide)
        self.assertIn("same Python", guide)
        self.assertIn("does not install", guide)
