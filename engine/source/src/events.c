#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "joker_history.h"
#include "hand_api.h"
#include "boss_optional.h"
#include "creation_core.h"
#include "joker_num_counters.h"
#pragma bank 255
#include "events.h"
#include "scoring.h"
#include "joker_config.h"
#include "joker_ids.h"
#include "shop.h"
#include "boss.h"
#include <string.h>
static void card_event(uint8_t event,const Card *cards,uint8_t count){JokerContext c;memset(&c,0,sizeof(c));c.event=event;c.cards=cards;c.card_count=count;gb_num_from_u32(&c.chips,1);gb_num_from_u32(&c.mult,1);joker_context_event(&c);}
void game_cards_removed(const Card *cards,uint8_t count) CORE_BANKED {card_event(JE_CARDS_REMOVED,cards,count);}
void game_cards_added(const Card *cards,uint8_t count) CORE_BANKED {card_event(JE_CARDS_ADDED,cards,count);}
void game_consumable_used(uint8_t kind,uint8_t id) CORE_BANKED {JokerContext c;if(!joker_history_used(&game,kind,id)){game.numeric_error=1;return;}if(kind==ITEM_PLANET&&id<12)game.used_planet_mask|=1u<<id;memset(&c,0,sizeof(c));c.event=JE_CONSUMABLE_USED;c.trigger_kind=kind;c.trigger_id=id;gb_num_from_u32(&c.chips,1);gb_num_from_u32(&c.mult,1);joker_context_event(&c);if((kind==ITEM_TAROT&&id!=0)||kind==ITEM_PLANET){game.last_consumable_kind=kind;game.last_consumable_id=id;}}
uint8_t game_change_hand_size(int16_t delta) CORE_BANKED {return hand_change_real_size(&game,delta);}
uint8_t game_generate_consumable(uint8_t kind,uint8_t id) CORE_BANKED {return game_generate_consumable_edition(kind,id,0);}
uint8_t game_add_joker(uint8_t id,uint8_t edition,uint8_t eternal) CORE_BANKED {JokerContext c;uint8_t index=game.joker_count;if((id==J_CHICOT&&!boss_chicot_preflight(&game))||id>=150||edition>ED_NEGATIVE||!inventory_joker_room(&game,edition)||game.next_joker_uid==65535u){game_action_error=2;return 0;}if(!pg_resize(index+1u,game.consumable_count)){game_action_error=2;return 0;}PG_JBYTE(&game,index,jokers,0u)=id;gb_num_from_u32(&PG_JNUM(&game,index,joker_state,1u),0);gb_num_from_u32(&PG_JNUM(&game,index,joker_extra_value,14u),0);PG_JBYTE(&game,index,joker_edition,9u)=edition;PG_JBYTE(&game,index,joker_eternal,10u)=eternal;PG_JBYTE(&game,index,joker_sell,11u)=joker_cost[id]/2;PG_JUID(&game,index)=++game.next_joker_uid;game.joker_count++;joker_history_sync_price(&game,index);memset(&c,0,sizeof(c));c.event=JE_ACQUIRE;c.joker_index=index;c.trigger_index=index;c.trigger_id=id;gb_num_from_u32(&c.chips,1);gb_num_from_u32(&c.mult,1);joker_context_event(&c);if(!creation_commit_active)boss_refresh_owned(&game);return 1;}
uint8_t game_remove_joker(uint8_t index) CORE_BANKED {uint8_t i;JokerContext c;if(index>=game.joker_count||PG_JBYTE(&game,index,joker_eternal,10u))return 0;memset(&c,0,sizeof(c));c.event=JE_REMOVE;c.joker_index=index;c.trigger_index=index;c.trigger_id=PG_JBYTE(&game,index,jokers,0u);gb_num_from_u32(&c.chips,1);gb_num_from_u32(&c.mult,1);joker_context_event(&c);
 for(i=index;i+1<game.joker_count;i++){PG_JBYTE(&game,i,jokers,0u)=PG_JBYTE(&game,i+1,jokers,0u);PG_JNUM(&game,i,joker_state,1u)=PG_JNUM(&game,i+1,joker_state,1u);PG_JNUM(&game,i,joker_extra_value,14u)=PG_JNUM(&game,i+1,joker_extra_value,14u);PG_JBYTE(&game,i,joker_edition,9u)=PG_JBYTE(&game,i+1,joker_edition,9u);PG_JBYTE(&game,i,joker_eternal,10u)=PG_JBYTE(&game,i+1,joker_eternal,10u);PG_JBYTE(&game,i,joker_sell,11u)=PG_JBYTE(&game,i+1,joker_sell,11u);PG_JUID(&game,i)=PG_JUID(&game,i+1);}pg_flags_remove(&game,index);game.joker_count--;pg_resize(game.joker_count,game.consumable_count);boss_refresh_owned(&game);return 1;}
