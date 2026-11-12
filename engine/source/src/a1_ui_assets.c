#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "a1_assets.h"
#include "a1_ui_assets_a1_ui_tiles.inc"

#include "a1_ui_assets_a1_ui_static_sources.inc"

#include "a1_ui_assets_a1_base_ids.inc"

#include "a1_ui_assets_a1_template_ids.inc"

#include "a1_ui_assets_a1_template_attrs.inc"

#include "a1_ui_assets_a1_palettes.inc"

#include "a1_ui_assets_a1_obj_palette.inc"

#include "a1_ui_assets_a1_cursor.inc"

#include "a1_ui_assets_a1_status_tiles.inc"

void a1_status_base_copy(uint8_t selected,uint8_t cell_x,uint8_t *out) BANKED {if(selected>5u)selected=5u;memcpy(out,a1_status_tiles+((uint16_t)selected*15u+cell_x-5u)*16u,16u);}
void a1_ui_tile_copy(uint8_t index,uint8_t *out) BANKED {memcpy(out,a1_ui_tiles+(uint16_t)a1_ui_static_sources[index]*16u,16u);}
void a1_ui_base_copy(uint8_t cell_x,uint8_t cell_y,uint8_t *out) BANKED {memcpy(out,a1_ui_tiles+(uint16_t)a1_base_ids[(uint16_t)cell_y*20u+cell_x]*16u,16u);}
uint8_t a1_ui_slot(uint8_t cell_x,uint8_t cell_y) BANKED {uint8_t slot=a1_template_ids[(uint16_t)cell_y*20u+cell_x];return slot<A1_UI_COUNT?255u:slot;}
void a1_ui_map_row_copy(uint8_t row,uint8_t *out) BANKED {memcpy(out,a1_template_ids+(uint16_t)row*20u,20u);memcpy(out+20u,a1_template_attrs+(uint16_t)row*20u,20u);}
void a1_palette_copy(uint8_t *out) BANKED {memcpy(out,a1_palettes,64u);memcpy(out+64u,a1_obj_palette,8u);}
void a1_cursor_copy(uint8_t *out) BANKED {memcpy(out,a1_cursor,32u);}
