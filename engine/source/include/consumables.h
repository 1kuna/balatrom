#include "banking.h"
#ifndef CONSUMABLES_H
#define CONSUMABLES_H
#include "game.h"
/* IDs exactly reference/catalog.json Tarot/Planet/Spectral order. Worker owns
 this implementation only; unsupported=0, supported/applicable=1. Conditions
 are enforced before consuming the inventory card. Selected bits target hand.
 All mutation cards preserve uid; sync after applies persistent master updates. */
uint8_t consumable_apply(GameState *g,uint8_t kind,uint8_t id) CORE_BANKED;
void game_sync_hand(void) CORE_BANKED;
uint8_t game_add_card(const Card *source) CORE_BANKED;
uint8_t game_remove_uid(uint16_t uid) CORE_BANKED;
#endif
