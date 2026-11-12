#include "banking.h"
#ifndef GAME_H
#define GAME_H
#include <stdint.h>
#include "gb_num.h"
#define HAND_CAP 8
#define JOKER_CAP 8
#define JOKER_RULE_BASE 5
#define DECK_CAP 128
#define HAND_STORAGE_CAP 128
#define HAND_VISIBLE_CAP 8
#define HAND_PLAY_CAP 5
#define CONSUMABLE_CAP 4
#define CONSUMABLE_RULE_BASE 2
#define PHASE_PACK 6
#define PHASE_BLOCKED 7
#define ITEM_JOKER 0
#define ITEM_PLANET 1
#define ITEM_TAROT 2
#define ITEM_SPECTRAL 3
#define ITEM_PLAYING 4
#define ENH_NONE 0
#define ENH_BONUS 1
#define ENH_MULT 2
#define ENH_WILD 3
#define ENH_GLASS 4
#define ENH_STEEL 5
#define ENH_STONE 6
#define ENH_GOLD 7
#define ENH_LUCKY 8
#define SEAL_NONE 0
#define SEAL_GOLD 1
#define SEAL_RED 2
#define SEAL_BLUE 3
#define SEAL_PURPLE 4
#define ED_NONE 0
#define ED_FOIL 1
#define ED_HOLO 2
#define ED_POLY 3
#define ED_NEGATIVE 4
#define PHASE_TITLE 0
#define PHASE_TABLE 1
#define PHASE_WIN 2
#define PHASE_SHOP 3
#define PHASE_LOSS 4
#define PHASE_COMPLETE 5
typedef struct {
 uint8_t rank, suit;
 uint16_t uid;
 uint8_t enhancement,seal,edition,flags;
 GbNum permanent_bonus;
} Card;
typedef struct {
 uint8_t phase, ante, blind, hands, discards, hand_count, deck_pos;
 uint8_t focus, joker_count, jokers[JOKER_CAP], shop[2];
 int32_t money;
 uint16_t seed;
 GbNum score, target, last_score;
 GbNum last_chips, last_mult;
 uint8_t last_hand;
 Card deck[DECK_CAP], hand[HAND_STORAGE_CAP];
 GbNum joker_state[JOKER_CAP];
 uint16_t hand_levels[12];
 uint8_t deck_count, draw_order[DECK_CAP];
 uint8_t shop_kind[2], consumable_count, consumable_kind[CONSUMABLE_CAP], consumables[CONSUMABLE_CAP];
 uint8_t ui_mode, consumable_focus, pack_ids[5], pack_count, pack_choices, pack_kind, pack_size[2], pack_bought[2];
 uint8_t boss_id,numeric_error,joker_debuff;
 uint8_t boss_disabled, boss_first_hand, boss_flipped, ectoplasm_used;
 int16_t hand_size;
 uint16_t boss_seen_hands, forced_card_uid;
 uint8_t joker_edition[JOKER_CAP], joker_eternal[JOKER_CAP], joker_sell[JOKER_CAP];
 uint8_t last_consumable_kind,last_consumable_id, gros_michel_extinct;
 uint16_t joker_uid[JOKER_CAP],next_joker_uid;
 uint8_t hands_max,discards_max;
 uint16_t probability_normal;
 uint16_t selected_uids[HAND_PLAY_CAP];
 uint8_t highlighted_count;
 uint8_t ox_target_hand,boss_uses[30];
 uint16_t next_card_uid;
 uint8_t joker_focus,joker_grab;
 uint16_t hands_played[12], hands_this_round[12];
 uint16_t used_planet_mask;
 uint8_t discards_used;
 uint8_t shop_pack_kind[2],first_shop_buffoon,pack_offer_focus;
 uint8_t pack_item_kind[5];
 Card pack_cards[5];
 uint16_t starting_deck_size;
 GbNum tarot_uses,joker_extra_value[JOKER_CAP];
 uint16_t price_inflation;
 uint8_t price_discount_percent;
 uint8_t consumable_edition[CONSUMABLE_CAP];
 uint16_t ectoplasm_uses;
} GameState;
extern GameState game;
extern uint16_t new_run_seed;
extern uint8_t game_action_error;
void game_new(uint16_t seed) CORE_BANKED;
void game_input(uint8_t pressed) CORE_BANKED;
void render_game(const GameState *state, uint8_t focus, uint8_t selected);
#endif
