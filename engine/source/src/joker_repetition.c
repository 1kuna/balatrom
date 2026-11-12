#include "paged_inventory.h"
#pragma bank 255
#include "joker_repetition.h"
#include <string.h>

/* Metadata is a local switch: no bank-local ROM pointer crosses a switch. */
uint8_t jc_compatible(uint8_t id) CORE_BANKED {
 if(id >= JOKER_COUNT) return 0;
/* OWNER_INSTALL_SLOT:blueprint_cases */ default: return 1;
 }
}

void jc_resolve(const GameState *g,uint8_t issuer,JokerCopy *o) CORE_BANKED {
 uint8_t current,next,id,depth=0;
 o->issuer_index=issuer; o->target_index=JR_NONE; o->copy_depth=0;
 o->status=JC_INVALID; o->immediate_target=JR_NONE;
 o->immediate_compatible=0;
 if(g->joker_count>PG_JMAX || issuer>=g->joker_count)return;
 current=issuer;
 id=PG_JBYTE(g,issuer,jokers,0u);
 if(id==J_BLUEPRINT || id==J_BRAINSTORM) {
  next=(id==J_BLUEPRINT)?(uint8_t)(issuer+1):0;
  if(next<g->joker_count && next!=issuer) {
   o->immediate_target=next;
   o->immediate_compatible=jc_compatible(PG_JBYTE(g,next,jokers,0u));
  }
 }
 for(;;) {
  id=PG_JBYTE(g,current,jokers,0u);
  if(id>=JOKER_COUNT){o->status=JC_INVALID;return;}
  if(PG_DEBUFF(g,current)){o->status=JC_DEBUFFED;return;}
  if(id!=J_BLUEPRINT && id!=J_BRAINSTORM) {
   o->target_index=current; o->status=JC_OK;return;
  }
  next=(id==J_BLUEPRINT)?(uint8_t)(current+1):0;
  if(next>=g->joker_count || next==current){o->status=JC_NO_TARGET;return;}
  depth++; o->copy_depth=depth;
  if(depth>g->joker_count+1){o->status=JC_DEPTH_LIMIT;return;}
  current=next;
 }
}

void jr_defaults(const GameState *g,JokerRepetitionContext *c) CORE_BANKED {
 uint8_t i;
 /* Same81 deterministic defaults, but avoid testing absent slots one by one.
  * Keep the bound even for invalid counts; jr_capture retains validation. */
 memset(c->extra,1,PG_JMAX);
 for(i=0;i<g->joker_count && i<PG_JMAX;i++)
  if(PG_JBYTE(g,i,jokers,0u)==J_HANGING_CHAD)c->extra[i]=2;
}

static void append(JokerRepetitionPacket *o,uint8_t kind,uint8_t issuer,
                   uint8_t target,uint8_t depth,uint8_t count) {
 JokerRepetitionRecipe record,*r=&record;uint8_t index;
 if(!count)return;
 if(o->recipe_count>=82u){o->valid=0;return;}
 index=o->recipe_count++;if(index<JR_RECIPE_CAP)r=&o->recipes[index];
 r->kind=kind;r->issuer_index=issuer;r->target_index=target;
 r->copy_depth=depth;r->repetitions=count;if(index>=JR_RECIPE_CAP)pg_recipe_put(index-JR_RECIPE_CAP,r);o->total_runs+=count;
}

void jr_capture(const GameState *g,const JokerRepetitionContext *c,
                JokerRepetitionPacket *o) CORE_BANKED {
 uint8_t i,count,id; JokerCopy copy;
 o->base_runs=0;o->recipe_count=0;o->total_runs=0;o->valid=0;
 /* Initialize unused recipes too: deterministic RAM packet, no stale fields. */
 for(i=0;i<JR_RECIPE_CAP;i++) {
  o->recipes[i].kind=JR_FROM_SEAL;o->recipes[i].issuer_index=JR_NONE;
  o->recipes[i].target_index=JR_NONE;o->recipes[i].copy_depth=0;
  o->recipes[i].repetitions=0;
 }
 if(g->joker_count>PG_JMAX || c->phase>JR_HELD_ROUND_END)return;
 o->valid=1;
 if(c->phase==JR_SCORED && c->card_debuffed)return;
 o->base_runs=1;o->total_runs=1;
 if(c->seal==SEAL_RED && !c->card_debuffed &&
    (c->phase==JR_SCORED || c->held_has_effect))
  append(o,JR_FROM_SEAL,JR_NONE,JR_NONE,0,1);
 for(i=0;i<g->joker_count;i++) {
  jc_resolve(g,i,&copy);
  if(copy.status==JC_INVALID){o->valid=0;return;}
  if(copy.status!=JC_OK)continue;
  id=PG_JBYTE(g,copy.target_index,jokers,0u);count=0;
  if(c->phase==JR_SCORED) {
   switch(id) {
   case J_HANGING_CHAD:
    if(c->first_scoring_card)count=c->extra[copy.target_index];break;
   case J_SOCK_AND_BUSKIN:
    if(c->is_face)count=c->extra[copy.target_index];break;
   case J_DUSK:
    if(c->hands_left==0)count=c->extra[copy.target_index];break;
   case J_SELZER: count=1;break;
   case J_HACK:
    if(c->rank>=2 && c->rank<=5)count=c->extra[copy.target_index];break;
   default: break;
   }
  } else if(id==J_MIME && c->held_has_effect)count=c->extra[copy.target_index];
  append(o,JR_FROM_JOKER,i,copy.target_index,copy.copy_depth,count);
 }
}
