#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "joker_history.h"
#pragma bank 255
#include "game.h"
#include "native_run_rng.h"
#include "hand_api.h"
#include "boss_optional.h"
#include "joker_ids.h"
#include "shop.h"
#include "consumables.h"
#include "scoring.h"
#include "boss_extensions.h"
#include "events.h"
#include "round_end.h"
#include "creation_core.h"
#include <gb/gb.h>
#include <string.h>
GameState game;
uint16_t new_run_seed=12345;
uint8_t game_action_error;

static uint8_t count_bits(uint8_t v){uint8_t n=0;while(v){n+=v&1;v>>=1;}return n;}
uint16_t game_random(void) CORE_BANKED {uint16_t rng;if(native_rng_mode()){if(!native_rng_roll(0,&rng)){game.numeric_error=1;return 0;}return rng;}rng=game.seed;rng^=rng<<7;rng^=rng>>9;rng^=rng<<8;game.seed=rng;return rng;}
uint16_t game_random_index(uint16_t bound) CORE_BANKED {uint16_t v;if(!bound){game.numeric_error=1;return 0;}if(!native_rng_mode())return game_random()%bound;if(!native_rng_roll(bound,&v)){game.numeric_error=1;return 0;}return v;}
uint16_t game_random_bounded(uint16_t bound) CORE_BANKED {uint16_t roll,limit;if(native_rng_mode()){if(!bound||!native_rng_roll(bound,&roll)){game.numeric_error=1;return 0;}return roll;}if(!bound||!game.seed){game.numeric_error=1;return 0;}limit=65535u-(65535u%bound);do{roll=game_random();}while(roll>limit);return (roll-1u)%bound;}
uint8_t game_probability_hit(uint16_t denominator) CORE_BANKED {uint16_t sample=game_random_bounded(denominator);return !game.numeric_error&&sample<game.probability_normal;}
static void deal(void){hand_refill(&game,game.boss_id==25&&!game.boss_disabled&&game.boss_flipped);}
uint8_t game_draw_to_hand(uint8_t count) CORE_BANKED {return hand_draw(&game,game.boss_id==25&&!game.boss_disabled&&game.boss_flipped?3:count,1);}
void game_refill_hand(void) CORE_BANKED {deal();}
void game_sync_hand(void) CORE_BANKED {uint8_t i,j;for(i=0;i<game.hand_count;i++)for(j=0;j<game.deck_count;j++)if(game.hand[i].uid==game.deck[j].uid){game.deck[j]=game.hand[i];break;}}
uint8_t game_add_card(const Card *source) CORE_BANKED {uint8_t i;uint16_t uid=0;if(game.deck_count>=DECK_CAP)return 0;for(i=0;i<game.deck_count;i++)if(game.deck[i].uid>uid)uid=game.deck[i].uid;if(game.next_card_uid<uid)game.next_card_uid=uid;if(game.next_card_uid==65535)return 0;game.deck[game.deck_count]=*source;game.deck[game.deck_count].uid=++game.next_card_uid;game.draw_order[game.deck_count]=game.deck_count;game.deck_count++;return 1;}
uint8_t game_remove_uid(uint16_t uid) CORE_BANKED {uint8_t i,j,index=255,n=0;for(i=0;i<game.deck_count;i++)if(game.deck[i].uid==uid){index=i;break;}if(index==255||game.deck_count<=1)return 0;
 for(i=0;i<game.deck_count;i++)if(game.draw_order[i]==index){for(j=i;j+1<game.deck_count;j++)game.draw_order[j]=game.draw_order[j+1];if(i<game.deck_pos&&game.deck_pos)game.deck_pos--;break;}
 for(i=index;i+1<game.deck_count;i++)game.deck[i]=game.deck[i+1];game.deck_count--;for(i=0;i<game.deck_count;i++)if(game.draw_order[i]>index)game.draw_order[i]--;
 hand_remove_uid(&game,uid);return 1;}
