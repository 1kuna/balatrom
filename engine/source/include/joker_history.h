#ifndef JOKER_HISTORY_H
#define JOKER_HISTORY_H
#include "joker_effects.h"
/* RAM operands; numerical backend nonreentrant. Tail fields persist in schema6.
   Run reset after actual deck construction, before first blind. */
uint8_t joker_history_reset(GameState *g) CORE_BANKED;
uint8_t joker_history_valid(const GameState *g) CORE_BANKED;
/* One invocation per successful actual consumable use, never per Joker callback.
   The caller is game_consumable_used (inventory, pack and creation-owner uses). */
uint8_t joker_history_used(GameState *g,uint8_t kind,uint8_t id) CORE_BANKED;
uint8_t joker_history_price_policy(GameState *g,uint16_t inflation,uint8_t discount_percent) CORE_BANKED;
uint8_t joker_history_sell_value(const GameState *g,uint8_t index,GbNum *out) CORE_BANKED;
void joker_history_sync_price(GameState *g,uint8_t index) CORE_BANKED;
uint8_t joker_history_sale_amount(const GameState *g,uint8_t index,int32_t *out) CORE_BANKED;
uint8_t joker_history_sell_total(const GameState *g,uint16_t excluded_uid,GbNum *out) CORE_BANKED;
/* Price owner updates original ability.extra_value by UID. Value must be valid
   nonnegative binary64 (Infinity allowed). Global inflation/discount come from the persisted price policy. Rental stickers
   are not represented in this save schema. */
uint8_t joker_history_extra_add(GameState *g,uint16_t uid,const GbNum *delta) CORE_BANKED;
/* Exact engine binding for IDs80/85/109; returns1 if score effect handled. */
uint8_t joker_history_effect(GameState *g,uint8_t id,const JokerContext *c,JokerEffect *out) CORE_BANKED;
#endif
