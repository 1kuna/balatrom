import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"tools"))
import generate_title as title
import generate_fonts as fonts
import prepare_overlay as overlay
import regenerate_ui_labels as ui

def values(path,symbol):
    text=Path(path).read_text()
    body=re.search(r"\b"+symbol+r"(?:\[[^]]*\])+\s*=\s*\{(.*?)\};",text,re.S)[1]
    return [int(v,0) for v in re.findall(r"0x[0-9a-fA-F]+|\b\d+\b",body)]

class NativeAssetTests(unittest.TestCase):
    def test_title_dimensions(self):
        d=title.build()
        self.assertEqual(len(d["tiles"]),1952)
        self.assertEqual(len(d["mapping"]),360)
        self.assertEqual(len(d["attrs"]),360)
        self.assertEqual(len(d["patch"]),28)
        self.assertLessEqual(d["authored_static_tiles"],119)
        self.assertEqual(set(d["attrs"]),{0,1})

    def test_seed_slot_isolation(self):
        d=title.build()
        self.assertEqual([(i,v) for i,v in enumerate(d["mapping"]) if v>=207],
                         [(272,207),(273,208),(274,209)])
        for i in range(0,28,4):
            self.assertLess(d["patch"][i+2],207)
        # Exhaust the entire 16-bit seed domain with the same indexing maths
        # as the C renderer. No dynamic write may leave the 48-byte region.
        for seed in range(65536):
            for i,char in enumerate(str(seed)):
                for row,mask in enumerate(title.DIGITS[int(char)]):
                    for j in range(3):
                        if mask&(4>>j):
                            x=2+i*4+j
                            address=(x>>3)*16+(row+1)*2+1
                            self.assertTrue(0<=address<48)

    def test_resume_contract(self):
        d=title.build()
        offsets=[d["patch"][i]|d["patch"][i+1]<<8 for i in range(0,28,4)]
        self.assertEqual(offsets,[223,228,229,230,231,232,233])
        a=title.render(d,False)
        b=title.render(d,True)
        changed=[(x,y) for y in range(144) for x in range(160) if a.getpixel((x,y))!=b.getpixel((x,y))]
        self.assertTrue(changed)
        self.assertTrue(all(88<=y<96 and (24<=x<32 or 64<=x<112) for x,y in changed))

    def test_highlight_contract(self):
        d=title.build()
        for row in (9,11):
            a=title.render(d,True)
            b=title.render(d,True,highlight=row)
            changed=[(x,y) for y in range(144) for x in range(160) if a.getpixel((x,y))!=b.getpixel((x,y))]
            self.assertTrue(changed)
            self.assertTrue(all(row*8<=y<(row+1)*8 and 24<=x<136 for x,y in changed))

    def test_font_coverage(self):
        out=fonts.build()
        self.assertEqual(len(out),4)
        for ch in map(chr,range(32,96)):
            self.assertIn(ch,fonts.SMALL)
            self.assertIn(ch,fonts.LARGE)
            self.assertEqual(len(fonts.SMALL[ch]),5)
            self.assertEqual(len(fonts.LARGE[ch]),7)
        for ch in "0123456789":
            self.assertEqual(fonts.SMALL[ch],title.DIGITS[int(ch)])

    def test_exhaustive_font_clipping(self):
        for master,width,height in ((fonts.SMALL,3,5),(fonts.LARGE,5,7)):
            data=fonts.masks(master,width,height)
            for code in range(32,96):
                for dx in range(-7,8):
                    offset=((code-32)*15+(dx+7))*8
                    for dy in range(-7,8):
                        expected=[0]*8
                        for y in range(height):
                            for x in range(width):
                                if master[chr(code)][y]&(1<<(width-1-x)) and 0<=x+dx<8 and 0<=y+dy<8:
                                    expected[y+dy] |= 128>>(x+dx)
                        actual=[0]*8
                        first=max(dy,0)
                        count=8-abs(dy)
                        start=offset+max(-dy,0)
                        actual[first:first+count]=data[start:start+count]
                        self.assertEqual(actual,expected)

    def test_reproducible(self):
        self.assertEqual(title.build(),title.build())
        self.assertEqual(fonts.build(),fonts.build())

