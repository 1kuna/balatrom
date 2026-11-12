#ifndef JOKER_CARD_MUTATION_H
#define JOKER_CARD_MUTATION_H
#include "joker_effects.h"
#define JCM_UNHANDLED 0
#define JCM_DONE 1
#define JCM_PARKING_ROLL 2
#define JCM_ERROR 3
#define JCM_MISPRINT_ROLL 4
/* Read/write RAM operands only. roll_value is either a uniform native0..23 Misprint draw or the result of the original
   named parking draw comparison (< probability_normal / 2); requested once
   for each eligible evaluation. No callback pointer crosses a bank switch. */
uint8_t joker_card_mutation_effect(GameState *g,uint8_t id,
 const JokerContext *c,JokerEffect *out,uint8_t roll_ready,uint8_t roll_value) CORE_BANKED;
/* Uses game_random's single 16-bit stream, so rolls differ from the desktop game.
   It must run before the ordinary handler in the shared bridge. */
void joker_card_mutation_game_event(GameState *g,uint8_t id,
 const JokerContext *c,JokerEffect *out) CORE_BANKED;
#endif
