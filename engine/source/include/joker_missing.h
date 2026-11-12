#ifndef TASK13_JOKER_MISSING_H
#define TASK13_JOKER_MISSING_H
#include "joker_effects.h"
/* Exact externally maintained snapshots; never infer absent history as zero. */
typedef struct {
 uint8_t starting_size_available,tarot_history_available,wallet_available;
 uint8_t slot_limit_available,sell_total_available,discard_final_available;
 uint8_t is_last_discarded;
 uint16_t starting_deck_size,joker_slot_limit;
 GbNum tarot_uses,effective_nonnegative_wallet,other_joker_sell_total;
} JokerMissingContext;
/* Atomic proposals, not committed mutations. Core validates UIDs/capacity and
   all reactive hooks before applying. Card indices refer to current hand. */
typedef struct {
 JokerEffect score;
 uint8_t implemented,numeric_status,gold_mask;
 uint8_t clone_card,clone_to_hand;
 Card cloned_card;
} JokerMissingEffect;
void joker_missing(const GameState *g,uint8_t id,const JokerContext *c,
 const JokerMissingContext *x,JokerMissingEffect *out) CORE_BANKED;
/* Policy only; joker_hand_shapes applies it in the straight/flush scan. */
typedef struct {uint8_t straight_min,flush_min,allow_single_rank_gaps,score_all;} JokerHandPolicy;
void joker_hand_policy(const GameState *g,JokerHandPolicy *out) CORE_BANKED;
typedef struct {uint8_t implemented,straight_mask,flush_mask;} JokerHandShapes;
void joker_hand_shapes(const GameState *g,const JokerContext *c,JokerHandShapes *out) CORE_BANKED;
#endif
