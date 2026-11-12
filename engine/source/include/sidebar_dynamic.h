/* Private dynamic assets. Tile maps have no fixed VRAM offset. */
#ifndef SIDEBAR_DYNAMIC_H
#define SIDEBAR_DYNAMIC_H
#define SIDEBAR_CARD_TILE_COUNT 43u
#define SIDEBAR_CARD_MAP_BYTES 8u
/* suit SHCD = 0,1,2,3; rank A,2..10,J,Q,K = 0..12. */
extern const unsigned char sidebar_card_tiles[688];
extern const unsigned char sidebar_card_maps[416];
/* 15 bits: row-major3x5, bit14 top-left;4px advance. */
extern const unsigned short sidebar_glyph3[128];
extern const unsigned short sidebar_card_palette[4];
/* Prototype jokers 0..11, each 6 tiles / 96 bytes, four RGB555 colors. */
extern const unsigned char sidebar_proto_joker_tiles[1152];
extern const unsigned short sidebar_proto_joker_palettes[48];
#endif
