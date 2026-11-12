#ifndef FRONTEND_PANEL_B_HOTPATH_H
#define FRONTEND_PANEL_B_HOTPATH_H
#include <gb/gb.h>
uint8_t panel_b_glyph_row_mask(uint8_t,uint8_t,int8_t) NONBANKED;
void panel_b_pack64(const uint8_t *,const uint8_t *,uint8_t *) OLDCALL NONBANKED;
void panel_b_pack_pair(const uint8_t *,const uint8_t [3][41],uint8_t *,uint8_t) NONBANKED;
uint8_t panel_b_card_span(uint8_t) NONBANKED;
#endif
