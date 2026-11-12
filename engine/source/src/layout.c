#include "game.h"
#include <stddef.h>
/* Build-specific ABI evidence for emulator QA; no game mutation hooks. */
const uint16_t qa_layout[]={sizeof(GameState),offsetof(GameState,phase),offsetof(GameState,money),offsetof(GameState,score),offsetof(GameState,target),offsetof(GameState,hand),offsetof(GameState,deck),offsetof(GameState,hand_levels),offsetof(GameState,consumable_count),offsetof(GameState,consumables),offsetof(GameState,consumable_kind),offsetof(GameState,shop_kind),offsetof(GameState,pack_count),offsetof(GameState,pack_ids),sizeof(Card),offsetof(GameState,seed),offsetof(GameState,deck_count),offsetof(GameState,joker_state)};
