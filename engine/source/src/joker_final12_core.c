#include "paged_inventory.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "joker_final12_core.h"
#include <string.h>
#define FATAL (GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)
uint8_t joker_final12_core(GameState *g,uint8_t id,const JokerContext *c,
 JokerEffect *out) CORE_BANKED {
 JokerMissingContext x;JokerMissingEffect e;uint8_t i;
 if(!g || !c || !out || g->joker_count>PG_JMAX ||
  g->hand_count>HAND_STORAGE_CAP || g->deck_count>DECK_CAP ||
  c->joker_index>=g->joker_count || PG_JBYTE(g,c->joker_index,jokers,0u)!=id)return 0;
 if(id!=J_STENCIL && id!=J_FACELESS && id!=J_BULL && id!=J_BOOTSTRAPS)return 0;
 memset(&x,0,sizeof(x));
 if(id==J_STENCIL){
  x.slot_limit_available=1;x.joker_slot_limit=JOKER_RULE_BASE;
  for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,joker_edition,9u)==ED_NEGATIVE)x.joker_slot_limit++;
 }else if(id==J_FACELESS){
  /* game.c dispatches one JE_DISCARD while the complete selected set is still
     in hand, before any removal: equivalent to source last-card predicate. */
  x.discard_final_available=1;x.is_last_discarded=c->event==JE_DISCARD_CARD?c->discard_last:1;
 }else {
  x.wallet_available=1;
  gb_num_from_u32(&x.effective_nonnegative_wallet,g->money>0?(uint32_t)g->money:0);
 }
 joker_missing(g,id,c,&x,&e);
 if(e.numeric_status&FATAL){g->numeric_error=1;return 1;}
 if(!e.score.implemented)return 0;
 *out=e.score;return 1;
}
