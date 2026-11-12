"""Native cache ABI primitives fed by private generated pixels, never old arrays.

The 8x16 pair record is two recipe bytes plus 96 plane bytes: 32 BG,
32 OBJ layer 0, 32 OBJ layer 1. No palette merging or approximate fallback.
Corpus enumeration, scene keys and retail mappings remain separate work.
"""
from itertools import combinations
from .archive import InputError
from .conversion import c_array, symbol, tile_bytes


MISS = 65535


def palettes(bg, obj):
    for rows, width in ((bg, 4), (obj, 3)):
        if not isinstance(rows, (list, tuple)) or len(rows) != 8:
            raise InputError("Native pair requires eight BG and eight OBJ palettes")
        for row in rows:
            if len(row) != width or any(type(c) is not int or not 0 <= c <= 255 for c in row):
                raise InputError("Invalid native palette color IDs")


def compile_pair(pixels, bg, obj):
    palettes(bg, obj)
    if len(pixels) != 128 or any(type(c) is not int or not 0 <= c <= 255 for c in pixels):
        raise InputError("Native pair requires 128 color IDs in 8x16 row order")
    needed = [set(pixels[:64]), set(pixels[64:])]
    assignment = None
    for layers in range(3):
        for objects in combinations(range(8), layers):
            covered = set().union(*(set(obj[p]) for p in objects))
            backgrounds = []
            for colors in needed:
                candidates = [p for p in range(8) if colors <= set(bg[p]) | covered]
                if not candidates:
                    break
                backgrounds.append(candidates[0])
            if len(backgrounds) == 2:
                assignment = backgrounds, objects
                break
        if assignment is not None:
            break
    if assignment is None:
        raise InputError("Native pair cannot be represented exactly within two overlay layers")
    backgrounds, objects = assignment
    base = [0] * 128
    overlay = [[0] * 128, [0] * 128]
    for i, color in enumerate(pixels):
        row = bg[backgrounds[i // 64]]
        if color in row:
            base[i] = row.index(color)
        else:
            for layer, pal in enumerate(objects):
                if color in obj[pal]:
                    overlay[layer][i] = obj[pal].index(color) + 1
                    break
            else:
                raise InputError("Native palette assignment is incomplete")
    recipe = backgrounds[0] | (backgrounds[1] << 3) | (len(objects) << 12)
    for layer, pal in enumerate(objects):
        recipe |= pal << (6 + layer * 3)
    planes = b"".join(tile_bytes(row, 8, 16)[0] + tile_bytes(row, 8, 16)[1]
                      for row in [base, *overlay])
    return recipe.to_bytes(2, "little") + planes


def decode_pair(record, bg, obj):
    """Independent ABI decoder for qualification of produced records."""
    palettes(bg, obj)
    if not isinstance(record, bytes) or len(record) != 98:
        raise InputError("Native pair record must be 98 bytes")
    recipe = int.from_bytes(record[:2], "little")
    layers = (recipe >> 12) & 3
    if layers > 2 or recipe & 0xc000:
        raise InputError("Invalid native pair recipe")
    indices = []
    for layer in range(3):
        data = record[2 + layer * 32:2 + (layer + 1) * 32]
        indices.append([((data[y * 2] >> (7 - x)) & 1) |
                        (((data[y * 2 + 1] >> (7 - x)) & 1) << 1)
                        for y in range(16) for x in range(8)])
    pixels = []
    for i in range(128):
        color = bg[(recipe >> (0 if i < 64 else 3)) & 7][indices[0][i]]
        # Assignments have no overlapping opaque layers; priority is immaterial.
        for layer in range(layers):
            index = indices[layer + 1][i]
            if index:
                color = obj[(recipe >> (6 + layer * 3)) & 7][index - 1]
        pixels.append(color)
    return pixels


def key_hash(key):
    h1, h2 = 40359, 20931
    for byte in key:
        h1 = ((h1 * 33) ^ byte) & 65535
        h2 = (((h2 << 5) | (h2 >> 11)) + byte) & 65535
    return h1, h2


def placement(keys):
    size = 8
    while size < len(keys) * 2:
        size *= 2
    if size > 1024:
        raise InputError("Native cache group slot budget exceeded")
    hashed = [key_hash(key) for key in keys]
    if len(set(hashed)) != len(hashed):
        raise InputError("Native key hash collision requires corpus repartitioning")
    for buckets in (32, 64, 128, 256, 512, 1024):
        for shift in (5, 8, 11, 3):
            ownership = [[] for _ in range(buckets)]
            for i, (h1, _) in enumerate(hashed):
                ownership[(h1 ^ (h1 >> shift)) & (buckets - 1)].append(i)
            slots = [-1] * size
            displacements = [0] * buckets
            success = True
            for bucket in sorted(range(buckets), key=lambda b: (-len(ownership[b]), b)):
                ids = ownership[bucket]
                if not ids:
                    continue
                for displacement in range(size):
                    targets = [(hashed[i][1] + displacement) & (size - 1) for i in ids]
                    if len(set(targets)) == len(targets) and all(slots[t] == -1 for t in targets):
                        for i, target in zip(ids, targets):
                            slots[target] = i
                        displacements[bucket] = displacement
                        break
                else:
                    success = False
                    break
            if success:
                return size, buckets, shift, displacements, slots
    raise InputError("Native cache cannot fit a bounded displacement table")


def emit_group(records, group=0, bank=255):
    if type(group) is not int or not 0 <= group <= 62 or type(bank) is not int or not 1 <= bank <= 255:
        raise InputError("Invalid cache group/bank; MISS handle must remain reserved")
    if not isinstance(records, dict) or not records:
        raise InputError("Cache needs generated key/record pairs")
    for key, record in records.items():
        if not isinstance(key, bytes) or not 1 <= len(key) <= 119:
            raise InputError("Native key exceeds scratch-buffer contract")
        if not isinstance(record, bytes) or len(record) != 98:
            raise InputError("Native cache payload must be a generated 98-byte record")
        recipe = int.from_bytes(record[:2], "little")
        if recipe & 0xc000 or ((recipe >> 12) & 3) > 2:
            raise InputError("Invalid cache record recipe")
    keys = sorted(records)
    size, buckets, shift, displacements, slots = placement(keys)
    key_blob, values, offsets, payload_offsets = bytearray(), bytearray(), {}, {}
    for key in keys:
        offsets[key] = len(key_blob)
        key_blob.extend(bytes([len(key)]) + key)
        record = records[key]
        if record not in payload_offsets:
            payload_offsets[record] = len(values)
            values.extend(record)
    indexes, handles = bytearray(), {}
    for slot, index in enumerate(slots):
        if index == -1:
            indexes.extend(b"\xff\xff\x00\x00")
        else:
            key = keys[index]
            indexes.extend(offsets[key].to_bytes(2, "little") + payload_offsets[records[key]].to_bytes(2, "little"))
            handles[key] = (group << 10) | slot
    disp = b"".join(d.to_bytes(2, "little") for d in displacements)
    total = len(key_blob) + len(values) + len(indexes) + len(disp)
    if total > 15360:
        raise InputError("Native cache data exceeds bank budget; repartition corpus")
    code = "#ifdef __SDCC\n#pragma bank %d\n#include <gb/gb.h>\n#else\n#include <stdint.h>\n#define BANKED\n#endif\n#include <string.h>\n" % bank
    code += "extern uint8_t raster_pixels[128],raster_planes[96];\n"
    for name, data in (("keys", key_blob), ("values", values), ("indexes", indexes), ("displacements", disp)):
        code += "static " + c_array(name, data)
    code += """
uint8_t pb_direct_read_%(group)d(uint16_t slot,uint16_t *recipe) BANKED {
 uint16_t at,offset;
 if(slot>=%(size)du || !recipe)return 0;
 at=slot*4u;
 if(indexes[at]==255u && indexes[at+1u]==255u)return 0;
 offset=(uint16_t)indexes[at+2u]|((uint16_t)indexes[at+3u]<<8);
 *recipe=(uint16_t)values[offset]|((uint16_t)values[offset+1u]<<8);
 memcpy(raster_planes,values+offset+2u,96u);return 1;
}
uint8_t pb_direct_group_%(group)d(uint8_t length,uint16_t *recipe) BANKED {
 uint16_t h1,h2,bucket,at,displacement,slot,offset;
 if(!length || length>119u || !recipe)return 0;
 h1=(uint16_t)raster_pixels[124]|((uint16_t)raster_pixels[125]<<8);
 h2=(uint16_t)raster_pixels[126]|((uint16_t)raster_pixels[127]<<8);
 bucket=(h1^(h1>>%(shift)du))&%(bucket_mask)du;at=bucket*2u;
 displacement=(uint16_t)displacements[at]|((uint16_t)displacements[at+1u]<<8);
 slot=(h2+displacement)&%(slot_mask)du;at=slot*4u;
 offset=(uint16_t)indexes[at]|((uint16_t)indexes[at+1u]<<8);
 if(offset==65535u || keys[offset]!=length || memcmp(keys+offset+1u,raster_pixels,length))return 0;
 if(!pb_direct_read_%(group)d(slot,recipe))return 0;
 raster_pixels[120]=(uint8_t)slot;raster_pixels[121]=(uint8_t)((slot>>8)|%(handle_high)du);
 return 1;
}
""" % {"group": group, "size": size, "shift": shift, "bucket_mask": buckets - 1,
       "slot_mask": size - 1, "handle_high": group << 2}
    return code.encode(), handles, {"group": group, "bank": bank, "records": len(keys),
                                   "unique_payloads": len(payload_offsets), "slots": size,
                                   "buckets": buckets, "data_bytes": total, "abi": "direct-cache-v1"}


def emit_handles(name, keys, handles):
    name = symbol(name)
    if not keys:
        raise InputError("Selection handle table cannot be empty")
    values = []
    for key in keys:
        if key is None:
            values.append(MISS)  # Explicit absent slot, never a replacement image.
        elif key not in handles or handles[key] == MISS:
            raise InputError("Required selection key has no locally generated cache handle")
        else:
            value = handles[key]
            if type(value) is not int or not 0 <= value < MISS:
                raise InputError("Invalid selection cache handle")
            values.append(value)
    return ("#include <stdint.h>\n" + c_array(name, values, 16)).encode()
