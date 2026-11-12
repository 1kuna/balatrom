#include "paged_inventory.h"
#include "hand_api.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "joker_num_counters.h"
#include "num_extra.h"
#include <string.h>
/* GBN_INF is a valid result flag; reject only invalid arithmetic. */
#define NUM_FATAL (GBN_DOMAIN | GBN_NAN | GBN_DIVZERO)
static uint8_t known(uint8_t id){switch(id){
 case J_RUNNER:case J_SQUARE:case J_TROUSERS:case J_GREEN_JOKER:
 case J_RIDE_THE_BUS:case J_RED_CARD:case J_FLASH:case J_POPCORN:
 case J_ICE_CREAM:case J_SELZER:case J_WEE:case J_CONSTELLATION:
 case J_HOLOGRAM:case J_RAMEN:case J_VAMPIRE:case J_OBELISK:
 case J_CAMPFIRE:case J_EGG:case J_ROCKET:return 1;default:return 0;}}
uint8_t joker_num_counter_init(uint8_t id,GbNum *state) CORE_BANKED {
 if(!state || !known(id))return 0;
 return gb_num_from_u32(state,id==J_POPCORN?20:id==J_ICE_CREAM?100:id==J_SELZER?10:
   id==J_RAMEN?2:id==J_CONSTELLATION || id==J_HOLOGRAM || id==J_VAMPIRE || id==J_OBELISK || id==J_CAMPFIRE || id==J_ROCKET?1:0)==GBN_OK;
}
uint8_t joker_initialize(uint8_t id,GbNum *state) CORE_BANKED {return joker_num_counter_init(id,state);}
static uint8_t increment(GbNum *state,int16_t delta,JokerNumCounterEffect *o){
 GbNum next;uint8_t status=gb_num_add_i16(&next,state,delta);o->numeric_status=status;
 if((status & NUM_FATAL))return 0;*state=next;return 1;
}
static uint8_t valid(const GbNum *state){GbNum zero;gb_num_from_u32(&zero,0);return gb_num_cmp(state,&zero)!=GBN_UNORDERED;}
void joker_num_counter_event(uint8_t id,GbNum *state,const JokerCounterContext *c,JokerNumCounterEffect *o) CORE_BANKED {
 uint8_t handled=0,status;int16_t delta=0;GbNum amount,next,threshold,count,one;
 /* Exact native round64(1/100), verified at both Ramen subtractions in r42.
  * Keep subtraction and <=1 destruction unchanged; no extra mutable state. */
 static const GbNum ramen_step={{0x7b,0x14,0xae,0x47,0xe1,0x7a,0x84,0x3f}};
 memset(o,0,sizeof(*o));gb_num_from_u32(&o->xmult,1);
 if(!state || !c || !known(id) || !valid(state))return;
 if(c->event==JCE_MAIN){
  if(id==J_SELZER)return;o->implemented=1;
  if(id==J_EGG || id==J_ROCKET)return;
  if(id==J_CONSTELLATION || id==J_HOLOGRAM || id==J_RAMEN || id==J_VAMPIRE || id==J_OBELISK || id==J_CAMPFIRE){
   gb_num_from_u32(&one,1);if(gb_num_cmp(state,&one)>0){o->xmult=*state;o->payload_flags=JNP_XMULT;}
  }else if(id==J_RUNNER || id==J_SQUARE || id==J_ICE_CREAM || id==J_WEE){o->chips_add=*state;o->payload_flags=JNP_CHIPS;}
  else{o->mult_add=*state;o->payload_flags=JNP_MULT;}return;
 }
 if(c->event==JCE_REPETITION_SCORED && id==J_SELZER){o->implemented=1;o->repetitions=1;return;}
 if(c->event==JCE_BEFORE_HAND){switch(id){
  case J_RUNNER:if(!c->hand_predicates_available)return;handled=1;if(c->straight)delta=15;break;
  case J_SQUARE:if(!c->played_count_available)return;handled=1;if(c->played_count==4)delta=4;break;
  case J_TROUSERS:if(!c->hand_predicates_available)return;handled=1;if(c->two_pair || c->full_house)delta=2;break;
  case J_GREEN_JOKER:handled=1;delta=1;break;
  case J_RIDE_THE_BUS:
   if(!c->scoring_faces_available)return;
   if(c->blueprint){o->implemented=1;return;}
   if(c->scoring_has_face){gb_num_from_u32(state,0);o->implemented=1;}
   else o->implemented=increment(state,1,o);return;
  case J_OBELISK:
   if(!c->visible_history_available)return;
   if(c->blueprint){o->implemented=1;return;}
   if(!c->other_visible_hand_ties_or_exceeds){gb_num_from_u32(state,1);o->implemented=1;return;}
   status=gb_num_ratio(&amount,1,5);if(!(status & NUM_FATAL))status=gb_num_add(&next,state,&amount);
   o->numeric_status=status;if((status & NUM_FATAL))return;*state=next;o->implemented=1;return;
  default:break;
 }}else if(c->event==JCE_DISCARD_CARD && id==J_RAMEN){
  o->implemented=1;if(c->blueprint)return;
  gb_num_from_u32(&one,1);if(gb_num_cmp(state,&one)<0){o->implemented=0;o->numeric_status=GBN_DOMAIN;return;}
  amount=ramen_step;status=gb_num_sub_clamp(&next,state,&amount);
  o->numeric_status=status;if((status & NUM_FATAL)){o->implemented=0;return;}
  if(gb_num_cmp(&next,&one)<=0)o->destroy_self=1;else *state=next;return;
 }else if(c->event==JCE_ROUND_END && (id==J_CAMPFIRE || id==J_ROCKET || id==J_EGG)){
  if(c->individual || c->repetition || c->blueprint){o->implemented=1;return;}
  if(id==J_EGG){o->implemented=increment(state,3,o);if(o->implemented)o->recalculate_sell_cost=1;return;}
  if(!c->round_boss_available)return;o->implemented=1;if(!c->round_is_boss)return;
  if(id==J_ROCKET)o->implemented=increment(state,2,o);else gb_num_from_u32(state,1);return;
 }else if(c->event==JCE_CARD_SOLD && id==J_CAMPFIRE){
  o->implemented=1;if(c->blueprint)return;status=gb_num_ratio(&amount,1,4);
  if(!(status & NUM_FATAL))status=gb_num_add(&next,state,&amount);o->numeric_status=status;
  if((status & NUM_FATAL)){o->implemented=0;return;}*state=next;return;
 }else if(c->event==JCE_VAMPIRE_DRAINED && id==J_VAMPIRE){
  if(!c->card_count_available)return;o->implemented=1;if(c->blueprint || !c->card_count)return;
  status=gb_num_ratio(&amount,1,10);status|=gb_num_from_u32(&count,c->card_count);
  if(!(status & NUM_FATAL))status=gb_num_mul(&threshold,&amount,&count);
  if(!(status & NUM_FATAL))status=gb_num_add(&next,state,&threshold);o->numeric_status=status;
  if((status & NUM_FATAL)){o->implemented=0;return;}*state=next;return;
 }else if(c->event==JCE_DISCARD_CARD && id==J_GREEN_JOKER){
  if(!c->discard_order_available)return;
  o->implemented=1;if(!c->blueprint && c->is_last_discarded){
   gb_num_from_u32(&amount,1);status=gb_num_sub_clamp(&next,state,&amount);o->numeric_status=status;
   if((status & NUM_FATAL)){o->implemented=0;return;}*state=next;
  }return;
 }else if(c->event==JCE_SKIP_BOOSTER && id==J_RED_CARD){handled=1;delta=3;
 }else if(c->event==JCE_REROLL_SHOP && id==J_FLASH){handled=1;delta=2;
 }else if(c->event==JCE_SCORED_CARD && id==J_WEE){
  if(!c->rank_available)return;handled=1;if(c->rank_id==2)delta=8;
 }else if((c->event==JCE_AFTER_HAND && (id==J_ICE_CREAM || id==J_SELZER)) ||
          (c->event==JCE_ROUND_END && id==J_POPCORN)){
  o->implemented=1;if(c->blueprint || (c->event==JCE_ROUND_END && (c->individual || c->repetition)))return;
  delta=id==J_POPCORN?4:id==J_ICE_CREAM?5:1;gb_num_from_u32(&threshold,delta);
  if(gb_num_cmp(state,&threshold)<=0)o->destroy_self=1;else o->implemented=increment(state,-delta,o);return;
 }else if(c->event==JCE_CONSUMABLE_USED && id==J_CONSTELLATION){
  if(!c->trigger_kind_available)return;o->implemented=1;
  /* Native ITEM_PLANET=1. Only actual Planet use, no purchase/pack opening. */
  if(c->blueprint || c->trigger_kind!=1)return;
  status=gb_num_ratio(&amount,1,10);if(!(status & NUM_FATAL))status=gb_num_add(&next,state,&amount);
  o->numeric_status=status;if((status & NUM_FATAL)){o->implemented=0;return;}*state=next;return;
 }else if(c->event==JCE_CARDS_ADDED && id==J_HOLOGRAM){
  if(!c->card_count_available)return;o->implemented=1;if(c->blueprint || !c->card_count)return;
  /* Stock: state = state + count * extra, extra=0.25; multiply then add. */
  status=gb_num_ratio(&amount,1,4);status|=gb_num_from_u32(&count,c->card_count);
  if(!(status & NUM_FATAL))status=gb_num_mul(&threshold,&count,&amount);
  if(!(status & NUM_FATAL))status=gb_num_add(&next,state,&threshold);
  o->numeric_status=status;if((status & NUM_FATAL)){o->implemented=0;return;}*state=next;return;
 }
 if(handled){if(c->blueprint || !delta)o->implemented=1;else o->implemented=increment(state,delta,o);}
}
uint8_t joker_steel_factor(uint8_t steel_count,GbNum *out) CORE_BANKED {
 GbNum increment,count,product,one;uint8_t status;
 /* 1/5 rounded to binary64 equals the owned Lua literal0.2. Preserve the
    original separate multiply and add; do not use rational (5+count)/5. */
 status=gb_num_ratio(&increment,1,5);status|=gb_num_from_u32(&count,steel_count);
 status|=gb_num_from_u32(&one,1);if(status & NUM_FATAL)return status;
 status=gb_num_mul(&product,&increment,&count);if(status & NUM_FATAL)return status;
 return gb_num_add(out,&one,&product);
}

