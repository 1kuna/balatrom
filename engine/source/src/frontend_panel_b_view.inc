#include "paged_inventory.h"
/* Pure actual-state projection for the panel B compositor. No palette policy,
 * drawing, rules, RNG, saves, global caches, or copied GameState. */
#ifndef FRONTEND_PANEL_B_VIEW_INC
#define FRONTEND_PANEL_B_VIEW_INC
#include "game.h"
#include "hand_api.h"
#include "boss.h"
#include "boss_optional.h"
#include <string.h>
#define PANEL_B_CARD_WIDTH 19u
#define PANEL_B_CARD_HEIGHT 30u
#define PANEL_B_CARD_SHADOW_WIDTH 20u
#define PANEL_B_JOKER_WIDTH 19u
#define PANEL_B_JOKER_HEIGHT 27u
#define PANEL_B_FACE 1u
#define PANEL_B_HIDDEN 2u
#define PANEL_B_STONE 3u
#define PANEL_B_INVALID 4u
#define PANEL_B_SELECTED 1u
#define PANEL_B_FOCUSED 2u
#define PANEL_B_BELL 4u
#define PANEL_B_DEBUFF 8u
#define PANEL_B_GRABBED 16u

typedef struct { uint8_t first, count, selected_mask, offpage_selected; } PanelBPage;
typedef struct {
    uint16_t uid;
    uint8_t x, y, kind, rank, suit, enhancement, edition, seal, cues;
} PanelBCard;
typedef struct { uint16_t uid; uint8_t x, y, kind, id, edition, cues; } PanelBJoker;

#ifndef PANEL_B_TYPES_ONLY
/* Fixed ROM keeps this bounded projection outside the crowded renderer bank.
 * hand_page uses its declared banked ABI and restores the caller's ROM bank. */
static uint8_t panel_b_page(const GameState *g, PanelBPage *out) NONBANKED {
    uint8_t i, selected = 0;
    memset(out, 0, sizeof(*out));
    if (!g || g->hand_count > HAND_STORAGE_CAP ||
        g->highlighted_count > HAND_PLAY_CAP ||
        (g->hand_count && g->focus >= g->hand_count)) return 0;
    out->count = hand_page(g, &out->first, &out->selected_mask);
    for (i = 0; i < out->count; i++) if (out->selected_mask & (1u << i)) selected++;
    if (selected > g->highlighted_count) return 0;
    out->offpage_selected = g->highlighted_count - selected;
    return 1;
}

/* Enable entity commands in the compositor/canary, not the legacy renderer
 * checkpoint that currently consumes only panel_b_page. */
#ifdef PANEL_B_ENTITY_VIEWS
static uint8_t panel_b_card_view(const GameState *g, const PanelBPage *page,
                               uint8_t slot, PanelBCard *out) {
    const Card *card;
    uint16_t index = (uint16_t)page->first + slot;
    memset(out, 0, sizeof(*out));
    if (slot >= HAND_VISIBLE_CAP || slot >= page->count ||
        index >= g->hand_count || index >= HAND_STORAGE_CAP) return 0;
    card = &g->hand[index];
    out->uid = card->uid; /* Internal identity; never encoded as hidden face ink. */
    out->x = 40u + slot * 14u;
    out->y = page->selected_mask & (1u << slot) ? 92u : 100u;
    if (page->selected_mask & (1u << slot)) out->cues |= PANEL_B_SELECTED;
    if (g->focus == index) out->cues |= PANEL_B_FOCUSED;
    if (g->boss_id == 12u && !g->boss_disabled &&
        g->forced_card_uid == card->uid) out->cues |= PANEL_B_BELL;
    /* Concealment precedes every face-dependent field, including overlays. */
    if (card->flags & CARD_FACE_DOWN) { out->kind = PANEL_B_HIDDEN; return 1; }
    if (card->flags & CARD_DEBUFF) out->cues |= PANEL_B_DEBUFF;
    out->enhancement = card->enhancement;
    out->edition = card->edition;
    out->seal = card->seal;
    if (card->enhancement == ENH_STONE) { out->kind = PANEL_B_STONE; return 1; }
    if (card->rank < 2u || card->rank > 14u || card->suit > 3u) {
        out->kind = PANEL_B_INVALID; return 1;
    }
    out->kind = PANEL_B_FACE;
    out->rank = card->rank;
    out->suit = card->suit;
    return 1;
}

#include "inventory_bounds.h"
static uint8_t panel_b_joker_view(const GameState *g, uint8_t slot, PanelBJoker *out) {
    uint8_t xslot=slot;slot+=inventory_joker_first(g);memset(out, 0, sizeof(*out));
    if (slot >= PG_JMAX || slot >= g->joker_count) return 0;
    out->uid = PG_JUID(g,slot);
    out->x = 40u + xslot * 23u;
    out->y = 7u;
    if (g->ui_mode == 2u && g->joker_focus == slot) {
        out->cues |= PANEL_B_FOCUSED;
        if (g->joker_grab) out->cues |= PANEL_B_GRABBED;
    }
    if (boss_joker_hidden(g, slot)) { out->kind = PANEL_B_HIDDEN; return 1; }
    if (PG_JBYTE(g,slot,jokers,0u) >= 150u) { out->kind = PANEL_B_INVALID; return 1; }
    out->kind = PANEL_B_FACE;
    out->id = PG_JBYTE(g,slot,jokers,0u);
    out->edition = PG_JBYTE(g,slot,joker_edition,9u);
    if (PG_DEBUFF(g,slot)) out->cues |= PANEL_B_DEBUFF;
    return 1;
}
#endif
#endif /* PANEL_B_TYPES_ONLY */
#endif
