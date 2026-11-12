#ifndef FRONTEND_PANEL_B_H
#define FRONTEND_PANEL_B_H
#include "game.h"
#include "hand_preview.h"
#include <gb/gb.h>
extern uint8_t frontend_table_mode;
extern uint8_t panel_b_allocation_error, panel_b_object_count, panel_b_peak_load;
extern uint8_t panel_b_rejected_count, panel_b_rejected_peak;
extern uint16_t panel_b_prospective_objects;
void panel_b_error_overlay(void) BANKED;
void panel_b_cache_reset(void) BANKED;
void frontend_table_enter(void) BANKED;
void frontend_table_exit(void) BANKED;
void frontend_capture_inputs(void) BANKED;
void render_panel_b(const GameState *g,const HandPreview *preview) BANKED;
uint8_t panel_b_fetch_icon(uint8_t id,uint8_t *tiles,uint16_t *palette) NONBANKED;
void panel_b_load_palette_recipe(uint16_t mask,uint8_t *out) NONBANKED;
void panel_b_load_family_static_recipe(uint8_t,uint8_t,uint8_t *) NONBANKED;
void panel_b_load_overlay_choices(uint8_t,uint16_t,uint8_t,uint8_t *) NONBANKED;
void panel_b_load_static_recipe(uint8_t pair,uint8_t *out) NONBANKED;
void panel_b_load_static_pair(uint8_t pair,uint8_t *out) NONBANKED;
#endif