static uint8_t active_face(const GameState *g,const Card *card){
 uint8_t i;if(card->flags&1)return 0;
 if(card->enhancement!=ENH_STONE && card->rank>=11 && card->rank<=13)return 1;
 for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==J_PAREIDOLIA && !(PG_DEBUFF(g,i)))return 1;
 return 0;
}
/* Private118: per-owned binary64 growth, with no additional saved fields.
 * Loyalty stores its acquisition-time global played-hand count; the global
 * history clock continues during debuff, matching source rather than a local
 * counter that pauses when the actor is inactive. MAIN sees current hand +1. */
static uint32_t played_total(const GameState*g){uint8_t i;uint32_t n=0;for(i=0;i<12u;i++)n+=g->hands_played[i];return n;}
static uint8_t growth118(GameState*g,uint8_t id,const JokerContext*c,uint8_t copy,JokerEffect*o){
 GbNum*state,next,step,one;uint32_t total,born;uint8_t status;
 if(id!=J_LOYALTY_CARD&&id!=J_LUCKY_CAT&&id!=J_HIT_THE_ROAD)return 0;
 o->implemented=1;state=&PG_JNUM(g,c->joker_index,joker_state,1u);
 if(c->event==JE_ACQUIRE){if(copy||c->trigger_index!=c->joker_index)return 1;
  status=gb_num_from_u32(state,id==J_LOYALTY_CARD?played_total(g):1u);
  if(status&NUM_FATAL)g->numeric_error=1;return 1;
 }
 if(c->event==JE_MAIN){
  if(id==J_LOYALTY_CARD){total=played_total(g);
   if(num_to_u32_exact(&born,state)||born>total){g->numeric_error=1;return 1;}
   if(total>born && (total-born)%6u==0){o->xmult_num=4;o->table_present=1;}return 1;
  }
  gb_num_from_u32(&one,1u);if(gb_num_cmp(state,&one)==GBN_UNORDERED){g->numeric_error=1;return 1;}
  if(gb_num_cmp(state,&one)>0){o->number_flags=4;o->xmult_number=*state;o->table_present=1;}return 1;
 }
 if(copy)return 1;
 if(id==J_HIT_THE_ROAD&&c->event==JE_ROUND_END){gb_num_from_u32(state,1u);return 1;}
 if(id==J_LUCKY_CAT&&c->event==JE_SCORED_CARD&&!c->debuffed&&c->lucky_trigger)status=gb_num_ratio(&step,1,4);
 else if(id==J_HIT_THE_ROAD&&c->event==JE_DISCARD_CARD&&!c->debuffed&&c->rank==11)status=gb_num_ratio(&step,1,2);
 else return 1;
 if(!(status&NUM_FATAL))status=gb_num_add(&next,state,&step);
 if(status&NUM_FATAL)g->numeric_error=1;else *state=next;
 return 1;
}
/* Optional direct binding to latest native context. Core still owns init,
   persistence, copied source identity and removal. Missing shop/repetition
   events are deliberately not synthesized from unrelated JE_* events. */
