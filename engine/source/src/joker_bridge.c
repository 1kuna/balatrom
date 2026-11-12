#include "paged_inventory.h"
#include "joker_history.h"
#include "hand_api.h"
#pragma bank 255
#include "joker_bridge.h"
#include "joker_final12_core.h"
#include "joker_card_mutation.h"
#include "joker_num_counters.h"
#include "gb_num_status.h"
#include <string.h>
#include "boss.h"
#include "creation_core.h"

/* Yorick owns an opaque eight-byte instance record: countdown byte0 (1..23),
 * followed by a little-endian54-bit integer Xmult (1..2^53). At2^53 source
 * binary64 +1 rounds back to2^53; countdown continues normally. */
static uint8_t yorick_valid(const GbNum *s){uint8_t i,any=0;if(!s->bits[0]||s->bits[0]>23||s->bits[7]>32)return 0;for(i=1;i<8;i++)any|=s->bits[i];if(!any)return 0;if(s->bits[7]==32)for(i=1;i<7;i++)if(s->bits[i])return 0;return 1;}
static void yorick_mult(GbNum *out,const GbNum *s){uint8_t k,high=53,bit,at;uint16_t exp;memset(out,0,sizeof(*out));while(high && !(s->bits[1+high/8]&(1u<<(high%8))))high--;exp=1023u+high;out->bits[6]=(exp&15u)<<4;out->bits[7]=exp>>4;for(k=0;k<high;k++)if(s->bits[1+k/8]&(1u<<(k%8))){at=k+52u-high;bit=at%8;out->bits[at/8]|=1u<<bit;}}
static uint8_t legendary(GameState *g,uint8_t id,JokerContext *c,JokerEffect *out,uint8_t copy){
 GbNum *state,next;uint8_t i,n=0;
 if(id!=J_CAINO&&id!=J_YORICK&&id!=J_CHICOT)return 0;
 memset(out,0,sizeof(*out));out->implemented=1;out->xmult_num=out->xmult_den=1;
 if(PG_DEBUFF(g,c->joker_index))return 1;
 state=&PG_JNUM(g,c->joker_index,joker_state,1u);
 if(c->event==JE_ACQUIRE && !copy && c->trigger_index==c->joker_index){
  if(id==J_CAINO)gb_num_from_u32(state,1);
  if(id==J_YORICK){memset(state,0,sizeof(*state));state->bits[0]=23;state->bits[1]=1;}
  if(id==J_CHICOT && !creation_commit_active && g->blind==2 && !g->boss_disabled)boss_disable(g);
 }
 if(id==J_CHICOT){if(c->event==JE_BLIND_BEGIN&&!copy&&g->blind==2&&!g->boss_disabled){if(!boss_chicot_preflight(g)){g->numeric_error=1;return 1;}boss_disable(g);}return 1;}
 if(id==J_CAINO && c->event==JE_CARDS_REMOVED&&!copy){
  uint8_t pareidolia=0;for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==J_PAREIDOLIA&&!PG_DEBUFF(g,i))pareidolia=1;
  for(i=0;i<c->card_count;i++)if(!(c->cards[i].flags&CARD_DEBUFF)&&((c->cards[i].enhancement!=ENH_STONE&&c->cards[i].rank>=11&&c->cards[i].rank<=13)||pareidolia))n++;
  if(n){if(gb_num_add_i16(&next,state,n)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)){g->numeric_error=1;return 1;}*state=next;}
 }
 if(id==J_YORICK){
  if(!yorick_valid(state)){g->numeric_error=1;return 1;}
  if(c->event==JE_DISCARD_CARD&&!copy){if(state->bits[0]>1)state->bits[0]--;else{state->bits[0]=23;if(state->bits[7]!=32){for(i=1;i<8;i++)if(++state->bits[i])break;}}}
 }
 if(c->event==JE_MAIN){out->number_flags=4;if(id==J_CAINO)out->xmult_number=*state;else yorick_mult(&out->xmult_number,state);}
 return 1;
}

void joker_bridge(GameState *g,uint8_t id,JokerContext *c,JokerEffect *out,uint8_t blueprint) CORE_BANKED {
 JokerCounterContext x;JokerNumCounterEffect payload;
 if(legendary(g,id,c,out,blueprint))return;
 if(joker_history_effect(g,id,c,out))return;
 if(joker_final12_core(g,id,c,out))return;
 if(c->event==JE_DISCARD_CARD&&id==J_TRADING&&!blueprint&&!(PG_DEBUFF(g,c->joker_index))&&!g->discards_used&&c->card_count==1){memset(out,0,sizeof(*out));out->xmult_num=out->xmult_den=1;out->implemented=1;out->money_add=3;out->remove_card=1;return;}
 joker_card_mutation_game_event(g,id,c,out);if(out->implemented)return;
 joker_effect(g,id,c,out);if(out->implemented){
 if(c->event==JE_HELD_CARD){uint8_t i,lowest=255,rank=15;
 if((id==J_BARON&&c->rank==13)||(id==J_SHOOT_THE_MOON&&c->rank==12))out->table_present=1;
 if(id==J_RAISED_FIST){for(i=0;i<g->hand_count;i++)if(!(hand_is_selected(g,i))&&g->hand[i].enhancement!=ENH_STONE&&rank>=g->hand[i].rank){rank=g->hand[i].rank;lowest=i;}if(c->card_index==lowest)out->table_present=1;}
 }return;}

 joker_num_counter_game_event(g,id,c,blueprint,out);if(out->implemented)return;
 memset(&x,0,sizeof(x));x.blueprint=blueprint;
 if(c->event==JE_SKIP_PACK)x.event=JCE_SKIP_BOOSTER;
 else if(c->event==JE_REROLL)x.event=JCE_REROLL_SHOP;
 else if(c->event==JE_REPETITION_SCORED)x.event=JCE_REPETITION_SCORED;
 else return;
 joker_num_counter_event(id,&PG_JNUM(g,c->joker_index,joker_state,1u),&x,&payload);
 if(payload.numeric_status&GBN_ERROR_MASK){g->numeric_error=1;return;}
 out->implemented=payload.implemented;out->destroy_self=payload.destroy_self;out->repetitions=payload.repetitions;out->number_flags=payload.payload_flags;out->chips_number=payload.chips_add;out->mult_number=payload.mult_add;out->xmult_number=payload.xmult;
}
