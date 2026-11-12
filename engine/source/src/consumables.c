#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "native_run_rng.h"
#include "joker_history.h"
#include "num_extra.h"
#include "hand_api.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "consumables.h"
#include "shop.h"
#include "events.h"
#include "boss.h"
#include "joker_config.h"
#include <string.h>

/* Native suits follow render.c's SDHC order, not Lua's SHDC pool. */
static const uint8_t suit_pool[4] = {0,2,1,3};
static const uint8_t enhancement_pool[7] = {
 ENH_BONUS,ENH_MULT,ENH_WILD,ENH_GLASS,ENH_STEEL,ENH_GOLD,ENH_LUCKY
};
static const uint8_t planet_hand[12] = {1,3,6,7,5,4,2,8,0,9,10,11};

static uint8_t hand_phase(const GameState *g) {
 return g->phase == PHASE_TABLE ||
  (g->phase == PHASE_PACK && (g->pack_kind == ITEM_TAROT ||
   g->pack_kind == ITEM_SPECTRAL || g->pack_kind == ITEM_PLANET));
}

/* Unknown active Boss rules cannot be approximated when transforming cards. */
static uint8_t card_context_supported(const GameState *g) {
 return g->blind!=2 || g->boss_disabled || boss_supported(g->boss_id);
}

/* Reject stale or aliased hand identities before sync can mutate master cards. */
static uint8_t valid_hand(const GameState *g) {
 uint8_t i,j,n;
 for(i=0;i<g->hand_count;i++) {
  if(g->hand[i].rank<2 || g->hand[i].rank>14 || g->hand[i].suit>3 ||
   !g->hand[i].uid) return 0;
  n=0;
  for(j=0;j<g->deck_count;j++) if(g->deck[j].uid==g->hand[i].uid) n++;
  if(n!=1) return 0;
  for(j=0;j<i;j++) if(g->hand[j].uid==g->hand[i].uid) return 0;
 }
 return 1;
}

static uint8_t selection(const GameState *g,uint8_t min,uint8_t max) {
 uint8_t i,n=0;
 if(!hand_phase(g) || !valid_hand(g)) return 0;
 for(i=0;i<g->hand_count;i++) if(hand_is_selected(g,i)) {
  if(i>=g->hand_count) return 0;
  n++;
 }
 return n>=min && n<=max;
}

static void sync_cards(GameState *g,int16_t one_index,uint8_t refresh) {
 uint8_t i;
 if(refresh) for(i=0;i<g->hand_count;i++)
  if(one_index==-2 || (one_index==-1 && hand_is_selected(g,i)) || one_index==i) game_refresh_card(&g->hand[i]);
 game_sync_hand();
}

static void finish(GameState *g,uint8_t refresh) {
 sync_cards(g,-1,refresh);
 hand_clear(g);
}

/* Preflight makes all add/remove helpers infallible under their contract.
   No RNG, inventory mutation, sync, or helper call precedes applicability. */
static uint8_t can_add(const GameState *g,uint8_t n,uint8_t removed,uint16_t *uid_floor) {
 uint8_t i;
 uint16_t maxuid=g->next_card_uid;
 if(g->deck_pos>g->deck_count || (uint16_t)g->deck_count+n-removed>DECK_CAP ||
  (uint16_t)g->hand_count+n-removed>HAND_STORAGE_CAP) return 0;
 for(i=0;i<g->deck_count;i++) if(g->deck[i].uid>maxuid) maxuid=g->deck[i].uid;
 if(maxuid>(uint16_t)(65535u-n)) return 0;
 *uid_floor=maxuid;
 return 1;
}

static uint8_t append_card(GameState *g,const Card *c) {
 uint8_t i,index;
 if(!game_add_card(c)) return 0;
 /* Directly held cards are consumed this blind. Preserve the undealt order. */
 index=g->deck_count-1;
 for(i=index;i>g->deck_pos;i--) g->draw_order[i]=g->draw_order[i-1];
 g->draw_order[g->deck_pos++]=index;
 g->hand[g->hand_count++]=g->deck[g->deck_count-1];
 game_refresh_card(&g->hand[g->hand_count-1]);
 g->deck[g->deck_count-1]=g->hand[g->hand_count-1];
 return 1;
}

