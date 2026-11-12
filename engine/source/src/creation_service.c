#include "raster_runtime.h"
#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "native_run_rng.h"
#include "joker_history.h"
#ifdef __SDCC
#pragma bank 255
#endif
#include "creation_service.h"
#include "events.h"
#include "boss.h"
#include "joker_ids.h"
#include <string.h>
#include <stddef.h>
#include "catalog_tables.h"

static const uint8_t offsets[4]={0,19,21,24},counts[4]={150,12,22,18};
static const uint8_t planet_hand[12]={1,3,6,7,5,4,2,8,0,9,10,11};
static const uint8_t pure_acquire[43]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,21,22,30,32,33,38,39,40,42,52,61,88,100,103,107,117,118,130,131,132,133,134,139,140,80,85,109};
static const uint8_t edition_cost[5]={0,2,3,5,5};
static uint8_t key_valid(uint8_t kind,uint8_t id){return kind<4&&id<counts[kind];}
static uint8_t key_get(const uint8_t *b,uint8_t kind,uint8_t id){return key_valid(kind,id)&&(b[offsets[kind]+id/8]&(1u<<(id%8)));}
static void key_set(uint8_t *b,uint8_t kind,uint8_t id,uint8_t set){uint8_t *v;if(!key_valid(kind,id))return;v=&b[offsets[kind]+id/8];if(set)*v|=1u<<(id%8);else *v&=~(1u<<(id%8));}
void creation_policy_key(uint8_t *bits,uint8_t kind,uint8_t id,uint8_t set) CORE_BANKED {if(bits)key_set(bits,kind,id,set);}
void creation_policy_all_unlocked(CreationPolicy *p) CORE_BANKED {uint8_t id;if(p)for(id=0;id<150;id++)key_set(p->unlocked,ITEM_JOKER,id,1);}
/* same djb2-xor value h=((h<<5)+h)^b mod 2^32, as a tight leaf.
 * The C uint32 <<5 loop dominated Tarot/Spectral use (~100 frames hashing the
 * GameState twice). Inputs/outputs pass through statics (no ABI assumptions);
 * only WRAM bank1/fixed memory is read, exactly as the C version did.
 * Output matches the C version bit for bit. */
static uint16_t hb_ptr,hb_cnt;static uint32_t hb_h;
static void hb_raw(void) __naked {
 __asm
 ld hl,#_hb_ptr
 ld a,(hl+)
 ld h,(hl)
 ld l,a
hb_loop:
 push hl
 ld hl,#_hb_cnt
 ld a,(hl+)
 or a,(hl)
 jr z,hb_done
 dec hl
 ld a,(hl)
 sub a,#1
 ld (hl+),a
 ld a,(hl)
 sbc a,#0
 ld (hl),a
 ld hl,#_hb_h
 ld a,(hl+)
 ld e,a
 ld a,(hl+)
 ld d,a
 ld a,(hl+)
 ld c,a
 ld b,(hl)
 sla e
 rl d
 rl c
 rl b
 sla e
 rl d
 rl c
 rl b
 sla e
 rl d
 rl c
 rl b
 sla e
 rl d
 rl c
 rl b
 sla e
 rl d
 rl c
 rl b
 ld hl,#_hb_h
 ld a,e
 add a,(hl)
 ld e,a
 inc hl
 ld a,d
 adc a,(hl)
 ld d,a
 inc hl
 ld a,c
 adc a,(hl)
 ld c,a
 inc hl
 ld a,b
 adc a,(hl)
 ld b,a
 pop hl
 ld a,(hl+)
 xor a,e
 ld e,a
 push hl
 ld hl,#_hb_h
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld (hl),c
 inc hl
 ld (hl),b
 pop hl
 jr hb_loop
hb_done:
 pop hl
 ret
 __endasm;
}
static uint32_t hash_bytes(const void *data,uint16_t count){hb_ptr=(uint16_t)data;hb_cnt=count;hb_h=5381UL;hb_raw();return hb_h;}
static uint16_t step(uint16_t *seed,uint32_t *draws,uint8_t*nr){uint16_t x;if(nr){if(!native_rng_step_count(nr,0,&x,draws))return 0;return x;}x=*seed;x^=(uint16_t)(x<<7);x^=x>>9;x^=(uint16_t)(x<<8);*seed=x;(*draws)++;return x;}
/* Local mirror of core game_random_bounded; seed remains staged. */
static uint16_t bounded(uint16_t *seed,uint32_t *draws,uint16_t bound,uint8_t*nr){uint16_t roll,limit;if(!bound)return 0;if(nr){if(!native_rng_step_count(nr,bound,&roll,draws))return 0;return roll;}limit=65535u-(65535u%bound);do{roll=step(seed,draws,nr);}while(roll>limit);return (roll-1u)%bound;}
static uint8_t pure(uint8_t id){uint8_t i;for(i=0;i<43;i++)if(pure_acquire[i]==id)return 1;return 0;}

