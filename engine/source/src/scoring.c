#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "hand_api.h"
#pragma bank 255
#include "scoring.h"
#include "shop.h"
#include "events.h"
#include "consumables.h"
#include "num_extra.h"
#include "boss_extensions.h"
#include "joker_bridge.h"
#include "joker_repetition.h"
#include "joker_missing.h"
#include <string.h>
uint8_t joker_has(uint8_t id) CORE_BANKED {uint8_t i;for(i=0;i<game.joker_count;i++)if(PG_JBYTE(&game,i,jokers,0u)==id&&!(PG_DEBUFF(&game,i)))return 1;return 0;}
static uint8_t good(uint8_t status){return !(status&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO));}
static uint8_t add(GbNum *v,int16_t delta){if(!delta)return 1;return good(gb_num_add_i16(v,v,delta));}
static uint8_t multiply(GbNum *v,uint16_t num,uint16_t den){GbNum factor;static const GbNum half={{0,0,0,0,0,0,0xf8,0x3f}};
 if(num==den)return 1;if(num==3&&den==2){factor=half;return good(gb_num_mul(v,v,&factor));}if(den==1){gb_num_from_u32(&factor,num);return good(gb_num_mul(v,v,&factor));}return good(gb_num_mul_ratio(v,v,num,den));}
static uint8_t apply(JokerContext *c,JokerEffect *o){if(!o->implemented)return 1;
 if(!add(&c->chips,o->chips_add)||!add(&c->mult,o->mult_add))return 0;
 if((o->number_flags&1)&&!good(gb_num_add(&c->chips,&c->chips,&o->chips_number)))return 0;
 if((o->number_flags&2)&&!good(gb_num_add(&c->mult,&c->mult,&o->mult_number)))return 0;
 if(o->number_flags&4){if(!good(gb_num_mul(&c->mult,&c->mult,&o->xmult_number)))return 0;}else if(!multiply(&c->mult,o->xmult_num,o->xmult_den))return 0;
 if(o->money_add&&!game_adjust_money(o->money_add))return 0;return 1;}
static uint8_t resolve_effect(uint8_t issuer,JokerContext *c,JokerEffect *out){JokerCopy copy;jc_resolve(&game,issuer,&copy);memset(out,0,sizeof(*out));out->xmult_num=out->xmult_den=1;if(copy.status==JC_INVALID)return 0;if(copy.status!=JC_OK)return 1;c->joker_index=copy.target_index;c->copy_depth=copy.copy_depth;c->issuer_index=issuer;if(c->event==JE_SHOP_END&&PG_JBYTE(&game,copy.target_index,jokers,0u)==J_PERKEO)return inventory_perkeo(&game);joker_bridge(&game,PG_JBYTE(&game,copy.target_index,jokers,0u),c,out,copy.copy_depth!=0);return !game.numeric_error;}
static uint8_t dispatch(JokerContext*c){uint8_t j,k,count=0,depth;uint16_t uid;JokerEffect out;depth=pg_destroy_enter();if(depth==255u)return 0;for(j=0;j<game.joker_count;j++){if(!resolve_effect(j,c,&out)||!apply(c,&out)){pg_destroy_leave();return 0;}if(out.destroy_self){pg_destroy_put(depth,count++,PG_JUID(&game,j));}}for(k=0;k<count;k++){uid=pg_destroy_get(depth,k);for(j=0;j<game.joker_count;j++)if(PG_JUID(&game,j)==uid){game_remove_joker(j);break;}}pg_destroy_leave();return 1;}
static uint8_t dispatch_main(JokerContext *c){uint8_t j,k;JokerEffect effect;for(j=0;j<game.joker_count;j++){
 if(!(PG_DEBUFF(&game,j))){if(PG_JBYTE(&game,j,joker_edition,9u)==ED_FOIL&&!add(&c->chips,50))return 0;if(PG_JBYTE(&game,j,joker_edition,9u)==ED_HOLO&&!add(&c->mult,10))return 0;}
 c->event=JE_MAIN;if(!resolve_effect(j,c,&effect)||!apply(c,&effect))return 0;
 c->event=JE_OTHER_JOKER;c->trigger_index=j;c->trigger_id=PG_JBYTE(&game,j,jokers,0u);for(k=0;k<game.joker_count;k++)if(!resolve_effect(k,c,&effect)||!apply(c,&effect))return 0;
 if(!(PG_DEBUFF(&game,j))&&PG_JBYTE(&game,j,joker_edition,9u)==ED_POLY&&!multiply(&c->mult,3,2))return 0;
 }return 1;}
