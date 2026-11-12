"""Deterministic payload generation from verified local members only."""
from collections import Counter
from io import BytesIO
import re
from .archive import InputError


def symbol(value):
    if not isinstance(value, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]{0,63}", value):
        raise InputError("Invalid generated C symbol")
    return value


def c_array(name, values, width=8):
    symbol(name)
    return "const uint%d_t %s[%d]={%s};\n" % (
        width, name, len(values), ",".join(str(n) for n in values))


def load_png(data, dimensions):
    try:
        from PIL import Image
    except ImportError as exc:
        raise InputError("PNG conversion requires the declared Pillow dependency") from exc
    try:
        image = Image.open(BytesIO(data))
        if image.format != "PNG" or image.size != tuple(dimensions):
            raise InputError("PNG dimensions or format differ from profile")
        if image.width * image.height > 4 * 1024 * 1024:
            raise InputError("PNG pixel count exceeds limit")
        image.load()
        image = image.convert("RGBA")
    except (OSError, ValueError, Image.DecompressionBombError) as exc:
        raise InputError("Cannot decode bounded PNG input") from exc
    return image


def quantize(image, crop, background):
    x, y, w, h = crop
    if x < 0 or y < 0 or w <= 0 or h <= 0 or x + w > image.width or y + h > image.height:
        raise InputError("PNG crop is outside image")
    if w % 8 or h % 8:
        raise InputError("Native crop dimensions must be multiples of eight")
    colors = []
    for red, green, blue, alpha in image.crop((x, y, x + w, y + h)).get_flattened_data():
        rgb = [(c * alpha + b * (255 - alpha) + 127) // 255
               for c, b in zip((red, green, blue), background)]
        colors.append((rgb[0] >> 3) | ((rgb[1] >> 3) << 5) | ((rgb[2] >> 3) << 10))
    # Palette is derived from the selected local pixels, never bundled art.
    palette = [c for c, _ in sorted(Counter(colors).items(), key=lambda p: (-p[1], p[0]))[:4]]
    palette += [palette[-1]] * (4 - len(palette))

    def distance(a, b):
        return sum((((a >> shift) & 31) - ((b >> shift) & 31)) ** 2 for shift in (0, 5, 10))

    color_map = {c: min(range(4), key=lambda i: (distance(c, palette[i]), i)) for c in set(colors)}
    return [color_map[c] for c in colors], palette


def tile_bytes(pixels, w, h):
    tiles = []
    for ty in range(0, h, 8):
        for tx in range(0, w, 8):
            tile = bytearray()
            for dy in range(8):
                row = pixels[(ty + dy) * w + tx:(ty + dy) * w + tx + 8]
                tile.extend((sum((v & 1) << (7 - dx) for dx, v in enumerate(row)),
                             sum(((v >> 1) & 1) << (7 - dx) for dx, v in enumerate(row))))
            tiles.append(bytes(tile))
    return tiles


def png_tiles(data, recipe):
    image = load_png(data, recipe["dimensions"])
    pixels, palette = quantize(image, recipe["crop"], recipe["background"])
    _, _, w, h = recipe["crop"]
    tiles, mapping, lookup = [], [], {}
    for key in tile_bytes(pixels, w, h):
        if key not in lookup:
            lookup[key] = len(tiles) // 16
            tiles.extend(key)
        mapping.append(lookup[key])
    if len(lookup) > recipe["max_tiles"] or len(lookup) > 256:
        raise InputError("Generated native tile budget exceeded")
    name = symbol(recipe["symbol"])
    code = "#include <stdint.h>\n" + c_array(name + "_tiles", tiles)
    code += c_array(name + "_map", mapping) + c_array(name + "_palette", palette, 16)
    return code.encode(), {"tiles": len(lookup), "map_entries": len(mapping), "palette": palette}


def native_records(data, recipe):
    """Emit the current sidebar's fixed six-tile/four-color record ABI.

    Each crop is 16x24, tile rows left-to-right/top-to-bottom, 96 plane bytes
    plus four RGB555 colors per record. No cross-record dedup or bundled input.
    A banked source and extern header are generated from the same validated data.
    """
    image = load_png(data, recipe["dimensions"])
    tiles, palettes = [], []
    for crop in recipe["crops"]:
        x, y, w, h = crop
        from PIL import Image
        resized = image.crop((x, y, x + w, y + h)).resize((16, 24), Image.Resampling.NEAREST)
        pixels, palette = quantize(resized, (0, 0, 16, 24), recipe["background"])
        record = b"".join(tile_bytes(pixels, 16, 24))
        if len(record) != 96:
            raise InputError("Native sidebar record size mismatch")
        tiles.extend(record)
        palettes.extend(palette)
    name = symbol(recipe["symbol"])
    tile_symbol = symbol(recipe["tile_symbol"])
    palette_symbol = symbol(recipe["palette_symbol"])
    data_bytes = len(tiles) + len(palettes) * 2
    if data_bytes > 15360:
        raise InputError("Native source data exceeds bank budget")
    code = "#ifdef __SDCC\n#pragma bank %d\n#include <gb/gb.h>\nBANKREF(%s)\n#else\n#include <stdint.h>\n#endif\n" % (recipe["bank"], tile_symbol)
    code += c_array(tile_symbol, tiles) + c_array(palette_symbol, palettes, 16)
    guard = name.upper() + "_GENERATED_H"
    header = "#ifndef %s\n#define %s\n#include <stdint.h>\n" % (guard, guard)
    header += "#define %s_RECORD_COUNT %du\n#define %s_RECORD_BYTES 96u\n" % (name.upper(), len(recipe["crops"]), name.upper())
    header += "extern const uint8_t %s[%d];\nextern const uint16_t %s[%d];\n" % (tile_symbol, len(tiles), palette_symbol, len(palettes))
    header += "#ifdef __SDCC\n#include <gb/gb.h>\nBANKREF_EXTERN(%s)\n#endif\n#endif\n" % tile_symbol
    return {name + ".c": code.encode(), name + ".h": header.encode()}, {
        "records": len(recipe["crops"]), "record_bytes": 96, "palette_entries": len(palettes),
        "data_bytes": data_bytes, "bank": recipe["bank"], "abi": "sidebar-six-tile-v1"}


