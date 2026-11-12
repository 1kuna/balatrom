#include "paged_inventory.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "boss_optional.h"
#include "hand_api.h"
uint8_t boss_optional_supported(uint8_t id) CORE_BANKED {
 switch(id) {
#ifdef BOSS_HAVE_HOOK
 case 3:return 1;
#endif
#ifdef BOSS_HAVE_FISH
 case 5:return 1;
#endif
#ifdef BOSS_HAVE_HOUSE
 case 10:return 1;
#endif
#ifdef BOSS_HAVE_MARK
 case 11:return 1;
#endif
#ifdef BOSS_HAVE_BELL
 case 12:return 1;
#endif
#ifdef BOSS_HAVE_WHEEL
 case 13:return 1;
#endif
#ifdef BOSS_HAVE_LEAF
 case 22:return 1;
#endif
/* Source overfull hand is integrated in hand_api.c. */
#ifdef BOSS_HAVE_SERPENT
 case 25:return 1;
#endif
#ifdef BOSS_HAVE_PILLAR
 case 26:return 1;
#endif
#ifdef BOSS_HAVE_ACORN
 case 28:return 1;
#endif
#ifdef BOSS_HAVE_HEART
 case 29:return 1;
#endif
 default:return 0;
 }
}
void boss_optional_begin(GameState *g) CORE_BANKED {
#ifdef BOSS_HAVE_HEART
 if(g->boss_id==29)g->boss_flipped=1;
#endif
#ifdef BOSS_HAVE_ACORN
 if(g->boss_id==28 && g->joker_count) {
  boss_core_hide_jokers(g,1);
  if(g->joker_count>1) {
   boss_core_shuffle_jokers(g);boss_core_shuffle_jokers(g);boss_core_shuffle_jokers(g);
  }
 }
#endif
 (void)g;
}
uint8_t boss_optional_card(GameState *g,const Card *c) CORE_BANKED {
#ifdef BOSS_HAVE_LEAF
 if(g->boss_id==22)return 1;
#endif
#ifdef BOSS_HAVE_PILLAR
 if(g->boss_id==26)return (c->flags&CARD_PLAYED_ANTE)?1:0;
#endif
 (void)g;(void)c;return 0;
}
uint8_t boss_play_allowed(GameState *g) CORE_BANKED {
#ifdef BOSS_HAVE_BELL
 uint8_t i;
 if(!g->boss_disabled && g->boss_id==12 && g->forced_card_uid) {
  for(i=0;i<g->hand_count && i<HAND_STORAGE_CAP;i++)
   if(g->hand[i].uid==g->forced_card_uid)return hand_is_selected(g,i)?1:0;
  return 0;
 }
#endif
 (void)g;return 1;
}
uint8_t boss_can_deselect(GameState *g,uint16_t uid) CORE_BANKED {
#ifdef BOSS_HAVE_BELL
 if(!g->boss_disabled && g->boss_id==12 && uid && uid==g->forced_card_uid)return 0;
#endif
 (void)g;(void)uid;return 1;
}
void boss_press_play(GameState *g) CORE_BANKED {
#ifdef BOSS_HAVE_HOOK
 uint8_t count,first,second;uint16_t uids[2];
#endif
 if(g->boss_disabled || !boss_optional_supported(g->boss_id))return;
#ifdef BOSS_HAVE_BELL
 if(g->boss_id==12){g->forced_card_uid=0;boss_core_force_selection(g,0);}
#endif
 if(g->boss_id==5 || g->boss_id==10 || g->boss_id==25)g->boss_flipped=1;
#ifdef BOSS_HAVE_HEART
 if(g->boss_id==29 && g->joker_count)g->boss_flipped=1;
#endif
#ifdef BOSS_HAVE_HOOK
 if(g->boss_id==3) {
  count=g->hand_count>HAND_STORAGE_CAP?HAND_STORAGE_CAP:g->hand_count;
  if(!count)return;
  first=boss_core_random_index(g,count,3);
  if(first>=count || g->numeric_error){g->numeric_error=1;return;}
  uids[0]=g->hand[first].uid;
  if(count>1) {
   second=boss_core_random_index(g,count-1,3);
   if(second>=count-1 || g->numeric_error){g->numeric_error=1;return;}
   if(second>=first)second++;
   uids[1]=g->hand[second].uid;
  }
  boss_core_hook_discard(g,uids,count>1?2:1);
 }
#endif
}
void boss_on_discard(GameState *g) CORE_BANKED {
 if(!g->boss_disabled && boss_optional_supported(g->boss_id) &&
    (g->boss_id==10 || g->boss_id==25))g->boss_flipped=1;
#ifdef BOSS_HAVE_BELL
 if(!g->boss_disabled && g->boss_id==12){g->forced_card_uid=0;boss_core_force_selection(g,0);}
#endif
}
void boss_on_card_draw(GameState *g,Card *c) CORE_BANKED {
 uint8_t flipped=0;
 if(g->boss_disabled || !boss_optional_supported(g->boss_id))return;
 switch(g->boss_id) {
#ifdef BOSS_HAVE_FISH
 case 5:flipped=g->boss_flipped;break;
#endif
#ifdef BOSS_HAVE_HOUSE
 case 10:flipped=!g->boss_flipped;break;
#endif
#ifdef BOSS_HAVE_MARK
 case 11:flipped=boss_card_is_face(g,c);break;
#endif
#ifdef BOSS_HAVE_WHEEL
 case 13:flipped=boss_core_wheel_flip(g);break;
#endif
 default:break;
 }
 /* Face-down orientation persists across arbitrary boss_card mutations. */
 c->flags=(c->flags & (uint8_t)~CARD_FACE_DOWN)|(flipped?CARD_FACE_DOWN:0);
 boss_card(g,c);
}
uint8_t boss_draw_count(GameState *g,uint8_t requested,uint8_t reason) CORE_BANKED {
#ifdef BOSS_HAVE_SERPENT
 if(!g->boss_disabled && g->boss_id==25 && boss_supported(25) &&
    (reason==1 || reason==2 || g->boss_flipped))return 3;
#endif
 (void)g;(void)reason;return requested;
}
void boss_optional_after_draw(GameState *g) CORE_BANKED {
#ifdef BOSS_HAVE_BELL
 uint8_t i,index,found=0;
#endif
#ifdef BOSS_HAVE_HEART
 uint8_t old,count=0,candidates[PG_JMAX],j,index2;
#endif
 if(g->boss_disabled)return;
#ifdef BOSS_HAVE_FISH
 if(g->boss_id==5)g->boss_flipped=0;
#endif
#ifdef BOSS_HAVE_BELL
 if(g->boss_id==12) {
  if(g->hand_count>HAND_STORAGE_CAP){g->numeric_error=1;return;}
  for(i=0;i<g->hand_count && i<HAND_STORAGE_CAP;i++)if(g->hand[i].uid==g->forced_card_uid)found=1;
  if(!found && g->hand_count) {
   index=boss_core_random_index(g,g->hand_count,12);
   if(index>=g->hand_count || g->numeric_error){g->numeric_error=1;return;}
   g->forced_card_uid=g->hand[index].uid;boss_core_force_selection(g,g->forced_card_uid);
  }
 }
#endif
#ifdef BOSS_HAVE_HEART
 if(g->boss_id==29 && g->boss_flipped && g->joker_count) {
  old=g->joker_debuff;
  for(j=0;j<g->joker_count && j<PG_JMAX;j++)
   if(!PG_DEBUFF(g,j) || g->joker_count<2)candidates[count++]=j;
  /* Source clears all old debuffs before selecting from the eligible list. */
  pg_flags_clear(g,0);
  if(count) {
   index2=boss_core_random_index(g,count,29);
   if(index2>=count || g->numeric_error){g->numeric_error=1;return;}
   pg_flag_set(g,candidates[index2],0,1);
  }
  g->boss_flipped=0;
 }
#endif
}
void boss_on_joker_sold(GameState *g) CORE_BANKED {
#ifdef BOSS_HAVE_LEAF
 if(g->boss_id==22 && !g->boss_disabled)boss_disable(g);
#endif
 (void)g;
}
void boss_optional_disable(GameState *g) CORE_BANKED {
 uint8_t i;
 if(g->boss_id==5 || g->boss_id==10 || g->boss_id==11 || g->boss_id==13) {
  for(i=0;i<g->hand_count && i<HAND_STORAGE_CAP;i++)g->hand[i].flags&=(uint8_t)~CARD_FACE_DOWN;
  for(i=0;i<g->deck_count && i<DECK_CAP;i++)g->deck[i].flags&=(uint8_t)~CARD_FACE_DOWN;
 }
#ifdef BOSS_HAVE_HEART
 if(g->boss_id==29)pg_flags_clear(g,0);
#endif
#ifdef BOSS_HAVE_BELL
 if(g->boss_id==12)boss_core_force_selection(g,0);
#endif
#ifdef BOSS_HAVE_ACORN
 if(g->boss_id==28)boss_core_hide_jokers(g,0);
#endif
}
void boss_optional_defeat(GameState *g) CORE_BANKED {
#ifdef BOSS_HAVE_HEART
 if(g->boss_id==29)pg_flags_clear(g,0);
#endif
#ifdef BOSS_HAVE_ACORN
 if(g->boss_id==28)boss_core_hide_jokers(g,0);
#endif
}
