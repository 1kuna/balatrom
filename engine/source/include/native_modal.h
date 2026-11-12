#ifndef NATIVE_MODAL_H
#define NATIVE_MODAL_H
#include "game.h"
#include <gb/gb.h>
#define NATIVE_BANK1_COLD_PREPARING 3u
void native_modal_render(const GameState *g) BANKED;
void native_modal_prime_idle(void) BANKED;
#endif
