#ifndef COOPERATIVE_NATIVE_H
#define COOPERATIVE_NATIVE_H
#include <gb/gb.h>
#include <stdint.h>
#include "game.h"
extern uint8_t coop_resident;
extern volatile uint8_t coop_active,coop_cancelled,coop_accepting,coop_committing;
extern volatile uint16_t coop_generation,coop_stage_generation;
uint8_t coop_begin(const GameState*) BANKED;
uint8_t coop_finish(void) BANKED;
uint8_t coop_checkpoint(void) BANKED;
volatile uint8_t *coop_lcdc_ptr(void) NONBANKED;
uint8_t *coop_ready_ptr(void) NONBANKED;
uint8_t *coop_live_ptr(void) NONBANKED;
void coop_display_off(void) NONBANKED;
void coop_palette_stop(void) NONBANKED;
void coop_palette_start(void) NONBANKED;
void coop_data(uint8_t,uint8_t,const uint8_t*) BANKED;
void coop_sprite_data(uint8_t,uint8_t,const uint8_t*) BANKED;
void coop_tiles(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t*,const uint8_t*) BANKED;
void coop_bkg_tiles(uint8_t,uint8_t,uint8_t,uint8_t,const uint8_t*) BANKED;
void coop_bg_palette(uint8_t,uint8_t,const uint16_t*) BANKED;
void coop_obj_palette(uint8_t,uint8_t,const uint16_t*) BANKED;
void coop_capture_palette(uint8_t,uint8_t,uint8_t*) BANKED;
void coop_sprite_tile(uint8_t,uint8_t) BANKED;
void coop_sprite_prop(uint8_t,uint8_t) BANKED;
void coop_move_sprite(uint8_t,uint8_t,uint8_t) BANKED;
uint8_t coop_busy_begin(const GameState*) BANKED;
void coop_busy_publish(void) BANKED;
void coop_busy_prepare(const GameState*) BANKED;
void coop_acquire(void) BANKED;
#endif
