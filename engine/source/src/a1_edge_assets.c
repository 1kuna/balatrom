#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "a1_assets.h"
#include "a1_edge_assets_a1_edge_tiles.inc"

#include "a1_edge_assets_a1_edge_recipes.inc"

void a1_edge_tile_copy(uint8_t index,uint8_t *out) BANKED {memcpy(out,a1_edge_tiles+(uint16_t)index*16u,16u);}
void a1_edge_recipe_copy(uint8_t side,uint8_t count,uint8_t selected,uint8_t *out) BANKED {uint16_t offset;if(count>3u)count=3u;offset=((uint16_t)(side&1u)*8u+(uint16_t)count*2u+(selected?1u:0u))*12u;memcpy(out,a1_edge_recipes+offset,12u);}
