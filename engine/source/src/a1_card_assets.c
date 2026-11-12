#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "a1_assets.h"
#include "a1_card_assets_a1_card_tiles.inc"

#include "a1_card_assets_a1_card_maps.inc"

#include "a1_card_assets_a1_card_ink_corners.inc"

void a1_card_tile_copy(uint8_t index,uint8_t *out) BANKED {memcpy(out,a1_card_tiles+(uint16_t)index*16u,16u);}
void a1_card_recipe_copy(uint8_t face,uint8_t *out) BANKED {memcpy(out,a1_card_maps+(uint16_t)(face<55u?face:54u)*24u,24u);}
void a1_card_corner_copy(uint8_t face,uint8_t corner,uint8_t *out) BANKED {memcpy(out,a1_card_ink_corners+(uint16_t)(face<55u?face:54u)*8u+(corner&3u)*2u,2u);}

/* Hand rendering consumes one six-byte recipe row at a time. */
void a1_card_recipe_row_copy(uint16_t offset,uint8_t *out) BANKED {memcpy(out,a1_card_maps+offset,6u);}