uint8_t creation_policy_defaults(const GameState *g,CreationPolicy *p) CORE_BANKED {
 uint8_t i,id;
 if(!g||!p||g->joker_count>PG_JMAX||g->consumable_count>PG_CMAX||g->pack_count>5)return 0;
 memset(p,0,sizeof(*p));p->ready=1;p->edition_rate=1;p->use_safe=1;p->inflation=g->price_inflation;p->discount_percent=g->price_discount_percent;
 for(i=0;i<150;i++)if(joker_default_unlocked[i])key_set(p->unlocked,ITEM_JOKER,i,1);
 for(i=1;i<4;i++)for(id=0;id<counts[i];id++)key_set(p->unlocked,i,id,1);
 for(i=0;i<43;i++)key_set(p->acquire_safe,ITEM_JOKER,pure_acquire[i],1);
 for(i=0;i<g->joker_count;i++){
  if(PG_JBYTE(g,i,jokers,0u)>=150)return 0;
  key_set(p->used,ITEM_JOKER,PG_JBYTE(g,i,jokers,0u),1);
  if(!pure(PG_JBYTE(g,i,jokers,0u)))p->use_safe=0;
 }
 for(i=0;i<g->consumable_count;i++){
  if(!key_valid(PG_CBYTE(g,i,consumable_kind,0u),PG_CBYTE(g,i,consumables,1u)))return 0;
  key_set(p->used,PG_CBYTE(g,i,consumable_kind,0u),PG_CBYTE(g,i,consumables,1u),1);
 }
 if(g->phase==PHASE_SHOP||g->phase==PHASE_PACK)for(i=0;i<2;i++)if(g->shop[i]!=255){
  if(!key_valid(g->shop_kind[i],g->shop[i]))return 0;
  key_set(p->used,g->shop_kind[i],g->shop[i],1);
 }
 if(g->phase==PHASE_PACK)for(i=0;i<g->pack_count;i++){
  if(!key_valid(g->pack_item_kind[i],g->pack_ids[i]))return 0;
  key_set(p->used,g->pack_item_kind[i],g->pack_ids[i],1);
 }
 return 1;
}
static uint8_t source_valid(const GameState *g,const CreationSource *s){
 if(s->mode==CREATION_NONE)return 1;
 if(!key_valid(s->kind,s->id)||s->kind==ITEM_JOKER)return 0;
 if(s->mode==CREATION_INVENTORY)return s->slot<g->consumable_count&&PG_CBYTE(g,s->slot,consumable_kind,0u)==s->kind&&PG_CBYTE(g,s->slot,consumables,1u)==s->id;
 return s->mode==CREATION_PACK&&g->phase==PHASE_PACK&&g->pack_choices&&s->slot<g->pack_count&&g->pack_item_kind[s->slot]==s->kind&&g->pack_ids[s->slot]==s->id;
}
static uint8_t showman(const GameState *g){uint8_t i;for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==120&&!(PG_DEBUFF(g,i)))return 1;return 0;}
static uint8_t eligible(const GameState *g,const CreationPolicy *p,const uint8_t *used,uint8_t sm,uint16_t enh,uint8_t kind,uint8_t id){
 if(key_get(p->banned,kind,id)||(key_get(used,kind,id)&&!sm))return 0;
 if(kind==ITEM_JOKER){
  if(!key_get(p->unlocked,kind,id)&&joker_rarity[id]!=4)return 0;
  if(joker_gate[id]&&!(enh&(1u<<joker_gate[id])))return 0;
  if((joker_flag[id]==1&&g->gros_michel_extinct)||(joker_flag[id]==2&&!g->gros_michel_extinct))return 0;
 }else{
  if(!key_get(p->unlocked,kind,id))return 0;
  if(kind==ITEM_PLANET&&id>=9&&!g->hands_played[planet_hand[id]])return 0;
  if(kind==ITEM_SPECTRAL&&id>=16)return 0;
 }
 return 1;
}
/* Pool slots retain UNAVAILABLE entries. Mixed Tarot/Planet ties use native
   Tarot-before-Planet order; original Lua ties have no specified stable order. */
