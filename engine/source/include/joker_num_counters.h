#include "banking.h"
#ifndef TASK13_JOKER_NUM_COUNTERS_H
#define TASK13_JOKER_NUM_COUNTERS_H
#include "joker_counter_context.h"
#include "joker_ids.h"
#include "joker_effects.h"
#include "gb_num.h"
#define JNP_CHIPS 1
#define JNP_MULT 2
#define JNP_XMULT 4
typedef struct {
 GbNum chips_add,mult_add,xmult;
 uint8_t payload_flags,repetitions,destroy_self,implemented,numeric_status,recalculate_sell_cost;
} JokerNumCounterEffect;
/* Separate optional numeric adapter. No GameState or shared JokerEffect ABI
   assumptions. Caller must copy flagged payloads into its exact effect ABI. */
uint8_t joker_num_counter_init(uint8_t id,GbNum *state) CORE_BANKED;
uint8_t joker_initialize(uint8_t id,GbNum *state) CORE_BANKED;
void joker_num_counter_event(uint8_t id,GbNum *state,const JokerCounterContext *context,JokerNumCounterEffect *out) CORE_BANKED;
uint8_t joker_steel_factor(uint8_t steel_count,GbNum *out) CORE_BANKED;
void joker_num_counter_game_event(GameState *game,uint8_t id,const JokerContext *context,uint8_t blueprint,JokerEffect *out) CORE_BANKED;
#endif
