#ifndef NATIVE_BUSY_H
#define NATIVE_BUSY_H
#include <gb/gb.h>
#include "game.h"
void native_busy_idle(void) BANKED;
void native_busy_title_resume(void) BANKED;
void native_busy_prepare_wait(const GameState *g) BANKED;
uint8_t native_busy_begin(const GameState *g) BANKED;
void native_busy_install_base(void) BANKED;
void native_busy_publish(void) BANKED;
#endif
