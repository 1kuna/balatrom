#ifndef RETAINED_CARET_H
#define RETAINED_CARET_H
#include "game.h"
#include <gb/gb.h>
extern uint8_t retained_caret_mode, retained_pressed;
void retained_caret_receipt(const GameState *) BANKED;
void retained_caret_acknowledge(const GameState *) BANKED;
uint8_t retained_caret_try(const GameState *) BANKED;
void retained_caret_reset(void) BANKED;
uint8_t retained_caret_install(uint8_t,const uint8_t *,uint8_t,uint8_t) BANKED;
#endif
