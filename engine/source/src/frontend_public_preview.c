#include "cooperative_native.h"
#include "paged_inventory.h"
#include "inventory_bounds.h"
#pragma bank 255
#include "frontend_public_preview.h"
#include "pb_public_preview.h"
#include "boss_optional.h"
#include "hand_api.h"
#include <string.h>

uint8_t frontend_public_jokers_key(const GameState *g,uint8_t key[5]) BANKED {
 uint8_t i,j,first=inventory_joker_first(g),debuff=0;
 for(i=0;i<5u;i++){
  key[i]=255u;
  j=i+first;if(j<g->joker_count&&!boss_joker_hidden(g,j)){
   key[i]=PG_JBYTE(g,j,jokers,0u);if(PG_DEBUFF(g,j))debuff|=1u<<i;
  }
 }
 return debuff;
}
uint8_t frontend_public_jokers_same(const GameState *g,const uint8_t key[5],uint8_t debuff) BANKED {
 uint8_t current[5];
 return frontend_public_jokers_key(g,current)==debuff&&!memcmp(current,key,5);
}

void frontend_public_hand_preview(const GameState *g,HandPreview *out) BANKED {
 PBPPCard cards[5];uint8_t order[5],i,n,known=0,hidden=0,type,result;const Card *c;
 uint16_t level;char rev[5];
 static const char *const names[12]={"HIGH CARD","PAIR","TWO PAIR","THREE OF A KIND","STRAIGHT","FLUSH","FULL HOUSE","FOUR OF A KIND","STRAIGHT FLUSH","FIVE OF A KIND","FLUSH HOUSE","FLUSH FIVE"};
 static const uint8_t bc[12]={/* OWNER_INSTALL_SLOT:bc */},bm[12]={/* OWNER_INSTALL_SLOT:bm */},lc[12]={/* OWNER_INSTALL_SLOT:lc */},lm[12]={/* OWNER_INSTALL_SLOT:lm */};
 if(!g||g->hand_count>HAND_STORAGE_CAP||g->highlighted_count>HAND_PLAY_CAP||g->joker_count>PG_JMAX)goto core;
 for(i=0;i<g->joker_count;i++){if(!(i&3u)&&coop_active&&!coop_checkpoint())return;
  if(boss_joker_hidden(g,i))hidden++;
  else if(!(PG_DEBUFF(g,i)))known|=pbpp_visible_role(PG_JBYTE(g,i,jokers,0u));
 }
 if(!hidden)goto core;
 n=hand_selected_play_order(g,order);
 if(!n){memset(out,0,sizeof(*out));out->valid=1;return;}
 for(i=0;i<n;i++){
  c=g->hand+order[i];
  /* Face-down/invalid cards keep core's existing early-return semantics. */
  if(c->flags&CARD_FACE_DOWN)goto core;
  cards[i].enhancement=c->enhancement;
  cards[i].rank=c->enhancement==ENH_STONE?0u:c->rank;
  cards[i].suit=c->enhancement==ENH_STONE?0u:c->suit;
 }
 result=pbpp_consensus(cards,n,known,hidden,&type);
 if(result==PBPP_INVALID)goto core;
 memset(out,0,sizeof(*out));out->valid=1;
 /* Accepted saves may contain zero levels. Availability follows public
  * consensus: unknown stays concealed, known level0 stays unavailable. */
 if(result==PBPP_UNKNOWN){out->hidden=1;strcpy(out->label,"???? LV?");return;}
 level=g->hand_levels[type];if(!level){out->valid=0;return;}
 out->level=level;out->hand_type=type;
 out->chips=bc[type]+(uint32_t)(level-1)*lc[type];out->mult=bm[type]+(uint32_t)(level-1)*lm[type];
 strcpy(out->label,names[type]);strcat(out->label," LV");
 n=0;do{rev[n++]='0'+level%10;level/=10;}while(level);
 i=strlen(out->label);while(n)out->label[i++]=rev[--n];out->label[i]=0;return;
core:
 hand_preview(g,out);
}

uint8_t frontend_public_preview_hidden(const GameState *g) BANKED {
 PBPPCard cards[5];uint8_t order[5],i,n,known=0,hidden=0,type;const Card *c;
 if(!g||g->hand_count>HAND_STORAGE_CAP||g->highlighted_count>HAND_PLAY_CAP||g->joker_count>PG_JMAX)return 1;
 if(!g->highlighted_count)return 0;
 for(i=0;i<g->joker_count;i++){if(!(i&3u)&&coop_active&&!coop_checkpoint())return 1;
  if(boss_joker_hidden(g,i))hidden++;
  else if(!(PG_DEBUFF(g,i)))known|=pbpp_visible_role(PG_JBYTE(g,i,jokers,0u));
 }
 /* All-visible recognition already comes from fresh core hand_preview. */
 if(!hidden)return 0;
 n=hand_selected_play_order(g,order);if(n>5u)return 1;if(!n)return 0;
 for(i=0;i<n;i++){
  if(order[i]>=g->hand_count)return 1;c=g->hand+order[i];
  if(c->flags&CARD_FACE_DOWN)return 1;
  cards[i].enhancement=c->enhancement;
  cards[i].rank=c->enhancement==ENH_STONE?0u:c->rank;
  cards[i].suit=c->enhancement==ENH_STONE?0u:c->suit;
 }
 return pbpp_consensus(cards,n,known,hidden,&type)!=PBPP_KNOWN;
}
