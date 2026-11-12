#ifndef OWNER_LEGACY_CARDS_H
#define OWNER_LEGACY_CARDS_H
#include <gb/gb.h>
void owner_legacy_cards_load(void) BANKED;
void owner_legacy_card_map(uint8_t,uint8_t *,uint8_t *) BANKED;
#endif
