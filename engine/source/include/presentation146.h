#ifndef PRESENTATION146_H
#define PRESENTATION146_H
#include "game.h"
/* Private preparation only: not registered, drawn or linked into a ROM.
 * Final numbers are opaque stored GbNum values; no rule/numeric/RNG calls. */
typedef struct {
 uint8_t active,phase,last_hand;
 GbNum chips,mult,last_total,run_total,target;
} Present146;
typedef struct {
 uint8_t valid,continue_visible,stable,hand_emphasis;
 uint8_t chips_step,mult_step,total_step,comparison_step,outcome_step,pulse_step;
} Present146Frame;
uint8_t present146_begin(Present146 *,const GameState *) BANKED;
void present146_end(Present146 *) BANKED;
void present146_sample(Present146 *,const GameState *,uint16_t,Present146Frame *) BANKED;
#endif
