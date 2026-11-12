#ifndef TASK13_JOKER_COUNTER_CONTEXT_H
#define TASK13_JOKER_COUNTER_CONTEXT_H
#include <stdint.h>
#define JCE_BEFORE_HAND 0
#define JCE_MAIN 1
#define JCE_AFTER_HAND 2
#define JCE_DISCARD_CARD 3
#define JCE_SKIP_BOOSTER 4
#define JCE_REROLL_SHOP 5
#define JCE_ROUND_END 6
#define JCE_SCORED_CARD 7
#define JCE_REPETITION_SCORED 8
#define JCE_CONSUMABLE_USED 9
#define JCE_CARDS_ADDED 10
#define JCE_CARD_SOLD 11
#define JCE_VAMPIRE_DRAINED 12
typedef struct {
 uint8_t event,blueprint,individual,repetition;
 uint8_t hand_predicates_available,straight,two_pair,full_house;
 uint8_t played_count_available,played_count;
 uint8_t scoring_faces_available,scoring_has_face;
 uint8_t discard_order_available,is_last_discarded;
 uint8_t rank_available;
 int16_t rank_id;
 uint8_t trigger_kind_available,trigger_kind,card_count_available,card_count;
 uint8_t round_boss_available,round_is_boss;
 uint8_t visible_history_available,other_visible_hand_ties_or_exceeds;
} JokerCounterContext;
#endif
