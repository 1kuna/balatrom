"""Declared native scene keys and dependent atlas tables.

No game text or art is bundled. Text bindings and pixels must be resolved from
verified private input. This enumerates declared states, not all faithful states.
"""
from .archive import InputError
from .conversion import c_array, load_png, tile_bytes
from .native_cache import compile_pair, emit_group


SPANS = (0, 0, 0, 0, 0, 16, 32, 32, 49, 49, 66, 67, 83, 100, 100, 117, 117, 134, 135, 135)


def scene_key(frame, pair, profile, texts):
    """Expand displayed geometry only: no UID, RNG, save or hidden face identity."""
    if not 0 <= pair < 180 or not 0 <= profile < 11:
        raise InputError("Scene pair/profile outside native contract")
    x0, y0 = pair % 20 * 8, pair // 20 * 16
    target = texts[frame["target"]] if frame["target"] is not None else ""
    if len(frame["glyphs"]) > 15 or len(target.encode("utf-8")) > 15:
        raise InputError("Declared scene exceeds native Frame glyph/target storage")
    for glyph in frame["glyphs"]:
        advance = 6 if glyph["font"] else 4
        if glyph["x"] + len(texts[glyph["binding"]].encode("utf-8")) * advance > 255:
            raise InputError("Declared glyph advancement would wrap native byte coordinates")
    key = [pair, profile, int(pair < 40 and x0 < 40 and len(target) > 5)]
    for slot, joker in enumerate(frame["jokers"][:5]):
        if pair < 60 and y0 < 35 and x0 < joker["x"] + 20 and x0 + 8 > joker["x"]:
            key.extend((241, slot, joker["x"], joker["kind"], joker["id"] if joker["kind"] == 1 else 0))
    key.append(242)
    for glyph in frame["glyphs"]:
        text = texts[glyph["binding"]]
        advance, height = (6, 7) if glyph["font"] else (4, 5)
        if y0 >= glyph["y"] + height or y0 + 16 <= glyph["y"]:
            continue
        for i, character in enumerate(text):
            x = glyph["x"] + i * advance
            if x >= x0 + 8 or x + advance <= x0:
                continue
            code = ord(character)
            if not 32 <= code <= 95:
                raise InputError("Scene text lacks native glyph coverage; no character substitution")
            key.extend((x, glyph["y"], glyph["font"], glyph["colour"], code))
    key.append(243)
    span = SPANS[pair % 20] if y0 >= 80 else 0
    for card in frame["cards"][span & 15:span >> 4]:
        if (x0 >= card["x"] + 20 or x0 + 8 <= card["x"] or
                y0 >= card["y"] + 31 or y0 + 16 <= card["y"] - 5):
            continue
        face = card["kind"] == 1
        key.extend((card["x"], card["y"], card["kind"], card["rank"] if face else 0,
                    card["suit"] if face else 0, card["cues"] & 15))
    if len(key) > 119 or any(not 0 <= v <= 255 for v in key):
        raise InputError("Scene key exceeds native scratch contract")
    return bytes(key)


def declarations(recipe):
    rows = []
    for view_index, view in enumerate(recipe["views"]):
        for family in recipe["families"]:
            for pair in sorted(view["pairs"]):
                rows.append((view_index, family["id"], pair))
    if len(rows) > 10000:
        raise InputError("Declared scene coverage exceeds limit")
    return rows


