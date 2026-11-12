#include "banking.h"
#ifndef SCORING_H
#define SCORING_H
#include "game.h"
#include "hand_preview.h"
#include "joker_effects.h"
uint8_t score_hand(void) CORE_BANKED;
void joker_event(uint8_t event) CORE_BANKED;
void joker_context_event(JokerContext *context) CORE_BANKED;
uint8_t joker_has(uint8_t id) CORE_BANKED;
uint8_t joker_discard_card_event(JokerContext *c) CORE_BANKED;
#endif
