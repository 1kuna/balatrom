#ifndef HAND_API_H
#define HAND_API_H
#include "game.h"
#include "joker_effects.h"
uint8_t hand_score_bit(const JokerContext *c,uint8_t index) CORE_BANKED;
uint8_t hand_scoring(const JokerContext *c,uint8_t index) CORE_BANKED;
Card *hand_card(GameState *g,uint8_t index) CORE_BANKED;
uint8_t hand_is_selected(const GameState *g,uint8_t index) CORE_BANKED;
uint8_t hand_toggle(GameState *g,uint8_t index) CORE_BANKED;
void hand_clear(GameState *g) CORE_BANKED;
uint8_t hand_force_uid(GameState *g,uint16_t uid) CORE_BANKED;
uint8_t hand_remove_uid(GameState *g,uint16_t uid) CORE_BANKED;
uint8_t hand_selected_insertion_order(const GameState *g,uint8_t out[HAND_PLAY_CAP]) CORE_BANKED;
uint8_t hand_selected_play_order(const GameState *g,uint8_t out[HAND_PLAY_CAP]) CORE_BANKED;
void hand_sort_rank(GameState *g) CORE_BANKED;
uint8_t hand_draw(GameState *g,uint16_t count,uint8_t bypass_logical_limit) CORE_BANKED;
uint8_t hand_refill(GameState *g,uint8_t serpent_after_action) CORE_BANKED;
uint8_t hand_change_real_size(GameState *g,int16_t delta) CORE_BANKED;
uint8_t hand_commit_selected(GameState *g) CORE_BANKED;
uint8_t hand_page(const GameState *g,uint8_t *first,uint8_t *selected_page) CORE_BANKED;
uint8_t hand_valid(const GameState *g) CORE_BANKED;
#endif
