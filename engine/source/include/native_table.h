#ifndef NATIVE_TABLE_H
#define NATIVE_TABLE_H
#include "game.h"
#include <gb/gb.h>
extern uint8_t native_table_live;
/* Private presentation ownership, never part of GameState or SRAM. */
#define NATIVE_BANK1_INVALID 0u
#define NATIVE_BANK1_VALID 1u
#define NATIVE_BANK1_SUSPENDED 2u
extern uint8_t native_table_bank1_owner;
void native_table_render(const GameState *) BANKED;
void native_table_receipt(const GameState *,uint8_t) BANKED;
void native_table_leave(void) BANKED;
#endif
