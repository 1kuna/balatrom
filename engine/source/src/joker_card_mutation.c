#include "paged_inventory.h"
#include "hand_api.h"
#pragma bank 255
#include "joker_card_mutation.h"
#include "shop.h"
#include "boss.h"
#include <string.h>
static uint8_t active_face(const GameState *g,const Card *card) {
 uint8_t i;
 if(card->flags&1)return 0;
 if(card->enhancement!=ENH_STONE && card->rank>=11 && card->rank<=13)return 1;
 for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==J_PAREIDOLIA && !(PG_DEBUFF(g,i)))return 1;
 return 0;
}
uint8_t joker_card_mutation_effect(GameState *g,uint8_t id,const JokerContext *c,
 JokerEffect *o,uint8_t roll_ready,uint8_t roll_value) CORE_BANKED {
 Card *card;GbNum next;uint8_t status,i;
 memset(o,0,sizeof(*o));o->xmult_num=o->xmult_den=1;
 if(id!=J_HIKER && id!=J_RESERVED_PARKING && id!=J_BUSINESS && id!=J_MIDAS_MASK && id!=J_MISPRINT && id!=J_BLOODSTONE)return JCM_UNHANDLED;
 o->implemented=1;
 if(g->hand_count>HAND_STORAGE_CAP || g->deck_count>DECK_CAP || g->joker_count>PG_JMAX ||
    c->joker_index>=g->joker_count || PG_JBYTE(g,c->joker_index,jokers,0u)!=id){g->numeric_error=1;return JCM_ERROR;}
 if(PG_DEBUFF(g,c->joker_index))return JCM_DONE;
 if(id==J_MISPRINT){
  if(c->event!=JE_MAIN)return JCM_DONE;
  if(!roll_ready)return JCM_MISPRINT_ROLL;
  if(roll_value>=24){g->numeric_error=1;return JCM_ERROR;}
  o->mult_add=roll_value;o->table_present=1;return JCM_DONE;
 }
 if(id==J_BLOODSTONE){
  uint8_t hearts=0;
  if(c->event!=JE_SCORED_CARD)return JCM_DONE;
  if(c->card_index>=g->hand_count){g->numeric_error=1;return JCM_ERROR;}
  card=&g->hand[c->card_index];
  if(c->debuffed||(card->flags&CARD_DEBUFF)||card->enhancement==ENH_STONE)return JCM_DONE;
  if(card->enhancement==ENH_WILD||card->suit==2)hearts=1;
  else if(card->suit==1)for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==J_SMEARED&&!pg_debuff(g,i))hearts=1;
  if(!hearts)return JCM_DONE;
  if(!roll_ready)return JCM_PARKING_ROLL;
  if(roll_value){o->xmult_num=3;o->xmult_den=2;o->table_present=1;}
  return JCM_DONE;
 }
 if(id==J_MIDAS_MASK){
  if(c->event==JE_BEFORE_HAND && !c->copy_depth){
   for(i=0;i<g->hand_count;i++)if(hand_scoring(c,i)&&active_face(g,&g->hand[i])){
    uint8_t j;g->hand[i].enhancement=ENH_GOLD;boss_card(g,&g->hand[i]);
    for(j=0;j<g->deck_count;j++)if(g->deck[j].uid==g->hand[i].uid){g->deck[j].enhancement=ENH_GOLD;boss_card(g,&g->deck[j]);}
   }
  }
  return JCM_DONE;
 }
 if((id==J_HIKER && c->event!=JE_SCORED_CARD) || (id==J_BUSINESS && c->event!=JE_SCORED_CARD) ||
    (id==J_RESERVED_PARKING && c->event!=JE_HELD_CARD))return JCM_DONE;
 if(c->card_index>=g->hand_count){g->numeric_error=1;return JCM_ERROR;}
 card=&g->hand[c->card_index];
 if(c->debuffed || (card->flags&1))return JCM_DONE;
 if(id==J_HIKER) {
  /* Source has no blueprint or retrigger guard: mutate on EVERY evaluation.
     Card base chips must have been evaluated before this individual phase. */
  status=gb_num_add_i16(&next,&card->permanent_bonus,5);
  if(status&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)){g->numeric_error=1;return JCM_ERROR;}
  card->permanent_bonus=next;
  /* Stable UID, never rank/suit, identifies the permanent master card. This
     also survives native remove/refill and copies with identical faces. */
  for(i=0;i<g->deck_count;i++)if(g->deck[i].uid==card->uid)g->deck[i].permanent_bonus=next;
  o->table_present=1;return JCM_DONE;
 }
 if(!active_face(g,card))return JCM_DONE;
 if(!roll_ready)return JCM_PARKING_ROLL;
 if(roll_value){o->money_add=id==J_BUSINESS?2:1;o->table_present=1;}
 /* Emitted money is applied by core game_adjust_money, with overflow failure
    propagated. Do not pay now: held first-run tables are captured before apply. */
 return JCM_DONE;
}
void joker_card_mutation_game_event(GameState *g,uint8_t id,const JokerContext *c,
 JokerEffect *o) CORE_BANKED {
 uint8_t status,hit;
 status=joker_card_mutation_effect(g,id,c,o,0,0);
 if(status==JCM_MISPRINT_ROLL){
  if(g!=&game){g->numeric_error=1;return;}
  hit=game_random_bounded(24);
  if(g->numeric_error)return;
  joker_card_mutation_effect(g,id,c,o,1,hit);return;
 }
 if(status!=JCM_PARKING_ROLL)return;
 if(g!=&game){g->numeric_error=1;return;}
 hit=game_probability_hit(2); /* one logical draw; unbiased native buckets, not the desktop game's streams */
 joker_card_mutation_effect(g,id,c,o,1,hit);
}
