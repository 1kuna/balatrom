#include "paged_inventory.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "joker_economy.h"
#include <string.h>
void joker_dollar_bonus(const GameState *g,uint8_t id,const JokerPayoutContext *c,JokerPayoutEffect *o) CORE_BANKED {
 uint8_t i,count=0;memset(o,0,sizeof(*o));
 if(!g || !c || g->deck_count>DECK_CAP)return;
 if(c->debuffed){o->implemented=1;return;}
 switch(id){
  case J_GOLDEN:o->numeric_status=gb_num_from_u32(&o->dollars,4);break;
  case J_CLOUD_9:
   for(i=0;i<g->deck_count;i++)if(g->deck[i].enhancement!=ENH_STONE && g->deck[i].rank==9)count++;
   o->numeric_status=gb_num_from_u32(&o->dollars,count);break;
  case J_DELAYED_GRAT:
   if(!c->discards_used_available)return;
   o->numeric_status=gb_num_from_u32(&o->dollars,c->discards_used?0:2*c->discards_left);break;
  case J_SATELLITE:
   if(!c->planet_history_available || (c->used_planet_mask&0xf000))return;
   for(i=0;i<12;i++)if(c->used_planet_mask&(1u<<i))count++;
   o->numeric_status=gb_num_from_u32(&o->dollars,count);break;
  case J_ROCKET:
   if(c->joker_index>=g->joker_count || PG_JBYTE(g,c->joker_index,jokers,0u)!=id)return;
   o->dollars=PG_JNUM(g,c->joker_index,joker_state,1u);break;
  default:return;
 }
 o->implemented=!(o->numeric_status & (GBN_DOMAIN | GBN_NAN | GBN_DIVZERO));
}
void joker_passive(uint8_t id,const JokerPassiveContext *c,JokerPassiveEffect *o) CORE_BANKED {
 int8_t direction;memset(o,0,sizeof(*o));o->probability_num=o->probability_den=1;
 if(!c || !c->target_identity_available || c->event>JPE_REMOVE)return;
 direction=c->event==JPE_ACQUIRE?1:-1;
 switch(id){
  case J_JUGGLER:o->hand_size_delta=direction;break;
  case J_DRUNKARD:o->discards_max_delta=o->discards_current_delta=direction;break;
  case J_MERRY_ANDY:o->hand_size_delta=-direction;o->discards_max_delta=o->discards_current_delta=3*direction;break;
  case J_TROUBADOUR:o->hand_size_delta=2*direction;o->hands_max_delta=-direction;break;
  case J_STUNTMAN:o->hand_size_delta=-2*direction;break;
  case J_CREDIT_CARD:o->bankrupt_floor_delta=-20*direction;break;
  case J_CHAOS:o->free_rerolls_delta=direction;break;
  case J_TO_THE_MOON:o->interest_amount_delta=direction;break;
  case J_OOPS:if(direction==1)o->probability_num=2;else o->probability_den=2;break;
  case J_ASTRONOMER:o->reprice_all=1;break;
  default:return;
 }
 o->implemented=1;
 if(!c->is_target){memset(o,0,sizeof(*o));o->probability_num=o->probability_den=1;o->implemented=1;}
}
