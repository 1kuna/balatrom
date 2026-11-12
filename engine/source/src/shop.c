#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "native_run_rng.h"
#include "joker_history.h"
#pragma bank 255
#include <gb/gb.h>
#include "shop.h"
#include "joker_ids.h"
#include "joker_config.h"
#include "scoring.h"
#include "events.h"
#include "consumables.h"
#include "creation_core.h"
#include "hand_api.h"
#include "boss.h"
#include "boss_optional.h"
#include <string.h>

/* Passives derive from actual active ownership; no lossy shadow fields or
 * creation callbacks. Zero-price cards remain buyable even below debt floor. */
static uint8_t shop_owned(const GameState *g,uint8_t id){uint8_t i,n=0;for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==id&&!PG_DEBUFF(g,i))n++;return n;}
uint8_t shop_can_spend(const GameState *g,uint16_t cost) CORE_BANKED {return !cost || g->money >= (int32_t)cost-20L*shop_owned(g,J_CREDIT_CARD);}
uint8_t shop_item_price(const GameState *g,uint8_t slot) CORE_BANKED {if(slot>=2||g->shop[slot]==255)return 0;return g->shop_kind[slot]==ITEM_JOKER?joker_cost[g->shop[slot]]:g->shop_kind[slot]==ITEM_PLANET&&shop_owned(g,J_ASTRONOMER)?0:3;}
uint8_t shop_pack_price(const GameState *g,uint8_t slot) CORE_BANKED {uint8_t size;if(slot>=2)return 0;if(g->shop_pack_kind[slot]==ITEM_PLANET&&shop_owned(g,J_ASTRONOMER))return 0;size=g->pack_size[slot];return size==0?4:size==1?6:8;}
static const uint8_t planet_hand[12]={1,3,6,7,5,4,2,8,0,9,10,11};
static uint8_t eligible_joker(uint8_t id,uint8_t offer){uint8_t i,found=0;if(joker_rarity[id]==4)return 0;for(i=0;i<game.joker_count;i++)if(PG_JBYTE(&game,i,jokers,0u)==id)return 0;if(offer&&game.shop_kind[0]==ITEM_JOKER&&game.shop[0]==id)return 0;
 if(id==J_STEEL_JOKER||id==J_STONE){for(i=0;i<game.deck_count;i++)if(game.deck[i].enhancement==(id==J_STEEL_JOKER?ENH_STEEL:ENH_STONE))found=1;return found;}return 1;}
static uint8_t roll_joker(uint8_t offer){uint16_t roll=game_random();uint8_t rarity=roll>62258u?3:roll>45875u?2:1,ids[150],n=0,i,id;
 for(i=0;i<joker_pool_count;i++){id=joker_pool[i];if(joker_rarity[id]==rarity&&eligible_joker(id,offer))ids[n++]=id;}return n?ids[game_random_index(n)]:J_JOKER;}
