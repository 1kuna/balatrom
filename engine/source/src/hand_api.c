#include "hand_api.h"
#include "boss_optional.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "hand_api.h"
#include "boss_optional.h"
#include <string.h>
#include "events.h"
#include "boss.h"
Card *hand_card(GameState *g,uint8_t index) CORE_BANKED {
 return index<g->hand_count && index<HAND_STORAGE_CAP?&g->hand[index]:0;
}
uint8_t hand_is_selected(const GameState *g,uint8_t index) CORE_BANKED {
 uint8_t s;if(index>=g->hand_count)return 0;
 for(s=0;s<g->highlighted_count && s<HAND_PLAY_CAP;s++)if(g->selected_uids[s]==g->hand[index].uid)return 1;
 return 0;
}
void hand_clear(GameState *g) CORE_BANKED {
 uint8_t i;uint16_t forced=0;
 if(g->boss_id==12 && !g->boss_disabled && g->forced_card_uid)for(i=0;i<g->hand_count;i++)if(g->hand[i].uid==g->forced_card_uid){forced=g->forced_card_uid;break;}
 g->highlighted_count=0;memset(g->selected_uids,0,sizeof(g->selected_uids));
 if(forced){g->selected_uids[0]=forced;g->highlighted_count=1;}
}
uint8_t hand_force_uid(GameState *g,uint16_t uid) CORE_BANKED {
 uint8_t i;
 if(!uid){g->forced_card_uid=0;return 1;}
 for(i=0;i<g->hand_count;i++)if(g->hand[i].uid==uid) {
  hand_clear(g);g->selected_uids[0]=uid;g->highlighted_count=1;
  g->forced_card_uid=uid;g->focus=i;return 1;
 }
 return 0;
}
uint8_t hand_remove_uid(GameState *g,uint16_t uid) CORE_BANKED {
 uint8_t i,j,n=0,removed=0;
 for(i=0;i<g->hand_count;i++)if(g->hand[i].uid==uid)removed=1;else g->hand[n++]=g->hand[i];
 g->hand_count=n;
 for(i=0;i<g->highlighted_count;i++)if(g->selected_uids[i]==uid) {
  for(j=i;j+1<g->highlighted_count;j++)g->selected_uids[j]=g->selected_uids[j+1];
  g->selected_uids[--g->highlighted_count]=0;break;
 }
 if(g->boss_id==12 && g->forced_card_uid==uid)g->forced_card_uid=0;
 if(!n || g->focus>=n)g->focus=0;
 return removed;
}
uint8_t hand_toggle(GameState *g,uint8_t index) CORE_BANKED {
 uint8_t s,j;uint16_t uid;if(index>=g->hand_count)return 0;uid=g->hand[index].uid;
 for(s=0;s<g->highlighted_count && s<HAND_PLAY_CAP;s++)if(g->selected_uids[s]==uid) {
  if(g->boss_id==12 && !g->boss_disabled && g->forced_card_uid==uid)return 0;
  for(j=s;j+1<g->highlighted_count;j++)g->selected_uids[j]=g->selected_uids[j+1];
  g->selected_uids[--g->highlighted_count]=0;return 1;
 }
 if(g->highlighted_count>=HAND_PLAY_CAP)return 0;
 g->selected_uids[g->highlighted_count++]=uid;return 1;
}
uint8_t hand_selected_insertion_order(const GameState *g,uint8_t out[HAND_PLAY_CAP]) CORE_BANKED {
 uint8_t i,s,n=0;
 for(s=0;s<g->highlighted_count && s<HAND_PLAY_CAP;s++)for(i=0;i<g->hand_count;i++)
  if(g->hand[i].uid==g->selected_uids[s]){out[n++]=i;break;}
 return n;
}
uint8_t hand_selected_play_order(const GameState *g,uint8_t out[HAND_PLAY_CAP]) CORE_BANKED {
 uint8_t i,n=0;
 for(i=0;i<g->hand_count && n<HAND_PLAY_CAP;i++)if(hand_is_selected(g,i))out[n++]=i;
 return n;
}
void hand_sort_rank(GameState *g) CORE_BANKED {
 uint8_t i,j;Card card;uint16_t focused_uid=g->hand_count&&g->focus<g->hand_count?g->hand[g->focus].uid:0;
 for(i=1;i<g->hand_count;i++) {
  card=g->hand[i];j=i;
  while(j && g->hand[j-1].rank>card.rank){g->hand[j]=g->hand[j-1];j--;}
  g->hand[j]=card;
 }
 if(focused_uid)for(i=0;i<g->hand_count;i++)if(g->hand[i].uid==focused_uid){g->focus=i;break;}
}
uint8_t hand_draw(GameState *g,uint16_t count,uint8_t bypass_logical_limit) CORE_BANKED {
 uint8_t n=0,index;
 while(count-- && g->deck_pos<g->deck_count) {
  if(!bypass_logical_limit && (g->hand_size<=0 || g->hand_count>=g->hand_size))break;
  if(g->hand_count>=HAND_STORAGE_CAP){g->numeric_error=1;break;}
  index=g->draw_order[g->deck_pos];
  if(index>=g->deck_count){g->numeric_error=1;break;}
  g->hand[g->hand_count++]=g->deck[index];g->deck_pos++;n++;
  game_refresh_card(&g->hand[g->hand_count-1]);boss_on_card_draw(g,&g->hand[g->hand_count-1]);
 }
 return n;
}
uint8_t hand_refill(GameState *g,uint8_t serpent_after_action) CORE_BANKED {
 uint16_t count;
 if(g->hand_size<=0 && !g->hand_count){g->phase=PHASE_LOSS;return 0;}
 if(serpent_after_action)return hand_draw(g,3,1);
 count=g->hand_size>g->hand_count?(uint16_t)(g->hand_size-g->hand_count):0;
 return hand_draw(g,count,0);
}
uint8_t hand_change_real_size(GameState *g,int16_t delta) CORE_BANKED {
 if((delta>0 && g->hand_size>32767-delta) ||
    (delta<0 && g->hand_size<(-32767-1)-delta)){g->numeric_error=1;return 0;}
 g->hand_size+=delta;
 /* Source draws delta on positive changes with held cards in table/draw state,
    even when the hand was already overfull. Prototype represents table here. */
 if(delta>0 && g->hand_size>1 && g->hand_count && g->phase==PHASE_TABLE){hand_draw(g,(uint16_t)delta,1);boss_after_draw(g,3);}
 return !g->numeric_error;
}
uint8_t hand_commit_selected(GameState *g) CORE_BANKED {
 uint8_t i,n=0,removed=0;
 for(i=0;i<g->hand_count;i++)if(hand_is_selected(g,i))removed++;else g->hand[n++]=g->hand[i];
 g->hand_count=n;g->focus=0;hand_clear(g);return removed;
}
uint8_t hand_page(const GameState *g,uint8_t *first,uint8_t *selected_page) CORE_BANKED {
 uint8_t i,n;
 *first=g->hand_count?(g->focus/HAND_VISIBLE_CAP)*HAND_VISIBLE_CAP:0;
 n=g->hand_count>*first?g->hand_count-*first:0;if(n>HAND_VISIBLE_CAP)n=HAND_VISIBLE_CAP;
 *selected_page=0;
 for(i=0;i<n;i++)if(hand_is_selected(g,*first+i))*selected_page|=1u<<i;
 return n;
}
uint8_t hand_valid(const GameState *g) CORE_BANKED {
 uint8_t i,j,found;
 if(g->deck_count>DECK_CAP || g->hand_count>g->deck_count || g->deck_pos>g->deck_count ||
    g->highlighted_count>HAND_PLAY_CAP || (g->phase==PHASE_TABLE && g->hand_count && g->focus>=g->hand_count))return 0;
 for(i=0;i<g->hand_count;i++) {
  if(!g->hand[i].uid)return 0;
  found=0;for(j=0;j<g->deck_count;j++)if(g->deck[j].uid==g->hand[i].uid)found=1;
  if(!found)return 0;
  for(j=0;j<i;j++)if(g->hand[j].uid==g->hand[i].uid)return 0;
 }
 for(i=0;i<g->highlighted_count;i++) {
  found=0;
  for(j=0;j<i;j++)if(g->selected_uids[i]==g->selected_uids[j])return 0;
  for(j=0;j<g->hand_count;j++)if(g->selected_uids[i]==g->hand[j].uid)found=1;
  if(!found)return 0;
 }
 return 1;
}

uint8_t hand_score_bit(const JokerContext *c,uint8_t index) CORE_BANKED {uint8_t p;for(p=0;p<c->played_count && p<HAND_PLAY_CAP;p++)if(c->played_index[p]==index)return 1u<<p;return 0;}
uint8_t hand_scoring(const JokerContext *c,uint8_t index) CORE_BANKED {return c->scoring_mask & hand_score_bit(c,index);}
