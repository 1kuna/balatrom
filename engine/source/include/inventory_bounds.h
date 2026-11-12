#ifndef INVENTORY_BOUNDS_H
#define INVENTORY_BOUNDS_H
#include "game.h"
uint8_t inventory_joker_limit(const GameState*) CORE_BANKED;
uint16_t inventory_consumable_limit(const GameState*) CORE_BANKED;
uint8_t inventory_joker_room(const GameState*,uint8_t) CORE_BANKED;
uint8_t inventory_consumable_room(const GameState*,uint8_t) CORE_BANKED;
uint8_t inventory_shop_end_allowed(const GameState*) CORE_BANKED;
uint8_t inventory_perkeo(GameState*) CORE_BANKED;
uint8_t inventory_valid(const GameState*) CORE_BANKED;
uint8_t inventory_joker_first(const GameState*) CORE_BANKED;
uint8_t inventory_consumable_first(const GameState*) CORE_BANKED;
uint8_t inventory_joker_edition(const GameState*,uint8_t) CORE_BANKED;
uint8_t inventory_native_dirty(const GameState*) CORE_BANKED;
void inventory_caption(const GameState*,uint8_t,char*) CORE_BANKED;
uint8_t game_generate_consumable_edition(uint8_t,uint8_t,uint8_t) CORE_BANKED;
#endif
