#include "cooperative_native.h"
#include "paged_inventory.h"
#include "hand_api.h"
#include "boss.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "joker_missing.h"
#include <string.h>
#include "num_extra.h"
#define NUM_FATAL (GBN_DOMAIN | GBN_NAN | GBN_DIVZERO)
static uint8_t active(const GameState *g,uint8_t id){uint8_t i;
 for(i=0;i<g->joker_count;i++){if(!(i&3u)&&coop_active&&!coop_checkpoint())return 0;if(PG_JBYTE(g,i,jokers,0u)==id&&!(PG_DEBUFF(g,i)))return 1;}
 return 0;
}
static uint8_t face(const GameState *g,const Card *p){
 if(p->flags&1)return 0;
 return (p->enhancement!=ENH_STONE && p->rank>=11 && p->rank<=13) || active(g,J_PAREIDOLIA);
}
static uint8_t known(uint8_t id){switch(id){
 case J_STENCIL:case J_DNA:case J_FACELESS:case J_MIDAS_MASK:
 case J_EROSION:case J_FORTUNE_TELLER:case J_BULL:case J_SWASHBUCKLER:case J_BOOTSTRAPS:return 1;
 default:return 0;
}}
void joker_hand_policy(const GameState *g,JokerHandPolicy *o) CORE_BANKED {
 memset(o,0,sizeof(*o));o->straight_min=o->flush_min=5;
 if(!g || g->joker_count>PG_JMAX)return;
 if(active(g,J_FOUR_FINGERS))o->straight_min=o->flush_min=4;
 o->allow_single_rank_gaps=active(g,J_SHORTCUT);o->score_all=active(g,J_SPLASH);
}
void joker_hand_shapes(const GameState *g,const JokerContext *c,JokerHandShapes *o) CORE_BANKED {
 JokerHandPolicy p;uint8_t ranks[15]={0},i,j,rank,n=0,t=0,count,suit,group_previous=0,group_members=0,group_cards=0;
 memset(o,0,sizeof(*o));
 if(!g || g->hand_count>HAND_STORAGE_CAP || g->joker_count>PG_JMAX || !c || c->played_count>5)return;
 joker_hand_policy(g,&p);
 for(i=0;i<c->played_count;i++){uint8_t index=c->played_index[i];n++;rank=g->hand[index].rank;
  if(g->hand[index].enhancement!=ENH_STONE && rank>1 && rank<15)ranks[rank]|=1u<<i;
 }
 o->implemented=1;if(n>5 || n<p.straight_min)return;
 /* Original get_straight scans low Ace, 2..King, high Ace. One missing
    rank is allowed between occupied ranks; separated gaps can repeat. */
 {uint8_t node,limit=p.allow_single_rank_gaps?2u:1u;
  for(node=1;node<=14;node++) {
   uint8_t selected=ranks[node==1?14:node];
   if(!selected)continue;
   if(group_members && node-group_previous>limit) {
    if(group_members>=p.straight_min){o->straight_mask=group_cards;break;}
    group_members=0;group_cards=0;
   }
   group_previous=node;group_members++;group_cards|=selected;
  }
  if(!o->straight_mask && group_members>=p.straight_min)o->straight_mask=group_cards;
 }
 /* Lua suit preference: Spades, Hearts, Clubs, Diamonds. Wild matches each;
    Stone never matches, debuff is bypassed for hand recognition. */
 for(j=0;j<4;j++){
  suit=j==0?0:j==1?2:j==2?3:1;t=count=0;
  for(i=0;i<c->played_count;i++)if(g->hand[c->played_index[i]].enhancement!=ENH_STONE &&
   (g->hand[c->played_index[i]].enhancement==ENH_WILD || g->hand[c->played_index[i]].suit==suit ||
   (active(g,J_SMEARED) && ((g->hand[c->played_index[i]].suit==0 || g->hand[c->played_index[i]].suit==3)==(suit==0 || suit==3))))){count++;t|=1u<<i;}
  if(count>=p.flush_min){o->flush_mask=t;break;}
 }
}
void joker_missing(const GameState *g,uint8_t id,const JokerContext *c,
 const JokerMissingContext *x,JokerMissingEffect *o) CORE_BANKED {
 uint8_t i,count=0;uint16_t empty;GbNum amount,two,quotient,snapshot;
 memset(o,0,sizeof(*o));o->score.xmult_num=o->score.xmult_den=1;
 if(!g || !c || !x || !known(id) || g->hand_count>HAND_STORAGE_CAP || g->deck_count>DECK_CAP ||
    g->joker_count>PG_JMAX || c->joker_index>=g->joker_count || PG_JBYTE(g,c->joker_index,jokers,0u)!=id)return;
 if(PG_DEBUFF(g,c->joker_index)){o->implemented=o->score.implemented=1;return;}
 if(id==J_MIDAS_MASK && c->event==JE_BEFORE_HAND){
  o->implemented=1;if(c->copy_depth)return;
  for(i=0;i<g->hand_count;i++)if((hand_scoring(c,i)) && face(g,&g->hand[i]))o->gold_mask|=hand_score_bit(c,i);
  return;
 }
 if(id==J_DNA && c->event==JE_BEFORE_HAND){
  o->implemented=1;if(!c->first_hand || c->played_count!=1)return;
  for(i=0;i<g->hand_count;i++)if(hand_is_selected(g,i)){count++;o->cloned_card=g->hand[i];}
  if(count!=1){o->implemented=0;return;}
  /* Source has no blueprint guard; new UID assigned by core at commit. */
  o->cloned_card.flags&=~CARD_FACE_DOWN;o->cloned_card.uid=0;o->clone_card=o->clone_to_hand=1;return;
 }
 if(id==J_FACELESS && (c->event==JE_DISCARD||c->event==JE_DISCARD_CARD)){
  if(!x->discard_final_available)return;
  o->implemented=o->score.implemented=1;if(!x->is_last_discarded)return;
  for(i=0;i<g->hand_count;i++)if((hand_is_selected(g,i)) && face(g,&g->hand[i]))count++;
  if(count>=3)o->score.money_add=5;return;
 }
 if(c->event!=JE_MAIN)return;
 switch(id){
 case J_EROSION:
  if(!x->starting_size_available)return;
  if(x->starting_deck_size>g->deck_count){o->numeric_status=gb_num_from_u32(&o->score.mult_number,4UL*(x->starting_deck_size-g->deck_count));o->score.number_flags=2;}break;
 case J_FORTUNE_TELLER:
  if(!x->tarot_history_available)return;
  snapshot=x->tarot_uses;gb_num_from_u32(&amount,0);if(gb_num_cmp(&snapshot,&amount)==GBN_UNORDERED){o->numeric_status=GBN_NAN;return;}
  if(gb_num_cmp(&snapshot,&amount)>0){o->score.mult_number=x->tarot_uses;o->score.number_flags=2;}break;
 case J_SWASHBUCKLER:
  if(!x->sell_total_available)return;
  snapshot=x->other_joker_sell_total;gb_num_from_u32(&amount,0);if(gb_num_cmp(&snapshot,&amount)==GBN_UNORDERED){o->numeric_status=GBN_NAN;return;}
  if(gb_num_cmp(&snapshot,&amount)>0){o->score.mult_number=x->other_joker_sell_total;o->score.number_flags=2;}break;
 case J_STENCIL:
  if(!x->slot_limit_available)return;
  if(x->joker_slot_limit>g->joker_count){
   empty=x->joker_slot_limit-g->joker_count;
   for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==J_STENCIL)count++;
   /* Count even debuffed Stencils, as update loop does, but target must be active. */
   o->numeric_status=gb_num_from_u32(&o->score.xmult_number,(uint32_t)empty+count);o->score.number_flags=4;
  }break;
 case J_BULL:case J_BOOTSTRAPS:
  if(!x->wallet_available)return;
  snapshot=x->effective_nonnegative_wallet;gb_num_from_u32(&amount,0);if(gb_num_cmp(&snapshot,&amount)==GBN_UNORDERED){o->numeric_status=GBN_NAN;return;}
  if(gb_num_cmp(&snapshot,&amount)>0){
   gb_num_from_u32(&two,2);
   if(id==J_BULL){o->numeric_status=gb_num_mul(&o->score.chips_number,&two,&snapshot);o->score.number_flags=1;}
   else {
    gb_num_from_u32(&amount,5);o->numeric_status=gb_num_div(&quotient,&snapshot,&amount);
    if(o->numeric_status & NUM_FATAL)return;
    o->numeric_status|=num_floor(&quotient,&quotient);if(o->numeric_status & NUM_FATAL)return;
    gb_num_from_u32(&amount,1);
    if(gb_num_cmp(&quotient,&amount)>=0){o->numeric_status|=gb_num_mul(&o->score.mult_number,&two,&quotient);o->score.number_flags=2;}
   }
  }break;
 default:return;
 }
 if(o->numeric_status & NUM_FATAL)return;
 o->implemented=o->score.implemented=1;
}