def native_pairs(data, recipe):
    """Compile pair/cache ABI using palettes sampled only from local PNG pixels."""
    from PIL import Image
    from .native_cache import compile_pair, emit_group, emit_handles
    image = load_png(data, recipe["dimensions"])
    background = recipe["background"]

    def color(pixel):
        red, green, blue, alpha = pixel
        rgb = [(c * alpha + b * (255 - alpha) + 127) // 255
               for c, b in zip((red, green, blue), background)]
        return (rgb[0] >> 3) | ((rgb[1] >> 3) << 5) | ((rgb[2] >> 3) << 10)

    bg_colors = [[color(image.getpixel(tuple(point))) for point in row] for row in recipe["bg_samples"]]
    obj_colors = [[color(image.getpixel(tuple(point))) for point in row] for row in recipe["obj_samples"]]
    rows = []
    for x, y, w, h in recipe["crops"]:
        region = image.crop((x, y, x + w, y + h)).resize((8, 16), Image.Resampling.NEAREST)
        rows.append([color(p) for p in region.get_flattened_data()])
    all_colors = sorted(set(c for row in [*rows, *bg_colors, *obj_colors] for c in row))
    if len(all_colors) > 256:
        raise InputError("Pair corpus exceeds native color-ID budget; no approximate fallback")
    ids = {c: i for i, c in enumerate(all_colors)}
    bg = [[ids[c] for c in row] for row in bg_colors]
    obj = [[ids[c] for c in row] for row in obj_colors]
    records = {bytes(key): compile_pair([ids[c] for c in pixels], bg, obj)
               for key, pixels in zip(recipe["keys"], rows)}
    code, handles, details = emit_group(records, recipe["group"], recipe["bank"])
    name = recipe["symbol"]
    selection = emit_handles(name + "_selection", [bytes(k) for k in recipe["keys"]], handles)
    selection += c_array(name + "_colors", all_colors, 16).encode()
    selection += c_array(name + "_bg_ids", [c for row in bg for c in row]).encode()
    selection += c_array(name + "_obj_ids", [c for row in obj for c in row]).encode()
    prefix = "#ifdef __SDCC\n#pragma bank %d\n#include <gb/gb.h>\nBANKREF(%s_selection)\n#endif\n" % (recipe["bank"], name)
    selection = prefix.encode() + selection
    guard = name.upper() + "_SELECTION_H"
    header = "#ifndef %s\n#define %s\n#include <stdint.h>\n" % (guard, guard)
    header += "extern const uint16_t %s_selection[%d];\n" % (name, len(recipe["keys"]))
    header += "extern const uint16_t %s_colors[%d];\n" % (name, len(all_colors))
    header += "extern const uint8_t %s_bg_ids[32];\nextern const uint8_t %s_obj_ids[24];\n" % (name, name)
    header += "#ifdef __SDCC\n#include <gb/gb.h>\nBANKREF_EXTERN(%s_selection)\n#endif\n#endif\n" % name
    details.update(abi="direct-cache-v1", color_ids=len(all_colors), handles=[handles[bytes(k)] for k in recipe["keys"]])
    return {name + ".c": code, name + "_selection.c": selection, name + "_selection.h": header.encode()}, details


def text_table(table, recipe):
    values = []
    for path in recipe["selectors"]:
        value = table
        for key in path:
            if not isinstance(value, dict) or key not in value:
                raise InputError("Required literal-table field is missing")
            value = value[key]
        if not isinstance(value, str) or not value or "\x00" in value:
            raise InputError("Text selector must identify a nonempty literal string")
        if len(value.encode("utf-8")) > recipe["max_bytes"]:
            raise InputError("Text exceeds native profile budget; no automatic renaming/truncation")
        values.append(value)

    def literal(value):
        # Fixed three-digit octal avoids escapes consuming following hex digits.
        return '"' + "".join(chr(b) if 32 <= b < 127 and b not in (34, 92)
                              else "\\%03o" % b for b in value.encode("utf-8")) + '"'

    name = symbol(recipe["symbol"])
    code = "/* Local-input text; keep generated output private. */\n"
    code += "const char *const %s[%d]={%s};\n" % (name, len(values), ",".join(map(literal, values)))
    return code.encode(), {"entries": len(values)}


def convert_recipe(raw, recipe, inputs=None):
    if recipe["kind"] == "native-scenes":
        from .scenes import compile_scene_atlas
        return compile_scene_atlas(raw, recipe, inputs)
    if recipe["kind"] == "native-records":
        return native_records(raw, recipe)
    if recipe["kind"] == "native-pairs":
        return native_pairs(raw, recipe)
    if recipe["kind"] == "png-tiles":
        data, details = png_tiles(raw, recipe)
    else:
        from .lua_literal import parse, parse_assignment
        table = parse(raw) if recipe["kind"] == "lua-text" else parse_assignment(raw, recipe["assignment"])
        data, details = text_table(table, recipe)
    return {recipe["symbol"] + ".c": data}, details
