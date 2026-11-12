#!/usr/bin/env python3
"""Author BalatROM title data from explicit geometric glyphs.

This generator reads no prior title artwork, game files, or font files.
The 20x18 tile map, tile IDs, seed slots, menu row positions and palette
swap ABI intentionally remain compatible with a1_title_runtime.c.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

# Title artwork. Rows are hand-drawn binary masks, not taken from the old
# title, the game files or a font.
FONT = {
    " ": [0,0,0,0,0,0,0],
    "A": [14,17,17,31,17,17,17], "B": [30,17,17,30,17,17,30],
    "C": [15,16,16,16,16,16,15], "D": [30,17,17,17,17,17,30],
    "E": [31,16,16,30,16,16,31], "F": [31,16,16,30,16,16,16],
    "G": [14,17,16,23,17,17,14], "H": [17,17,17,31,17,17,17],
    "I": [14,4,4,4,4,4,14], "J": [7,2,2,2,2,18,12],
    "K": [17,18,20,24,20,18,17], "L": [16,16,16,16,16,16,31],
    "M": [17,27,21,21,17,17,17], "N": [17,25,21,21,19,17,17],
    "O": [14,17,17,17,17,17,14], "P": [30,17,17,30,16,16,16],
    "Q": [14,17,17,17,21,18,13], "R": [30,17,17,30,20,18,17],
    "S": [15,16,16,14,1,1,30], "T": [31,4,4,4,4,4,4],
    "U": [17,17,17,17,17,17,14], "V": [17,17,17,17,17,10,4],
    "W": [17,17,17,21,21,27,17], "X": [17,17,10,4,10,17,17],
    "Y": [17,17,10,4,4,4,4], "Z": [31,1,2,4,8,16,31],
    "0": [14,17,19,21,25,17,14], "1": [4,12,4,4,4,4,14],
    "2": [14,17,1,2,4,8,31], "3": [30,1,1,14,1,1,30],
    "4": [2,6,10,18,31,2,2], "5": [31,16,16,30,1,1,30],
    "6": [14,16,16,30,17,17,14], "7": [31,1,2,4,8,8,8],
    "8": [14,17,17,14,17,17,14], "9": [14,17,17,15,1,1,14],
    ".": [0,0,0,0,0,6,6], "-": [0,0,0,31,0,0,0],
}
DIGITS = [
    [7,5,5,5,7], [2,6,2,2,7], [6,1,2,4,7], [6,1,2,1,6],
    [5,5,7,1,1], [7,4,6,1,6], [3,4,6,5,2], [7,1,2,2,2],
    [2,5,2,5,2], [2,5,3,1,6],
]
W, H, BASE, CAPACITY, SEED = 160,144,88,122,207

def rgb15(r: int, g: int, b: int) -> int:
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)

GREEN = rgb15(32,104,72)
CREAM = rgb15(248,240,216)
GOLD = rgb15(240,192,64)
INK = rgb15(24,40,32)
CORAL = rgb15(232,80,64)
PALETTES = [[GREEN,GOLD,CREAM,INK], [GREEN,CORAL,CREAM,INK],
            [GREEN,CREAM,GOLD,INK]]

def canvas():
    return [[0] * W for _ in range(H)]

def rect(pic, x, y, w, h, color):
    assert 0 <= x < W and 0 <= y < H and x+w <= W and y+h <= H
    for yy in range(y,y+h):
        pic[yy][x:x+w] = [color] * w

def text(pic, value, x, y, color=2, sx=1, sy=1, gap=1):
    for char in value:
        for yy, row in enumerate(FONT[char]):
            for xx in range(5):
                if row & (16 >> xx):
                    rect(pic,x+xx*sx,y+yy*sy,sx,sy,color)
        x += 5*sx+gap

def tile(pic, tx, ty):
    data=[]
    for yy in range(ty*8,ty*8+8):
        low=high=0
        for xx in range(tx*8,tx*8+8):
            low = (low<<1) | (pic[yy][xx] & 1)
            high = (high<<1) | ((pic[yy][xx]>>1) & 1)
        data.extend([low,high])
    return bytes(data)

def build():
    pic=canvas()
    # Geometric wordmark built only from letters; nothing from the original
    # logo or the old title is reused.
    text(pic,"BALATROM",10,26,color=3,sx=3,sy=3,gap=3)
    text(pic,"BALATROM",9,24,color=2,sx=3,sy=3,gap=3)
    # Coral lower face supplies warmth without reproducing the old logo.
    face=canvas()
    text(face,"BALATROM",9,24,color=1,sx=3,sy=3,gap=3)
    for y in range(41,45):
        for x in range(W):
            if face[y][x]:
                pic[y][x]=1
    rect(pic,24,53,112,1,1)
    for x in (58,72,86,100):
        rect(pic,x,59,3,3,2)
    # Control labels occupy the unchanged row9/row11 highlight region.
    text(pic,"START",24,72,1)
    text(pic,"NEW RUN",64,72,2)
    text(pic,"NO SAVE",64,88,2)
    # Original geometric seed controls, with three runtime-owned tile slots.
    for dy in range(5):
        extent=2-abs(dy-2)
        rect(pic,26-extent,105+dy,1+extent,1,1)
        rect(pic,36,105+dy,1+extent,1,1)
    text(pic,"SEED",64,104,2)
    text(pic,"SELECT",24,120,1)
    text(pic,"RANDOM SEED",64,120,2)
    text(pic,"V1.0.0",62,136,2)

    attrs=[0]*360
    for ty in range(3,6):
        for tx in range(1,19):
            attrs[ty*20+tx]=1
    # The underbar is coral, and the four small geometric marks are cream.
    for tx in range(3,17):
        attrs[6*20+tx]=1

    tiles=[]
    ids={}
    def add(data):
        if data not in ids:
            ids[data]=BASE+len(tiles)
            tiles.append(data)
        return ids[data]
    add(bytes(16))
    mapping=[add(tile(pic,x,y)) for y in range(18) for x in range(20)]
    # Seed writes must never mutate a tile used anywhere else.
    for i in range(3):
        mapping[13*20+12+i]=SEED+i

    saved=[r.copy() for r in pic]
    rect(saved,64,88,48,8,0)
    text(saved,"A",24,88,1)
    text(saved,"RESUME",64,88,2)
    patch=[]
    for col in [3,8,9,10,11,12,13]:
        offset=11*20+col
        patch.extend([offset &255,offset>>8,add(tile(saved,col,11)),0])
    authored=len(tiles)
    assert authored <= SEED-BASE, f"title uses {authored} static tiles; budget 119"
    tiles.extend([bytes(16)]*(CAPACITY-authored))
    assert len(tiles)==122 and all(BASE<=v<210 for v in mapping)
    assert [i for i,v in enumerate(mapping) if v>=SEED] == [272,273,274]
    return dict(tiles=b"".join(tiles),mapping=mapping,attrs=attrs,patch=patch,
                authored_static_tiles=authored)

def array(name,data,ctype="uint8_t"):
    lines=[f"static const {ctype} {name}[{len(data)}]={{"]
    step=16 if ctype=="uint8_t" else 4
    for i in range(0,len(data),step):
        lines.append(",".join(f"0x{v:02x}" if ctype=="uint8_t" else f"0x{v:04x}"
                              for v in data[i:i+step])+("," if i+step<len(data) else ""))
    return "\n".join(lines)+"\n};\n"

def render(data, resume=False, seed=12345, highlight=None):
    from PIL import Image
    tiles=bytearray(data["tiles"])
    mapping=data["mapping"].copy()
    attrs=data["attrs"].copy()
    if resume:
        for i in range(0,28,4):
            pos=data["patch"][i] | data["patch"][i+1]<<8
            mapping[pos]=data["patch"][i+2]
            attrs[pos]=data["patch"][i+3]
    if highlight is not None:
        attrs[highlight*20+3:highlight*20+17]=[2]*14
    for i,char in enumerate(str(seed)):
        for row,bits in enumerate(DIGITS[int(char)]):
            for j in range(3):
                if bits & (4>>j):
                    x=2+i*4+j
                    index=(SEED-BASE+(x>>3))*16+(row+1)*2
                    tiles[index+1] |= 128>>(x&7)
    rgb=[]
    for p in PALETTES:
        rgb.append([((v&31)*255//31,((v>>5)&31)*255//31,((v>>10)&31)*255//31) for v in p])
    image=Image.new("RGB",(W,H))
    for y in range(H):
        for x in range(W):
            loc=(y>>3)*20+(x>>3)
            off=(mapping[loc]-BASE)*16+(y&7)*2
            bit=7-(x&7)
            value=((tiles[off]>>bit)&1) | (((tiles[off+1]>>bit)&1)<<1)
            image.putpixel((x,y),rgb[attrs[loc]][value])
    return image

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    data=build()
    content="/* Original BalatROM native title. Title geometry is defined in asset-authoring/tools/generate_title.py.\n * No prior title image, game archive, or external font is read by this recipe.\n */\n"
    for name,key in [("title_A_tiles","tiles"),("title_A_map","mapping"),
                     ("title_A_attrs","attrs"),("title_A_resume_patch","patch")]:
        content+=array(name,data[key])
    content+=array("title_A_palettes",sum(PALETTES,[]),"palette_color_t")
    content+=array("balatrom_title_digits",sum(DIGITS,[]))
    (args.output/"title_A_assets.inc").write_text(content)
    previews=[("title-new-run",False,12345,None),
              ("title-resume",True,65535,None),
              ("title-start-highlight",False,0,9),
              ("title-resume-highlight",True,1,11)]
    for name,resume,seed,highlight in previews:
        im=render(data,resume,seed,highlight)
        im.save(args.output/(name+".png"))
        im.resize((640,576),resample=0).save(args.output/(name+"-4x.png"))
    report={"schema":1,"title":"BALATROM","authored_static_tiles":data["authored_static_tiles"],
            "reserved_seed_tiles":[207,208,209],"loaded_tiles":122,"map":[20,18],
            "palettes":3,"resume_patch_entries":7,"input_asset_files":[],
            "title_sha256":hashlib.sha256(content.encode()).hexdigest(),
            "visual_check":"Preview images only; check the title in a built ROM."}
    (args.output/"GENERATION.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))

if __name__ == "__main__":
    main()
