#ifndef BOSS_EXTENSIONS_H
#define BOSS_EXTENSIONS_H
#include "boss.h"
/* Define capability macros only when core actually wires these calls.
   Hook declarations never imply implementations in the shared core. */
#ifdef BOSS_HAVE_DRAW_HOOK
void boss_core_draw_one(GameState *g) CORE_BANKED;
#endif
/* Ox target must be the round's previously saved target, set after begin.
   hand_type is native 0=High Card .. 11=Flush Five. */
void boss_set_ox_hand(GameState *g, uint8_t hand_type) CORE_BANKED;
/* Flint: after JE_BEFORE_HAND / level-up base refresh, before scored cards. */
void boss_modify_base(GameState *g, JokerContext *c) CORE_BANKED;
#endif
