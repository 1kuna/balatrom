#include "paged_inventory.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "joker_history.h"
#include "joker_config.h"
#include "num_extra.h"
#include <string.h>
#define FATAL (GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)
/* Check the nonnegative binary64 domain directly instead of calling
 * gb_num_cmp(v,+0): +0, subnormal, finite and +infinity are valid; NaN of
 * either sign and every negative encoding (including -0) are invalid.
 * Straight-line integer code reads all fraction bytes, needs no shared
 * numeric scratch and does not modify the value. */
static uint8_t valid_num(const GbNum *v){
 uint8_t fraction=v->bits[0]|v->bits[1]|v->bits[2]|v->bits[3]|v->bits[4]|v->bits[5]|(v->bits[6]&15u);
 uint16_t exponent=((uint16_t)(v->bits[7]&127u)<<4)|(v->bits[6]>>4);
 uint8_t all=(uint16_t)((exponent^2047u)-1u)>>15;
 uint8_t nonzero=(uint16_t)(0u-fraction)>>15;
 return 1u^((v->bits[7]>>7)|(all&nonzero));
}
/* Read and compare both UID bytes before deciding. */
static uint8_t uid_distinct(uint16_t a,uint16_t b){
 uint16_t difference=a^b;
 return (uint8_t)difference|(uint8_t)(difference>>8);
}
uint8_t joker_history_valid(const GameState *g) CORE_BANKED {
 uint8_t i,j;
 if(!g || !g->starting_deck_size || g->starting_deck_size>DECK_CAP ||
  g->joker_count>PG_JMAX || g->price_discount_percent>100 || !valid_num(&g->tarot_uses))return 0;
 for(i=0;i<g->joker_count;i++){
  if(PG_JBYTE(g,i,jokers,0u)>=150 || PG_JBYTE(g,i,joker_edition,9u)>ED_NEGATIVE ||
   !PG_JUID(g,i) || PG_JUID(g,i)>g->next_joker_uid || !valid_num(&PG_JNUM(g,i,joker_extra_value,14u)))return 0;
  for(j=0;j<i;j++)if(!uid_distinct(PG_JUID(g,j),PG_JUID(g,i)))return 0;
 }return 1;
}
uint8_t joker_history_reset(GameState *g) CORE_BANKED {
 uint8_t i;
 if(!g || !g->deck_count || g->deck_count>DECK_CAP)return 0;
 g->starting_deck_size=g->deck_count;g->price_inflation=0;g->price_discount_percent=0;gb_num_from_u32(&g->tarot_uses,0);
 for(i=0;i<PG_JMAX;i++)gb_num_from_u32(&PG_JNUM(g,i,joker_extra_value,14u),0);
 return 1;
}
uint8_t joker_history_used(GameState *g,uint8_t kind,uint8_t id) CORE_BANKED {
 GbNum next;
 if(!g || !joker_history_valid(g))return 0;
 if(kind!=ITEM_TAROT)return 1;
 if(id>=22)return 0;
 if(gb_num_add_i16(&next,&g->tarot_uses,1)&FATAL)return 0;
 g->tarot_uses=next;return 1;
}
uint8_t joker_history_price_policy(GameState *g,uint16_t inflation,uint8_t discount) CORE_BANKED {
 uint8_t i;if(!g || discount>100 || g->joker_count>PG_JMAX)return 0;g->price_inflation=inflation;g->price_discount_percent=discount;
 for(i=0;i<g->joker_count;i++)joker_history_sync_price(g,i);return 1;
}
uint8_t joker_history_sell_value(const GameState *g,uint8_t index,GbNum *out) CORE_BANKED {
 uint32_t cost;uint16_t sell;GbNum base,extra;
 if(!g || !out || index>=g->joker_count || g->joker_count>PG_JMAX ||
  PG_JBYTE(g,index,jokers,0u)>=150 || PG_JBYTE(g,index,joker_edition,9u)>ED_NEGATIVE || g->price_discount_percent>100)return 0;
 /* Card:set_cost: source base+inflation+edition, then +.5, percentage,
    /100, floor. In this uint16 native inflation range all predivision
    half-integer products are exactly representable in binary64. */
 cost=(uint32_t)joker_cost[PG_JBYTE(g,index,jokers,0u)]+g->price_inflation;
 switch(PG_JBYTE(g,index,joker_edition,9u)){case ED_FOIL:cost+=2;break;case ED_HOLO:cost+=3;break;
  case ED_POLY:case ED_NEGATIVE:cost+=5;break;default:break;}
 cost=((cost*2+1)*(100-g->price_discount_percent))/200;
 if(!cost)cost=1;sell=(uint16_t)(cost/2);if(!sell)sell=1;
 extra=PG_JNUM(g,index,joker_extra_value,14u);gb_num_from_u32(&base,sell);
 return !(gb_num_add(out,&base,&extra)&FATAL);
}
/* Byte cache supports the existing small UI only. Never read it for effects. */
void joker_history_sync_price(GameState *g,uint8_t index) CORE_BANKED {
 GbNum amount;uint32_t n=0;
 if(!g || index>=g->joker_count || !joker_history_sell_value(g,index,&amount))return;
 if(num_to_u32_exact(&n,&amount) || n>255)PG_JBYTE(g,index,joker_sell,11u)=255;else PG_JBYTE(g,index,joker_sell,11u)=(uint8_t)n;
}
uint8_t joker_history_sale_amount(const GameState *g,uint8_t index,int32_t *out) CORE_BANKED {
 GbNum amount;uint32_t n;
 if(!out || !joker_history_valid(g) || !joker_history_sell_value(g,index,&amount) ||
  num_to_u32_exact(&n,&amount) || n>2147483647UL || g->money>2147483647L-(int32_t)n)return 0;
 *out=(int32_t)n;return 1;
}
uint8_t joker_history_sell_total(const GameState *g,uint16_t excluded_uid,GbNum *out) CORE_BANKED {
 GbNum total,price,next;uint8_t i;
 if(!out || !joker_history_valid(g))return 0;
 gb_num_from_u32(&total,0);
 for(i=0;i<g->joker_count;i++)if(!excluded_uid || PG_JUID(g,i)!=excluded_uid){
  if(!joker_history_sell_value(g,i,&price) || (gb_num_add(&next,&total,&price)&FATAL))return 0;
  total=next;
 }
 *out=total;return 1;
}
uint8_t joker_history_extra_add(GameState *g,uint16_t uid,const GbNum *delta) CORE_BANKED {
 GbNum d,next;uint8_t i;
 if(!delta || !uid || !joker_history_valid(g))return 0;d=*delta;
 if(!valid_num(&d))return 0;
 for(i=0;i<g->joker_count;i++)if(PG_JUID(g,i)==uid){
  if(gb_num_add(&next,&PG_JNUM(g,i,joker_extra_value,14u),&d)&FATAL)return 0;
  PG_JNUM(g,i,joker_extra_value,14u)=next;joker_history_sync_price(g,i);return 1;
 }
 return 0;
}
uint8_t joker_history_effect(GameState *g,uint8_t id,const JokerContext *c,JokerEffect *out) CORE_BANKED {
 GbNum delta;
 if(!g || !c || !out || c->joker_index>=g->joker_count ||
  PG_JBYTE(g,c->joker_index,jokers,0u)!=id ||
  (id!=J_EROSION && id!=J_FORTUNE_TELLER && id!=J_SWASHBUCKLER && id!=J_EGG))return 0;
 if(id==J_EGG){
  if(c->event!=JE_ROUND_END)return 0;
  memset(out,0,sizeof(*out));out->xmult_num=out->xmult_den=1;out->implemented=1;
  if(c->copy_depth || c->retrigger || (PG_DEBUFF(g,c->joker_index)))return 1;
  gb_num_from_u32(&delta,3);
  if(!joker_history_extra_add(g,PG_JUID(g,c->joker_index),&delta)){g->numeric_error=1;return 1;}
  PG_JNUM(g,c->joker_index,joker_state,1u)=PG_JNUM(g,c->joker_index,joker_extra_value,14u);return 1;
 }
 if(c->event!=JE_MAIN)return 0;
 if(!joker_history_valid(g)){g->numeric_error=1;return 1;}
 memset(out,0,sizeof(*out));out->xmult_num=out->xmult_den=1;out->implemented=1;
 if(PG_DEBUFF(g,c->joker_index))return 1;
 if(id==J_EROSION){
  if(g->starting_deck_size>g->deck_count){
   if(gb_num_from_u32(&out->mult_number,4UL*(g->starting_deck_size-g->deck_count))&FATAL){g->numeric_error=1;return 1;}
   out->number_flags=2;
  }
 }else if(id==J_FORTUNE_TELLER){
  gb_num_from_u32(&delta,0);
  if(gb_num_cmp(&g->tarot_uses,&delta)>0){out->mult_number=g->tarot_uses;out->number_flags=2;}
 }else{
  if(!joker_history_sell_total(g,PG_JUID(g,c->joker_index),&out->mult_number)){
   g->numeric_error=1;return 1;
  }
  gb_num_from_u32(&delta,0);
  if(gb_num_cmp(&out->mult_number,&delta)>0)out->number_flags=2;
 }
 return 1;
}
