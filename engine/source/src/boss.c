#include "paged_inventory.h"
#include "hand_api.h"
#pragma bank 255
#ifdef __SDCC
#pragma bank 255
#endif
#include "boss_extensions.h"
#include "shop.h"
#include "num_extra.h"
#ifdef BOSS_ENABLE_OPTIONAL
#include "boss_optional.h"
#endif
/* ID order is catalog.json order, NOT game.lua's `order` field. */
#define OX 2
#define MOUTH 4
#define CLUB 6
#define MANACLE 7
#define TOOTH 8
#define WALL 9
#define ARM 14
#define PSYCHIC 15
#define GOAD 16
#define WATER 17
#define EYE 18
#define PLANT 19
#define NEEDLE 20
#define HEAD 21
#define VESSEL 23
#define WINDOW 24
#define FLINT 27

static uint8_t has_joker(const GameState *g,uint8_t id) {
 uint8_t i;
 for(i=0;i<g->joker_count && i<PG_JMAX;i++)
  if(PG_JBYTE(g,i,jokers,0u)==id && !(PG_DEBUFF(g,i)))return 1;
 return 0;
}
static uint8_t face(const GameState *g,const Card *c) {
 return (c->enhancement!=ENH_STONE && c->rank>=11 && c->rank<=13)
  || has_joker(g,J_PAREIDOLIA);
}
static uint8_t suit(const GameState *g,const Card *c,uint8_t s) {
 if(c->enhancement==ENH_STONE)return 0;
 if(c->enhancement==ENH_WILD || c->suit==s)return 1;
 /* Native SDHC encoding: red suits 1/2, black suits 0/3. */
 return has_joker(g,J_SMEARED) &&
  ((c->suit==1 || c->suit==2)==(s==1 || s==2));
}
uint8_t boss_supported(uint8_t id) CORE_BANKED {
 switch(id) {
 case 0:case 1:case MOUTH:case CLUB:case WALL:case ARM:case PSYCHIC:
 case GOAD:case WATER:case EYE:case PLANT:case NEEDLE:case HEAD:
 case VESSEL:case WINDOW:case 25:case 12:return 1;
#ifdef BOSS_HAVE_OX_TARGET
 case OX:return 1;
#endif
#ifdef BOSS_HAVE_DRAW_HOOK
 case MANACLE:return 1;
#endif
 case TOOTH:return 1;
#ifdef BOSS_HAVE_BASE_HOOK
 case FLINT:return 1;
#endif
 default:
#ifdef BOSS_ENABLE_OPTIONAL
  return boss_optional_supported(id);
#else
  return 0;
#endif
 }
}
uint8_t boss_begin(GameState *g) CORE_BANKED {
 uint8_t i;
 GbNum target,factor;
 uint8_t scale;
 if(!boss_supported(g->boss_id))return BOSS_UNSUPPORTED;
 if(g->boss_id==MANACLE && g->hand_size==(-32767-1)){g->numeric_error=1;return BOSS_UNSUPPORTED;}
 /* Caller supplies fresh round resources and generic Boss target (ante*2).
    Check target overflow before changing any state. */
 scale=g->boss_id==WALL?2:g->boss_id==VESSEL?3:1;
 gb_num_from_u32(&factor,scale);
 if(gb_num_mul(&target,&g->target,&factor)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)){g->numeric_error=1;return BOSS_UNSUPPORTED;}
 if(g->boss_id==NEEDLE){
  gb_num_from_u32(&factor,2);
  if(gb_num_div(&target,&target,&factor)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)){g->numeric_error=1;return BOSS_UNSUPPORTED;}
 }
 g->boss_disabled=0;g->boss_first_hand=1;g->boss_flipped=0;
 g->boss_seen_hands=0;pg_flags_clear(g,0);
 g->forced_card_uid=0;

 g->target=target;
 switch(g->boss_id) {
 case WATER:g->boss_flipped=g->discards;g->discards=0;break;
 case NEEDLE:
  g->boss_flipped=g->hands>1?g->hands-1:0;
  if(g->hands>1)g->hands=1;
  break;
 case MANACLE:g->hand_size--;break;
 default:break;
 }
#ifdef BOSS_ENABLE_OPTIONAL
 boss_optional_begin(g);
