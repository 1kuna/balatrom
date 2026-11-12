#include "banking.h"
#ifndef JOKER_EFFECTS_H
#define JOKER_EFFECTS_H
#include "game.h"
#include "joker_ids.h"
/* Events run in left-to-right joker order. This API is provisional: numerical
   adapter replaces fixed fields before fractional/xmult effects are accepted. */
#define JE_BEFORE_HAND 0
#define JE_SCORED_CARD 1
#define JE_HELD_CARD 2
#define JE_MAIN 3
#define JE_AFTER_HAND 4
#define JE_DISCARD 5
#define JE_ROUND_END 6
#define JE_SHOP_END 7
#define JE_BLIND_BEGIN 8
#define JE_SELL 9
#define JE_CARDS_REMOVED 10
#define JE_CARDS_ADDED 11
#define JE_CONSUMABLE_USED 12
#define JE_ACQUIRE 13
#define JE_REMOVE 14
#define JE_SKIP_PACK 15
#define JE_REROLL 16
#define JE_REPETITION_SCORED 17
#define JE_REPETITION_HELD 18
#define JE_OTHER_JOKER 19
#define JE_PRE_DISCARD 20
#define JE_DISCARD_CARD 21
typedef struct {
 uint8_t event, joker_index, hand_type, played_count;
 uint8_t pairs,trips,quads,straight,flush;
 uint8_t card_index, rank, suit, scoring_count, retrigger;
 uint8_t first_hand,last_hand;
 GbNum chips,mult;
 uint8_t played_index[HAND_PLAY_CAP]; /* scoring_mask bits address these dense slots */
 uint8_t scoring_mask,enhancement,seal,edition,debuffed;
 uint8_t trigger_kind,trigger_id,trigger_index,card_count;
 uint8_t copy_depth,issuer_index,lucky_trigger,discard_last,discard_hook;
 const Card *cards;
} JokerContext;
typedef struct {
 int16_t chips_add,mult_add,money_add;
 uint16_t xmult_num,xmult_den;
 uint8_t repetitions, destroy_self, implemented,table_present,remove_card;
 uint8_t number_flags; /* bit0fullchips bit1fullmultadd bit2fullxmult */
 GbNum chips_number,mult_number,xmult_number;
} JokerEffect;
/* Pure effects only, return explicit implemented=0 for unsupported.
 No source state changes. Read GameState, use joker_state[] for mutable counters.
 Engine applies chips/addmult/xmult sequentially, with numerical backend pending.
 The exact event/context dependencies must be declared for every supported ID. */
void joker_effect(GameState *game,uint8_t id,JokerContext *context,JokerEffect *out) CORE_BANKED;
#endif