static uint8_t pool(const GameState *g,const CreationPolicy *p,const uint8_t *used,uint8_t sm,uint16_t enh,uint8_t kind,uint8_t rarity,uint8_t *keys){
 uint8_t i,id,n=0,available=0,k;
 if(kind==CREATION_TAROT_PLANET){
  for(i=0;i<22;i++){
   id=order_tarot[i];keys[n++]=eligible(g,p,used,sm,enh,ITEM_TAROT,id)?id:255;
   if(i<12){id=order_planet[i];keys[n++]=eligible(g,p,used,sm,enh,ITEM_PLANET,id)?128+id:255;}
  }
 }else for(i=0;i<counts[kind];i++){
  id=kind==ITEM_JOKER?order_joker[i]:kind==ITEM_PLANET?order_planet[i]:kind==ITEM_TAROT?order_tarot[i]:order_spectral[i];
  if(kind==ITEM_JOKER&&joker_rarity[id]!=rarity)continue;
  keys[n++]=eligible(g,p,used,sm,enh,kind,id)?id:255;
 }
 for(k=0;k<n;k++)if(keys[k]!=255)available=1;
 if(!available){keys[0]=kind==ITEM_PLANET?8:kind==ITEM_SPECTRAL?2:kind==ITEM_JOKER?0:11;n=1;}
 return n;
}
static uint8_t select_one(const GameState *g,const CreationPolicy *p,const uint8_t *used,uint8_t sm,uint16_t enh,const CreationRequest *r,uint16_t *seed,uint32_t *draws,CreationItem *out,uint8_t*nr){
 uint16_t chosen=65535u,start,roll;
 uint8_t keys[150],rarity=1,n,key;
 uint32_t cost;
 if(r->kind>CREATION_TAROT_PLANET||r->stream>CS_STANDARD||r->rarity>4||(r->rarity&&r->rarity!=3&&r->rarity!=4))return 0;
 if(r->forced_kind!=255){
  if(!key_valid(r->forced_kind,r->forced_id))return 0;
  if((r->kind==ITEM_JOKER)!=(r->forced_kind==ITEM_JOKER))return 0;
  if(!key_get(p->banned,r->forced_kind,r->forced_id))chosen=(uint16_t)r->forced_kind*256+r->forced_id;
 }
 if(r->forced_kind==255&&r->soulable&&!key_get(p->banned,ITEM_SPECTRAL,16)){
  if((r->kind==ITEM_TAROT||r->kind==ITEM_SPECTRAL||r->kind==CREATION_TAROT_PLANET)&&(!key_get(used,ITEM_SPECTRAL,16)||sm))
   if((uint32_t)step(seed,draws,nr)*1000>65536UL*997)chosen=784;
  if((r->kind==ITEM_PLANET||r->kind==ITEM_SPECTRAL)&&(!key_get(used,ITEM_SPECTRAL,17)||sm))
   if((uint32_t)step(seed,draws,nr)*1000>65536UL*997)chosen=785;
  if(chosen!=65535u&&key_get(p->banned,(uint8_t)(chosen>>8),(uint8_t)chosen))chosen=65535u;
 }
 if(chosen==65535u){
  if(r->kind==ITEM_JOKER){
   if(r->rarity==CREATION_RARE)rarity=3;
   else {roll=step(seed,draws,nr);rarity=r->rarity==CREATION_LEGENDARY?4:(uint32_t)roll*100>65536UL*95?3:(uint32_t)roll*10>65536UL*7?2:1;}
  }
  n=pool(g,p,used,sm,enh,r->kind,rarity,keys);start=*seed;
  do {key=keys[bounded(seed,draws,n,nr)];if(key==255&&!nr&&*seed==start)return 0;}while(key==255);
  chosen=r->kind==CREATION_TAROT_PLANET?
   (key&128?256u+(key&127):512u+key):(uint16_t)r->kind*256+key;
 }
 memset(out,0,sizeof(*out));out->kind=(uint8_t)(chosen>>8);out->id=(uint8_t)chosen;
 if(out->kind==ITEM_JOKER){
  /* Original ordinary edition draw: negative .003, poly .006*rate,
     holo .02*rate, foil .04*rate, in that priority order. */
  roll=step(seed,draws,nr);
  out->edition=(uint32_t)roll*1000>65536UL*997?ED_NEGATIVE:
   (uint32_t)roll*1000>65536UL*(1000-6*p->edition_rate)?ED_POLY:
   (uint32_t)roll*1000>65536UL*(1000-20*p->edition_rate)?ED_HOLO:
   (uint32_t)roll*1000>65536UL*(1000-40*p->edition_rate)?ED_FOIL:ED_NONE;
  if(!key_get(p->acquire_safe,ITEM_JOKER,out->id))return 0;
  out->eternal=p->all_eternal&&joker_eternal[out->id];
  cost=(((uint32_t)joker_cost[out->id]+p->inflation+edition_cost[out->edition])*2+1)*(100-p->discount_percent)/200;
  if(cost<1)cost=1;cost/=2;if(cost<1)cost=1;out->sell=cost>255?255:(uint8_t)cost;
 }
 return 1;
}