static uint8_t roll_planet(void){uint8_t ids[12],i,n=9;for(i=0;i<9;i++)ids[i]=i;for(i=9;i<12;i++)if(game.hands_played[planet_hand[i]])ids[n++]=i;return ids[game_random_index(n)];}
static uint8_t roll_tarot(void){return game_random_bounded(22);}
uint8_t shop_background_take(void) CORE_BANKED;
static void roll_items(void){uint8_t i,roll;if(shop_background_take())return;for(i=0;i<2;i++){
 roll=game_random_index(28);
 if(roll>=24){game.shop_kind[i]=ITEM_TAROT;game.shop[i]=roll_tarot();}
 else if(roll>=20){game.shop_kind[i]=ITEM_PLANET;game.shop[i]=roll_planet();}
 else{game.shop_kind[i]=ITEM_JOKER;game.shop[i]=roll_joker(i);}
}}
/* Aggregated source Booster weights over all five families. */
static void roll_pack(uint8_t slot){uint16_t v;
 if(!game.first_shop_buffoon){game.first_shop_buffoon=1;game.shop_pack_kind[slot]=ITEM_JOKER;game.pack_size[slot]=0;return;}
 v=game_random_bounded(2242);
 if(v<650){game.shop_pack_kind[slot]=ITEM_TAROT;game.pack_size[slot]=v<400?0:v<600?1:2;}
 else if(v<1300){v-=650;game.shop_pack_kind[slot]=ITEM_PLANET;game.pack_size[slot]=v<400?0:v<600?1:2;}
 else if(v<1950){v-=1300;game.shop_pack_kind[slot]=ITEM_PLAYING;game.pack_size[slot]=v<400?0:v<600?1:2;}
 else if(v<2145){v-=1950;game.shop_pack_kind[slot]=ITEM_JOKER;game.pack_size[slot]=v<120?0:v<180?1:2;}
 else{v-=2145;game.shop_pack_kind[slot]=ITEM_SPECTRAL;game.pack_size[slot]=v<60?0:v<90?1:2;}
}
void shop_open(void) CORE_BANKED {uint8_t i;roll_items();for(i=0;i<2;i++){roll_pack(i);game.pack_bought[i]=0;}game.focus=game.ui_mode=0;game.phase=PHASE_SHOP;}
static void pack_close(void){game_sync_hand();hand_clear(&game);game.hand_count=game.deck_pos=game.focus=game.ui_mode=0;game.phase=PHASE_SHOP;}
static uint8_t roll_pack_item(uint8_t kind,uint8_t count){uint8_t id,i,j,ids[150],n=0,duplicate;
 if(kind==ITEM_JOKER){uint16_t roll=game_random();uint8_t rarity=roll>62258u?3:roll>45875u?2:1;
 for(i=0;i<joker_pool_count;i++){id=joker_pool[i];duplicate=0;for(j=0;j<count;j++)if(game.pack_ids[j]==id)duplicate=1;
 for(j=0;j<2;j++)if(game.shop_kind[j]==ITEM_JOKER&&game.shop[j]==id)duplicate=1;
 if(!duplicate&&joker_rarity[id]==rarity&&eligible_joker(id,0))ids[n++]=id;}return n?ids[game_random_bounded(n)]:J_JOKER;}
 for(i=0;i<(kind==ITEM_TAROT?22:kind==ITEM_PLANET?12:16);i++){
 if(kind==ITEM_PLANET&&i>=9&&!game.hands_played[planet_hand[i]])continue;
 duplicate=0;for(j=0;j<count;j++)if(game.pack_ids[j]==i)duplicate=1;
 for(j=0;j<game.consumable_count;j++)if(PG_CBYTE(&game,j,consumable_kind,0u)==kind&&PG_CBYTE(&game,j,consumables,1u)==i)duplicate=1;
 for(j=0;j<2;j++)if(game.shop_kind[j]==kind&&game.shop[j]==i)duplicate=1;
 if(!duplicate)ids[n++]=i;}
 return n?ids[game_random_bounded(n)]:(kind==ITEM_PLANET?0:kind==ITEM_TAROT?9:3);
}
static void standard_card(Card *card){uint16_t front,roll;uint8_t enhanced;static const uint8_t ranks[13]={2,3,4,5,6,7,8,9,14,11,13,12,10};static const uint8_t suits[4]={3,1,2,0};
 memset(card,0,sizeof(*card));enhanced=game_random_bounded(10000)>=6000;
 if(enhanced)card->enhancement=1+game_random_bounded(8);
 front=game_random_bounded(52);card->rank=ranks[front%13];card->suit=suits[front/13];
 roll=game_random_bounded(10000);card->edition=roll>=9880?ED_POLY:roll>=9600?ED_HOLO:roll>=9200?ED_FOIL:ED_NONE;
 if(game_random_bounded(10000)>=8000){static const uint8_t seals[4]={SEAL_PURPLE,SEAL_GOLD,SEAL_BLUE,SEAL_RED};card->seal=seals[game_random_bounded(4)];}
}
static void pack_open(uint8_t slot){uint8_t i,size=game.pack_size[slot],kind=game.shop_pack_kind[slot];
 if(kind>=ITEM_PLANET&&kind<=ITEM_SPECTRAL&&!creation_core_offer_begin()){game.numeric_error=1;game.phase=PHASE_BLOCKED;return;}
 game.pack_kind=kind;game.pack_count=(kind==ITEM_JOKER||kind==ITEM_SPECTRAL)?(size?4:2):(size?5:3);game.pack_choices=size==2?2:1;
 for(i=0;i<game.pack_count;i++){game.pack_item_kind[i]=kind;
 if(kind==ITEM_PLAYING){standard_card(&game.pack_cards[i]);game.pack_ids[i]=i;}
 else if(kind==ITEM_JOKER)game.pack_ids[i]=roll_pack_item(kind,i);
 else if(!creation_core_offer(kind,&game.pack_item_kind[i],&game.pack_ids[i])){game.numeric_error=1;game.phase=PHASE_BLOCKED;return;}}
 game.focus=game.ui_mode=game.pack_offer_focus=0;game.phase=PHASE_PACK;
 if(kind==ITEM_TAROT||kind==ITEM_SPECTRAL){game_sync_hand();hand_clear(&game);game.hand_count=game.deck_pos=0;hand_draw(&game,game.hand_size>0?game.hand_size:0,0);}
}
uint8_t consumable_use(uint8_t slot) CORE_BANKED {uint8_t i,kind,id;if(slot>=game.consumable_count)return 0;kind=PG_CBYTE(&game,slot,consumable_kind,0u);id=PG_CBYTE(&game,slot,consumables,1u);if(creation_effect_supported(kind,id)){uint8_t ok=creation_core_use(CREATION_INVENTORY,slot,kind,id)&CREATION_OK;if(!ok)game_action_error=2;return ok!=0;}if(!consumable_apply(&game,kind,id)){game_action_error=kind==ITEM_SPECTRAL&&id==8?2:1;return 0;}game_consumable_used(kind,id);
 for(i=slot;i+1<game.consumable_count;i++){PG_CBYTE(&game,i,consumables,1u)=PG_CBYTE(&game,i+1,consumables,1u);PG_CBYTE(&game,i,consumable_edition,2u)=PG_CBYTE(&game,i+1,consumable_edition,2u);PG_CBYTE(&game,i,consumable_kind,0u)=PG_CBYTE(&game,i+1,consumable_kind,0u);}game.consumable_count--;pg_resize(game.joker_count,game.consumable_count);game.consumable_focus=0;if(!game.consumable_count)game.ui_mode=0;return 1;}