static uint8_t destroy_glass(const JokerContext *c){Card removed[HAND_PLAY_CAP];uint8_t count=0,i;
 for(i=0;i<game.hand_count;i++)if(hand_scoring(c,i)&&game.hand[i].enhancement==ENH_GLASS&&!(game.hand[i].flags&CARD_DEBUFF)&&game_probability_hit(4)){removed[count]=game.hand[i];removed[count++].flags|=8;}
 if(!count)return 1;if(!game_cards_event_preflight(count,0))return 0;game_sync_hand();for(i=0;i<count;i++)if(!game_remove_uid(removed[i].uid))return 0;
 /* UID selection removal preserves every unshattered played card. */
 game_cards_removed(removed,count);return !game.numeric_error;}
static uint8_t card_face(const Card *card){return (card->enhancement!=ENH_STONE&&card->rank>=11&&card->rank<=13)||joker_has(J_PAREIDOLIA);}
static uint8_t capture_repetitions(JokerContext *c,uint8_t held,uint8_t effect_table,JokerRepetitionPacket *packet){JokerRepetitionContext query;uint8_t i;memset(&query,0,sizeof(query));query.phase=held?JR_HELD:JR_SCORED;query.card_debuffed=c->debuffed;query.seal=c->seal;query.is_face=card_face(&game.hand[c->card_index]);query.rank=c->rank;query.hands_left=game.hands-1;query.held_has_effect=effect_table;
 query.first_scoring_card=0;for(i=0;i<game.hand_count;i++)if(hand_scoring(c,i)){query.first_scoring_card=i==c->card_index;break;}jr_defaults(&game,&query);jr_capture(&game,&query,packet);return packet->valid;}
