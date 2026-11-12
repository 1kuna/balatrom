#ifndef PB_LOWER_ROM_CACHE_H
#define PB_LOWER_ROM_CACHE_H
#include "frontend_panel_b_internal.h"
#include "frontend_panel_b_overlay.h"
/* Exact complete-palette key, supported source-ID masks, then sparse ROM
 * lookup. Hit1 writes an exact canonical recipe (including65535 rejection);
 * miss0 preserves output and requires the original solver. maximum0..2.
 * Inputs/outputs must be disjoint; invalid/null/alias also returns miss0.
 * No state/UID/pointer hash, publication, or mutable cache globals.
 */
uint8_t pb_lower_rom_recipe(const PBOverlayPalettes *,const uint8_t *,
                            uint8_t maximum,uint16_t *recipe) BANKED;
/* Only lower aligned8x16 strips with no intersecting public glyph/card/cue
 * and the complete frozen face palette qualify. Hit1 copies96 canonical
 * BG/OBJ bytes and recipe; miss0 preserves both outputs. Origin repairs,
 * object/scanline admission and staging remain caller-owned and mandatory.
 */
uint8_t pb_lower_rom_static(const Frame *,uint8_t x,uint8_t y,
                            uint8_t *planes96,uint16_t *recipe) BANKED;
#endif
