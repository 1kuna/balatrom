/* Stable native catalog0..149. Private; generated. */
#ifndef SIDEBAR_JOKERS_H
#define SIDEBAR_JOKERS_H
#include <gb/gb.h>
#define SIDEBAR_JOKER_COUNT 150u
#define SIDEBAR_JOKER_RECORD_BYTES 96u
typedef struct { uint8_t bank; const uint8_t *tiles; const uint16_t *palette; } SidebarJokerRef;
extern const uint8_t sidebar_joker_tiles_0[7200];
extern const uint16_t sidebar_joker_palettes_0[300];
BANKREF_EXTERN(sidebar_joker_tiles_0)
extern const uint8_t sidebar_joker_tiles_1[7200];
extern const uint16_t sidebar_joker_palettes_1[300];
BANKREF_EXTERN(sidebar_joker_tiles_1)
uint8_t sidebar_joker_ref(uint8_t id, SidebarJokerRef *out) NONBANKED;
#endif
