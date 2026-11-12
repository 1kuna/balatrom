#ifndef PRIVATE_SHOP154_H
#define PRIVATE_SHOP154_H
#include <gb/gb.h>
#include <gb/cgb.h>
extern uint8_t shop154_staging;
void shop154_data(uint8_t first,uint8_t count,const uint8_t *data) NONBANKED OLDCALL;
void shop154_tiles(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *data) NONBANKED OLDCALL;
void shop154_palette(uint8_t first,uint8_t count,const palette_color_t *data) NONBANKED OLDCALL;
void shop154_commit(void) BANKED;
#include "game.h"
void shop154_start(const GameState *g) BANKED;
void shop156_upload(uint8_t,uint8_t,const uint8_t *) BANKED;
uint8_t shop157_sidebar(const GameState*,uint8_t,uint8_t*) BANKED;
#endif
