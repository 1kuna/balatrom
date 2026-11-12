#include "banking.h"
#ifndef BOSS_H
#define BOSS_H
#include "game.h"
#include "joker_effects.h"
#define CARD_DEBUFF 1
#define CARD_FACE_DOWN 2
#define CARD_PLAYED_ANTE 4
#define BOSS_ALLOWED 1
#define BOSS_NO_SCORE 0
#define BOSS_UNSUPPORTED 2
/* Stable IDs match reference/catalog.json Blind0..29 (Small0 Big1).
 Read/write supplied GameState only; external card removal/addition is not implied.
 Outputs unsupported for a missing hook or unsupported effect; pool excludes it.
 Core calls begin after deck shuffle, before first deal; card on each hand card
 after deal/mutation; before_hand after classification and history increments,
 before reading level-dependent base chips; after_hand before refill.
 Context includes selected/scoring masks and contained hand predicates.
 Owned RNG backend currently game_random(), not vanilla seed-parity. */
uint8_t boss_supported(uint8_t id) CORE_BANKED;
uint8_t boss_begin(GameState *g) CORE_BANKED;
void boss_card(GameState *g,Card *card) CORE_BANKED;
uint8_t boss_before_hand(GameState *g,JokerContext *c) CORE_BANKED;
void boss_modify_base(GameState *g,JokerContext *c) CORE_BANKED;
void boss_set_ox_hand(GameState *g,uint8_t hand_type) CORE_BANKED;
void boss_after_hand(GameState *g,JokerContext *c) CORE_BANKED;
void boss_after_draw(GameState *g,uint8_t reason) CORE_BANKED; /* 0blindstart 1play 2discard */
void boss_defeat(GameState *g) CORE_BANKED;
void boss_disable(GameState *g) CORE_BANKED;
uint8_t boss_chicot_preflight(const GameState *g) CORE_BANKED;
#endif
