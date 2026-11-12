"""Symbol and size contract for the existing sidebar data; no art or text.

Matching names and lengths doesn't check crop order or how the scene looks.
Mirrors sidebar_jokers.h and sidebar_items.h.
"""
from .archive import InputError


SIDEBAR = {
    "sidebar_joker_tiles_0": ("sidebar_joker_palettes_0", 75),
    "sidebar_joker_tiles_1": ("sidebar_joker_palettes_1", 75),
    "sidebar_planet_tiles": ("sidebar_planet_palettes", 12),
    "sidebar_celestial_tiles": ("sidebar_celestial_palettes", 8),
    "sidebar_tarot_tiles": ("sidebar_tarot_palettes", 22),
    "sidebar_spectral_tiles": ("sidebar_spectral_palettes", 18),
    "sidebar_booster_other_tiles": ("sidebar_booster_other_palettes", 24),
    "sidebar_enhancement_tiles": ("sidebar_enhancement_palettes", 8),
}


def require_sidebar_contract(recipes):
    found = {}
    for recipe in recipes:
        if recipe.get("kind") != "native-records":
            continue
        tiles = recipe.get("tile_symbol")
        if tiles not in SIDEBAR:
            continue
        if tiles in found:
            raise InputError("Duplicate native sidebar binding")
        palette, count = SIDEBAR[tiles]
        if recipe.get("palette_symbol") != palette or len(recipe.get("crops", [])) != count:
            raise InputError("Native sidebar symbol/count contract mismatch: " + tiles)
        found[tiles] = {"tiles": count * 96, "palette_entries": count * 4}
    if set(found) != set(SIDEBAR):
        raise InputError("Incomplete native sidebar families; no bundled fallback")
    return found