void joker_context_event(JokerContext *context) CORE_BANKED {if(!dispatch(context))game.numeric_error=1;}
void joker_event(uint8_t event) CORE_BANKED {JokerContext c;memset(&c,0,sizeof(c));c.event=event;gb_num_from_u32(&c.chips,1);gb_num_from_u32(&c.mult,1);if(!dispatch(&c))game.numeric_error=1;}
uint8_t joker_discard_card_event(JokerContext *c) CORE_BANKED {uint8_t j,removed=0;JokerEffect out;for(j=0;j<game.joker_count;j++){if(!resolve_effect(j,c,&out)||!apply(c,&out)){game.numeric_error=1;return 0;}if(out.remove_card)removed=1;if(out.destroy_self){game_remove_joker(j);j--;}}return removed;}
uint8_t score_hand(void) CORE_BANKED {
 JokerHandShapes shapes;JokerHandPolicy policy;JokerRepetitionPacket packet;JokerEffect held_effect;uint8_t held_count,has_table,j;uint16_t rep,total_reps;uint8_t counts[15]={0},suits[4]={0},i,r,s,n=0,hi=0,run=0,five=0,scored=0,mask=0,flush_suit=255;JokerContext c;
 static const uint8_t bc[12]={/* OWNER_INSTALL_SLOT:bc */};
 static const uint8_t bm[12]={/* OWNER_INSTALL_SLOT:bm */};
 static const uint8_t lc[12]={/* OWNER_INSTALL_SLOT:lc */};
 static const uint8_t lm[12]={/* OWNER_INSTALL_SLOT:lm */};
 memset(&c,0,sizeof(c));c.played_count=hand_selected_play_order(&game,c.played_index);c.first_hand=1;for(i=0;i<12;i++)if(game.hands_this_round[i])c.first_hand=0;c.last_hand=game.hands==1;
 for(i=0;i<game.hand_count;i++)if(hand_is_selected(&game,i)){n++;if(game.hand[i].enhancement==ENH_STONE)continue;r=game.hand[i].rank;counts[r]++;if(r>hi)hi=r;
 for(s=0;s<4;s++)if(game.hand[i].suit==s||game.hand[i].enhancement==ENH_WILD)suits[s]++;}
 if(!n)return 0;c.played_count=n;
 for(i=2;i<15;i++){if(counts[i]==2)c.pairs++;if(counts[i]==3)c.trips++;if(counts[i]>=4)c.quads++;if(counts[i]==5)five=1;if(counts[i])run++;else run=0;if(run>=5)c.straight=1;}
 if(counts[14]&&counts[2]&&counts[3]&&counts[4]&&counts[5])c.straight=1;
 for(i=0;i<4;i++)if(suits[i]==5){c.flush=1;flush_suit=i;}
 joker_hand_policy(&game,&policy);joker_hand_shapes(&game,&c,&shapes);if(!shapes.implemented)goto blocked;c.straight=shapes.straight_mask!=0;c.flush=shapes.flush_mask!=0;
 if(five&&c.flush)c.hand_type=11;else if(c.trips&&c.pairs&&c.flush)c.hand_type=10;else if(five)c.hand_type=9;else if(c.straight&&c.flush)c.hand_type=8;else if(c.quads)c.hand_type=7;else if(c.trips&&c.pairs)c.hand_type=6;else if(c.flush)c.hand_type=5;else if(c.straight)c.hand_type=4;else if(c.trips)c.hand_type=3;else if(c.pairs>=2)c.hand_type=2;else if(c.pairs)c.hand_type=1;
 for(i=0;i<game.hand_count;i++)if(hand_is_selected(&game,i)){r=game.hand[i].rank;if(game.hand[i].enhancement==ENH_STONE||(c.hand_type==0?r==hi:c.hand_type==1||c.hand_type==2?counts[r]==2:c.hand_type==3?counts[r]==3:c.hand_type==7?counts[r]>=4:1)){
 if(c.hand_type==0&&game.hand[i].enhancement!=ENH_STONE){if(scored)continue;scored=1;}mask|=hand_score_bit(&c,i);c.scoring_count++;}}
 if(c.hand_type==4)mask=shapes.straight_mask;else if(c.hand_type==5)mask=shapes.flush_mask;else if(c.hand_type==8)mask=shapes.straight_mask|shapes.flush_mask;
 for(i=0;i<game.hand_count;i++)if((hand_is_selected(&game,i))&&game.hand[i].enhancement==ENH_STONE)mask|=hand_score_bit(&c,i);
 if(policy.score_all)mask=(1u<<c.played_count)-1;c.scoring_count=0;for(i=0;i<game.hand_count;i++)if(mask&hand_score_bit(&c,i))c.scoring_count++;
 if(game.hands_played[c.hand_type]==65535u||game.hands_this_round[c.hand_type]==65535u)goto blocked;
 game.hands_played[c.hand_type]++;game.hands_this_round[c.hand_type]++;
 {uint8_t allowed=boss_before_hand(&game,&c);if(allowed==BOSS_UNSUPPORTED)goto blocked;if(allowed==BOSS_NO_SCORE){game.last_hand=c.hand_type;gb_num_from_u32(&game.last_score,0);gb_num_from_u32(&game.last_chips,0);gb_num_from_u32(&game.last_mult,0);boss_after_hand(&game,&c);return 1;}}

 c.scoring_mask=mask;gb_num_from_u32(&c.chips,bc[c.hand_type]+(uint32_t)(game.hand_levels[c.hand_type]-1)*lc[c.hand_type]);gb_num_from_u32(&c.mult,bm[c.hand_type]+(uint32_t)(game.hand_levels[c.hand_type]-1)*lm[c.hand_type]);
 c.event=JE_BEFORE_HAND;if(!dispatch(&c))goto blocked;gb_num_from_u32(&c.chips,bc[c.hand_type]+(uint32_t)(game.hand_levels[c.hand_type]-1)*lc[c.hand_type]);gb_num_from_u32(&c.mult,bm[c.hand_type]+(uint32_t)(game.hand_levels[c.hand_type]-1)*lm[c.hand_type]);boss_modify_base(&game,&c);if(game.numeric_error)goto blocked;
 for(i=0;i<game.hand_count;i++)if(mask&hand_score_bit(&c,i)){
 Card *card=&game.hand[i];c.card_index=i;c.rank=card->enhancement==ENH_STONE?0:card->rank;c.suit=card->enhancement==ENH_STONE?255:card->suit;c.enhancement=card->enhancement;c.seal=card->seal;c.edition=card->edition;c.debuffed=card->flags&1;
 if(c.debuffed)continue;if(!capture_repetitions(&c,0,0,&packet))goto blocked;total_reps=packet.total_runs;
 for(rep=0;rep<total_reps;rep++){c.retrigger=rep!=0;
 if(!add(&c.chips,card->enhancement==ENH_STONE?50:card->rank==14?11:card->rank>10?10:card->rank)||!good(gb_num_add(&c.chips,&c.chips,&card->permanent_bonus)))goto blocked;
 c.lucky_trigger=0;if(card->enhancement==ENH_LUCKY){if(game_probability_hit(5)){if(!add(&c.mult,20))goto blocked;c.lucky_trigger=1;}if(game_probability_hit(15)){if(!game_adjust_money(20))goto blocked;c.lucky_trigger=1;}}
 if(card->seal==SEAL_GOLD&&!game_adjust_money(3))goto blocked;
 if(card->enhancement==ENH_BONUS&&!add(&c.chips,30))goto blocked;if(card->enhancement==ENH_MULT&&!add(&c.mult,4))goto blocked;
 if(card->enhancement==ENH_GLASS&&!multiply(&c.mult,2,1))goto blocked;
 if(card->edition==ED_FOIL&&!add(&c.chips,50))goto blocked;if(card->edition==ED_HOLO&&!add(&c.mult,10))goto blocked;
 if(card->edition==ED_POLY&&!multiply(&c.mult,3,2))goto blocked;
 c.event=JE_SCORED_CARD;if(!dispatch(&c))goto blocked;
 }
 }
 for(i=0;i<game.hand_count;i++)if(!(hand_is_selected(&game,i))){
 Card *card=&game.hand[i];c.card_index=i;c.rank=card->enhancement==ENH_STONE?0:card->rank;c.suit=card->enhancement==ENH_STONE?255:card->suit;c.enhancement=card->enhancement;c.seal=card->seal;c.edition=card->edition;c.debuffed=card->flags&CARD_DEBUFF;c.retrigger=0;c.event=JE_HELD_CARD;
 has_table=!c.debuffed&&card->enhancement==ENH_STEEL;held_count=game.joker_count;
 for(j=0;j<held_count;j++){if(!resolve_effect(j,&c,&held_effect))goto blocked;pg_effect_put(j,&held_effect);if(held_effect.table_present)has_table=1;}
 if(!capture_repetitions(&c,1,has_table,&packet))goto blocked;total_reps=packet.total_runs;
 for(rep=0;rep<total_reps;rep++){c.retrigger=rep!=0;if(!c.debuffed&&card->enhancement==ENH_STEEL&&!multiply(&c.mult,3,2))goto blocked;
 if(rep==0){for(j=0;j<held_count;j++){pg_effect_get(j,&held_effect);if(!apply(&c,&held_effect))goto blocked;}}else{c.event=JE_HELD_CARD;if(!dispatch(&c))goto blocked;}}
 }
 c.event=JE_MAIN;if(!dispatch_main(&c))goto blocked;

 game.last_hand=c.hand_type;game.last_chips=c.chips;game.last_mult=c.mult;if(!good(gb_num_mul(&game.last_score,&c.chips,&c.mult))||!good(num_floor(&game.last_score,&game.last_score)))goto blocked;
 if(!good(gb_num_add(&game.score,&game.score,&game.last_score)))goto blocked;
 if(!destroy_glass(&c))goto blocked;
 c.event=JE_AFTER_HAND;if(!dispatch(&c))goto blocked;boss_after_hand(&game,&c);return 1;
 blocked:game.numeric_error=1;game.phase=PHASE_BLOCKED;return 0;
}
