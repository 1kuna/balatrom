"""Synthetic-only updater contracts: no game, compiler, network or user saves."""
import copy
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import tempfile
import threading
import unittest
import zipfile

from builder import update as u

NOW = datetime(2026, 10, 12, tzinfo=timezone.utc)


def rom_bytes():
    data = bytearray(512)
    data[0x143] = 0x80
    return bytes(data)


def source_zip(extra=None):
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w") as archive:
        for name in ["builder/__main__.py", "engine/manifest.json", "compatibility/owned_profiles.json"]:
            archive.writestr(name, "synthetic fixture")
        for name, data in (extra or {}).items():
            info = zipfile.ZipInfo(name)
            info.filename = name  # Preserve malformed spellings even on Windows.
            archive.writestr(info, data)
    return stream.getvalue()


def fixture(version="1.0.0", preview=False, compatible=()):
    tag = "v" + version
    metadata = {"schema": 1, "version": version, "channel": "preview" if preview else "stable",
                "minimum_updater": "0.1.0", "changelog": "Synthetic update",
                "source_install": {"archive_sha256": "a" * 64, "manifest_sha256": "b" * 64},
                "source_asset": "balatrom-source.zip", "expected_rom_sha256": u.digest(rom_bytes()),
                "expires_at": "2026-10-13T00:00:00Z",
                "save": {"format_id": "synthetic-save-rng1", "bytes": 4,
                         "compatible_versions": list(compatible), "compatible_rom_sha256": [u.digest(rom_bytes())] if compatible else []}}
    source = source_zip()
    raw = json.dumps(metadata).encode()
    def asset(name, data):
        return {"name": name, "size": len(data), "digest": "sha256:" + u.digest(data),
                "browser_download_url": f"https://github.com/{u.REPOSITORY}/releases/download/{tag}/{name}"}
    release = {"tag_name": tag, "draft": False, "prerelease": preview,
               "published_at": "2026-10-11T00:00:00Z",
               "assets": [asset("balatrom-update.json", raw), asset("balatrom-source.zip", source)]}
    calls = []
    def reader(url, limit, cancel):
        calls.append(url)
        u.check_cancel(cancel)
        if url == u.REPOSITORY_API:
            return json.dumps({"id": 123, "full_name": u.REPOSITORY}).encode()
        if url.endswith("/latest"):
            return json.dumps(release).encode()
        if url.endswith("?per_page=100"):
            return json.dumps([release]).encode()
        if url.endswith("balatrom-update.json"):
            return raw
        return source
    return release, metadata, reader, calls