static void sort_jokers(const GameState *g,uint16_t *indices,uint8_t n) {
 uint8_t i,j;uint16_t keep;
 for(i=1;i<n;i++) {
  keep=indices[i];j=i;
  while(j && PG_JUID(g,indices[j-1])>PG_JUID(g,keep)) {
   indices[j]=indices[j-1];j--;
  }
  indices[j]=keep;
 }
}

uint8_t consumable_apply(GameState *g,uint8_t kind,uint8_t id) CORE_BANKED {
 uint8_t i,j,n=0,first=0,last=0,value=0,mode=0;
 uint16_t uid[HAND_STORAGE_CAP],keep_uid,add;
 Card copy;
 Card payload[HAND_CAP];
 GbNum joker_copy,joker_extra_copy,sell_total_num,sell_cap_num;
 /* Helpers and RNG are global: accepting another object would mutate two states. */
 if(g!=&game || g->hand_count>HAND_STORAGE_CAP || g->deck_count>DECK_CAP ||
  g->joker_count>PG_JMAX) return 0;
 if(g->phase!=PHASE_TABLE && g->phase!=PHASE_SHOP && g->phase!=PHASE_PACK) return 0;
 if(kind==ITEM_PLANET) {
  if(id>=12 || g->hand_levels[planet_hand[id]]==65535u) return 0;
  g->hand_levels[planet_hand[id]]++;
  return 1;
 }
 if(kind==ITEM_TAROT) {
  switch(id) {
  case 0:
   /* Free-slot subset: API does not identify an imminent consumed source slot. */
   if(g->consumable_count>=PG_CMAX ||
    !((g->last_consumable_kind==ITEM_TAROT && g->last_consumable_id>0 &&
       g->last_consumable_id<22) ||
      (g->last_consumable_kind==ITEM_PLANET && g->last_consumable_id<12))) return 0;
   return game_generate_consumable(g->last_consumable_kind,g->last_consumable_id);
  case 1: value=ENH_LUCKY; n=2; break;
  case 3: value=ENH_MULT; n=2; break;
  case 5: value=ENH_BONUS; n=2; break;
  case 6: value=ENH_WILD; n=1; break;
  case 7: value=ENH_STEEL; n=1; break;
  case 8: value=ENH_GLASS; n=1; break;
  case 15: value=ENH_GOLD; n=1; break;
  case 16: value=ENH_STONE; n=1; break;
  case 17: value=1; n=3; mode=1; break; /* Diamonds */
  case 18: value=3; n=3; mode=1; break; /* Clubs */
  case 19: value=2; n=3; mode=1; break; /* Hearts */
  case 21: value=0; n=3; mode=1; break; /* Spades */
  case 9:
   add=g->money<=0 ? 0 : g->money<20 ? (uint16_t)g->money : 20;
   if(g->money>2147483647L-(int32_t)add) return 0;
   g->money+=add;
   return 1;
  case 14:
   if(!joker_history_sell_total(g,0,&sell_total_num))return 0;
   gb_num_from_u32(&sell_cap_num,50);
   if(gb_num_cmp(&sell_total_num,&sell_cap_num)>=0)add=50;
   else{uint32_t exact;if(num_to_u32_exact(&exact,&sell_total_num))return 0;add=(uint16_t)exact;}
   if(g->money>2147483647L-(int32_t)add) return 0;
   g->money+=add;
   return 1;
  case 10:
   for(i=0;i<g->joker_count;i++) if(PG_JBYTE(g,i,joker_edition,9u)==ED_NONE) uid[n++]=i;
   if(!n) return 0;
   sort_jokers(g,uid,n);
   /* Native modulo chance, followed only on success by candidate/edition draws. */
   if(!game_probability_hit(4)) return 1;
   i=(uint8_t)uid[game_random_index(n)];
   value=game_poll_edition(1,1);
   PG_JBYTE(g,i,joker_edition,9u)=value==255 ? ED_NONE : value;joker_history_sync_price(g,i);
   return 1;
  case 11: n=2; mode=2; break;
  case 12:
   if(!selection(g,1,2) || !g->highlighted_count || g->highlighted_count>2) return 0;
   value=0;
   for(i=g->highlighted_count;i>0;i--) {
    {uint8_t index[5];if(hand_selected_insertion_order(g,index)!=g->highlighted_count)return 0;j=index[i-1];}
    if(j>=g->hand_count || !(hand_is_selected(g,j))) return 0;
    payload[n]=g->hand[j];uid[n++]=g->hand[j].uid;
   }
   if(g->deck_count<=n || !game_cards_event_preflight(n,0)) return 0;
   game_sync_hand();
   for(i=0;i<n;i++) game_remove_uid(uid[i]);
   game_cards_removed(payload,n);
   g->highlighted_count=0;
   return 1;
  case 13:
   if(!card_context_supported(g) || !selection(g,2,2)) return 0;
   for(i=0;i<g->hand_count;i++) if(hand_is_selected(g,i)) {
    if(!n++) first=i;
    last=i;
   }
   keep_uid=g->hand[first].uid;
   value=g->hand[first].flags&CARD_FACE_DOWN;
   g->hand[first]=g->hand[last];
   g->hand[first].uid=keep_uid;
   g->hand[first].flags=(g->hand[first].flags&~CARD_FACE_DOWN)|value;
   sync_cards(g,first,1);
   hand_clear(g);
   return 1;
  default: return 0; /* explicit missing-context effects; see coverage.json */
  }
  if(!card_context_supported(g) || !selection(g,1,n)) return 0;
  for(i=0;i<g->hand_count;i++) if(hand_is_selected(g,i)) {
   if(mode==1) g->hand[i].suit=value;
   else if(mode==2) g->hand[i].rank=g->hand[i].rank==14 ? 2 : g->hand[i].rank+1;
   else {
    g->hand[i].enhancement=value;
    /* set_ability rebuilds ability (played_this_ante is not preserved). */
    g->hand[i].flags&=~CARD_PLAYED_ANTE;
   }
  }
  finish(g,1);
  return 1;
 }
 if(kind!=ITEM_SPECTRAL) return 0;
 switch(id) {
 case 3: value=SEAL_GOLD; break;
 case 11: value=SEAL_RED; break;
 case 13: value=SEAL_BLUE; break;
 case 14: value=SEAL_PURPLE; break;
 case 4:
  if(!selection(g,1,1)) return 0;
  for(i=0;i<g->hand_count;i++) if(hand_is_selected(g,i)) first=i;
  if(g->hand[first].edition!=ED_NONE) return 0;
  value=game_poll_edition(1,1);
  g->hand[first].edition=value==255 ? ED_NONE : value;
  game_sync_hand();
  return 1;
 case 6:
  if(!card_context_supported(g) || !hand_phase(g) || g->hand_count<=1 || !valid_hand(g)) return 0;
  value=suit_pool[game_random_index(4)];
  for(i=0;i<g->hand_count;i++) g->hand[i].suit=value;
  sync_cards(g,-2,1);
  return 1;
 case 7:
  if(!card_context_supported(g) || !hand_phase(g) || g->hand_count<=1 || !valid_hand(g) ||
   g->hand_size==-32768) return 0;
  value=2+game_random_index(13);
  for(i=0;i<g->hand_count;i++) g->hand[i].rank=value;
  game_change_hand_size(-1);
  sync_cards(g,-2,1);
  return 1;
 case 12:
  /* Hex's edition transform is safe only when no other Joker can dissolve.
     Validate every possible candidate before drawing, avoiding partial effects. */
  for(i=0;i<g->joker_count;i++) if(PG_JBYTE(g,i,joker_edition,9u)==ED_NONE) {
   for(j=0;j<g->joker_count;j++)
    if(j!=i && !PG_JBYTE(g,j,joker_eternal,10u)) mode=1;
   uid[n++]=i;
  }
  if(!n) return 0;
  if(mode && (pg_any_debuff(g) || !game_cards_event_preflight(0,0))) return 0;
  if(mode) for(j=0;j<g->joker_count;j++)
   if(!PG_JBYTE(g,j,joker_eternal,10u) && PG_JBYTE(g,j,joker_edition,9u)==ED_NEGATIVE) return 0;
  sort_jokers(g,uid,n);
  i=(uint8_t)uid[game_random_index(n)];
  PG_JBYTE(g,i,joker_edition,9u)=ED_POLY;joker_history_sync_price(g,i);
  if(mode) {
   j=0;
   while(j<g->joker_count) {
    if(j!=i && !PG_JBYTE(g,j,joker_eternal,10u)) {
     game_remove_joker(j);
     if(j<i) i--;
    } else j++;
   }
  }
  return 1;
 case 8:
  /* Source pool is editionless Jokers in current order; penalty grows1,2,...
     Physical/hand bounds are checked before any random draw or mutation. */
  if(!pg_fits((uint16_t)g->joker_count+1u,g->consumable_count)||g->ectoplasm_uses>=32767u||
     g->hand_size<=(int16_t)(g->ectoplasm_uses+1u)||!joker_history_valid(g))return 0;
  for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,joker_edition,9u)==ED_NONE)uid[n++]=i;
  if(!n)return 0;
  i=(uint8_t)uid[game_random_index(n)];
  if(!game_change_hand_size(-(int16_t)(g->ectoplasm_uses+1u)))return 0;
  PG_JBYTE(g,i,joker_edition,9u)=ED_NEGATIVE;g->ectoplasm_uses++;joker_history_sync_price(g,i);return !g->numeric_error;
 case 10:
  /* Ankh subset: known pure scoring Jokers; no negative-edition repricing,
     passive acquire/remove reactions, debuff reset, or UID wrap approximation. */
  if(!g->joker_count || !pg_fits((uint16_t)g->joker_count+1u,g->consumable_count) || pg_any_debuff(g) ||
   g->next_joker_uid==65535u || !game_cards_event_preflight(0,0)) return 0;
  for(i=0;i<g->joker_count;i++) {
   if(PG_JBYTE(g,i,joker_edition,9u)>ED_POLY || !PG_JUID(g,i) ||
    PG_JUID(g,i)>g->next_joker_uid) return 0;
   for(j=0;j<i;j++) if(PG_JUID(g,i)==PG_JUID(g,j)) return 0;
   uid[n++]=i;
  }
  sort_jokers(g,uid,n);
  first=(uint8_t)uid[game_random_index(n)];
  value=PG_JBYTE(g,first,jokers,0u);mode=PG_JBYTE(g,first,joker_edition,9u);
  last=PG_JBYTE(g,first,joker_eternal,10u);add=PG_JBYTE(g,first,joker_sell,11u);
  joker_copy=PG_JNUM(g,first,joker_state,1u);joker_extra_copy=PG_JNUM(g,first,joker_extra_value,14u);
  keep_uid=PG_JUID(g,first);
  i=0;
  while(i<g->joker_count) {
   if(PG_JUID(g,i)!=keep_uid && !PG_JBYTE(g,i,joker_eternal,10u)) game_remove_joker(i);
   else i++;
  }
  i=g->joker_count;
  game_add_joker(value,mode,last);
  PG_JNUM(g,i,joker_state,1u)=joker_copy;PG_JNUM(g,i,joker_extra_value,14u)=joker_extra_copy;
  PG_JBYTE(g,i,joker_sell,11u)=(uint8_t)add;joker_history_sync_price(g,i);
  return 1;
 case 9:
  if(!hand_phase(g) || g->hand_count<=1 || !valid_hand(g)) return 0;
  n=g->hand_count<5 ? g->hand_count : 5;
  if(g->deck_count<=n || g->money>2147483627L ||
   !game_cards_event_preflight(n,0)) return 0;
  for(i=0;i<g->hand_count;i++) uid[i]=g->hand[i].uid;
  for(i=1;i<g->hand_count;i++) {
   keep_uid=uid[i];j=i;
   while(j && uid[j-1]>keep_uid) {uid[j]=uid[j-1];j--;}
   uid[j]=keep_uid;
  }
  /* Same descending Fisher-Yates draw order as original pseudoshuffle;
     native 16-bit modulo backend, not Lua randomseed parity. */
  for(i=g->hand_count-1;i>0;i--) {
   j=game_random_index((i+1));keep_uid=uid[i];uid[i]=uid[j];uid[j]=keep_uid;
  }
  for(i=0;i<n;i++) for(j=0;j<g->hand_count;j++)
   if(g->hand[j].uid==uid[i]) payload[i]=g->hand[j];
  game_sync_hand();
  for(i=n;i>0;i--) game_remove_uid(uid[i-1]);
  g->money+=20;
  game_cards_removed(payload,n);
  g->highlighted_count=0;
  return 1;
 case 15:
  if(!card_context_supported(g) || !selection(g,1,1) || !can_add(g,2,0,&add) || !game_cards_event_preflight(0,2)) return 0;
  for(i=0;i<g->hand_count;i++) if(hand_is_selected(g,i)) copy=g->hand[i];
  /* copy_card creates a new front-facing card; facing is not copied. */
  copy.flags&=~CARD_FACE_DOWN;
  g->next_card_uid=add;
  game_sync_hand();
  if(!append_card(g,&copy)){g->numeric_error=1;return 0;}
  payload[0]=g->hand[g->hand_count-1];
  if(!append_card(g,&copy)){g->numeric_error=1;return 0;}
  payload[1]=g->hand[g->hand_count-1];
  game_cards_added(payload,2);
  return 1; /* Cryptid does not unhighlight in original */
 case 0: case 1: case 2:
  n=id==0 ? 3 : id==1 ? 2 : 4;
  if(!card_context_supported(g) || !hand_phase(g) || g->hand_count<=1 ||
   !valid_hand(g) || !can_add(g,n,1,&add) || g->deck_count<=1 ||
   !game_cards_event_preflight(1,n)) return 0;
  /* Prototype draws preserve original effect-level call order, not Lua seeds. */
  /* Lua pseudorandom_element sorts Card sort_id: native UID is creation order. */
  for(i=0;i<g->hand_count;i++) uid[i]=g->hand[i].uid;
  for(i=1;i<g->hand_count;i++) {
   keep_uid=uid[i];j=i;
   while(j && uid[j-1]>keep_uid) {uid[j]=uid[j-1];j--;}
   uid[j]=keep_uid;
  }
  /* Reserve the complete UID range before removal, RNG or callbacks. */
  g->next_card_uid=add;
  keep_uid=uid[game_random_index(g->hand_count)];
  for(i=0;i<g->hand_count;i++) if(g->hand[i].uid==keep_uid) payload[0]=g->hand[i];
  game_sync_hand();
  game_remove_uid(keep_uid);
  game_cards_removed(payload,1);
  g->highlighted_count=0;
  for(i=0;i<n;i++) {
   /* Byte-zero representation is also the GbNum zero initializer in v05. */
   memset(&copy,0,sizeof(copy));
   copy.rank=id==1 ? 14 : (id==0 ? 11+game_random_index(3) : 2+game_random_index(9));
   copy.suit=suit_pool[game_random_index(4)];
   copy.enhancement=enhancement_pool[game_random_index(7)];
   copy.uid=0; copy.seal=SEAL_NONE; copy.edition=ED_NONE;
   copy.flags=0;
   if(!append_card(g,&copy)){g->numeric_error=1;return 0;}
   payload[i]=g->hand[g->hand_count-1];
  }
  game_cards_added(payload,n);
  return 1;
 case 17:
  for(i=0;i<12;i++) if(g->hand_levels[i]==65535u) return 0;
  for(i=0;i<12;i++) g->hand_levels[i]++;
  return 1;
 default: return 0;
 }
 if(!selection(g,1,1)) return 0;
 for(i=0;i<g->hand_count;i++) if(hand_is_selected(g,i)) g->hand[i].seal=value;
 finish(g,0);
 return 1;
}