void shop_input(uint8_t pressed) CORE_BANKED {uint8_t i,index,cost;
 if(pressed&J_LEFT)game.focus=game.focus?game.focus-1:3;
 if(pressed&J_RIGHT)game.focus=(game.focus+1)%4;
 if(pressed&J_A){index=game.focus;cost=shop_item_price(&game,index);
 if(index<2&&game.shop[index]!=255){
 if((game.shop_kind[index]==ITEM_PLANET||game.shop_kind[index]==ITEM_TAROT)&&shop_can_spend(&game,cost)&&!inventory_consumable_room(&game,ED_NONE)){game_action_error=2;return;}
 if(game.shop_kind[index]==ITEM_JOKER){cost=shop_item_price(&game,index);if(shop_can_spend(&game,cost)&&!inventory_joker_room(&game,ED_NONE)){game_action_error=2;return;}if(shop_can_spend(&game,cost)&&inventory_joker_room(&game,ED_NONE)){if(game_add_joker(game.shop[index],ED_NONE,0)){game.money-=cost;game.shop[index]=255;}}}
 else if((game.shop_kind[index]==ITEM_PLANET||game.shop_kind[index]==ITEM_TAROT)&&shop_can_spend(&game,cost)&&inventory_consumable_room(&game,ED_NONE)){if(game_generate_consumable(game.shop_kind[index],game.shop[index])){game.money-=cost;game.shop[index]=255;}}
 }else if(index>=2&&!game.pack_bought[index-2]){
 uint8_t size=game.pack_size[index-2];cost=shop_pack_price(&game,index-2);if(shop_can_spend(&game,cost)){game.money-=cost;game.pack_bought[index-2]=1;pack_open(index-2);}

 }}
 if((pressed&J_B)&&game.joker_count){if(!PG_JBYTE(&game,game.joker_count-1,joker_eternal,10u)){uint8_t slot=game.joker_count-1;int32_t value;if(!joker_history_sale_amount(&game,slot,&value)){game_action_error=2;return;}joker_event(JE_SELL);if(game_remove_joker(slot)){game_adjust_money(value);boss_on_joker_sold(&game);}}}
 if((pressed&J_SELECT)&&shop_can_spend(&game,5)){game.money-=5;roll_items();joker_event(JE_REROLL);}
}
void pack_input(uint8_t pressed) CORE_BANKED {uint8_t i,id,ok,kind;
 if(game.ui_mode==3){
 if(pressed&J_LEFT)game.focus=game.focus?game.focus-1:game.hand_count-1;
 if(pressed&J_RIGHT)game.focus=(game.focus+1)%game.hand_count;
 if(pressed&J_A)hand_toggle(&game,game.focus);
 if(pressed&J_SELECT)hand_sort_rank(&game);
 if(pressed&(J_DOWN|J_START|J_B)){game.ui_mode=0;game.focus=game.pack_offer_focus;}return;}
 if((pressed&J_UP)&&game.hand_count&&(game.pack_kind==ITEM_TAROT||game.pack_kind==ITEM_SPECTRAL)){game.pack_offer_focus=game.focus;game.focus=0;game.ui_mode=3;return;}
 if(!game.pack_count)return;
 if(pressed&J_LEFT)game.focus=game.focus?game.focus-1:game.pack_count-1;
 if(pressed&J_RIGHT)game.focus=(game.focus+1)%game.pack_count;
 if(pressed&J_A){id=game.pack_ids[game.focus];kind=game.pack_item_kind[game.focus];
 if(game.pack_kind==ITEM_PLAYING){ok=game_cards_event_preflight(0,1)&&game_add_card(&game.pack_cards[game.focus]);if(ok)game_cards_added(&game.deck[game.deck_count-1],1);}
 else if(kind==ITEM_JOKER)ok=game_add_joker(id,ED_NONE,0);
 else if(creation_effect_supported(kind,id)){
 ok=creation_core_use(CREATION_PACK,game.focus,kind,id)&CREATION_OK;
 if(ok&&game.phase==PHASE_SHOP)pack_close();if(!ok)game_action_error=2;return;
 }else{ok=consumable_apply(&game,kind,id);if(ok)game_consumable_used(kind,id);}
 if(!ok)game_action_error=kind==ITEM_SPECTRAL&&id==8?2:1;
 if(ok){game.pack_choices--;for(i=game.focus;i+1<game.pack_count;i++){game.pack_ids[i]=game.pack_ids[i+1];game.pack_item_kind[i]=game.pack_item_kind[i+1];game.pack_cards[i]=game.pack_cards[i+1];}game.pack_count--;game.focus=0;if(!game.pack_choices)pack_close();}}
 if(pressed&(J_START|J_B)){joker_event(JE_SKIP_PACK);pack_close();}
}

