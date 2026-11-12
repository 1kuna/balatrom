#ifndef A1_ASSETS_H
#define A1_ASSETS_H
#include <gb/gb.h>
#define A1_CARD_COUNT 159u
#define A1_SPECIAL_COUNT 18u
#define A1_EDGE_COUNT 10u
#define A1_UI_COUNT 52u
#define A1_UI_BASE_COUNT 96u
#define A1_UI_DYNAMIC_COUNT 176u
#define A1_UI_RESIDENT_COUNT 228u
void a1_edge_tile_copy(uint8_t index,uint8_t *out) BANKED;
void a1_edge_recipe_copy(uint8_t side,uint8_t count,uint8_t selected,uint8_t *out) BANKED;
void a1_status_base_copy(uint8_t selected,uint8_t cell_x,uint8_t *out) BANKED;
#define A1_RAIL_BACK_BASE 236u
#define A1_RAIL_BACK_COUNT 6u
void a1_rail_back_tile_copy(uint8_t index,uint8_t *out) BANKED;
void a1_rail_back_recipe_copy(uint8_t *out) BANKED;
uint8_t a1_ui_slot(uint8_t cell_x,uint8_t cell_y) BANKED;
void a1_card_tile_copy(uint8_t index,uint8_t *out) BANKED;
void a1_card_recipe_copy(uint8_t face,uint8_t *out) BANKED;
void a1_card_recipe_row_copy(uint16_t offset,uint8_t *out) BANKED;
void a1_card_corner_copy(uint8_t face,uint8_t corner,uint8_t *out) BANKED;
void a1_ui_tile_copy(uint8_t index,uint8_t *out) BANKED;
void a1_ui_base_copy(uint8_t cell_x,uint8_t cell_y,uint8_t *out) BANKED;
void a1_ui_map_row_copy(uint8_t row,uint8_t *out) BANKED;
void a1_palette_copy(uint8_t *out) BANKED;
void a1_cursor_copy(uint8_t *out) BANKED;
void a1_font3_mask_copy(uint8_t ch,int8_t dx,int8_t dy,uint8_t *out) BANKED;
void a1_font5_mask_copy(uint8_t ch,int8_t dx,int8_t dy,uint8_t *out) BANKED;
#endif
