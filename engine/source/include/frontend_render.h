#ifndef FRONTEND_RENDER_H
#define FRONTEND_RENDER_H
#include <stdint.h>
#include <gb/gb.h>
/* Main must use this rising-edge reader, preserving edges captured during render.
 * Returns at most one queued physical press; never repeats held keys. */
uint8_t frontend_read_pressed(void) BANKED;
void frontend_animation_tick(void) BANKED;
/* Hand controls sampled against a transitioning TABLE never replay on its
 * replacement cards. Physical releases remain tracked while locked. */
void frontend_table_transition_begin(void) BANKED;
void frontend_table_transition_cancel(void) BANKED;
void frontend_table_transition_end(void) BANKED;
extern uint8_t frontend_input_overflow;
#endif