uint8_t creation_prepare(const GameState *g,const CreationPolicy *p,const CreationSource *s,const CreationRequest *r,uint8_t count,CreationPlan *plan) CORE_BANKED {
 uint8_t i,j,used[CREATION_KEYS],sm,croom,jroom,jcreated=0,jnegative=0;
 uint16_t seed,enh=0;uint8_t native[20],*nr=0;
 uint32_t draws=0;
 if(!plan)return 0;memset(plan,0,sizeof(*plan));plan->error=CREATION_ERROR_INPUT;
 if(g!=&game||!p||!s||!r||!count||count>CREATION_MAX||!p->ready||!g->seed||
  (p->edition_rate!=1&&p->edition_rate!=2&&p->edition_rate!=4)||p->discount_percent>100||
  g->numeric_error||g->consumable_count>PG_CMAX||g->joker_count>PG_JMAX||g->deck_count>DECK_CAP||g->pack_count>5||!source_valid(g,s))return 0;
 if(s->mode!=CREATION_NONE&&(g->numeric_error||(g->phase!=PHASE_TABLE&&g->phase!=PHASE_SHOP&&g->phase!=PHASE_PACK)))return 0;
 if(s->mode!=CREATION_NONE&&!p->use_safe){plan->error=CREATION_ERROR_UNSUPPORTED;return 0;}
 i=g->consumable_count-(s->mode==CREATION_INVENTORY);
 if(!pg_fits((uint16_t)g->joker_count+p->joker_buffer,(uint16_t)i+p->consumable_buffer))return 0;
 {uint16_t limit=inventory_consumable_limit(g)-(s->mode==CREATION_INVENTORY&&PG_CBYTE(g,s->slot,consumable_edition,2u)==ED_NEGATIVE);croom=limit>i+p->consumable_buffer?limit-i-p->consumable_buffer:0;}if(croom>PG_CMAX-i-p->consumable_buffer)croom=PG_CMAX-i-p->consumable_buffer;jroom=PG_JMAX-g->joker_count-p->joker_buffer;
 memcpy(used,p->used,sizeof used);seed=g->seed;sm=showman(g);if(native_rng_mode()){native_rng_export(native);if(!native_rng_valid(native))return 0;memcpy(plan->native_before,native,20u);nr=native;}
 for(i=0;i<g->deck_count;i++)if(g->deck[i].enhancement<=ENH_LUCKY)enh|=1u<<g->deck[i].enhancement;
 for(i=0;i<count;i++){
  if(!r[i].count||r[i].count>CREATION_MAX)return 0;
  for(j=0;j<r[i].count;j++){
   if(r[i].kind==ITEM_JOKER ? !jroom : !croom)break;
   if(plan->count>=CREATION_MAX)return 0;
   if(!select_one(g,p,used,sm,enh,&r[i],&seed,&draws,&plan->items[plan->count],nr)){plan->error=CREATION_ERROR_UNSUPPORTED;return 0;}
   {CreationItem *item=&plan->items[plan->count];
    if(item->kind==ITEM_JOKER){if(!jroom||g->joker_count+jcreated+p->joker_buffer>=inventory_joker_limit(g)+jnegative+(item->edition==ED_NEGATIVE)||(uint32_t)g->next_joker_uid+jcreated>=65535u)return 0;jroom--;jcreated++;jnegative+=item->edition==ED_NEGATIVE;if(item->id==120)sm=1;}
    else {if(!croom)return 0;croom--;}
    key_set(used,item->kind,item->id,1);
   }
   plan->count++;
  }
 }
 if(!plan->count){plan->error=CREATION_ERROR_ROOM;return 0;}
 if(!pg_fits((uint16_t)g->joker_count+jcreated,(uint16_t)(g->consumable_count-(s->mode==CREATION_INVENTORY))+plan->count-jcreated))return 0;plan->game_preimage=pg_hash(hash_bytes(g,sizeof(*g)));plan->policy_preimage=hash_bytes(p,sizeof(*p));
 plan->seed_before=g->seed;plan->seed_after=seed;plan->draws=draws;plan->source=*s;
 plan->inventory_count=g->consumable_count;plan->pack_count=g->pack_count;plan->pack_choices=g->pack_choices;plan->joker_count=g->joker_count;
 plan->next_joker_uid=g->next_joker_uid;plan->money_before=g->money;
 memcpy(plan->inventory_kind,g->consumable_kind,sizeof plan->inventory_kind);memcpy(plan->inventory_id,g->consumables,sizeof plan->inventory_id);memcpy(plan->pack_ids,g->pack_ids,sizeof plan->pack_ids);memcpy(plan->pack_kinds,g->pack_item_kind,sizeof plan->pack_kinds);
 plan->set_money_zero=s->mode!=CREATION_NONE&&s->kind==ITEM_SPECTRAL&&s->id==5;
 if(nr)memcpy(plan->native_after,nr,20u);plan->error=0;plan->valid=1;plan->seal=hash_bytes(plan,offsetof(CreationPlan,seal));return 1;
}
typedef struct {
  GbNum state[JOKER_CAP],history_tarot,history_extra[JOKER_CAP];
  uint16_t history_start,history_inflation;uint8_t history_discount;
  uint16_t uid[JOKER_CAP],planet_mask,probability;
  uint8_t consumable_ed[CONSUMABLE_CAP];
  uint8_t ids[JOKER_CAP],edition[JOKER_CAP],eternal[JOKER_CAP],sell[JOKER_CAP];
  int16_t hand_size;
  uint8_t phase,focus,ui,cfocus,numeric,last_kind,last_id,debuff,hands_max,discards_max;
} CreationUndo;
typedef char creation_undo_scratch_fits[(sizeof(CreationUndo)<=512u)?1:-1];
/* Bank5D700..D8FF, disjoint full overflow undoD000..D6B1. This stack
 * image exists only while copying before callbacks or restoring afterward. */
