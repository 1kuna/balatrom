#ifndef FRONTEND_PUBLIC_PREVIEW_H
#define FRONTEND_PUBLIC_PREVIEW_H
#include <gb/gb.h>
#include "game.h"
#include "hand_preview.h"
/* Display-only consensus over visible faces/roles.
 * A hidden identity, effect, UID or actual core result never selects profiles. */
uint8_t frontend_public_preview_hidden(const GameState *g) BANKED;
/* Acquire renderer fields from public consensus before private recognition.
 * Unsupported or invalid public inputs retain the existing core fallback. */
void frontend_public_hand_preview(const GameState *g,HandPreview *out) BANKED;
/* Legacy invalidation compares public Joker faces/debuff marks only. Hidden and
 * inactive slots use the same token; reuse the caller's existing cache bytes. */
uint8_t frontend_public_jokers_key(const GameState *g,uint8_t key[5]) BANKED;
uint8_t frontend_public_jokers_same(const GameState *g,const uint8_t key[5],uint8_t debuff) BANKED;
#endif
