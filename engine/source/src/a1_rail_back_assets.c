#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "a1_assets.h"
#include "a1_rail_back_assets_a1_rail_back_tiles.inc"

#include "a1_rail_back_assets_a1_rail_back_recipe.inc"

void a1_rail_back_tile_copy(uint8_t index,uint8_t *out) BANKED {memcpy(out,a1_rail_back_tiles+(uint16_t)index*16u,16u);}
void a1_rail_back_recipe_copy(uint8_t *out) BANKED {memcpy(out,a1_rail_back_recipe,12u);}
