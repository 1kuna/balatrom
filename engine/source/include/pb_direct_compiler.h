#ifndef PB_DIRECT_COMPILER_H
#define PB_DIRECT_COMPILER_H
#include "frontend_panel_b_internal.h"
#include "frontend_panel_b_overlay.h"
uint8_t pb_atlas_profile(const PBOverlayPalettes *) BANKED;
uint8_t pb_atlas_preclassified(const Frame *,const PBOverlayPalettes *,uint8_t,uint8_t,uint16_t *,uint16_t *) BANKED;
void pb_atlas_semantics(const Frame *,uint8_t *) BANKED;
uint8_t pb_atlas_static(const Frame *,const PBOverlayPalettes *,uint8_t,uint8_t,uint16_t *,uint16_t *) BANKED;
uint8_t pb_atlas_copy(uint16_t,uint16_t *) BANKED;
void pb_ferry_put(uint8_t,uint16_t,const uint8_t *,uint8_t) BANKED;
void pb_ferry_get(uint8_t,uint16_t,uint8_t *,uint8_t) BANKED;
#endif
