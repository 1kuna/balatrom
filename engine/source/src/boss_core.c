#include "paged_inventory.h"
#include "native_run_rng.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "boss_optional.h"
#include "hand_api.h"
#include "events.h"
#include "scoring.h"
#include "shop.h"
#include "consumables.h"
#include "creation_core.h"
#include <string.h>
/* Native nonzero16-bit xorshift stream, uniform by rejection. Named desktop
   stream/seed parity is not represented by GameState. */
uint8_t boss_core_random_index(GameState *g,uint8_t count,uint8_t stream) CORE_BANKED {
 uint16_t v,limit;(void)stream;if(g!=&game || !count || !g->seed){g->numeric_error=1;return 255;}
 if(native_rng_mode())return game_random_bounded(count);
 limit=65535u-(65535u%count);do{v=game_random()-1u;}while(v>=limit);return v%count;
}
uint8_t boss_core_wheel_flip(GameState *g) CORE_BANKED {if(g!=&game){g->numeric_error=1;return 0;}return game_probability_hit(7);}
void boss_core_force_selection(GameState *g,uint16_t uid) CORE_BANKED {hand_force_uid(g,uid);}
uint8_t boss_jokers_hidden(const GameState *g) CORE_BANKED {return g->phase==PHASE_TABLE && g->boss_id==28 && !g->boss_disabled && pg_any_hidden(g);}
uint8_t boss_joker_hidden(const GameState *g,uint8_t index) CORE_BANKED {return boss_jokers_hidden(g) && index<g->joker_count && pg_hidden(g,index);}
void boss_core_hide_jokers(GameState*g,uint8_t hidden) CORE_BANKED {uint8_t i;pg_flags_clear(g,1);for(i=0;i<g->joker_count;i++)pg_flag_set(g,i,1,hidden);}
void boss_core_shuffle_jokers(GameState *g) CORE_BANKED {
 uint8_t i,j;if(g!=&game){g->numeric_error=1;return;}
 for(i=g->joker_count;i>1;i--){j=boss_core_random_index(g,i,28);if(g->numeric_error)return;if(j!=i-1){game_move_joker(i-1,j);game_move_joker(j+1,i-1);}}
}
/* All current native shop-admitted discard responders are dispatched per card;
   remove_card supports future discard destruction without charging resources. */
uint8_t boss_discard_selected(GameState *g,uint8_t hook) CORE_BANKED {
 Card cards[HAND_PLAY_CAP],destroyed[HAND_PLAY_CAP];uint8_t indices[HAND_PLAY_CAP],n,i,d=0;JokerContext c;
 if(g!=&game || (n=hand_selected_play_order(g,indices))==0)return 0;
 for(i=0;i<n;i++)cards[i]=g->hand[indices[i]];
 memset(&c,0,sizeof(c));c.event=JE_PRE_DISCARD;c.cards=cards;c.card_count=n;c.discard_hook=hook;
 gb_num_from_u32(&c.chips,1);gb_num_from_u32(&c.mult,1);joker_context_event(&c);if(g->numeric_error)return 0;
 for(i=0;i<n;i++) {
  if(!(cards[i].flags&CARD_DEBUFF)&&cards[i].seal==SEAL_PURPLE&&g->consumable_count<PG_CMAX)if(!creation_core_generate(ITEM_TAROT,CS_PURPLE)){g->numeric_error=1;return 0;}
  c.event=JE_DISCARD_CARD;c.card_index=indices[i];c.rank=cards[i].enhancement==ENH_STONE?0:cards[i].rank;c.suit=cards[i].suit;c.enhancement=cards[i].enhancement;c.edition=cards[i].edition;c.seal=cards[i].seal;c.debuffed=cards[i].flags&CARD_DEBUFF;c.discard_last=i+1==n;
  if(joker_discard_card_event(&c)){destroyed[d]=cards[i];if(cards[i].enhancement==ENH_GLASS)destroyed[d].flags|=8;d++;}if(g->numeric_error)return 0;
 }
 game_sync_hand();hand_commit_selected(g);if(g->boss_id==12)g->forced_card_uid=0;
 for(i=0;i<d;i++)if(!game_remove_uid(destroyed[i].uid)){g->numeric_error=1;return 0;}
 if(d)game_cards_removed(destroyed,d);
 return !g->numeric_error;
}
void boss_core_hook_discard(GameState *g,const uint16_t *uids,uint8_t count) CORE_BANKED {
 uint8_t i,j;hand_clear(g);for(i=0;i<count;i++)for(j=0;j<g->hand_count;j++)if(g->hand[j].uid==uids[i]){hand_toggle(g,j);break;}
 if(count && !boss_discard_selected(g,1))g->numeric_error=1;
}
void boss_stage_play(GameState *g) CORE_BANKED {
 Card played[HAND_PLAY_CAP];uint16_t insertion[HAND_PLAY_CAP],focused=0;uint8_t indices[HAND_PLAY_CAP],n,i,j;
 if(g->boss_id!=3 || g->boss_disabled){boss_press_play(g);return;}
 n=hand_selected_play_order(g,indices);if(!n)return;
 for(i=0;i<n;i++){played[i]=g->hand[indices[i]];insertion[i]=g->selected_uids[i];}
 if(g->focus<g->hand_count)focused=g->hand[g->focus].uid;
 hand_commit_selected(g);boss_press_play(g);
 /* Native scorer views a combined array; restore staged play at the right in
    original physical order. Held cards remained ordered and Hook saw none of
    these staged cards. Selection insertion order remains distinct. */
 for(i=0;i<n;i++)g->hand[g->hand_count++]=played[i];
 hand_clear(g);for(i=0;i<n;i++)g->selected_uids[i]=insertion[i];g->highlighted_count=n;
 for(j=0;j<g->hand_count;j++)if(g->hand[j].uid==focused){g->focus=j;break;}
}

void boss_mark_played(GameState *g) CORE_BANKED {uint8_t i,j;for(i=0;i<g->hand_count;i++)if(hand_is_selected(g,i)){g->hand[i].flags|=CARD_PLAYED_ANTE;for(j=0;j<g->deck_count;j++)if(g->deck[j].uid==g->hand[i].uid){g->deck[j].flags|=CARD_PLAYED_ANTE;break;}}}

void boss_refresh_owned(GameState *g) CORE_BANKED {uint8_t i;for(i=0;i<g->deck_count;i++)boss_card(g,&g->deck[i]);for(i=0;i<g->hand_count;i++)boss_card(g,&g->hand[i]);}