static void remove_selected(uint8_t reason){game_sync_hand();hand_commit_selected(&game);if(game.boss_id==12)game.forced_card_uid=0;if(reason){deal();boss_after_draw(&game,reason);}}
static uint8_t choose_boss(void){
 static const uint8_t minante[30]={0,0,6,1,2,2,1,1,3,2,2,2,0,2,2,1,1,2,3,4,2,1,0,0,1,5,1,2,0,0};
 uint8_t ids[30],n=0,i,minimum=255,showdown;
 for(i=2;i<30;i++){showdown=(i==12||i==22||i==23||i==28||i==29);if(boss_supported(i)&&((game.ante==8&&showdown)||(game.ante!=8&&!showdown&&minante[i]<=game.ante))&&game.boss_uses[i]<minimum)minimum=game.boss_uses[i];}
 for(i=2;i<30;i++){showdown=(i==12||i==22||i==23||i==28||i==29);if(boss_supported(i)&&((game.ante==8&&showdown)||(game.ante!=8&&!showdown&&minante[i]<=game.ante))&&game.boss_uses[i]==minimum)ids[n++]=i;}
 if(!n){game.numeric_error=1;return 255;}i=ids[game_random_index(n)];game.boss_uses[i]++;return i;
}
static void begin_blind(void){uint8_t i,j,tmp;

 game.boss_id=game.blind<2?game.blind:choose_boss();static const uint16_t bases[8]={300,800,2000,5000,11000,20000,35000,50000};
 gb_num_from_u32(&game.target,(uint32_t)bases[game.ante-1]*(game.blind+2)/2);gb_num_from_u32(&game.score,0);game.hands=game.hands_max;game.discards=game.discards_max;
 game.deck_pos=game.hand_count=game.focus=0;hand_clear(&game);game.ui_mode=0;game.discards_used=0;memset(game.hands_this_round,0,sizeof(game.hands_this_round));
 for(i=0;i<game.deck_count;i++){game.draw_order[i]=i;game.deck[i].flags&=~(CARD_DEBUFF|CARD_FACE_DOWN);if(game.blind==0)game.deck[i].flags&=~CARD_PLAYED_ANTE;}
 for(i=game.deck_count-1;i>0;i--){j=game_random_index((i+1));tmp=game.draw_order[i];game.draw_order[i]=game.draw_order[j];game.draw_order[j]=tmp;}
 if(boss_begin(&game)==BOSS_UNSUPPORTED){game.phase=PHASE_BLOCKED;return;}boss_set_ox_hand(&game,game.ox_target_hand);deal();if(game.phase==PHASE_LOSS)return;boss_after_draw(&game,0);game.phase=PHASE_TABLE;joker_event(JE_BLIND_BEGIN);
}
void game_new(uint16_t seed) CORE_BANKED {uint8_t i;memset(&game,0,sizeof(game));pg_reset();seed=seed?seed:12345;game.seed=seed;if(!native_rng_begin(seed)){game.numeric_error=1;game.phase=PHASE_BLOCKED;return;}game.money=4;game.ante=1;game.deck_count=52;game.next_card_uid=52;game.hand_size=8;game.hands_max=4;game.discards_max=3;game.probability_normal=1;game.last_consumable_id=255;
 for(i=0;i<52;i++){game.deck[i].rank=2+i%13;game.deck[i].suit=i/13;game.deck[i].uid=i+1;}
 for(i=0;i<12;i++)game.hand_levels[i]=1;if(!joker_history_reset(&game)){game.numeric_error=1;game.phase=PHASE_BLOCKED;return;}begin_blind();}
