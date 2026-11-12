#ifndef BOSS_OPTIONAL_H
#define BOSS_OPTIONAL_H
#include "boss_extensions.h"
/* All extended capabilities require actual core integration, not just a define. */
uint8_t boss_optional_supported(uint8_t id) CORE_BANKED;
void boss_optional_begin(GameState *g) CORE_BANKED;
uint8_t boss_optional_card(GameState *g,const Card *c) CORE_BANKED;
void boss_optional_after_draw(GameState *g) CORE_BANKED;
void boss_optional_disable(GameState *g) CORE_BANKED;
uint8_t boss_card_is_face(GameState *g,const Card *c) CORE_BANKED;
/* Call after selection validation, BEFORE selected cards leave the hand. */
uint8_t boss_play_allowed(GameState *g) CORE_BANKED;
uint8_t boss_can_deselect(GameState *g,uint16_t uid) CORE_BANKED;
/* Call AFTER played cards leave hand, BEFORE classification/joker callbacks. */
void boss_press_play(GameState *g) CORE_BANKED;
/* Call for a real player discard, before new cards draw. Hook is excluded. */
void boss_on_discard(GameState *g) CORE_BANKED;
/* Call on each newly drawn card, not on sort or arbitrary card mutation. */
void boss_on_card_draw(GameState *g,Card *c) CORE_BANKED;
/* Core calls BEFORE drawing; reason=0blindstart,1play,2discard,3other. */
uint8_t boss_draw_count(GameState *g,uint8_t requested,uint8_t reason) CORE_BANKED;
/* Sale event only; joker destruction/removal must not trigger Leaf. */
void boss_on_joker_sold(GameState *g) CORE_BANKED;
/* Round defeat: restore Manacle size WITHOUT drawing, reveal Acorn. */
void boss_optional_defeat(GameState *g) CORE_BANKED;

#if defined(BOSS_HAVE_HOOK) || defined(BOSS_HAVE_BELL) || defined(BOSS_HAVE_HEART)
/* Return index in [0,count); stream=3Hook,12Bell,29Heart. RNG error sets numeric_error. */
uint8_t boss_core_random_index(GameState *g,uint8_t count,uint8_t stream) CORE_BANKED;
#endif
#ifdef BOSS_HAVE_HOOK
/* Forced discard of held mask: full discard/seal/destroy events, no resource debit,
   no discards_used increment, no refill. The played cards are staged separately. */
void boss_core_hook_discard(GameState *g,const uint16_t *uids,uint8_t count) CORE_BANKED;
#endif
#ifdef BOSS_HAVE_BELL
/* Clear highlights and force/highlight supplied UID. UID0 clears force only. */
void boss_core_force_selection(GameState *g,uint16_t uid) CORE_BANKED;
#endif
#ifdef BOSS_HAVE_WHEEL
/* Draw exactly one source-style Bernoulli test probability_normal / 7. */
uint8_t boss_core_wheel_flip(GameState *g) CORE_BANKED;
#endif
#ifdef BOSS_HAVE_ACORN
/* Preserve complete joker metadata/UIDs; shuffle called three times. */
void boss_core_shuffle_jokers(GameState *g) CORE_BANKED;
void boss_core_hide_jokers(GameState *g,uint8_t hidden) CORE_BANKED;
#endif
void boss_refresh_owned(GameState *g) CORE_BANKED;
void boss_mark_played(GameState *g) CORE_BANKED;
void boss_stage_play(GameState *g) CORE_BANKED;
uint8_t boss_discard_selected(GameState *g,uint8_t hook) CORE_BANKED;
uint8_t boss_joker_hidden(const GameState *g,uint8_t index) CORE_BANKED;
uint8_t boss_jokers_hidden(const GameState *g) CORE_BANKED;
#endif
