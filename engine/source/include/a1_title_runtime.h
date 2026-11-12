#ifndef A1_TITLE_RUNTIME_H
#define A1_TITLE_RUNTIME_H
#include <gb/gb.h>
/* TITLE-only glyph lease: three seed tiles inside the existing HUD allocation.
 * Caller owns allocation/cache; module adds no mutable WRAM. */
void a1_title_paint(uint16_t seed,uint8_t resume,uint8_t *seed_slot,uint8_t *hud,uint8_t full) BANKED;
void a1_title_request(uint8_t key) BANKED;
#endif
