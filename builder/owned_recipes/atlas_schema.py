"""Symbolic overlay palette profiles; no raster or RGB input.

ABI: eight four-slot BG palettes, then eight three-slot OBJ palettes.
Profiles 0..9 rotate the BG ring and two OBJ rings in canonical order;
profile10 reverses both. This recreates the existing renderer exactly.
"""
def values():
    result=[]
    base=[4,5,6,7,8,9]+[4,5,6]*6
    for shift in range(10):
        result.extend([(i+shift%4)%4 for i in range(32)])
        result.extend([4+((v-4+shift//4)%3) if v<7 else 7+((v-7+shift//4)%3) for v in base])
    result.extend([3-i%4 for i in range(32)])
    result.extend([10-v if v<7 else 16-v for v in base])
    assert len(result)==616 and all(0<=v<=9 for v in result)
    return result

def derive():
    body="static const uint8_t atlas_palettes[616]={"+",".join(map(str,values()))+"};\n"
    return {"generated/pb_atlas_atlas_palettes.inc":body.encode()}
