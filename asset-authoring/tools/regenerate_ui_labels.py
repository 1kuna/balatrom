#!/usr/bin/env python3
"""Replace all inspected baked UI lettering, preserving generic UI geometry.

Reads only hash-pinned native geometric UI roots. Every inspected glyph box
is cleared before new glyphs are painted, so differing legacy letter shapes
cannot survive through a glyph-mask subtraction. Tile aliases must agree.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
from generate_fonts import SMALL
from generate_title import array

LABELS=[
    ("JOKERS",49,1,2,0), ("ITEMS",119,1,2,0), ("SCORE",3,26,2,1),
    ("LVL",2,49,1,0), ("DECK",133,50,3,0), ("HANDS",1,74,1,0),
    ("DISC",23,74,1,0), ("ANTE",1,113,1,0), ("ROUND",20,113,1,0),
    ("A",16,138,0,1), ("PICK",23,138,1,0), ("ST",48,138,0,1),
    ("PLAY",59,138,1,0), ("B",84,138,0,1), ("DISC",91,138,1,0),
    ("SEL",116,138,0,1), ("SORT",131,138,1,0),
]
INPUTS = {
    "a1_ui_tiles": "02746dbdb76f0384d6e940e61512f337ddb22ae117b6a531ac06d95c35c45d67",
    "a1_base_ids": "ac9c019aa4e7327e6c955fe54f979661f10984667277b440b7dae52394dae16b",
    "a1_template_ids": "a3456e36d094a9dc6072af0ca10bacc6831c2add0ec79c4e53ea14d3659b31db",
    "a1_template_attrs": "7b026a906d021757a285f23b01b14c0a41e115e0fdcdd0532ab88eb5f04e40b4",
    "a1_status_tiles": "6996f4bb194e7037ea2f7124a5c4fbeb6b91c3208f211a486433ebe3be2aede7"
}

def literal(path,symbol):
    text=path.read_text()
    found=re.findall(r"\b"+re.escape(symbol)+r"(?:\[[^]]*\])+\s*=\s*\{([^}]+)\}",text)
    if len(found)!=1 or not re.fullmatch(r"\s*(?:0x[\da-fA-F]+|\d+)(?:\s*,\s*(?:0x[\da-fA-F]+|\d+))*\s*",found[0]):
        raise ValueError("Native literal shape changed: "+symbol)
    return [int(n.strip(),0) for n in found[0].split(",")]

def pixel(tiles,index,x,y):
    off=index*16+y*2
    bit=7-x
    return ((tiles[off]>>bit)&1) | (((tiles[off+1]>>bit)&1)<<1)

def tile(pic,tx,ty):
    data=[]
    for y in range(ty*8,ty*8+8):
        low=high=0
        for x in range(tx*8,tx*8+8):
            low=(low<<1) | (pic[y][x]&1)
            high=(high<<1) | ((pic[y][x]>>1)&1)
        data.extend([low,high])
    return data

def label(pic,value,x,y,fg,bg):
    for yy in range(y,y+5):
        pic[yy][x:x+len(value)*4-1]=[bg]*(len(value)*4-1)
    for i,char in enumerate(value):
        for yy,bits in enumerate(SMALL[char]):
            for xx in range(3):
                if bits&(4>>xx):
                    pic[y+yy][x+i*4+xx]=fg

def build(source:Path):
    gen=source/"generated"
    for symbol,expected in INPUTS.items():
        path=gen/f"a1_ui_assets_{symbol}.inc"
        if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest()!=expected:
            raise ValueError("Unsupported UI source: "+symbol)
    old=literal(gen/"a1_ui_assets_a1_ui_tiles.inc","a1_ui_tiles")
    ids=literal(gen/"a1_ui_assets_a1_base_ids.inc","a1_base_ids")
    status=literal(gen/"a1_ui_assets_a1_status_tiles.inc","a1_status_tiles")
    assert len(old)==1536 and len(ids)==360 and len(status)==1440
    assert all(0<=i<96 for i in ids)
    base=[[pixel(old,ids[(y//8)*20+x//8],x%8,y%8) for x in range(160)] for y in range(144)]
    new=[row.copy() for row in base]
    for value,x,y,fg,bg in LABELS:
        label(new,value,x,y,fg,bg)
    # A generic 3x3 multiplication cross is re-authored as explicit geometry.
    for yy in range(3):
        for xx in range(3):
            new[62+yy][19+xx]=3 if xx==yy or xx+yy==2 else 0
    per_id={}
    for y in range(18):
        for x in range(20):
            ident=ids[y*20+x]
            proposed=tile(new,x,y)
            if ident in per_id and per_id[ident]!=proposed:
                raise ValueError(f"UI alias conflict: base tile {ident}, cell {x},{y}")
            per_id[ident]=proposed
    replaced=old.copy()
    for ident,proposed in per_id.items():
        replaced[ident*16:ident*16+16]=proposed
    strips=[]
    status_after=[]
    for selected in range(6):
        strip=[[pixel(status,selected*15+x//8,x%8,y) for x in range(120)] for y in range(8)]
        label(strip,"SEL",70,2,2,0)
        strips.append(strip)
        for x in range(15):
            status_after.extend(tile(strip,x,0))
    outputs={
        "generated/a1_ui_assets_a1_ui_tiles.inc":array("a1_ui_tiles",replaced).replace("static const","const",1),
        "generated/a1_ui_assets_a1_status_tiles.inc":array("a1_status_tiles",status_after).replace("static const","const",1),
    }
    outputs={p:"/* Baked glyphs regenerated from original BalatROM cell masters. */\n"+v for p,v in outputs.items()}
    changed=[i for i in range(96) if old[i*16:i*16+16]!=replaced[i*16:i*16+16]]
    return outputs,{"labels":LABELS,"changed_base_tile_ids":changed,
                    "status_pixel_bytes_changed":sum(a!=b for a,b in zip(status,status_after)),
                    "base_tile_aliases_preserved":True,"static_map_and_attributes_unchanged":True,
                    "status_rows":6,"status_label_box":[70,2,11,5]},new,strips

def render(source,pic,path):
    from PIL import Image
    gen=source/"generated"
    attrs=literal(gen/"a1_ui_assets_a1_template_attrs.inc","a1_template_attrs")
    values=literal(gen/"a1_ui_assets_a1_palettes.inc","a1_palettes")
    # Palettes are emitted as raw bytes, not palette_color_t words.
    if len(values)==64:
        values=[values[i]|values[i+1]<<8 for i in range(0,64,2)]
    assert len(values)==32
    colors=[((v&31)*255//31,((v>>5)&31)*255//31,((v>>10)&31)*255//31) for v in values]
    image=Image.new("RGB",(160,144))
    for y in range(144):
        for x in range(160):
            image.putpixel((x,y),colors[(attrs[y//8*20+x//8]&7)*4+pic[y][x]])
    image.save(path)
    image.resize((640,576),resample=0).save(path.with_stem(path.stem+"-4x"))

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source",type=Path,required=True)
    ap.add_argument("--output",type=Path,required=True)
    args=ap.parse_args()
    outputs,receipt,pic,strips=build(args.source)
    args.output.mkdir(parents=True,exist_ok=True)
    for relative,content in outputs.items():
        path=args.output/relative
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(content)
    render(args.source,pic,args.output/"base-ui.png")
    receipt['outputs']={p:hashlib.sha256(v.encode()).hexdigest() for p,v in outputs.items()}
    (args.output/"UI-GENERATION.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(json.dumps(receipt,indent=2))

if __name__=="__main__":
    main()
