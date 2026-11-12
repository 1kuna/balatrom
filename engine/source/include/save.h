#include "banking.h"
#ifndef SAVE_H
#define SAVE_H
#include "game.h"
uint8_t game_save(const GameState *state) CORE_BANKED;
uint8_t game_load(GameState *state) CORE_BANKED;
extern uint8_t resume_available;
#endif
