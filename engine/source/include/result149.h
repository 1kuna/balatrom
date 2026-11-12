#ifndef RESULT149_H
#define RESULT149_H
#include <gb/gb.h>
#include "game.h"
uint8_t result149_begin(const GameState *) BANKED;
void result149_idle(void) BANKED;
void result149_input(uint8_t) BANKED;
void result149_cancel(void) BANKED;
extern uint8_t result149_active;
#endif