#endif
 /* Recompute blind-only debuffs on every owned copy, preserving other flags. */
 for(i=0;i<g->deck_count && i<DECK_CAP;i++)boss_card(g,&g->deck[i]);
 for(i=0;i<g->hand_count && i<HAND_STORAGE_CAP;i++)boss_card(g,&g->hand[i]);
 return BOSS_ALLOWED;
}
void boss_card(GameState *g,Card *card) CORE_BANKED {
 uint8_t debuff=0;
 if(!boss_supported(g->boss_id))return;
 if(!g->boss_disabled) {
  switch(g->boss_id) {
  case CLUB:debuff=suit(g,card,3);break;
  case GOAD:debuff=suit(g,card,0);break;
  case HEAD:debuff=suit(g,card,2);break;
  case WINDOW:debuff=suit(g,card,1);break;
  case PLANT:debuff=face(g,card);break;
  default:
#ifdef BOSS_ENABLE_OPTIONAL
   debuff=boss_optional_card(g,card);
#endif
   break;
  }
 }
 card->flags=(card->flags & (uint8_t)~CARD_DEBUFF) | debuff;
}
void boss_set_ox_hand(GameState *g,uint8_t hand_type) CORE_BANKED {
 if(g->boss_id==OX)g->ox_target_hand=hand_type;
}
uint8_t boss_before_hand(GameState *g,JokerContext *c) CORE_BANKED {
 uint16_t bit;
 if(!boss_supported(g->boss_id))return BOSS_UNSUPPORTED;
 if(g->boss_disabled)return BOSS_ALLOWED;
 if(c->hand_type>=12)return BOSS_UNSUPPORTED;
 bit=(uint16_t)1u<<c->hand_type;
 switch(g->boss_id) {
 case OX:
  if(g->ox_target_hand>=12)return BOSS_UNSUPPORTED;
  if(g->ox_target_hand==c->hand_type)g->money=0;
  break;
 case ARM:if(g->hand_levels[c->hand_type]>1)g->hand_levels[c->hand_type]--;break;
 case MOUTH:
  if(g->boss_seen_hands && !(g->boss_seen_hands & bit))return BOSS_NO_SCORE;
  g->boss_seen_hands=bit;break;
 case EYE:
  if(g->boss_seen_hands & bit)return BOSS_NO_SCORE;
  g->boss_seen_hands|=bit;break;
 case PSYCHIC:if(c->played_count<5)return BOSS_NO_SCORE;break;
 case TOOTH:
  if(g->money<(-2147483647L-1)+(int32_t)c->played_count) {
   g->numeric_error=1;return BOSS_UNSUPPORTED;
  }
  g->money-=(int32_t)c->played_count;break;
 default:break;
 }
 return BOSS_ALLOWED;
}
void boss_modify_base(GameState *g,JokerContext *c) CORE_BANKED {
 GbNum half,chips,mult,one;
 if(g->boss_disabled || g->boss_id!=FLINT || !boss_supported(FLINT))return;
 {static const GbNum literal={{0,0,0,0,0,0,0xe0,0x3f}};half=literal;}gb_num_from_u32(&one,1);
 /* Source does separate binary64 multiply, add 0.5, floor, then clamps. */
 if(gb_num_mul(&chips,&c->chips,&half)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO) ||
    gb_num_add(&chips,&chips,&half)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO) || num_floor(&chips,&chips)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO) ||
    gb_num_mul(&mult,&c->mult,&half)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO) ||
    gb_num_add(&mult,&mult,&half)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO) || num_floor(&mult,&mult)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)) {
  g->numeric_error=1;return;
 }
 if(gb_num_cmp(&mult,&one)<0)mult=one;
 c->chips=chips;c->mult=mult;
}
void boss_after_hand(GameState *g,JokerContext *c) CORE_BANKED {
 (void)c;
 if(boss_supported(g->boss_id))g->boss_first_hand=0;
}
void boss_after_draw(GameState *g,uint8_t reason) CORE_BANKED {
 uint8_t i;
 (void)reason;
 if(!g->boss_disabled && g->boss_id==12 && g->hand_count){
  uint8_t found=0;for(i=0;i<g->hand_count;i++)if(g->hand[i].uid==g->forced_card_uid)found=1;
  if(!found){uint8_t pick=boss_core_random_index(g,g->hand_count,12);if(pick>=g->hand_count || g->numeric_error)return;hand_force_uid(g,g->hand[pick].uid);}
 }
#ifdef BOSS_ENABLE_OPTIONAL
 boss_optional_after_draw(g);
#endif
 for(i=0;i<g->hand_count && i<HAND_STORAGE_CAP;i++)boss_card(g,&g->hand[i]);
}
void boss_disable(GameState *g) CORE_BANKED {
 uint8_t i;
 GbNum divisor,target;
 if(g->boss_disabled || !boss_supported(g->boss_id))return;
 if(g->boss_id==MANACLE && g->hand_size==32767){g->numeric_error=1;return;}
 if((g->boss_id==WATER && (uint16_t)g->discards+g->boss_flipped>255) ||
    (g->boss_id==NEEDLE && (uint16_t)g->hands+g->boss_flipped>255)) {
  g->numeric_error=1;return;
 }
 if(g->boss_id==WALL || g->boss_id==VESSEL) {
  gb_num_from_u32(&divisor,g->boss_id==WALL?2:3);
  if(gb_num_div(&target,&g->target,&divisor)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO)){g->numeric_error=1;return;}
  g->target=target;
 }
 g->boss_disabled=1;
 switch(g->boss_id) {
 case WATER:g->discards+=g->boss_flipped;break;
 case NEEDLE:g->hands+=g->boss_flipped;break;
 case WALL:case VESSEL:break;
 case MANACLE:
  g->hand_size++;
#ifdef BOSS_HAVE_DRAW_HOOK
  boss_core_draw_one(g);
#endif
  break;
 default:break;
 }
