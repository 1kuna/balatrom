#!/usr/bin/env python3
"""Independent native 3x5 and 5x7 glyph masters and ABI-stable mask tables.

All masters are explicit authored cell data. No font or prior glyph file is
read. Tiny standard letter skeletons may necessarily resemble other fonts;
this records how this particular deliverable was made, not legal clearance.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from generate_title import FONT, array

SMALL = {
    " ":[0,0,0,0,0], "!":[2,2,2,0,2], '"':[5,5,0,0,0],
    "#":[5,7,5,7,5], "$": [2,7,6,3,2],
    "%":[5,1,2,4,5], "&":[2,5,2,5,3], "'": [2,2,0,0,0],
    "(":[1,2,2,2,1], ")":[4,2,2,2,4], "*":[0,5,2,5,0],
    "+":[0,2,7,2,0], ",":[0,0,0,2,4], "-":[0,0,7,0,0],
    ".":[0,0,0,0,2], "/":[1,1,2,4,4],
    "0":[7,5,5,5,7], "1":[2,6,2,2,7], "2":[6,1,2,4,7],
    "3":[6,1,2,1,6], "4":[5,5,7,1,1], "5":[7,4,6,1,6],
    "6":[3,4,6,5,2], "7":[7,1,2,2,2], "8":[2,5,2,5,2],
    "9":[2,5,3,1,6], ":":[0,2,0,2,0], ";":[0,2,0,2,4],
    "<":[1,2,4,2,1], "=":[0,7,0,7,0], ">":[4,2,1,2,4],
    "?":[6,1,2,0,2], "@":[2,5,7,4,3],
    "A":[2,5,7,5,5], "B":[6,5,6,5,6], "C":[3,4,4,4,3],
    "D":[6,5,5,5,6], "E":[7,4,6,4,7], "F":[7,4,6,4,4],
    "G":[3,4,5,5,3], "H":[5,5,7,5,5], "I":[7,2,2,2,7],
    "J":[1,1,1,5,2], "K":[5,5,6,5,5], "L":[4,4,4,4,7],
    "M":[5,7,5,5,5], "N":[5,7,7,5,5], "O":[2,5,5,5,2],
    "P":[6,5,6,4,4], "Q":[2,5,5,3,1], "R":[6,5,6,5,5],
    "S":[3,4,2,1,6], "T":[7,2,2,2,2], "U":[5,5,5,5,2],
    "V":[5,5,5,2,2], "W":[5,5,5,7,5], "X":[5,5,2,5,5],
    "Y":[5,5,2,2,2], "Z":[7,1,2,4,7], "[":[3,2,2,2,3],
    "\\":[4,4,2,1,1], "]":[6,2,2,2,6], "^":[2,5,0,0,0],
    "_":[0,0,0,0,7], "`":[4,2,0,0,0], "{":[1,2,6,2,1],
    "|":[2,2,2,2,2], "}":[4,2,3,2,4], "~":[0,3,6,0,0],
}
LARGE = dict(FONT)
LARGE.update({
    "!": [4,4,4,4,4,0,4], '"':[10,10,10,0,0,0,0],
    "#": [10,10,31,10,31,10,10], "$": [4,15,20,14,5,30,4],
    "%": [25,25,2,4,8,19,19], "&": [12,18,20,8,21,18,13],
    "'": [4,4,8,0,0,0,0], "(": [2,4,8,8,8,4,2],
    ")": [8,4,2,2,2,4,8], "*": [0,21,14,31,14,21,0],
    "+": [0,4,4,31,4,4,0], ",": [0,0,0,0,0,4,8],
    "/": [1,2,2,4,8,8,16], ":": [0,4,4,0,4,4,0],
    ";": [0,4,4,0,4,4,8], "<": [1,2,4,8,4,2,1],
    "=": [0,0,31,0,31,0,0], ">": [16,8,4,2,4,8,16],
    "?": [14,17,1,2,4,0,4], "@": [14,17,23,21,23,16,14],
    "[": [14,8,8,8,8,8,14], "\\": [16,8,8,4,2,2,1],
    "]": [14,2,2,2,2,2,14], "^": [4,10,17,0,0,0,0],
    "_": [0,0,0,0,0,0,31],
})

def masks(master,width,height):
    out=[]
    for code in range(32,96):
        rows=master[chr(code)]
        for dx in range(-7,8):
            for y in range(8):
                bits=0
                if y<height:
                    for x in range(width):
                        if rows[y] & (1<<(width-1-x)) and 0<=x+dx<8:
                            bits |= 128>>(x+dx)
                out.append(bits)
    assert len(out)==7680
    return out

def build():
    compact=[]
    for code in range(128):
        ch=chr(code).upper() if 97<=code<=122 else chr(code)
        compact.extend(SMALL.get(ch,SMALL["?"]))
    large=[LARGE[chr(code)] for code in range(32,96)]
    assert len(compact)==640 and len(large)==64
    assert all(0<=v<=7 for v in compact)
    assert all(len(row)==7 and all(0<=v<=31 for v in row) for row in large)
    result={
        "src/frontend_font_rows.inc":array("frontend_font_rows",compact),
        "generated/frontend_font5_font5.inc":
            "static const unsigned char font5[64][7]={\n"+
            ",\n".join("{"+",".join(map(str,row))+"}" for row in large)+"\n};\n",
        "generated/a1_font_3_a1_font3_masks.inc":
            array("a1_font3_masks",masks(SMALL,3,5)).replace("static const","const",1),
        "generated/a1_font_5_a1_font5_masks.inc":
            array("a1_font5_masks",masks(LARGE,5,7)).replace("static const","const",1),
    }
    return {path:"/* Original BalatROM cell font; tools/generate_fonts.py. */\n"+value
            for path,value in result.items()}

def atlas(master,width,height,path):
    from PIL import Image
    image=Image.new("RGB",(8*12,8*12),(24,40,32))
    for code in range(32,96):
        ox=((code-32)%8)*12+2
        oy=((code-32)//8)*12+2
        for y,row in enumerate(master[chr(code)]):
            for x in range(width):
                if row & (1<<(width-1-x)):
                    image.putpixel((ox+x,oy+y),(248,240,216))
    image.save(path)
    image.resize((384,384),resample=0).save(path.with_stem(path.stem+"-4x"))

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--output",type=Path,required=True)
    args=ap.parse_args()
    outputs=build()
    for relative,content in outputs.items():
        path=args.output/relative
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(content)
    atlas(SMALL,3,5,args.output/"font3-atlas.png")
    atlas(LARGE,5,7,args.output/"font5-atlas.png")
    report={"schema":1,"input_font_files":[],"compact":[128,5,3],
            "regular":[64,7,5],"masks":[64,15,8],
            "files":{p:hashlib.sha256(v.encode()).hexdigest() for p,v in outputs.items()},
            "remaining":"Regenerate cold templates and any baked UI glyphs against target source."}
    (args.output/"FONT-GENERATION.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    main()
