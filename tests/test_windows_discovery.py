"""Portable fixtures only: fake registry + temporary POSIX filesystem as Windows stand-in."""
from pathlib import Path
import os
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

from builder.archive import InputError
from builder.windows_discovery import discover_windows, windows_steam_roots


class FakeArchive:
    fingerprints = {}
    def __init__(self, path):self.path=Path(path)
    def __enter__(self):return self
    def __exit__(self,*args):pass
    def fingerprint(self):
        value=next(v for k,v in self.fingerprints.items() if k.casefold()==str(self.path).casefold())
        if isinstance(value,Exception):raise value
        return value
    def assert_unchanged(self):pass


class WindowsDiscoveryContracts(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name).resolve();FakeArchive.fingerprints={}

    def installation(self, name='Steam', *, installed=True, filename='Balatro.exe'):
        root=self.root/name;apps=root/'steamapps';apps.mkdir(parents=True)
        (apps/'appmanifest_2379780.acf').write_text('"AppState" { "appid" "2379780" "StateFlags" "'+('4' if installed else '0')+'" "installdir" "Balatro" "buildid" "123" }')
        game=apps/'common/Balatro'/filename;game.parent.mkdir(parents=True);game.write_bytes(b'SYNTHETIC, NOT RETAIL')
        return root,game

    def run_discovery(self, roots):
        return discover_windows(archive_sha='archive',manifest_sha='members',roots_provider=lambda:roots,archive_factory=FakeArchive)

    def test_fake_registry_and_conventional_roots_read_only(self):
        first=self.root/'SteamRegistry';second=self.root/'SteamMachine'
        mapping={('HKCU',r'Software\Valve\Steam','SteamPath'):str(first),('HKLM',r'SOFTWARE\WOW6432Node\Valve\Steam','InstallPath'):str(second)}
        opened=[]
        class Key:
            def __init__(self,hive,name):self.hive=hive;self.name=name
            def __enter__(self):return self
            def __exit__(self,*args):pass
        def open_key(hive,name):opened.append((hive,name));return Key(hive,name)
        fake=types.SimpleNamespace(HKEY_CURRENT_USER='HKCU',HKEY_LOCAL_MACHINE='HKLM',OpenKey=open_key,QueryValueEx=lambda key,value:(mapping[(key.hive,key.name,value)],1))
        env={'ProgramFiles(x86)':str(self.root/'Program Files (x86)'),'ProgramFiles':str(self.root/'Program Files'),'LOCALAPPDATA':str(self.root/'Local')}
        with patch.dict(sys.modules,{'winreg':fake}),patch.dict(os.environ,env,clear=True):roots=windows_steam_roots(home=self.root/'FakeHome')
        self.assertEqual(roots,[first,second,self.root/'Program Files (x86)/Steam',self.root/'Program Files/Steam',self.root/'Local/Steam',self.root/'Local/Programs/Steam'])
        self.assertEqual(len(opened),2)

    def test_secondary_library_vdf_finds_supported(self):
        primary=self.root/'Primary';(primary/'steamapps').mkdir(parents=True)
        other,game=self.installation('Second Library')
        (primary/'steamapps/libraryfolders.vdf').write_text('"libraryfolders" { "0" { "path" "'+str(other)+'" } }')
        FakeArchive.fingerprints[str(game)]={'archive_sha256':'archive','manifest_sha256':'members'}
        r=self.run_discovery([primary]);self.assertEqual((r['status'],r['selected_game']),('supported',str(game)))
        self.assertFalse(r['game_launched']);self.assertFalse(r['game_profiles_or_saves_accessed'])

    def test_both_fingerprints_required_not_members_only(self):
        root,game=self.installation();FakeArchive.fingerprints[str(game)]={'archive_sha256':'different fused EXE','manifest_sha256':'members'}
        r=self.run_discovery([root]);self.assertEqual(r['status'],'found_unqualified');self.assertIsNone(r['selected_game'])

    def test_wrong_member_manifest_also_rejected(self):
        root,game=self.installation();FakeArchive.fingerprints[str(game)]={'archive_sha256':'archive','manifest_sha256':'wrong'}
        self.assertEqual(self.run_discovery([root])['status'],'found_unqualified')

    def test_absent_does_not_call_archive(self):
        with patch.object(FakeArchive,'__init__',side_effect=AssertionError('no archive should be opened')):
            self.assertEqual(self.run_discovery([self.root/'No Steam'])['status'],'not_found')

    def test_installed_but_incomplete_is_not_absent(self):
        root,_=self.installation(installed=False)
        self.assertEqual(self.run_discovery([root])['status'],'found_unqualified')

    def test_corrupt_library_metadata_is_incomplete_not_install_instruction(self):
        root=self.root/'Steam';(root/'steamapps').mkdir(parents=True);(root/'steamapps/libraryfolders.vdf').write_text('"bad" {')
        self.assertEqual(self.run_discovery([root])['status'],'discovery_incomplete')

    def test_multiple_supported_requires_manual_choice(self):
        roots=[]
        for name in ['One','Two']:
            root,game=self.installation(name);roots.append(root);FakeArchive.fingerprints[str(game)]={'archive_sha256':'archive','manifest_sha256':'members'}
        r=self.run_discovery(roots);self.assertEqual(r['status'],'multiple_supported');self.assertIsNone(r['selected_game'])

    def test_unreadable_archive_reports_found_unqualified(self):
        root,game=self.installation();FakeArchive.fingerprints[str(game)]=InputError('synthetic corrupt archive')
        self.assertEqual(self.run_discovery([root])['status'],'found_unqualified')

    def test_excessive_roots_fail_closed(self):
        r=self.run_discovery([self.root/str(i) for i in range(65)])
        self.assertEqual(r['status'],'discovery_incomplete')

    def test_relative_registry_hint_is_not_cwd_search(self):
        with patch('builder.windows_discovery.conventional_roots',return_value=[Path('relative')]),patch.dict(os.environ,{'LOCALAPPDATA':str(self.root/'Local')},clear=True):
            roots=windows_steam_roots(home=self.root)
        self.assertNotIn(Path('relative'),roots)

    def test_expanded_registry_hint_deduplicates_known_local_root(self):
        with patch('builder.windows_discovery.conventional_roots',return_value=[Path('%LOCALAPPDATA%/Steam')]),patch.dict(os.environ,{'LOCALAPPDATA':str(self.root/'Local')},clear=True):
            roots=windows_steam_roots(home=self.root)
        self.assertEqual(roots.count(self.root/'Local/Steam'),1)

    def test_fingerprint_budget_does_not_choose_from_partial_scan(self):
        roots=[]
        for i in range(17):
            root,_=self.installation(str(i));roots.append(root)
        with patch.object(FakeArchive,'__init__',side_effect=AssertionError('no partial fingerprint scan')):
            r=self.run_discovery(roots)
        self.assertEqual(r['status'],'discovery_incomplete');self.assertIsNone(r['selected_game'])


if __name__=='__main__':unittest.main()
