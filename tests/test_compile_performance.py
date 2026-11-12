import json
from pathlib import Path
import tempfile
import threading
import time
import unittest
from unittest.mock import patch
from builder.compile_cache import ObjectCache, context_key
from builder.owned_install import compile_fresh, _scheduled_entries

class CacheContracts(unittest.TestCase):
    def test_context_includes_headers_sdk_flags_and_bank(self):
        with tempfile.TemporaryDirectory(prefix='compiler root with spaces-') as t:
            r=Path(t);v=r/'view';s=r/'sdk';v.mkdir();s.mkdir()
            (v/'unit.c').write_text('source');(v/'unit.h').write_text('header');(s/'lcc').write_text('tool')
            d={'flags':['-a'],'bank':3}; a=context_key(v,s,d)
            for path in (v/'unit.h',s/'lcc'):
                old=path.read_text();path.write_text(old+'change')
                self.assertNotEqual(a,context_key(v,s,d));path.write_text(old)
            self.assertNotEqual(a,context_key(v,s,{'flags':['-b'],'bank':3}))
            self.assertNotEqual(a,context_key(v,s,{'flags':['-a'],'bank':4}))
    def test_corruption_is_miss_and_context_never_crosses(self):
        with tempfile.TemporaryDirectory(prefix='compiler root with spaces-') as t:
            c=ObjectCache(t,'a');c.put('unit',b'relocatable');self.assertEqual(c.get('unit'),b'relocatable')
            self.assertIsNone(ObjectCache(t,'b').get('unit'))
            p=c.path('unit');r=json.loads(p.read_text());r['object_hex']='00';p.write_text(json.dumps(r))
            self.assertIsNone(c.get('unit'))
    def test_symlink_context_rejected(self):
        with tempfile.TemporaryDirectory(prefix='compiler root with spaces-') as t:
            r=Path(t);(r/'link').symlink_to('/tmp')
            with self.assertRaises(ValueError):context_key(r,r,{})

class SchedulingContracts(unittest.TestCase):
    def test_measured_long_units_start_first_without_changing_descriptor(self):
        names=['first','render','last','scoring','creation_service','pb_direct_group_24','raster_prepare','tail']
        entries=[{'name':name} for name in names]
        self.assertEqual([e['name'] for e in _scheduled_entries(entries)],
                         ['pb_direct_group_24','scoring','raster_prepare','creation_service','render','first','last','tail'])
        self.assertEqual([e['name'] for e in entries],names)

    def test_parallel_bound_link_order_and_cache_reuse(self):
        with tempfile.TemporaryDirectory(prefix='compiler root with spaces-') as t:
            r=Path(t);sdk=r/'sdk';(sdk/'bin').mkdir(parents=True);(sdk/'bin/lcc').write_text('compiler')
            descriptor={'compile_flags':[], 'objects':[{'name':'unit'+str(i),'source':'unit.c','bank':None} for i in range(198)],'link_flags':['<objects.lk>','<ROM-output>']}
            active=0;peak=0;count=0;lock=threading.Lock()
            class Compiler:
                returncode=0
                def __init__(self,cmd,cwd,*,env=None):
                    nonlocal active,peak
                    self.cmd=cmd;self.cwd=cwd
                    assert all(not any(c.isspace() for c in a) for a in cmd[1:]),cmd
                    assert all(not Path(env[k]).is_absolute() and not any(c.isspace() for c in env[k]) and (cwd/env[k]).is_dir() for k in ('TMP','TEMP','TMPDIR'))
                    with lock:active+=1;peak=max(peak,active)
                def communicate(self,timeout=None):
                    nonlocal active,count
                    time.sleep(.001)
                    if '-c' in self.cmd:
                        (self.cwd/ self.cmd[self.cmd.index('-o')+1]).write_bytes(b'valid')
                        with lock:count+=1
                    else:
                        rom=self.cwd/self.cmd[-1];rom.write_bytes(b'ROM');rom.with_suffix('.noi').write_text('DEF s__CODE_0 0x100\nDEF l__CODE_0 0x100\nDEF s__INITIALIZED 0xC000\nDEF l__INITIALIZED 0x100\n')
                    with lock:active-=1
                    return '',''
                def close(self):pass
                def terminate_tree(self):pass
            with patch('builder.owned_install.ManagedCompiler',Compiler),patch('builder.owned_install.object_areas',return_value=('valid',{})),patch('builder.owned_install.validate_rom',return_value={'sha256':'expected'}):
                for index in (1,2):
                    out=r/str(index);(out/'view').mkdir(parents=True);(out/'view/unit.c').write_text('source')
                    result=compile_fresh(out,descriptor,sdk/'bin/lcc',workers=4,cache_dir=r/'cache')
                    self.assertEqual([e['name'] for e in result['compiled']],[e['name'] for e in descriptor['objects']])
                    self.assertEqual(result['cached_game_objects'],0 if index==1 else 198)
                    self.assertEqual(result['fresh_compiler_units'],198 if index==1 else 0)
            self.assertEqual(count,198);self.assertGreater(peak,1);self.assertLessEqual(peak,4)
    def test_worker_limit_rejected_before_output(self):
        with tempfile.TemporaryDirectory(prefix='compiler root with spaces-') as t:
            for value in (0,5,True):
                with self.assertRaisesRegex(Exception,'workers'):compile_fresh(Path(t),{},Path('lcc'),workers=value)

    def test_parallel_failure_drains_running_groups(self):
        import subprocess
        with tempfile.TemporaryDirectory(prefix='compiler root with spaces-') as t:
            r=Path(t);(r/'view').mkdir();(r/'view/unit.c').write_text('source')
            made=[];lock=threading.Lock()
            class Compiler:
                returncode=0
                def __init__(self,cmd,cwd,*,env=None):
                    self.name=Path(cmd[cmd.index('-o')+1]).stem;self.stopped=False;self.closed=False
                    with lock:made.append(self)
                def communicate(self,timeout=None):
                    if timeout is None or self.stopped:return '',''
                    time.sleep(.01)
                    if self.name=='bad':self.returncode=1;return '', 'synthetic failure'
                    raise subprocess.TimeoutExpired('mock',.01)
                def terminate_tree(self):self.stopped=True
                def close(self):self.closed=True
            entries=[{'name':name,'source':'unit.c','bank':None} for name in ('wait','bad','later')]
            with patch('builder.owned_install.ManagedCompiler',Compiler):
                with self.assertRaisesRegex(Exception,'bad'):
                    compile_fresh(r,{'compile_flags':[],'objects':entries},Path('lcc'),workers=2)
            self.assertTrue(all(p.closed for p in made))
            self.assertTrue(all(p.stopped for p in made if p.name!='bad'))