/* Private native-only speculative stock preparation. Bank7 D000..D0FF is
 * below the existing Frame lease at D100. Header64 +150 candidate bytes end
 * at D0D5; telemetry livesD0F0..D0FB. No authoritative state changes on idle.
 * One draw, eight deck cards or four pool entries per slice; strict dependency equality is
 * checked immediately before stock/RNG commit. An8-bit input epoch restarts pending work; epoch wrap cannot admit stale stock because the consumer independently checks the full semantic key. */
#include "native_run_rng.h"
#include "raster_runtime.h"
#define SB_BASE 0xd000u
#define SB_CANDIDATES 0xd040u
#define SB_STATS 0xd0f0u
static void sb_counter(uint8_t index){uint8_t b[2];uint16_t n;raster_get_block(7u,SB_STATS+index*2u,b,2u);n=b[0]|((uint16_t)b[1]<<8);if(n<65535u)n++;b[0]=n;b[1]=n>>8;raster_put_block(7u,SB_STATS+index*2u,b,2u);}
static uint8_t sb_key(uint8_t h[64],uint8_t write){
 uint8_t i,flags=0,planets=0,meta[20];if(game.joker_count>8u||game.joker_count>JOKER_CAP)return 0;
 native_rng_export(meta);if(!native_rng_valid(meta))return 0;
 for(i=0;i<game.deck_count;i++){if(game.deck[i].enhancement==ENH_STEEL)flags|=1u;if(game.deck[i].enhancement==ENH_STONE)flags|=2u;}
 for(i=9u;i<12u;i++)if(game.hands_played[planet_hand[i]])planets|=1u<<(i-9u);
 if(write){h[12]=game.joker_count;memset(h+13u,0,8u);memcpy(h+13u,game.jokers,game.joker_count);h[21]=flags;h[22]=planets;h[23]=0;memcpy(h+24u,meta,20u);memcpy(h+44u,meta,20u);return 1;}
 return h[12]==game.joker_count&&!memcmp(h+13u,game.jokers,game.joker_count)&&h[21]==flags&&h[22]==planets&&!memcmp(h+24u,meta,20u);
}
static uint8_t sb_eligible(const uint8_t h[64],uint8_t id){uint8_t i;if(joker_rarity[id]==4u)return 0;for(i=0;i<h[12];i++)if(h[13u+i]==id)return 0;if(h[4]&&h[8]==ITEM_JOKER&&h[10]==id)return 0;if(id==J_STEEL_JOKER)return h[21]&1u;if(id==J_STONE)return h[21]&2u;return 1;}
static void sb_done(uint8_t h[64],uint8_t id){h[10u+h[4]]=id;if(++h[4]==2u)h[3]=9u;else h[3]=1u;}
void shop_background_reset(void) CORE_BANKED {uint8_t zero[12];memset(zero,0,12u);raster_put_block(7u,SB_BASE,zero,4u);raster_put_block(7u,SB_STATS,zero,12u);raster_put_block(4u,0xdfd0u,zero,1u);}
void shop_background_input(void) CORE_BANKED {uint8_t epoch;raster_get_block(4u,0xdfd0u,&epoch,1u);epoch++;raster_put_block(4u,0xdfd0u,&epoch,1u);}
static uint8_t sb_start(uint8_t h[64],uint8_t epoch){uint8_t i,meta[20];if(game.joker_count>8u||game.joker_count>JOKER_CAP)return 0;native_rng_export(meta);if(!native_rng_valid(meta))return 0;h[12]=game.joker_count;memcpy(h+13u,game.jokers,game.joker_count);h[23]=epoch;for(i=9u;i<12u;i++)if(game.hands_played[planet_hand[i]])h[22]|=1u<<(i-9u);memcpy(h+24u,meta,20u);memcpy(h+44u,meta,20u);return 1;}
void shop_background_slice_end(void) CORE_BANKED {}
void shop_background_idle(void) CORE_BANKED {
 uint8_t h[64],i,id,ids[12],n,epoch;const Card *card;uint16_t value,start,elapsed;
 if(game.phase!=PHASE_TABLE&&game.phase!=PHASE_WIN&&game.phase!=PHASE_SHOP)return;
 /* Native UI/IRQ owners stay active. Enter near VBlank, never mask an entire
  * slice; bank ferry critical regions retain their existing <=16byte bound. */
 if(LY_REG<144u&&LY_REG>3u)return;if(native_rng_mode()!=1u)return;start=sys_time;
 raster_get_block(7u,SB_BASE,h,64u);
 if(h[0]==0x53u&&h[1]==0x42u&&h[2]==1u&&h[3]==9u)return; /* consumer checks the full key */
 raster_get_block(4u,0xdfd0u,&epoch,1u);
 if(h[0]!=0x53u||h[1]!=0x42u||h[2]!=1u||!h[3]||h[23]!=epoch){
  if(h[0]==0x53u&&h[1]==0x42u&&h[3])sb_counter(2u);
  memset(h,0,64u);h[0]=0x53u;h[1]=0x42u;h[2]=1u;h[3]=10u;if(!sb_start(h,epoch))return;
 }
 if(h[3]==10u){card=game.deck+h[5];for(i=0;i<8u&&h[5]<game.deck_count;i++,h[5]++,card++){if(card->enhancement==ENH_STEEL)h[21]|=1u;if(card->enhancement==ENH_STONE)h[21]|=2u;}if(h[5]==game.deck_count){h[5]=0;h[3]=1u;}}
 else if(h[3]==1u){if(!native_rng_step(h+44u,28u,&value))return;h[8u+h[4]]=value>=24u?ITEM_TAROT:value>=20u?ITEM_PLANET:ITEM_JOKER;h[3]=2u;}
 else if(h[3]==2u){
  if(h[8u+h[4]]==ITEM_JOKER){if(!native_rng_step(h+44u,0u,&value))return;h[7]=value>62258u?3u:value>45875u?2u:1u;h[5]=h[6]=0;h[3]=3u;}
  else if(h[8u+h[4]]==ITEM_TAROT){if(!native_rng_step(h+44u,22u,&value))return;sb_done(h,value);}
  else{n=9u;for(i=0;i<9u;i++)ids[i]=i;for(i=9u;i<12u;i++)if(h[22]&(1u<<(i-9u)))ids[n++]=i;if(!native_rng_step(h+44u,n,&value))return;sb_done(h,ids[value]);}
 }
 else if(h[3]==3u){
  for(i=0;i<4u&&h[5]<joker_pool_count;i++){id=joker_pool[h[5]++];if(joker_rarity[id]==h[7]&&sb_eligible(h,id)){if(h[6]>=150u)return;raster_put_block(7u,SB_CANDIDATES+h[6],&id,1u);h[6]++;}}
  if(h[5]==joker_pool_count){if(!h[6])sb_done(h,J_JOKER);else h[3]=4u;}
 }
 else if(h[3]==4u){if(!native_rng_step(h+44u,h[6],&value))return;raster_get_block(7u,SB_CANDIDATES+value,&id,1u);sb_done(h,id);}
 else{return;}
 raster_put_block(7u,SB_BASE,h,64u);sb_counter(3u);elapsed=sys_time-start;
 {uint8_t old[2];uint16_t peak;raster_get_block(7u,SB_STATS+8u,old,2u);peak=old[0]|((uint16_t)old[1]<<8);if(elapsed>peak){old[0]=elapsed;old[1]=elapsed>>8;raster_put_block(7u,SB_STATS+8u,old,2u);}if(elapsed>1u)sb_counter(5u);}
 shop_background_slice_end();
}
uint8_t shop_background_take(void) CORE_BANKED {
 uint8_t h[64],zero=0;if(native_rng_mode()!=1u)return 0;raster_get_block(7u,SB_BASE,h,64u);
 if(h[0]!=0x53u||h[1]!=0x42u||h[2]!=1u||h[3]!=9u||!sb_key(h,0u)){sb_counter(1u);raster_put_block(7u,SB_BASE+3u,&zero,1u);return 0;}
 if(!native_rng_import(h+44u))return 0;
 game.shop_kind[0]=h[8];game.shop_kind[1]=h[9];game.shop[0]=h[10];game.shop[1]=h[11];raster_put_block(7u,SB_BASE+3u,&zero,1u);sb_counter(0u);return 1;
}
