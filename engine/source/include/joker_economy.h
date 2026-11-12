#include "banking.h"
#ifndef TASK13_JOKER_ECONOMY_H
#define TASK13_JOKER_ECONOMY_H
#include "joker_effects.h"
typedef struct {
 uint8_t joker_index,debuffed,discards_used_available,discards_used,discards_left;
 uint8_t planet_history_available;
 uint16_t used_planet_mask;
} JokerPayoutContext;
typedef struct {GbNum dollars;uint8_t implemented,numeric_status;} JokerPayoutEffect;
void joker_dollar_bonus(const GameState *game,uint8_t id,const JokerPayoutContext *context,JokerPayoutEffect *out) CORE_BANKED;
#define JPE_ACQUIRE 0
#define JPE_REMOVE 1
typedef struct {uint8_t event,target_identity_available,is_target;} JokerPassiveContext;
typedef struct {
 int8_t hand_size_delta,discards_max_delta,discards_current_delta,hands_max_delta;
 int8_t free_rerolls_delta,interest_amount_delta;
 int16_t bankrupt_floor_delta;
 uint8_t probability_num,probability_den,reprice_all,implemented;
} JokerPassiveEffect;
void joker_passive(uint8_t id,const JokerPassiveContext *context,JokerPassiveEffect *out) CORE_BANKED;
#endif