static void play(void){if(!game.highlighted_count||!boss_play_allowed(&game))return;
 boss_mark_played(&game);boss_stage_play(&game);if(game.numeric_error){game.phase=PHASE_BLOCKED;return;}
 if(!score_hand())return;game.hands--;remove_selected(gb_num_cmp(&game.score,&game.target)<0&&game.hands>0);
 if(gb_num_cmp(&game.score,&game.target)>=0){if(!game_round_end()){game.numeric_error=1;game.phase=PHASE_BLOCKED;return;}
 boss_defeat(&game);if(game.numeric_error){game.phase=PHASE_BLOCKED;return;}
 if(game.blind==2){uint8_t h;game.ox_target_hand=0;for(h=1;h<12;h++)if(game.hands_played[h]>game.hands_played[game.ox_target_hand])game.ox_target_hand=h;}game.phase=PHASE_WIN;}else if(!game.hands||!game.hand_count)game.phase=PHASE_LOSS;
}
void game_input(uint8_t pressed) CORE_BANKED {
 game_action_error=0;
 if((game.phase==PHASE_TABLE||game.phase==PHASE_SHOP)&&game.joker_count&&(pressed&J_DOWN)&&!game.ui_mode){game.ui_mode=2;game.joker_focus=0;game.joker_grab=0;return;}
 if(game.ui_mode==2){
 if(pressed&(J_DOWN|J_START|J_SELECT)){game.ui_mode=0;game.joker_grab=0;return;}
 if((pressed&J_LEFT)&&game.joker_focus){if(game.joker_grab)game_move_joker(game.joker_focus,game.joker_focus-1);game.joker_focus--;}
 if((pressed&J_RIGHT)&&game.joker_focus+1<game.joker_count){if(game.joker_grab)game_move_joker(game.joker_focus,game.joker_focus+1);game.joker_focus++;}
 if(pressed&J_A)game.joker_grab^=1;
 if((pressed&J_B)&&!PG_JBYTE(&game,game.joker_focus,joker_eternal,10u)){int32_t value;if(!joker_history_sale_amount(&game,game.joker_focus,&value)){game_action_error=2;return;}joker_event(JE_SELL);if(game_remove_joker(game.joker_focus)){game_adjust_money(value);boss_on_joker_sold(&game);}if(game.joker_focus>=game.joker_count)game.joker_focus=game.joker_count?game.joker_count-1:0;game.joker_grab=0;if(!game.joker_count)game.ui_mode=0;}return;
 }

 if((game.phase==PHASE_TABLE||game.phase==PHASE_SHOP)&&game.consumable_count){
 if(pressed&J_UP)game.ui_mode^=1;
 if(game.ui_mode){if(pressed&J_LEFT)game.consumable_focus=game.consumable_focus?game.consumable_focus-1:game.consumable_count-1;if(pressed&J_RIGHT)game.consumable_focus=(game.consumable_focus+1)%game.consumable_count;if(pressed&J_A)consumable_use(game.consumable_focus);if(pressed&(J_B|J_DOWN))game.ui_mode=0;return;}}

 switch(game.phase){
 case PHASE_TITLE:if(pressed&J_LEFT)new_run_seed--;if(pressed&J_RIGHT)new_run_seed++;if(pressed&J_SELECT)new_run_seed=sys_time^((uint16_t)DIV_REG<<8);if(!new_run_seed)new_run_seed=1;if(pressed&J_START)game_new(new_run_seed);break;
 case PHASE_TABLE:
 if(!game.hand_count){game.phase=PHASE_LOSS;break;}
 if(pressed&J_LEFT)game.focus=game.focus?game.focus-1:game.hand_count-1;
 if(pressed&J_RIGHT)game.focus=(game.focus+1)%game.hand_count;
 if(pressed&J_A)hand_toggle(&game,game.focus);
 if(pressed&J_SELECT)hand_sort_rank(&game);
 if((pressed&J_B)&&game.highlighted_count&&game.discards){if(!boss_discard_selected(&game,0)){game.phase=PHASE_BLOCKED;return;}game.discards_used++;game.discards--;boss_on_discard(&game);deal();boss_after_draw(&game,2);}

 if(pressed&J_START)play();break;
 case PHASE_WIN:if(pressed&(J_A|J_START))shop_open();break;
 case PHASE_SHOP:
 shop_input(pressed);
 if(pressed&J_START){if(!inventory_shop_end_allowed(&game)){game_action_error=2;return;}joker_event(JE_SHOP_END);if(++game.blind==3){game.blind=0;game.ante++;}if(game.ante>8)game.phase=PHASE_COMPLETE;else begin_blind();}break;
 case PHASE_PACK:pack_input(pressed);break;
 default:if(pressed&J_START)game.phase=PHASE_TITLE;break;
 }
}