def compile_scene_atlas(raw, recipe, inputs):
    from .lua_literal import parse, parse_assignment
    image = load_png(raw, recipe["dimensions"])
    texts = {}
    for binding, source in recipe["texts"].items():
        data = inputs[source["member"]]
        table = parse(data) if source["assignment"] is None else parse_assignment(data, source["assignment"])
        value = table
        for key in source["selector"]:
            if not isinstance(value, dict) or key not in value:
                raise InputError("Scene text binding is absent from local input")
            value = value[key]
        if not isinstance(value, str) or not value or len(value.encode()) > 256 or "\0" in value:
            raise InputError("Invalid local scene text")
        texts[binding] = value
    background = recipe["background"]

    def color(pixel):
        r, g, b, a = pixel
        channels = [(v * a + bg * (255 - a) + 127) // 255 for v, bg in zip((r, g, b), background)]
        return (channels[0] >> 3) | ((channels[1] >> 3) << 5) | ((channels[2] >> 3) << 10)

    family_colors = {}
    all_colors = set()
    for family in recipe["families"]:
        rows = [[color(image.getpixel(tuple(p))) for p in row]
                for row in [*family["bg_samples"], *family["obj_samples"]]]
        family_colors[family["id"]] = rows
        all_colors.update(c for row in rows for c in row)
    declared = declarations(recipe)
    raw_pixels = {}
    for view_index, family_id, pair in declared:
        view = recipe["views"][view_index]
        x = view["origin"][0] + pair % 20 * 8
        y = view["origin"][1] + pair // 20 * 16
        colors = [color(p) for p in image.crop((x, y, x + 8, y + 16)).get_flattened_data()]
        raw_pixels[(view_index, family_id, pair)] = colors
        all_colors.update(colors)
    if len(all_colors) > 256:
        raise InputError("Scene corpus exceeds color-ID budget; no approximate fallback")
    color_values = sorted(all_colors)
    ids = {value: i for i, value in enumerate(color_values)}
    families = {identifier: [[ids[c] for c in row] for row in rows] for identifier, rows in family_colors.items()}
    records, row_keys = {}, {}
    for row in declared:
        view_index, family_id, pair = row
        key = scene_key(recipe["views"][view_index]["frame"], pair, family_id, texts)
        pal = families[family_id]
        record = compile_pair([ids[c] for c in raw_pixels[row]], pal[:8], pal[8:])
        if key in records and records[key] != record:
            raise InputError("Same normalized scene key has different pixels; declare missing state")
        records[key] = record
        row_keys[row] = key
    # Partition bounded groups. Shrink deterministically when byte/hash budgets
    # cannot fit; never drop records or reuse historical planes.
    keys = sorted(records)
    files, handles, groups = {}, {}, []
    offset = 0
    while offset < len(keys):
        group = len(groups)
        if group > 62:
            raise InputError("Scene atlas exceeds native handle-group space")
        count = min(recipe["group_records"], len(keys) - offset)
        while count:
            subset = {k: records[k] for k in keys[offset:offset + count]}
            try:
                code, bound, detail = emit_group(subset, group, recipe["bank_start"] + group)
                break
            except InputError:
                count -= 1
        if not count:
            raise InputError("Scene record cannot fit a native group")
        files["pb_direct_group_%d.c" % group] = code
        handles.update(bound)
        groups.append(detail)
        offset += count
    prefix = recipe["symbol"]
    table_banks, tables = [], []
    rows = sorted(declared)
    for table_index, start in enumerate(range(0, len(rows), 1024)):
        shard = rows[start:start + 1024]
        bank = recipe["bank_start"] + len(groups) + table_index
        name = prefix + "_rows_" + str(table_index)
        code = "#ifdef __SDCC\n#pragma bank %d\n#include <gb/gb.h>\nBANKREF(%s)\n#endif\n#include <stdint.h>\n" % (bank, name)
        code += c_array(name, [handles[row_keys[row]] for row in shard], 16)
        code += c_array(name + "_keys", [(view << 16) | (family << 8) | pair for view, family, pair in shard], 32)
        files[name + ".c"] = code.encode()
        tables.append({"symbol": name, "count": len(shard), "row_start": start, "bank": bank})
        table_banks.append(bank)
    palette_bank = recipe["bank_start"] + len(groups) + len(tables)
    if palette_bank > 127:
        raise InputError("Scene atlas allocation exceeds 2 MiB bank space")
    palette_name = prefix + "_families"
    code = "#ifdef __SDCC\n#pragma bank %d\n#include <gb/gb.h>\nBANKREF(%s)\n#endif\n#include <stdint.h>\n" % (palette_bank, palette_name)
    family_ids = sorted(families)
    code += c_array(palette_name, [c for identifier in family_ids for row in families[identifier] for c in row])
    code += c_array(prefix + "_family_ids", family_ids)
    code += c_array(prefix + "_colors", color_values, 16)
    files[palette_name + ".c"] = code.encode()
    guard = prefix.upper() + "_SCENE_ATLAS_H"
    header = "#ifndef %s\n#define %s\n#include <stdint.h>\n#ifdef __SDCC\n#include <gb/gb.h>\n#define SCENE_BANKED BANKED\n#else\n#define SCENE_BANKED\n#endif\n" % (guard, guard)
    for group in range(len(groups)):
        header += "uint8_t pb_direct_read_%d(uint16_t,uint16_t*) SCENE_BANKED;\nuint8_t pb_direct_group_%d(uint8_t,uint16_t*) SCENE_BANKED;\n" % (group, group)
    for table in tables:
        name, count = table["symbol"], table["count"]
        header += "extern const uint16_t %s[%d];\nextern const uint32_t %s_keys[%d];\n" % (name, count, name, count)
        header += "#ifdef __SDCC\nBANKREF_EXTERN(%s)\n#endif\n" % name
    header += "extern const uint8_t %s[%d];\nextern const uint8_t %s_family_ids[%d];\nextern const uint16_t %s_colors[%d];\n" % (palette_name, len(family_ids) * 56, prefix, len(family_ids), prefix, len(color_values))
    header += "#ifdef __SDCC\nBANKREF_EXTERN(%s)\n#endif\n#endif\n" % palette_name
    files[prefix + ".h"] = header.encode()
    details = {"abi": "declared-scene-atlas-v1", "declared_rows": len(rows), "unique_keys": len(keys),
               "groups": groups, "tables": tables, "palette_bank": palette_bank, "family_ids": family_ids,
               "row_handles": [handles[row_keys[row]] for row in rows],
               "row_codes": [(view << 16) | (family << 8) | pair for view, family, pair in rows],
               "record_checksums": [sum(records[row_keys[row]]) & 65535 for row in rows],
               "source_banks": {**{"pb_direct_group_%d.c" % g["group"]: g["bank"] for g in groups},
                                **{t["symbol"] + ".c": t["bank"] for t in tables}, palette_name + ".c": palette_bank},
               "exhaustive_faithful_coverage": False}
    return files, details