static void creation_undo_store(const GameState*g){CreationUndo undo;uint16_t at;uint8_t n;const uint8_t*bytes=(const uint8_t*)&undo;
memcpy(undo.consumable_ed,g->consumable_edition,sizeof undo.consumable_ed);undo.history_tarot=g->tarot_uses;undo.history_start=g->starting_deck_size;undo.history_inflation=g->price_inflation;undo.history_discount=g->price_discount_percent;memcpy(undo.history_extra,g->joker_extra_value,sizeof undo.history_extra);
 memcpy(undo.state,g->joker_state,sizeof undo.state);memcpy(undo.uid,g->joker_uid,sizeof undo.uid);
 memcpy(undo.ids,g->jokers,sizeof undo.ids);memcpy(undo.edition,g->joker_edition,sizeof undo.edition);
 memcpy(undo.eternal,g->joker_eternal,sizeof undo.eternal);memcpy(undo.sell,g->joker_sell,sizeof undo.sell);
 undo.planet_mask=g->used_planet_mask;undo.probability=g->probability_normal;
 undo.phase=g->phase;undo.focus=g->focus;undo.ui=g->ui_mode;undo.cfocus=g->consumable_focus;undo.numeric=g->numeric_error;
 undo.last_kind=g->last_consumable_kind;undo.last_id=g->last_consumable_id;undo.debuff=g->joker_debuff;
 undo.hand_size=g->hand_size;undo.hands_max=g->hands_max;undo.discards_max=g->discards_max;
 for(at=0;at<sizeof(undo);at+=n){n=sizeof(undo)-at>16u?16u:sizeof(undo)-at;raster_put_block(5u,0xd700u+at,bytes+at,n);}}