#ifdef BOSS_ENABLE_OPTIONAL
 boss_optional_disable(g);
#endif
 g->boss_flipped=0;g->forced_card_uid=0;
 for(i=0;i<g->deck_count && i<DECK_CAP;i++)boss_card(g,&g->deck[i]);
 for(i=0;i<g->hand_count && i<HAND_STORAGE_CAP;i++)boss_card(g,&g->hand[i]);
}

#ifdef BOSS_ENABLE_OPTIONAL
uint8_t boss_card_is_face(GameState *g,const Card *c) CORE_BANKED {return face(g,c);}
#endif

void boss_defeat(GameState *g) CORE_BANKED {uint8_t i;if(g->boss_id==7&&!g->boss_disabled&&boss_supported(7)){if(g->hand_size==32767){g->numeric_error=1;return;}g->hand_size++;}boss_optional_defeat(g);g->boss_disabled=1;pg_flags_clear(g,0);pg_flags_clear(g,1);g->forced_card_uid=0;for(i=0;i<g->deck_count;i++)g->deck[i].flags&=~(CARD_DEBUFF|CARD_FACE_DOWN);for(i=0;i<g->hand_count;i++)g->hand[i].flags&=~(CARD_DEBUFF|CARD_FACE_DOWN);}

/* Acquisition preflight runs before inventory/RNG/source mutation. The deferred
 * disable below is nonfailing and does not draw RNG after this succeeds. */
uint8_t boss_chicot_preflight(const GameState *g) CORE_BANKED {
 GbNum divisor,target;uint8_t index;
 if(g->blind!=2 || g->boss_disabled)return 1;
 if(!boss_supported(g->boss_id)||g->deck_count>DECK_CAP||g->hand_count>HAND_STORAGE_CAP||g->deck_pos>g->deck_count)return 0;
 if(g->boss_id==WATER && (uint16_t)g->discards+g->boss_flipped>255)return 0;
 if(g->boss_id==NEEDLE && (uint16_t)g->hands+g->boss_flipped>255)return 0;
 if(g->boss_id==MANACLE){
  if(g->hand_size==32767)return 0;
  if(g->deck_pos<g->deck_count){index=g->draw_order[g->deck_pos];if(g->hand_count>=HAND_STORAGE_CAP||index>=g->deck_count)return 0;}
 }
 if(g->boss_id==WALL||g->boss_id==VESSEL){gb_num_from_u32(&divisor,g->boss_id==WALL?2:3);if(gb_num_div(&target,&g->target,&divisor)&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO))return 0;}
 return 1;
}
