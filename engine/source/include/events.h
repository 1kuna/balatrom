#include "banking.h"
#ifndef EVENTS_H
#define EVENTS_H
#include "game.h"
uint8_t game_cards_event_preflight(uint8_t removed_count,uint8_t added_count) CORE_BANKED;
void game_refresh_card(Card *card) CORE_BANKED;
uint8_t game_adjust_money(int32_t delta) CORE_BANKED;
uint8_t game_draw_to_hand(uint8_t count) CORE_BANKED;
void game_refill_hand(void) CORE_BANKED;
void game_cards_removed(const Card *cards,uint8_t count) CORE_BANKED;
void game_cards_added(const Card *cards,uint8_t count) CORE_BANKED;
void game_consumable_used(uint8_t kind,uint8_t id) CORE_BANKED;
uint8_t game_add_joker(uint8_t id,uint8_t edition,uint8_t eternal) CORE_BANKED;
uint8_t game_remove_joker(uint8_t index) CORE_BANKED;
void game_move_joker(uint8_t from,uint8_t to) CORE_BANKED;
uint8_t game_change_hand_size(int16_t delta) CORE_BANKED;
uint8_t game_generate_consumable(uint8_t kind,uint8_t id) CORE_BANKED;
uint8_t game_poll_edition(uint8_t no_negative,uint8_t guaranteed) CORE_BANKED;
#endif
