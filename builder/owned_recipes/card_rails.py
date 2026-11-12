"""Card rail recipes; pure transforms of locally derived cards."""
import re
RANKS = "A 2 3 4 5 6 7 8 9 10 J Q K".split()
RANK_GLYPHS = {   # 5x6, drawn at x2..6 y2..7 of the corner tile
 "A": [".###.", "#...#", "#...#", "#####", "#...#", "#...#"],
 "2": [".###.", "#...#", "...#.", "..#..", ".#...", "#####"],
 "3": ["####.", "....#", ".###.", "....#", "....#", "####."],
 "4": ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#."],
 "5": ["#####", "#....", "####.", "....#", "....#", "####."],
 "6": [".###.", "#....", "####.", "#...#", "#...#", ".###."],
 "7": ["#####", "....#", "...#.", "..#..", "..#..", "..#.."],
 "8": [".###.", "#...#", ".###.", "#...#", "#...#", ".###."],
 "9": [".###.", "#...#", "#...#", ".####", "....#", ".###."],
 "10": ["#..#.", "#.#.#", "#.#.#", "#.#.#", "#.#.#", "#..#."],
 "J": ["..###", "...#.", "...#.", "...#.", "#..#.", ".##.."],
 "Q": [".###.", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"],
 "K": ["#...#", "#..#.", "###..", "#.#..", "#..#.", "#...#"],
}
SUIT_GLYPHS = {   # 5x6 index suits
 "S": ["..#..", ".###.", "#####", "#####", "..#..", ".###."],
 "C": [".###.", ".###.", "#.#.#", "#####", "..#..", ".###."],
 "D": ["..#..", ".###.", "#####", "#####", ".###.", "..#.."],
 "H": [".#.#.", "#####", "#####", "#####", ".###.", "..#.."],
}
PIP8 = {          # native centre pips, card x8..15
 "S": ["...##...", "..####..", ".######.", "########", "########", ".######.", "...##...", "..####.."],
 "C": ["..####..", "..####..", "##.##.##", "########", "##.##.##", "...##...", "..####..", "........"],
 "D": ["...##...", "..####..", ".######.", "########", "########", ".######.", "..####..", "...##..."],
 "H": [".##..##.", "########", "########", "########", ".######.", "..####..", "...##...", "........"],
}
CROWN8 = ["#..##..#", "##.##.##", "########", "########"]
PIP12 = {         # legacy big pips, card x2..13 (heart 10 rows)
 "S": [".....##.....", "....####....", "...######...", "..########..", ".##########.", "############",
       "############", "############", ".###.##.###.", ".....##.....", "....####....", "...######..."],
 "H": ["..##....##..", ".####..####.", "############", "############", "############", ".##########.",
       "..########..", "...######...", "....####....", ".....##....."],
 "C": ["....####....", "...######...", "...######...", "...######...", "....####....", ".###.##.###.",
       "############", "############", "############", ".###.##.###.", ".....##.....", "...######..."],
 "D": [".....##.....", "....####....", "....####....", "...######...", "..########..", ".##########.",
       ".##########.", "..########..", "...######...", "....####....", "....####....", ".....##....."],
}
CROWN12 = ["#....##....#", "##..####..##", "############", "############"]

NATIVE_GROUPS = "SCDH"   # native_table.c face(): suits[4]={0,2,3,1}; palettes 3..6
LEGACY_GROUPS = "SHCD"   # render.c suit_map {0,3,1,2}; palette 1
GAME_SUITS = "SDHC"      # GameState suit enum (token()/details())
RED = set("DH")
NATIVE_SLOT0, LEGACY_SLOT0 = 11, 7

def tile(rows):
    out = []
    for r in rows:
        lo = hi = 0
        for x, ch in enumerate(r):
            c = int(ch); lo |= (c & 1) << (7 - x); hi |= (c >> 1) << (7 - x)
        out += [lo, hi]
    return out

def blank(c): return [[c] * 8 for _ in range(8)]

def stamp(grid, glyph, x0, y0, ink):
    for dy, row in enumerate(glyph):
        for dx, ch in enumerate(row):
            if ch == "#" and 0 <= x0 + dx < 8 and 0 <= y0 + dy < 8: grid[y0 + dy][x0 + dx] = ink

def done(g): return tile(["".join(str(c) for c in r) for r in g])

def native_tiles():   # 0 felt, 1 paper, 2 border, 3 ink
    t = []
    for r in RANKS:
        g = blank(1)
        for i in range(8): g[0][i] = 2; g[i][0] = 2
        g[0][0] = 0; stamp(g, RANK_GLYPHS[r], 2, 2, 3); t.append(done(g))
    for s in NATIVE_GROUPS:
        g = blank(1)
        for y in range(8): g[y][0] = 2
        stamp(g, SUIT_GLYPHS[s], 2, 1, 3); t.append(done(g))
    for half in (0, 1):
        for s in NATIVE_GROUPS:
            g = blank(1); stamp(g, PIP8[s], 0, 4 - 8 * half, 3); t.append(done(g))
    g = blank(1); stamp(g, CROWN8, 0, 3, 3); t.append(done(g))
    for s in NATIVE_GROUPS:
        g = blank(1); stamp(g, PIP8[s], 0, 0, 3); t.append(done(g))
    assert len(t) == 30
    return t

def legacy_tiles():   # palette1: 0 teal ink/corner, 1 red ink, 2 paper
    t = []
    for ink in (0, 1):
        for r in RANKS:
            g = blank(2); g[0][0] = 0; stamp(g, RANK_GLYPHS[r], 2, 2, ink); t.append(done(g))
    for s in LEGACY_GROUPS:
        g = blank(2); g[0][0] = 0; stamp(g, SUIT_GLYPHS[s], 2, 2, 1 if s in RED else 0); t.append(done(g))
    for half in (0, 1):
        for s in LEGACY_GROUPS:
            top = 10 if len(PIP12[s]) == 12 else 11
            g = blank(2); stamp(g, [r[:6] for r in PIP12[s]], 2, top - 8 - 8 * half, 1 if s in RED else 0); t.append(done(g))
    for half in (0, 1):
        for s in LEGACY_GROUPS:
            ink = 1 if s in RED else 0; g = blank(2)
            if half == 0: stamp(g, [r[:6] for r in CROWN12], 2, 2, ink)
            stamp(g, [r[:4] for r in PIP8[s]], 4, 7 - 8 * half, ink); t.append(done(g))
    assert len(t) == 46
    return t

# ---- authored recipes (were C functions; now baked into the tables) ----
def native_row(face, row):
    group, rank = face // 13, face % 13
    a = 8 | (3 + group); court = rank >= 10
    if row == 0: return [(11 + rank, a), (1, a), (2, a | 0x20)]
    if row == 1: return [(24 + group, a), (36 if court else 28 + group, a), (9, a | 0x20)]
    # Rev3 (visual review): no rotated bottom index. A rotated 9 peeking under a
    # raised neighbour read as "6"; bottom-right is now the plain frame (as face 54).
    if row == 2: return [(9, a), ((37 if court else 32) + group, a), (9, a | 0x20)]
    return [(2, a | 0x40), (1, a | 0x40), (2, a | 0x60)]

def native_corners(face):  # row0col0,row0col2,row3col0,row3col2 == recipe cells
    r0, r3 = native_row(face, 0), native_row(face, 3)
    return [r0[0], r0[2], r3[0], r3[2]]

def legacy_map(face):
    group, rank = face // 13, face % 13
    centre, lower = (45 + group, 49 + group) if rank >= 10 else (37 + group, 41 + group)
    r = (20 if group & 1 else 7) + rank
    # The bottom row uses plain corner tile1 (Y / XY flip); map[7]
    # remains the debuff/lock overlay cell.
    return list(zip([r, 33 + group, centre, centre, lower, lower, 1, 1], [9, 41, 9, 41, 9, 41, 73, 105]))

def flat(pairs_or_tiles): return [b for p in pairs_or_tiles for b in p]

# ---- .inc array I/O preserving the original text exactly outside the braces ----
ARRAY_RE = r"(%s\[(\d+)\]\s*=\s*\{)(.*?)(\};)"

def get_array(text, name):
    m = re.search(ARRAY_RE % re.escape(name), text, re.S)
    vals = [int(v) for v in re.findall(r"\d+", m.group(3))]
    assert len(vals) == int(m.group(2)), name
    return vals

def set_array(text, name, vals):
    m = re.search(ARRAY_RE % re.escape(name), text, re.S)
    assert len(vals) == int(m.group(2)), "length change for " + name
    return text[:m.start(3)] + ",".join(str(v) for v in vals) + text[m.end(3):]


def transform(arrays,legacy):
 out={k:list(v) for k,v in arrays.items()}
 out["a1_card_tiles"][NATIVE_SLOT0*16:(NATIVE_SLOT0+30)*16]=flat(native_tiles())
 out["a1_card_maps"][:52*24]=flat(c for f in range(52) for row in range(4) for c in native_row(f,row))
 out["a1_card_ink_corners"][:52*8]=flat(c for f in range(52) for c in native_corners(f))
 t=legacy;v=get_array(t,"owner_legacy_tiles");v[LEGACY_SLOT0*16:(LEGACY_SLOT0+46)*16]=flat(legacy_tiles());t=set_array(t,"owner_legacy_tiles",v)
 v=get_array(t,"owner_legacy_maps");v[:52*16]=flat(c for f in range(52) for c in legacy_map(f));t=set_array(t,"owner_legacy_maps",v)
 return out,t