static void creation_undo_restore(GameState*g){CreationUndo undo;uint16_t at;uint8_t n;uint8_t*bytes=(uint8_t*)&undo;
 for(at=0;at<sizeof(undo);at+=n){n=sizeof(undo)-at>16u?16u:sizeof(undo)-at;raster_get_block(5u,0xd700u+at,bytes+at,n);}
memcpy(g->consumable_edition,undo.consumable_ed,sizeof undo.consumable_ed);g->tarot_uses=undo.history_tarot;g->starting_deck_size=undo.history_start;g->price_inflation=undo.history_inflation;g->price_discount_percent=undo.history_discount;memcpy(g->joker_extra_value,undo.history_extra,sizeof undo.history_extra);
 memcpy(g->joker_state,undo.state,sizeof undo.state);memcpy(g->joker_uid,undo.uid,sizeof undo.uid);
 memcpy(g->jokers,undo.ids,sizeof undo.ids);memcpy(g->joker_edition,undo.edition,sizeof undo.edition);
 memcpy(g->joker_eternal,undo.eternal,sizeof undo.eternal);memcpy(g->joker_sell,undo.sell,sizeof undo.sell);
 g->used_planet_mask=undo.planet_mask;g->probability_normal=undo.probability;
 g->phase=undo.phase;g->focus=undo.focus;g->ui_mode=undo.ui;g->consumable_focus=undo.cfocus;g->numeric_error=undo.numeric;
 g->last_consumable_kind=undo.last_kind;g->last_consumable_id=undo.last_id;g->joker_debuff=undo.debuff;
 g->hand_size=undo.hand_size;g->hands_max=undo.hands_max;g->discards_max=undo.discards_max;
}
uint8_t creation_commit(GameState *g,const CreationPolicy *p,CreationPlan *plan) CORE_BANKED {
 uint8_t i,slot,flags=CREATION_OK;
 /* Compact undo for the declared callback footprint; no deck/hand shadow.
    Capabilities MUST reject callbacks writing outside these fields or doing
    external effects. Original supported using/acquire handlers fit this set. */
 uint8_t old_numeric;

 if(g!=&game||!p||!plan||!plan->valid)return 0;
 if(hash_bytes(plan,offsetof(CreationPlan,seal))!=plan->seal)return 0;

 if(pg_hash(hash_bytes(g,sizeof(*g)))!=plan->game_preimage)return 0;
 if(hash_bytes(p,sizeof(*p))!=plan->policy_preimage)return 0;
 {uint8_t live[20];native_rng_export(live);if(live[0]!=plan->native_before[0]||(live[0]&&memcmp(live,plan->native_before,20u)))return 0;}
 if(
  g->seed!=plan->seed_before||g->money!=plan->money_before||g->next_joker_uid!=plan->next_joker_uid||g->joker_count!=plan->joker_count||
  g->consumable_count!=plan->inventory_count||g->pack_count!=plan->pack_count||g->pack_choices!=plan->pack_choices||
  memcmp(g->consumable_kind,plan->inventory_kind,sizeof plan->inventory_kind)||memcmp(g->consumables,plan->inventory_id,sizeof plan->inventory_id)||memcmp(g->pack_ids,plan->pack_ids,sizeof plan->pack_ids)||memcmp(g->pack_item_kind,plan->pack_kinds,sizeof plan->pack_kinds)||!source_valid(g,&plan->source))return 0;
 for(i=0;i<plan->count;i++)if(plan->items[i].kind==ITEM_JOKER&&plan->items[i].id==J_CHICOT&&!boss_chicot_preflight(g))return 0;
 pg_undo(0);creation_undo_store(g);old_numeric=g->numeric_error;
 if(p->inflation!=g->price_inflation||p->discount_percent!=g->price_discount_percent)goto rollback;
 slot=plan->source.slot;
 if(plan->source.mode==CREATION_INVENTORY){
  for(i=slot;i+1<g->consumable_count;i++){PG_CBYTE(g,i,consumable_kind,0u)=PG_CBYTE(g,i+1,consumable_kind,0u);PG_CBYTE(g,i,consumables,1u)=PG_CBYTE(g,i+1,consumables,1u);PG_CBYTE(g,i,consumable_edition,2u)=PG_CBYTE(g,i+1,consumable_edition,2u);}
  g->consumable_count--;pg_resize(g->joker_count,g->consumable_count);g->consumable_focus=0;flags|=CREATION_SOURCE_CONSUMED;
 }else if(plan->source.mode==CREATION_PACK){
  for(i=slot;i+1<g->pack_count;i++){g->pack_ids[i]=g->pack_ids[i+1];g->pack_item_kind[i]=g->pack_item_kind[i+1];}g->pack_count--;g->pack_choices--;g->focus=0;
  if(!g->pack_choices)g->phase=PHASE_SHOP;flags|=CREATION_SOURCE_CONSUMED;
 }
 g->seed=plan->seed_after;if(plan->native_before[0]&&!native_rng_import(plan->native_after))goto rollback;
 if(plan->source.mode!=CREATION_NONE){game_consumable_used(plan->source.kind,plan->source.id);flags|=CREATION_USE_DISPATCHED;}
 if(g->numeric_error!=old_numeric||g->seed!=plan->seed_after)goto rollback;
 if(plan->native_before[0]){uint8_t live[20];native_rng_export(live);if(memcmp(live,plan->native_after,20u))goto rollback;}
 for(i=0;i<plan->count;i++){
  CreationItem *item=&plan->items[i];
  if(item->kind==ITEM_JOKER){slot=g->joker_count;if(!game_add_joker(item->id,item->edition,item->eternal))goto rollback;PG_JBYTE(g,slot,joker_sell,11u)=item->sell;}
  else if(!game_generate_consumable(item->kind,item->id))goto rollback;
  if(g->numeric_error!=old_numeric||g->seed!=plan->seed_after)goto rollback;
 if(plan->native_before[0]){uint8_t live[20];native_rng_export(live);if(memcmp(live,plan->native_after,20u))goto rollback;}
 }
 if(plan->set_money_zero)g->money=0;
 if(!g->consumable_count)g->ui_mode=0;
 /* No rollback-producing operation follows. Chicot acquisition was deferred
  * by creation_commit_active; its entire boss footprint is applied only now. */
 for(i=0;i<plan->count;i++)if(plan->items[i].kind==ITEM_JOKER&&plan->items[i].id==J_CHICOT&&g->blind==2&&!g->boss_disabled)boss_disable(g);
 plan->valid=0;return flags;
rollback:
 pg_undo(1); if(plan->native_before[0])native_rng_import(plan->native_before);
 g->seed=plan->seed_before;g->money=plan->money_before;g->next_joker_uid=plan->next_joker_uid;g->joker_count=plan->joker_count;
 g->consumable_count=plan->inventory_count;g->pack_count=plan->pack_count;g->pack_choices=plan->pack_choices;
 memcpy(g->consumable_kind,plan->inventory_kind,sizeof plan->inventory_kind);memcpy(g->consumables,plan->inventory_id,sizeof plan->inventory_id);memcpy(g->pack_ids,plan->pack_ids,sizeof plan->pack_ids);memcpy(g->pack_item_kind,plan->pack_kinds,sizeof plan->pack_kinds);
 creation_undo_restore(g);
 return 0;
}

/* Offer generation does not reserve inventory capacity or consume a source.
 Caller owns committing the staged seed and generated used-key snapshot. */
uint8_t creation_preview_offer(const GameState *g,CreationPolicy *p,const CreationRequest *request,CreationItem *item,uint16_t *seed) CORE_BANKED {
 uint8_t i,native[20],*nr=0;uint16_t enh=0,staged;uint32_t draws=0;
 if(g!=&game||!p||!request||!item||!seed||!p->ready||!*seed||request->kind==ITEM_JOKER||g->deck_count>DECK_CAP)return 0;
 for(i=0;i<g->deck_count;i++)if(g->deck[i].enhancement<=ENH_LUCKY)enh|=1u<<g->deck[i].enhancement;
 if(native_rng_mode()){native_rng_export(native);if(!native_rng_valid(native))return 0;nr=native;}
 staged=*seed;if(!select_one(g,p,p->used,showman(g),enh,request,&staged,&draws,item,nr))return 0;
 if(nr&&!native_rng_import(nr))return 0;
 key_set(p->used,item->kind,item->id,1);*seed=staged;return 1;
}
