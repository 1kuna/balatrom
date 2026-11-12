"""Render static captions from the native font and UI geometry.

No template image or retail files are read. Underlying scene/font rights
remain separate from this deterministic technical recipe.
"""
from pathlib import Path
import re


def literal_array(path: Path, symbol: str, count: int | None = None) -> list[int]:
    matches = re.findall(r'\b' + re.escape(symbol) + r'(?:\[[^]]*\])+\s*=\s*\{([^}]+)\}', path.read_text())
    if len(matches) != 1 or not re.fullmatch(r'\s*(?:0x[\da-fA-F]+|\d+)(?:\s*,\s*(?:0x[\da-fA-F]+|\d+))*\s*', matches[0]):
        raise ValueError('Expected one literal native array: ' + symbol)
    values = [int(n.strip(), 16 if n.strip().startswith('0x') else 10) for n in matches[0].split(',')]
    if count is not None and len(values) != count:
        raise ValueError('Native array shape changed: ' + symbol)
    return values


def derive(packet: Path) -> dict[str, bytes]:
    source = packet / 'source'
    ui = {name: literal_array(source / f'generated/a1_ui_assets_{name}.inc', name,
                             1536 if name == 'a1_ui_tiles' else 360)
          for name in ('a1_base_ids', 'a1_ui_tiles', 'a1_template_ids', 'a1_template_attrs')}
    font = literal_array(source / 'src/frontend_font_rows.inc', 'frontend_font_rows', 640)
    text = (source / 'src/R74_plain_templates.inc').read_text()
    captions = re.search(r'plain_caption_names\[52\]\[18\]\s*=\s*\{(.*?)\};', text, re.S)
    if captions is None:
        raise ValueError('Missing native caption contract')
    names = re.findall(r'"([A-Z ]+)"', captions[1])
    if len(names) != 52 or len(set(names)) != 52 or any(not 1 <= len(n) <= 17 for n in names):
        raise ValueError('Native caption shape changed')
    if any(not 0 <= v <= 255 for values in ui.values() for v in values) or any(not 0 <= v <= 7 for v in font):
        raise ValueError('Native glyph/tile input out of range')
    if any(v >= 96 for v in ui['a1_base_ids']):
        raise ValueError('Base tile index exceeds scene tiles')
    plain: list[int] = []
    for row in range(5):
        for col in range(15):
            if (row == 2 and col >= 10) or (row == 4 and col < 11):
                continue
            index = ui['a1_base_ids'][(row + 5) * 20 + col + 5]
            plain.extend(ui['a1_ui_tiles'][index * 16:index * 16 + 16])
    for name in names:
        origin = 85 - 2 * len(name)
        for col in range(11):
            index = ui['a1_base_ids'][9 * 20 + col + 5]
            tile = ui['a1_ui_tiles'][index * 16:index * 16 + 16].copy()
            for y in range(8):
                for x in range(8):
                    gx, gy = 40 + col * 8 + x, 72 + y
                    for position, character in enumerate(name):
                        dx, dy = gx - (origin + position * 4), gy - 74
                        if 0 <= dx < 3 and 0 <= dy < 5 and font[ord(character) * 5 + dy] & (4 >> dx):
                            tile[y * 2] &= ~(128 >> x) & 255
                            tile[y * 2 + 1] |= 128 >> x
            plain.extend(tile)
    for name in ('a1_template_ids', 'a1_template_attrs'):
        plain.extend(ui[name][row * 20 + col] for row in range(5, 10) for col in range(5, 20))
    if len(plain) != 10246:
        raise ValueError('Cold template ABI changed')
    return {'generated/R74_plain_templates_plain_templates.inc':
            ('const uint8_t plain_templates[10246]={' + ','.join(map(str, plain)) + '};\n').encode()}
