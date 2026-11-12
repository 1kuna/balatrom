#ifndef JOKER_FINAL12_CORE_H
#define JOKER_FINAL12_CORE_H
#include "joker_missing.h"
/* Bind before card_mutation/effect/counter fallbacks. Only four IDs enabled.
   Current core settles scoring payouts immediately, so dollar_buffer is zero.
   If deferred payout queues are introduced, replace wallet capture explicitly.
   RAM pointers only, same numeric nonreentrancy contract as engine. */
uint8_t joker_final12_core(GameState *g,uint8_t id,const JokerContext *c,
 JokerEffect *out) CORE_BANKED;
#endif
