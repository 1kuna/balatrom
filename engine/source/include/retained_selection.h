#ifndef RETAINED_SELECTION_H
#define RETAINED_SELECTION_H
#include "frontend_panel_b_internal.h"
#include "frontend_panel_b_overlay.h"
/* Bank 7 D100..D537 holds the retained Frame. Stored g is a generation
 * word, glyph pointers are Frame-relative offsets, page.first=255 invalidates.
 * D538..D56F owns the exact upper profile. No common RAM or fixed ROM added. */
uint8_t retained_selection_load(Frame *,PBOverlayPalettes *,const GameState *,const HandPreview *,uint8_t,uint8_t *) BANKED;
void retained_selection_store(Frame *,const PBOverlayPalettes *) BANKED;
void retained_selection_save_keys(const Frame *) BANKED;
void retained_selection_invalidate(void) BANKED;
void retained_selection_get(uint8_t,uint16_t,uint8_t *,uint8_t) BANKED;
void retained_selection_put(uint8_t,uint16_t,const uint8_t *,uint8_t) BANKED;
uint8_t retained_selection_bg_guard(uint8_t,const uint8_t *) BANKED;
void retained_atomic_reset(void) BANKED;
void retained_atomic_full(uint8_t *) BANKED;
uint8_t retained_atomic_publish(const uint8_t *) BANKED;
void retained_oam_prepend(uint8_t) BANKED;
#endif
