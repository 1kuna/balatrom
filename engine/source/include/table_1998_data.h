#ifndef TABLE_1998_DATA_H
#define TABLE_1998_DATA_H
#include <stdint.h>
#define TABLE_1998_STATIC_TILES 161u
#define TABLE_1998_CARD_BASE 65u
#define TABLE_1998_JOKER_BASE 224u
#define TABLE_1998_CURSOR_TILE 120u
#define TABLE_1998_HAND_X 16u
#define TABLE_1998_HAND_Y 96u
#define TABLE_1998_SELECTED_Y 88u
extern const uint8_t table_1998_tiles[2576];
extern const uint8_t table_1998_card_maps[416];
extern const uint16_t table_1998_palettes[32];
extern const uint16_t table_1998_obj_palette[4];
extern const uint8_t table_1998_cursor[32];
extern const uint8_t table_1998_joker_families[150];
extern const uint8_t table_1998_back_map[8];
extern const uint8_t table_1998_stone_map[8];
extern const uint8_t table_1998_invalid_map[8];
extern const uint8_t table_1998_footer_map[20];
#endif
