#include "paged_inventory.h"
#include "hand_api.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "joker_effects.h"
#include "joker_config.h"
#include <string.h>
/* Domain: original default Joker parameters, ordinary rank/suit cards, no
   debuffs/stickers/editions. The engine must omit debuffed-card/Joker effects.
   Context predicates represent contained poker hands, not exclusive hand type.
   This module does not implement acquisition/passive/lifecycle behavior. */
static uint8_t has_joker(const GameState *g,uint8_t id) {
 uint8_t i; for(i=0;i<g->joker_count && i<PG_JMAX;i++)if(PG_JBYTE(g,i,jokers,0u)==id && !(PG_DEBUFF(g,i)))return 1;return 0;
}
static uint8_t face(const GameState *g,uint8_t rank) {
 return (rank>=11 && rank<=13) || has_joker(g,J_PAREIDOLIA);
}
/* Native suit convention: Spades=0, Diamonds=1, Hearts=2, Clubs=3.
   Smeared merges red suits and black suits. Wild/Stone require richer context. */
static uint8_t suit_is(const GameState *g,uint8_t actual,uint8_t expected,uint8_t enhancement) {
 if(enhancement==ENH_STONE)return 0;
 if(enhancement==ENH_WILD)return 1;
 if(actual>3)return 0;
 if(actual==expected)return 1;
 if(!has_joker(g,J_SMEARED))return 0;
 return ((actual==1 || actual==2)==(expected==1 || expected==2));
}
static uint8_t card_suit(const GameState *g,const Card *card,uint8_t expected,uint8_t bypass,uint8_t flush_calc){
 if(card->enhancement==ENH_STONE)return 0;
 if(flush_calc){
  if(card->enhancement==ENH_WILD && !(card->flags&1))return 1;
  return suit_is(g,card->suit,expected,ENH_NONE);
 }
 if((card->flags&1) && !bypass)return 0;
 return suit_is(g,card->suit,expected,card->enhancement);
}
static uint8_t card_face(const GameState *g,const Card *card){
 if(card->flags&1)return 0;
 return face(g,card->enhancement==ENH_STONE?0:card->rank);
}
static uint8_t all_suits(const GameState *g,const JokerContext *c,uint8_t seeing){
 static const uint8_t flower_order[4]={2,1,0,3};
 static const uint8_t seeing_order[4]={3,1,0,2};
 uint8_t have=0,i,j,suit;const Card *card;
 for(i=0;i<g->hand_count;i++)if(hand_scoring(c,i)){
  card=&g->hand[i];if(card->enhancement==ENH_WILD)continue;
  for(j=0;j<4;j++){
   suit=flower_order[j];
   if(card_suit(g,card,suit,seeing?0:1,0) && (seeing || !(have&(1u<<suit)))){
    have|=1u<<suit;if(!seeing)break;
   }
  }
 }
 for(i=0;i<g->hand_count;i++)if(hand_scoring(c,i)){
  card=&g->hand[i];if(card->enhancement!=ENH_WILD)continue;
  for(j=0;j<4;j++){
   suit=seeing?seeing_order[j]:flower_order[j];
   if(!(have&(1u<<suit)) && card_suit(g,card,suit,0,0)){have|=1u<<suit;break;}
  }
 }
 return seeing?((have&8) && (have&7)):have==15;
}
void joker_effect(GameState *g,uint8_t id,JokerContext *c,JokerEffect *o) CORE_BANKED {
 uint8_t pair,three,two_pair,i,tally,lowest;GbNum fraction,count,product,one;
 memset(o,0,sizeof(*o));o->xmult_num=o->xmult_den=1;
 if(!g || !c)return;
 if(g->hand_count>HAND_STORAGE_CAP || g->joker_count>PG_JMAX)return;
 if(c->joker_index<PG_JMAX && (PG_DEBUFF(g,c->joker_index))){o->implemented=1;return;}
 pair=c->pairs || c->trips || c->quads;
 three=c->trips || c->quads;
 two_pair=c->pairs>=2 || (c->pairs && c->trips);
 if(c->event==JE_MAIN){
  if(g->deck_count>DECK_CAP)return;
  o->implemented=1;
  switch(id){
   case J_JOKER:o->mult_add=4;break;
   case J_JOLLY:if(pair)o->mult_add=8;break;
   case J_ZANY:if(three)o->mult_add=12;break;
   case J_MAD:if(two_pair)o->mult_add=10;break;
   case J_CRAZY:if(c->straight)o->mult_add=12;break;
   case J_DROLL:if(c->flush)o->mult_add=10;break;
   case J_SLY:if(pair)o->chips_add=50;break;
   case J_WILY:if(three)o->chips_add=100;break;
   case J_CLEVER:if(two_pair)o->chips_add=80;break;
   case J_DEVIOUS:if(c->straight)o->chips_add=100;break;
   case J_CRAFTY:if(c->flush)o->chips_add=80;break;
   case J_HALF:if(c->played_count<=3)o->mult_add=20;break;
   case J_ABSTRACT:o->mult_add=3*g->joker_count;break;
   case J_BANNER:o->chips_add=30*g->discards;break;
   case J_MYSTIC_SUMMIT:if(g->discards==0)o->mult_add=15;break;
   case J_BLUE_JOKER:
    /* v03 draw pile is the unconsumed suffix of draw_order[]. */
    if(g->deck_count>DECK_CAP || g->deck_pos>g->deck_count){o->implemented=0;break;}
    o->chips_add=2*(g->deck_count-g->deck_pos);break;
   case J_STONE:
    tally=0;for(i=0;i<g->deck_count;i++)if(g->deck[i].enhancement==ENH_STONE)tally++;
    o->chips_add=25*tally;break;
   case J_STEEL_JOKER:
    tally=0;for(i=0;i<g->deck_count;i++)if(g->deck[i].enhancement==ENH_STEEL)tally++;
    if(gb_num_ratio(&fraction,1,5) || gb_num_from_u32(&count,tally) ||
       gb_num_from_u32(&one,1) || gb_num_mul(&product,&fraction,&count) ||
       gb_num_add(&o->xmult_number,&one,&product)){o->implemented=0;break;}
    o->number_flags=4;break;
   case J_DRIVERS_LICENSE:
    tally=0;for(i=0;i<g->deck_count;i++)if(g->deck[i].enhancement!=ENH_NONE)tally++;
    if(tally>=16)o->xmult_num=3;break;
   case J_SUPERNOVA:
    /* Engine increments selected hand history before scoring, as stock does. */
    if(c->hand_type>=12){o->implemented=0;break;}
    if(gb_num_from_u32(&o->mult_number,g->hands_played[c->hand_type])){o->implemented=0;break;}
    o->number_flags=2;break;
   case J_CARD_SHARP:
    if(c->hand_type>=12){o->implemented=0;break;}
    if(g->hands_this_round[c->hand_type]>1)o->xmult_num=3;break;
   case J_BLACKBOARD:
    tally=1;for(i=0;i<g->hand_count;i++)if(!(hand_is_selected(g,i)))
     if(!card_suit(g,&g->hand[i],3,0,1) && !card_suit(g,&g->hand[i],0,0,1))tally=0;
    if(tally)o->xmult_num=3;break;
   case J_FLOWER_POT:if(all_suits(g,c,0))o->xmult_num=3;break;
   case J_SEEING_DOUBLE:if(all_suits(g,c,1))o->xmult_num=2;break;
   case J_ACROBAT:if(c->last_hand)o->xmult_num=3;break;
   case J_DUO:if(pair)o->xmult_num=2;break;
   case J_TRIO:if(three)o->xmult_num=3;break;
   case J_FAMILY:if(c->quads)o->xmult_num=4;break;
   case J_ORDER:if(c->straight)o->xmult_num=3;break;
   case J_TRIBE:if(c->flush)o->xmult_num=2;break;
   /* Score handlers only; these IDs have additional lifecycle/passive hooks.
      Coverage file expressly excludes full-card implementation. */
   case J_GROS_MICHEL:o->mult_add=15;break;
   case J_CAVENDISH:o->xmult_num=3;break;
   case J_STUNTMAN:o->chips_add=250;break;
   default:o->implemented=0;break;
  }
 } else if(c->event==JE_SCORED_CARD){
  if(c->debuffed){o->implemented=1;return;}
  if(c->enhancement!=ENH_STONE && (c->rank<2 || c->rank>14 || c->suit>3))return;
  o->implemented=1;
  switch(id){
   case J_TICKET:if(c->enhancement==ENH_GOLD)o->money_add=4;break;
   case J_ROUGH_GEM:if(suit_is(g,c->suit,1,c->enhancement))o->money_add=1;break;
   case J_GREEDY_JOKER:if(suit_is(g,c->suit,1,c->enhancement))o->mult_add=3;break;
   case J_LUSTY_JOKER:if(suit_is(g,c->suit,2,c->enhancement))o->mult_add=3;break;
   case J_WRATHFUL_JOKER:if(suit_is(g,c->suit,0,c->enhancement))o->mult_add=3;break;
   case J_GLUTTENOUS_JOKER:if(suit_is(g,c->suit,3,c->enhancement))o->mult_add=3;break;
   case J_FIBONACCI:if(c->rank==2 || c->rank==3 || c->rank==5 || c->rank==8 || c->rank==14)o->mult_add=8;break;
   case J_SCARY_FACE:if(face(g,c->enhancement==ENH_STONE?0:c->rank))o->chips_add=30;break;
   case J_SMILEY:if(face(g,c->enhancement==ENH_STONE?0:c->rank))o->mult_add=5;break;
   case J_EVEN_STEVEN:if(c->rank>=2 && c->rank<=10 && !(c->rank&1))o->mult_add=4;break;
   case J_ODD_TODD:if((c->rank<=10 && (c->rank&1)) || c->rank==14)o->chips_add=31;break;
   case J_SCHOLAR:if(c->rank==14){o->chips_add=20;o->mult_add=4;}break;
   case J_WALKIE_TALKIE:if(c->rank==4 || c->rank==10){o->chips_add=10;o->mult_add=4;}break;
   case J_ARROWHEAD:if(suit_is(g,c->suit,0,c->enhancement))o->chips_add=50;break;
   case J_ONYX_AGATE:if(suit_is(g,c->suit,3,c->enhancement))o->mult_add=7;break;
   case J_PHOTOGRAPH:
    if(c->card_index>=g->hand_count || !(hand_scoring(c,c->card_index))){o->implemented=0;break;}
    for(i=0;i<g->hand_count;i++)if((hand_scoring(c,i)) && card_face(g,&g->hand[i])){if(i==c->card_index)o->xmult_num=2;break;}
    break;
   case J_TRIBOULET:if(c->rank==12 || c->rank==13)o->xmult_num=2;break;
   default:o->implemented=0;break;
  }
  } else if(c->event==JE_OTHER_JOKER && id==J_BASEBALL){
  o->implemented=1;
  if(c->trigger_index<g->joker_count && c->trigger_index!=c->joker_index && c->trigger_id<150 && joker_rarity[c->trigger_id]==2){o->xmult_num=3;o->xmult_den=2;}
 } else if(c->event==JE_HELD_CARD){
  if(c->debuffed){o->implemented=1;return;}
  if(c->enhancement!=ENH_STONE && (c->rank<2 || c->rank>14))return;
  switch(id){
   case J_BARON:o->implemented=1;if(c->rank==13){o->xmult_num=3;o->xmult_den=2;}break;
   case J_RAISED_FIST:
    if(c->card_index>=g->hand_count || (hand_is_selected(g,c->card_index)))break;
    o->implemented=1;lowest=255;tally=15;
    for(i=0;i<g->hand_count;i++)if(!(hand_is_selected(g,i)) && g->hand[i].enhancement!=ENH_STONE && tally>=g->hand[i].rank){tally=g->hand[i].rank;lowest=i;}
    if(lowest==c->card_index && !(g->hand[lowest].flags&1))o->mult_add=2*(tally==14?11:tally>10?10:tally);
    break;
   case J_SHOOT_THE_MOON:o->implemented=1;if(c->rank==12)o->mult_add=13;break;
   default:break;
  }
 }
}