class SourceBoundTests(unittest.TestCase):
    def setUp(self):
        self.source_root=ROOT.parent
        self.source=self.source_root/"engine/source"

    def test_ui_bounds_and_aliases(self):
        outputs,receipt,pic,strips=ui.build(self.source)
        old=ui.literal(self.source/"generated/a1_ui_assets_a1_ui_tiles.inc","a1_ui_tiles")
        ids=ui.literal(self.source/"generated/a1_ui_assets_a1_base_ids.inc","a1_base_ids")
        allowed={(x,y) for text,ox,oy,fg,bg in ui.LABELS for y in range(oy,oy+5) for x in range(ox,ox+len(text)*4-1)}
        allowed|={(x,y) for y in range(62,65) for x in range(19,22)}
        for y in range(144):
            for x in range(160):
                previous=ui.pixel(old,ids[y//8*20+x//8],x%8,y%8)
                if (x,y) not in allowed:
                    self.assertEqual(pic[y][x],previous)
        status=ui.literal(self.source/"generated/a1_ui_assets_a1_status_tiles.inc","a1_status_tiles")
        for s in range(6):
            for y in range(8):
                for x in range(120):
                    if not(70<=x<81 and 2<=y<7):
                        self.assertEqual(strips[s][y][x],ui.pixel(status,s*15+x//8,x%8,y))
        self.assertTrue(receipt["base_tile_aliases_preserved"])
        self.assertEqual(len(outputs),2)

    def test_preparation_guards(self):
        with tempfile.TemporaryDirectory(prefix="balatrom-guards-") as temporary:
            root=Path(temporary)
            source=root/"source-tree/engine/source"
            for relative in overlay.EXPECTED|overlay.DERIVATION_INPUTS:
                path=source/relative
                path.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(self.source/relative,path)
            recipe=root/"source-tree/builder/owned_recipes/plain_templates.py"
            recipe.parent.mkdir(parents=True)
            shutil.copyfile(self.source_root/"builder/owned_recipes/plain_templates.py",recipe)
            before={str(p.relative_to(root/"source-tree")):p.read_bytes() for p in (root/"source-tree").rglob("*") if p.is_file()}
            with self.assertRaisesRegex(ValueError,"outside the input source tree"):
                overlay.prepare(root/"source-tree",root/"source-tree/nested-output")
            existing=root/"existing"
            existing.mkdir()
            (existing/"checkpoint").write_text("preserve")
            with self.assertRaisesRegex(ValueError,"already exists"):
                overlay.prepare(root/"source-tree",existing)
            self.assertEqual((existing/"checkpoint").read_text(),"preserve")
            link=root/"link"
            link.symlink_to(root/"absent")
            with self.assertRaisesRegex(ValueError,"symbolic link"):
                overlay.prepare(root/"source-tree",link)
            dependency=source/"src/R74_plain_templates.inc"
            original=dependency.read_bytes()
            dependency.write_bytes(original+b"\n")
            with self.assertRaisesRegex(ValueError,"Unsupported source hash"):
                overlay.prepare(root/"source-tree",root/"bad-output")
            self.assertFalse((root/"bad-output").exists())
            dependency.write_bytes(original)
            result=overlay.prepare(root/"source-tree",root/"valid-output")
            self.assertTrue(result["originals_unchanged"])
            after={str(p.relative_to(root/"source-tree")):p.read_bytes() for p in (root/"source-tree").rglob("*") if p.is_file()}
            self.assertEqual(before,after)
            self.assertFalse(any((root/"source-tree").rglob("__pycache__")))

    def test_final_artifact_identity(self):
        with tempfile.TemporaryDirectory() as temporary:
            output=Path(temporary)/"assets"
            receipt=overlay.prepare(self.source_root,output)
            for row in receipt["source_changes"]:
                path=output/"engine/source"/row["path"]
                self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),row["after_sha256"])
                self.assertEqual(path.stat().st_size,row["bytes"])
                self.assertEqual(path.read_bytes(),(self.source/row["path"]).read_bytes())
            for relative,content in fonts.build().items():
                self.assertEqual((self.source/relative).read_text(),content)
            for relative,content in ui.build(self.source)[0].items():
                self.assertEqual((self.source/relative).read_text(),content)

if __name__=="__main__":
    unittest.main()