void game_move_joker(uint8_t from,uint8_t to) CORE_BANKED {uint8_t a,b,tmp;GbNum state;uint16_t uid;if(from>=game.joker_count||to>=game.joker_count)return;for(a=from;a!=to;a=b){b=a<to?a+1:a-1;tmp=PG_JBYTE(&game,a,jokers,0u);PG_JBYTE(&game,a,jokers,0u)=PG_JBYTE(&game,b,jokers,0u);PG_JBYTE(&game,b,jokers,0u)=tmp;state=PG_JNUM(&game,a,joker_state,1u);PG_JNUM(&game,a,joker_state,1u)=PG_JNUM(&game,b,joker_state,1u);PG_JNUM(&game,b,joker_state,1u)=state;state=PG_JNUM(&game,a,joker_extra_value,14u);PG_JNUM(&game,a,joker_extra_value,14u)=PG_JNUM(&game,b,joker_extra_value,14u);PG_JNUM(&game,b,joker_extra_value,14u)=state;tmp=PG_JBYTE(&game,a,joker_edition,9u);PG_JBYTE(&game,a,joker_edition,9u)=PG_JBYTE(&game,b,joker_edition,9u);PG_JBYTE(&game,b,joker_edition,9u)=tmp;tmp=PG_JBYTE(&game,a,joker_eternal,10u);PG_JBYTE(&game,a,joker_eternal,10u)=PG_JBYTE(&game,b,joker_eternal,10u);PG_JBYTE(&game,b,joker_eternal,10u)=tmp;tmp=PG_JBYTE(&game,a,joker_sell,11u);PG_JBYTE(&game,a,joker_sell,11u)=PG_JBYTE(&game,b,joker_sell,11u);PG_JBYTE(&game,b,joker_sell,11u)=tmp;uid=PG_JUID(&game,a);PG_JUID(&game,a)=PG_JUID(&game,b);PG_JUID(&game,b)=uid;{pg_flags_swap(&game,a,b);}}}
/* Original Aura cutoff proportions on the native RNG; the desktop game's
 named streams and binary64 RNG are not reproduced. 255 means no edition. */
uint8_t game_poll_edition(uint8_t no_negative,uint8_t guaranteed) CORE_BANKED {uint16_t roll=game_random();(void)no_negative;if(!guaranteed)return 255;if(roll>55705)return ED_POLY;if(roll>32767)return ED_HOLO;if(roll>0)return ED_FOIL;return 255;}

uint8_t game_cards_event_preflight(uint8_t removed_count,uint8_t added_count) CORE_BANKED {uint8_t i,j,safe;if(removed_count>=game.deck_count||(uint16_t)game.deck_count+added_count-removed_count>DECK_CAP)return 0;
 /* Only known pure scoring Jokers with no add/remove reactions are eligible.
 Reactive handlers must extend this whitelist and capacity reservation together. */
 for(i=0;i<game.joker_count;i++){safe=PG_JBYTE(&game,i,jokers,0u)>=J_CAINO&&PG_JBYTE(&game,i,jokers,0u)<=J_PERKEO;for(j=0;j<joker_pool_count;j++)if(PG_JBYTE(&game,i,jokers,0u)==joker_pool[j])safe=1;
 if(PG_JBYTE(&game,i,jokers,0u)==J_HOLOGRAM){safe=1;if(added_count&&!(PG_DEBUFF(&game,i))){GbNum preview=PG_JNUM(&game,i,joker_state,1u);JokerCounterContext x;JokerNumCounterEffect out;memset(&x,0,sizeof(x));x.event=JCE_CARDS_ADDED;x.card_count_available=1;x.card_count=added_count;joker_num_counter_event(J_HOLOGRAM,&preview,&x,&out);if(!out.implemented||(out.numeric_status&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)))return 0;}}if(PG_JBYTE(&game,i,jokers,0u)==J_CAINO&&removed_count&&!PG_DEBUFF(&game,i)){GbNum preview;if(gb_num_add_i16(&preview,&PG_JNUM(&game,i,joker_state,1u),removed_count)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO))return 0;}if(!safe)return 0;}return 1;}
void game_refresh_card(Card *card) CORE_BANKED {boss_card(&game,card);}
uint8_t game_adjust_money(int32_t delta) CORE_BANKED {if((delta>0&&game.money>2147483647L-delta)||(delta<0&&game.money<(-2147483647L-1)-delta)){game.numeric_error=1;return 0;}game.money+=delta;return 1;}

void boss_core_draw_one(GameState *g) CORE_BANKED {if(g==&game){game_draw_to_hand(1);boss_after_draw(g,0);}}