void joker_num_counter_game_event(GameState *g,uint8_t id,const JokerContext *c,uint8_t blueprint,JokerEffect *o) CORE_BANKED {
 JokerCounterContext x;JokerNumCounterEffect payload;uint8_t i;
 memset(o,0,sizeof(*o));o->xmult_num=o->xmult_den=1;
 if(!g || !c || c->joker_index>=g->joker_count || c->joker_index>=PG_JMAX ||
    PG_JBYTE(g,c->joker_index,jokers,0u)!=id || g->hand_count>HAND_STORAGE_CAP)return;
 if(PG_DEBUFF(g,c->joker_index)){o->implemented=1;return;}
 if(growth118(g,id,c,blueprint,o))return;
 if(id==J_EGG && c->event==JE_ROUND_END)return; /* price recomputation hook not yet present */
 memset(&x,0,sizeof(x));x.blueprint=blueprint;
 switch(c->event){
  case JE_BEFORE_HAND:
   x.event=JCE_BEFORE_HAND;x.hand_predicates_available=x.played_count_available=x.scoring_faces_available=1;
   x.straight=c->straight;x.two_pair=c->pairs>=2 || (c->pairs && c->trips);
   x.full_house=c->pairs && c->trips;x.played_count=c->played_count;
   /* Private119: source evaluates after incrementing current-hand history.
    * Standard nine hands are visible initially; the three secret hands become
    * visible on first play. Their native played counters persist this fact.
    * Source compares any other visible hand >= current, including ties. */
   if(id==J_OBELISK){
    if(c->hand_type>=12u || !g->hands_played[c->hand_type]){g->numeric_error=1;return;}
    x.visible_history_available=1;
    for(i=0;i<12u;i++)if(i!=c->hand_type && (i<9u || g->hands_played[i]) &&
      g->hands_played[i]>=g->hands_played[c->hand_type])x.other_visible_hand_ties_or_exceeds=1;
   }
   for(i=0;i<g->hand_count;i++)if((hand_scoring(c,i)) && active_face(g,&g->hand[i]))x.scoring_has_face=1;
   break;
  case JE_MAIN:x.event=JCE_MAIN;break;
  case JE_AFTER_HAND:x.event=JCE_AFTER_HAND;break;
  case JE_DISCARD:case JE_DISCARD_CARD:
   if(!g->highlighted_count)return;
   /* Aggregate real-discard event equals Green Joker's once-at-last-card
      decrement; no other per-card discard effects are implemented here. */
   x.event=JCE_DISCARD_CARD;x.discard_order_available=1;x.is_last_discarded=c->event==JE_DISCARD?1:c->discard_last;break;
  case JE_ROUND_END:x.event=JCE_ROUND_END;x.round_boss_available=1;x.round_is_boss=g->blind==2;break;
  case JE_SCORED_CARD:
   if(c->debuffed){o->implemented=1;return;}
   x.event=JCE_SCORED_CARD;x.rank_available=1;x.rank_id=c->enhancement==ENH_STONE?-1:c->rank;break;
  case JE_ACQUIRE:
   /* Separate trigger identity survives the dispatch loop's joker_index. */
   if(c->trigger_index!=c->joker_index){o->implemented=1;return;}
   if(blueprint){o->implemented=1;return;}
   o->implemented=joker_num_counter_init(id,&PG_JNUM(g,c->joker_index,joker_state,1u));return;
  case JE_REMOVE:
   /* Counter has no removal passive; core compacts state together with UID. */
   if(known(id))o->implemented=1;return;
  case JE_CONSUMABLE_USED:
   if(c->trigger_kind>ITEM_SPECTRAL)return;
   x.event=JCE_CONSUMABLE_USED;x.trigger_kind_available=1;x.trigger_kind=c->trigger_kind;break;
  case JE_CARDS_ADDED:
   if(c->card_count && !c->cards)return;
   x.event=JCE_CARDS_ADDED;x.card_count_available=1;x.card_count=c->card_count;break;
  default:return;
 }
 if(id==J_RAMEN && c->event==JE_DISCARD){
  memset(&payload,0,sizeof(payload));
  for(i=0;i<g->hand_count;i++)if(hand_is_selected(g,i)){
   JokerNumCounterEffect step;joker_num_counter_event(id,&PG_JNUM(g,c->joker_index,joker_state,1u),&x,&step);
   if(step.numeric_status & NUM_FATAL){g->numeric_error=1;return;}
   payload.implemented=step.implemented;payload.destroy_self|=step.destroy_self;
  }
 }else joker_num_counter_event(id,&PG_JNUM(g,c->joker_index,joker_state,1u),&x,&payload);
 if(payload.numeric_status & NUM_FATAL){g->numeric_error=1;return;}
 o->implemented=payload.implemented;o->destroy_self=payload.destroy_self;o->repetitions=payload.repetitions;
 if(payload.recalculate_sell_cost){
  /* v05 uint8 sell metadata cannot safely express unbounded Egg value and
     current cost modifiers. Core must recompute from the GbNum extra value. */
  g->numeric_error=1;return;
 }
 o->number_flags=payload.payload_flags;o->chips_number=payload.chips_add;
 o->mult_number=payload.mult_add;o->xmult_number=payload.xmult;
}