class UpdateContracts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.game = self.root / "owned" / "game.love"
        self.game.parent.mkdir()
        self.game.write_text("not retail data")
        self.library = u.Updater(self.root / "library")

    def tearDown(self):
        self.temporary.cleanup()

    def build(self, source, game, output, cancel):
        u.check_cancel(cancel)
        output.mkdir()
        rom = output / "balatrom.gbc"
        rom.write_bytes(rom_bytes())
        return rom

    def apply(self, version="1.0.0", compatible=(), **kwargs):
        _, _, reader, _ = fixture(version, compatible=compatible)
        metadata = u.discover("0.0.0", reader=reader, now=NOW, repository_id=123)
        if kwargs.get("save_path") and not kwargs.get("new_save") and "previous_rom" not in kwargs:
            kwargs["previous_rom"] = self.library.active_rom()
        return self.library.apply(metadata, approved=True, game_path=self.game,
                                  fingerprint=lambda p: {"archive_sha256": "a" * 64, "manifest_sha256": "b" * 64},
                                  build=self.build, reader=reader, **kwargs)

    def test_end_to_end_update_export_and_rollback_retain_both_versions(self):
        self.apply()
        self.apply("1.1.0")
        target = self.root / "balatrom-1.1.0.gbc"
        self.library.export(target)
        self.assertEqual(target.read_bytes(), rom_bytes())
        self.assertEqual(self.library.rollback(approved=True), "1.0.0")
        self.assertEqual(self.library.state()["highest"], "1.1.0")
        self.assertTrue((self.library.root / "v1.1.0" / "balatrom.gbc").exists())

    def test_no_download_or_build_without_explicit_approval(self):
        _, _, reader, calls = fixture()
        metadata = u.discover("0.0.0", reader=reader, now=NOW, repository_id=123)
        calls.clear()
        with self.assertRaisesRegex(u.UpdateError, "approval"):
            self.library.apply(metadata, approved=False, game_path=self.game,
                               fingerprint=lambda p: {}, build=self.build, reader=reader)
        self.assertEqual(calls, [])
        self.assertIsNone(self.library.state()["active"])

    def test_bad_fingerprint_and_hash_leave_active_version_unchanged(self):
        self.apply()
        _, _, reader, _ = fixture("1.1.0")
        metadata = u.discover("0.0.0", reader=reader, now=NOW, repository_id=123)
        with self.assertRaisesRegex(u.UpdateError, "not supported"):
            self.library.apply(metadata, approved=True, game_path=self.game,
                               fingerprint=lambda p: {}, build=self.build, reader=reader)
        def bad(url, limit, cancel):
            return b"tampered"
        with self.assertRaisesRegex(u.UpdateError, "SHA-256"):
            self.library.apply(metadata, approved=True, game_path=self.game,
                               fingerprint=lambda p: metadata["source_install"], build=self.build, reader=bad)
        self.assertEqual(self.library.state()["active"], "1.0.0")

    def test_build_failure_or_cancel_never_activates_partial_version(self):
        self.apply()
        _, _, reader, _ = fixture("1.1.0")
        metadata = u.discover("0.0.0", reader=reader, now=NOW, repository_id=123)
        cancel = threading.Event()
        def cancelled(source, game, output, flag):
            rom = self.build(source, game, output, flag)
            flag.set()
            return rom
        with self.assertRaises(u.Cancelled):
            self.library.apply(metadata, approved=True, game_path=self.game,
                               fingerprint=lambda p: metadata["source_install"], build=cancelled,
                               cancel=cancel, reader=reader)
        self.assertEqual(self.library.state()["active"], "1.0.0")
        self.assertFalse((self.library.root / "v1.1.0").exists())

    def test_stable_never_offers_preview_or_an_older_version(self):
        _, _, reader, _ = fixture("2.0.0-rc.1", preview=True)
        self.assertIsNone(u.discover("1.0.0", reader=reader, now=NOW, repository_id=123))
        _, _, reader, _ = fixture("1.0.0")
        self.assertIsNone(u.discover("2.0.0", reader=reader, now=NOW, repository_id=123))

    def test_expired_preview_is_skipped(self):
        _, _, reader, _ = fixture("2.0.0-rc.1", preview=True)
        later = datetime(2026, 10, 14, tzinfo=timezone.utc)
        self.assertIsNone(u.discover("1.0.0", "preview", reader=reader, now=later, repository_id=123))

    def test_save_copy_needs_exact_tested_version_compatibility(self):
        self.apply()
        save = self.root / "synthetic.sav"
        save.write_bytes(b"SAVE")
        with self.assertRaisesRegex(u.UpdateError, "compatibility"):
            self.apply("1.1.0", save_path=save)
        self.apply("1.1.0", compatible=("1.0.0",), save_path=save)
        self.assertEqual(save.read_bytes(), b"SAVE")
        self.assertEqual((self.library.root / "v1.1.0" / "save-backup.bin").read_bytes(), b"SAVE")
        self.library.export(self.root / "new-version.gbc")
        self.assertEqual((self.root / "new-version.save-backup").read_bytes(), b"SAVE")

    def test_unknown_save_contract_can_start_new_without_modifying_old_save(self):
        self.apply()
        save = self.root / "synthetic.sav"
        save.write_bytes(b"SAVE")
        self.apply("2.0.0", save_path=save, new_save=True)
        self.assertFalse(self.library.state()["versions"]["2.0.0"]["carried_save"])
        self.assertEqual(save.read_bytes(), b"SAVE")

    def test_export_refuses_basename_with_existing_save(self):
        self.apply()
        target = self.root / "old.gbc"
        save = target.with_suffix(".sav")
        save.write_bytes(b"existing progression")
        with self.assertRaisesRegex(u.UpdateError, "existing save"):
            self.library.export(target)
        self.assertFalse(target.exists())
        self.assertEqual(save.read_bytes(), b"existing progression")

    def test_pyboy_ram_and_rtc_neighbors_block_export_without_modification(self):
        self.apply()
        for suffix in (".ram", ".rtc"):
            with self.subTest(suffix=suffix):
                target = self.root / ("pyboy" + suffix + ".gbc")
                sidecar = Path(str(target) + suffix)
                sidecar.write_bytes(b"existing progression")
                with self.assertRaisesRegex(u.UpdateError, "existing save"):
                    self.library.export(target)
                self.assertFalse(target.exists())
                self.assertEqual(sidecar.read_bytes(), b"existing progression")

    def test_old_carried_save_receipt_fails_closed_with_clear_error(self):
        self.apply()
        save = self.root / "synthetic.sav"
        save.write_bytes(b"SAVE")
        self.apply("1.1.0", compatible=("1.0.0",), save_path=save)
        state = self.library.state()
        del state["versions"]["1.1.0"]["save_sha256"]
        u.atomic_json(self.library.state_path, state)
        target = self.root / "old-receipt.gbc"
        with self.assertRaisesRegex(u.UpdateError, "integrity record"):
            self.library.export(target)
        self.assertFalse(target.exists())
        self.assertEqual(save.read_bytes(), b"SAVE")

    def test_export_cannot_overwrite_existing_rom(self):
        self.apply()
        path = self.root / "existing.gbc"
        path.write_bytes(b"existing")
        with self.assertRaises(FileExistsError):
            self.library.export(path)
        self.assertEqual(path.read_bytes(), b"existing")

    def test_zip_traversal_and_case_collisions_are_rejected(self):
        for entries in [{"../escape": "bad"}, {"CON.txt": "bad"}, {"../": "bad"},
                        {"Foo.py": "a", "foo.py": "b"}, {"a\\b": "bad"}]:
            with self.subTest(entries=entries):
                with tempfile.TemporaryDirectory() as out:
                    with self.assertRaises(u.UpdateError):
                        u.extract_source(source_zip(entries), Path(out), threading.Event())
        self.assertFalse((self.root / "escape").exists())

    def test_raw_nul_archive_name_is_rejected_before_extraction(self):
        with tempfile.TemporaryDirectory() as td:
            destination = Path(td) / "source"
            destination.mkdir()
            with self.assertRaises(u.UpdateError):
                u.extract_source(source_zip({"safe\x00hidden": "bad"}), destination, threading.Event())
            self.assertFalse((destination / "safe").exists())

    def test_trusted_url_and_duplicate_metadata_fail_closed(self):
        for url in ["http://github.com/1kuna/balatrom/releases/download/v1/a.zip",
                    "https://evil.test/a.zip", "https://github.com/other/repo/releases/download/v1/a.zip"]:
            with self.assertRaises(u.UpdateError):
                u.trusted_url(url)
        with self.assertRaisesRegex(u.UpdateError, "Duplicate"):
            u.parse_json(b'{"schema":1,"schema":2}')

    def test_repository_identity_must_be_pinned_and_match(self):
        _, _, reader, _ = fixture()
        with self.assertRaisesRegex(u.UpdateError, "qualified"):
            u.discover("0.0.0", reader=reader)
        with self.assertRaisesRegex(u.UpdateError, "identity changed"):
            u.discover("0.0.0", reader=reader, repository_id=999)
        with self.assertRaises(u.UpdateError):
            u.trusted_url("https://api.github.com/repos/1kuna/legacy/releases", redirect=True)

    def test_no_release_is_distinct_from_offline(self):
        _, _, reader, _ = fixture()
        def missing(url, limit, cancel):
            if url.endswith("/latest"):
                raise u.NoRelease("none")
            return reader(url, limit, cancel)
        self.assertIsNone(u.discover("0.0.0", reader=missing, repository_id=123))
        def offline(url, limit, cancel):
            raise u.UpdateError("offline")
        with self.assertRaisesRegex(u.UpdateError, "offline"):
            u.discover("0.0.0", reader=offline, repository_id=123)

    def test_companion_collision_removes_new_export_only(self):
        self.apply()
        save = self.root / "synthetic.sav"
        save.write_bytes(b"SAVE")
        self.apply("1.1.0", compatible=("1.0.0",), save_path=save)
        target = self.root / "new.gbc"
        target.with_suffix(".save-backup").write_bytes(b"existing")
        with self.assertRaises(FileExistsError):
            self.library.export(target)
        self.assertFalse(target.exists())
        self.assertEqual(target.with_suffix(".save-backup").read_bytes(), b"existing")

    def test_concurrent_update_lock_preserves_state(self):
        with self.library.lock():
            with self.assertRaisesRegex(u.UpdateError, "already running"):
                with self.library.lock():
                    pass
        self.assertIsNone(self.library.state()["active"])


if __name__ == "__main__":
    unittest.main()
