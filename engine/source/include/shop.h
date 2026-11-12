#include "banking.h"
#ifndef SHOP_H
#define SHOP_H
#include "game.h"
uint16_t game_random(void) CORE_BANKED;
uint16_t game_random_bounded(uint16_t bound) CORE_BANKED;
uint8_t game_probability_hit(uint16_t denominator) CORE_BANKED;
uint8_t shop_can_spend(const GameState *g,uint16_t cost) CORE_BANKED;
uint8_t shop_item_price(const GameState *g,uint8_t slot) CORE_BANKED;
uint8_t shop_pack_price(const GameState *g,uint8_t slot) CORE_BANKED;
void shop_open(void) CORE_BANKED;
void shop_input(uint8_t pressed) CORE_BANKED;
void pack_input(uint8_t pressed) CORE_BANKED;
uint8_t consumable_use(uint8_t slot) CORE_BANKED;
extern const char *planet_names[12];
#endif
