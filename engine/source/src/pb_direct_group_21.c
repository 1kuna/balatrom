#include "paged_inventory.h"
#pragma bank 56
#include <gb/gb.h>
uint8_t pb_direct_read_21(uint16_t slot,uint16_t *r) BANKED {(void)slot;(void)r;return 0;}
uint8_t pb_direct_group_21(uint8_t n,uint16_t *r) BANKED {(void)n;(void)r;return 0;}

#include "native_run_rng.h"
#include "joker_history.h"
#include "hand_api.h"

#include <gb/gb.h>
#include "save.h"
#include <string.h>
extern uint8_t resume_available;
/* Explicit stack ABI, with no globals or IRQ/bank changes. The rotate/XOR is
 * byte-identical to the format10 checksum, including a zero-length payload.
 * RLCA exports original D bit 7; RL E then RL D rotate DE left once without
 * a data-dependent branch. Eight-way unrolling retains every byte in order. */
static uint16_t checksum(const uint8_t *data,uint16_t size) __sdcccall(0) __naked {
 __asm
 ldhl sp,#2
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld h,d
 ld l,e
 ld de,#0xb471
 ld a,b
 or c
 ret z
 /* Process the 0..7 leading bytes, then count eight-byte groups. The
  * residual decrements cannot borrow from B because C starts with that
  * residual. Dividing the remaining BC by eight is therefore exact. */
 ld a,c
 and #7
 jr z,44110$
44111$:
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 dec c
 ld a,c
 and #7
 jr nz,44111$
44110$:
 srl b
 rr c
 srl b
 rr c
 srl b
 rr c
 ld a,b
 or c
 ret z
 ld a,c
 or a
 jr z,44112$
 inc b
44112$:
44113$:
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 ld a,d
 rlca
 rl e
 rl d
 ld a,(hl+)
 xor e
 ld e,a
 dec c
 jr nz,44113$
 dec b
 jr nz,44113$
 ret
 __endasm;
}
/* Keep header invalidation, body copy and final magic commit in their original
 * order. This copies the complete payload; persistence is never deferred. */
static void copy_bytes(uint8_t *dest,const uint8_t *source,uint16_t size) __sdcccall(0) __naked {
 __asm
 ldhl sp,#2
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ld a,(hl+)
 ld h,(hl)
 ld l,a
 push hl
 ld h,b
 ld l,c
 pop bc
 ld a,b
 or c
 ret z
 /* Process the 0..7 leading bytes, then count eight-byte groups. The
  * residual decrements cannot borrow from B because C starts with that
  * residual. Dividing the remaining BC by eight is therefore exact. */
 ld a,c
 and #7
 jr z,44120$
44121$:
 ld a,(hl+)
 ld (de),a
 inc de
 dec c
 ld a,c
 and #7
 jr nz,44121$
44120$:
 srl b
 rr c
 srl b
 rr c
 srl b
 rr c
 ld a,b
 or c
 ret z
 ld a,c
 or a
 jr z,44122$
 inc b
44122$:
44123$:
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 ld a,(hl+)
 ld (de),a
 inc de
 dec c
 jr nz,44123$
 dec b
 jr nz,44123$
 ret
 __endasm;
}
static uint16_t checksum_meta(uint16_t c,const uint8_t meta[20]){uint8_t i;for(i=0;i<20u;i++)c=((c<<1)|(c>>15))^meta[i];return c;}
#include "legacy_inventory_state.h"
#include "inventory_bounds.h"
static const uint16_t inventory_fields[][4]={
{offsetof(LegacyInventoryState,phase),offsetof(GameState,phase),sizeof(((LegacyInventoryState*)0)->phase),sizeof(((GameState*)0)->phase)},
{offsetof(LegacyInventoryState,ante),offsetof(GameState,ante),sizeof(((LegacyInventoryState*)0)->ante),sizeof(((GameState*)0)->ante)},
{offsetof(LegacyInventoryState,blind),offsetof(GameState,blind),sizeof(((LegacyInventoryState*)0)->blind),sizeof(((GameState*)0)->blind)},
{offsetof(LegacyInventoryState,hands),offsetof(GameState,hands),sizeof(((LegacyInventoryState*)0)->hands),sizeof(((GameState*)0)->hands)},
{offsetof(LegacyInventoryState,discards),offsetof(GameState,discards),sizeof(((LegacyInventoryState*)0)->discards),sizeof(((GameState*)0)->discards)},
{offsetof(LegacyInventoryState,hand_count),offsetof(GameState,hand_count),sizeof(((LegacyInventoryState*)0)->hand_count),sizeof(((GameState*)0)->hand_count)},
{offsetof(LegacyInventoryState,deck_pos),offsetof(GameState,deck_pos),sizeof(((LegacyInventoryState*)0)->deck_pos),sizeof(((GameState*)0)->deck_pos)},
{offsetof(LegacyInventoryState,focus),offsetof(GameState,focus),sizeof(((LegacyInventoryState*)0)->focus),sizeof(((GameState*)0)->focus)},
{offsetof(LegacyInventoryState,joker_count),offsetof(GameState,joker_count),sizeof(((LegacyInventoryState*)0)->joker_count),sizeof(((GameState*)0)->joker_count)},
{offsetof(LegacyInventoryState,jokers),offsetof(GameState,jokers),sizeof(((LegacyInventoryState*)0)->jokers),sizeof(((GameState*)0)->jokers)},
{offsetof(LegacyInventoryState,shop),offsetof(GameState,shop),sizeof(((LegacyInventoryState*)0)->shop),sizeof(((GameState*)0)->shop)},
{offsetof(LegacyInventoryState,money),offsetof(GameState,money),sizeof(((LegacyInventoryState*)0)->money),sizeof(((GameState*)0)->money)},
{offsetof(LegacyInventoryState,seed),offsetof(GameState,seed),sizeof(((LegacyInventoryState*)0)->seed),sizeof(((GameState*)0)->seed)},
{offsetof(LegacyInventoryState,score),offsetof(GameState,score),sizeof(((LegacyInventoryState*)0)->score),sizeof(((GameState*)0)->score)},
{offsetof(LegacyInventoryState,target),offsetof(GameState,target),sizeof(((LegacyInventoryState*)0)->target),sizeof(((GameState*)0)->target)},
{offsetof(LegacyInventoryState,last_score),offsetof(GameState,last_score),sizeof(((LegacyInventoryState*)0)->last_score),sizeof(((GameState*)0)->last_score)},
{offsetof(LegacyInventoryState,last_chips),offsetof(GameState,last_chips),sizeof(((LegacyInventoryState*)0)->last_chips),sizeof(((GameState*)0)->last_chips)},
{offsetof(LegacyInventoryState,last_mult),offsetof(GameState,last_mult),sizeof(((LegacyInventoryState*)0)->last_mult),sizeof(((GameState*)0)->last_mult)},
{offsetof(LegacyInventoryState,last_hand),offsetof(GameState,last_hand),sizeof(((LegacyInventoryState*)0)->last_hand),sizeof(((GameState*)0)->last_hand)},
{offsetof(LegacyInventoryState,deck),offsetof(GameState,deck),sizeof(((LegacyInventoryState*)0)->deck),sizeof(((GameState*)0)->deck)},
{offsetof(LegacyInventoryState,hand),offsetof(GameState,hand),sizeof(((LegacyInventoryState*)0)->hand),sizeof(((GameState*)0)->hand)},
{offsetof(LegacyInventoryState,joker_state),offsetof(GameState,joker_state),sizeof(((LegacyInventoryState*)0)->joker_state),sizeof(((GameState*)0)->joker_state)},
{offsetof(LegacyInventoryState,hand_levels),offsetof(GameState,hand_levels),sizeof(((LegacyInventoryState*)0)->hand_levels),sizeof(((GameState*)0)->hand_levels)},
{offsetof(LegacyInventoryState,deck_count),offsetof(GameState,deck_count),sizeof(((LegacyInventoryState*)0)->deck_count),sizeof(((GameState*)0)->deck_count)},
{offsetof(LegacyInventoryState,draw_order),offsetof(GameState,draw_order),sizeof(((LegacyInventoryState*)0)->draw_order),sizeof(((GameState*)0)->draw_order)},
{offsetof(LegacyInventoryState,shop_kind),offsetof(GameState,shop_kind),sizeof(((LegacyInventoryState*)0)->shop_kind),sizeof(((GameState*)0)->shop_kind)},
{offsetof(LegacyInventoryState,consumable_count),offsetof(GameState,consumable_count),sizeof(((LegacyInventoryState*)0)->consumable_count),sizeof(((GameState*)0)->consumable_count)},
{offsetof(LegacyInventoryState,consumable_kind),offsetof(GameState,consumable_kind),sizeof(((LegacyInventoryState*)0)->consumable_kind),sizeof(((GameState*)0)->consumable_kind)},
{offsetof(LegacyInventoryState,consumables),offsetof(GameState,consumables),sizeof(((LegacyInventoryState*)0)->consumables),sizeof(((GameState*)0)->consumables)},
{offsetof(LegacyInventoryState,ui_mode),offsetof(GameState,ui_mode),sizeof(((LegacyInventoryState*)0)->ui_mode),sizeof(((GameState*)0)->ui_mode)},
{offsetof(LegacyInventoryState,consumable_focus),offsetof(GameState,consumable_focus),sizeof(((LegacyInventoryState*)0)->consumable_focus),sizeof(((GameState*)0)->consumable_focus)},
{offsetof(LegacyInventoryState,pack_ids),offsetof(GameState,pack_ids),sizeof(((LegacyInventoryState*)0)->pack_ids),sizeof(((GameState*)0)->pack_ids)},
{offsetof(LegacyInventoryState,pack_count),offsetof(GameState,pack_count),sizeof(((LegacyInventoryState*)0)->pack_count),sizeof(((GameState*)0)->pack_count)},
{offsetof(LegacyInventoryState,pack_choices),offsetof(GameState,pack_choices),sizeof(((LegacyInventoryState*)0)->pack_choices),sizeof(((GameState*)0)->pack_choices)},
{offsetof(LegacyInventoryState,pack_kind),offsetof(GameState,pack_kind),sizeof(((LegacyInventoryState*)0)->pack_kind),sizeof(((GameState*)0)->pack_kind)},
{offsetof(LegacyInventoryState,pack_size),offsetof(GameState,pack_size),sizeof(((LegacyInventoryState*)0)->pack_size),sizeof(((GameState*)0)->pack_size)},
{offsetof(LegacyInventoryState,pack_bought),offsetof(GameState,pack_bought),sizeof(((LegacyInventoryState*)0)->pack_bought),sizeof(((GameState*)0)->pack_bought)},
{offsetof(LegacyInventoryState,boss_id),offsetof(GameState,boss_id),sizeof(((LegacyInventoryState*)0)->boss_id),sizeof(((GameState*)0)->boss_id)},
{offsetof(LegacyInventoryState,numeric_error),offsetof(GameState,numeric_error),sizeof(((LegacyInventoryState*)0)->numeric_error),sizeof(((GameState*)0)->numeric_error)},
{offsetof(LegacyInventoryState,joker_debuff),offsetof(GameState,joker_debuff),sizeof(((LegacyInventoryState*)0)->joker_debuff),sizeof(((GameState*)0)->joker_debuff)},
{offsetof(LegacyInventoryState,boss_disabled),offsetof(GameState,boss_disabled),sizeof(((LegacyInventoryState*)0)->boss_disabled),sizeof(((GameState*)0)->boss_disabled)},
{offsetof(LegacyInventoryState,boss_first_hand),offsetof(GameState,boss_first_hand),sizeof(((LegacyInventoryState*)0)->boss_first_hand),sizeof(((GameState*)0)->boss_first_hand)},
{offsetof(LegacyInventoryState,boss_flipped),offsetof(GameState,boss_flipped),sizeof(((LegacyInventoryState*)0)->boss_flipped),sizeof(((GameState*)0)->boss_flipped)},
{offsetof(LegacyInventoryState,ectoplasm_used),offsetof(GameState,ectoplasm_used),sizeof(((LegacyInventoryState*)0)->ectoplasm_used),sizeof(((GameState*)0)->ectoplasm_used)},
{offsetof(LegacyInventoryState,hand_size),offsetof(GameState,hand_size),sizeof(((LegacyInventoryState*)0)->hand_size),sizeof(((GameState*)0)->hand_size)},
{offsetof(LegacyInventoryState,boss_seen_hands),offsetof(GameState,boss_seen_hands),sizeof(((LegacyInventoryState*)0)->boss_seen_hands),sizeof(((GameState*)0)->boss_seen_hands)},
{offsetof(LegacyInventoryState,forced_card_uid),offsetof(GameState,forced_card_uid),sizeof(((LegacyInventoryState*)0)->forced_card_uid),sizeof(((GameState*)0)->forced_card_uid)},
{offsetof(LegacyInventoryState,joker_edition),offsetof(GameState,joker_edition),sizeof(((LegacyInventoryState*)0)->joker_edition),sizeof(((GameState*)0)->joker_edition)},
{offsetof(LegacyInventoryState,joker_eternal),offsetof(GameState,joker_eternal),sizeof(((LegacyInventoryState*)0)->joker_eternal),sizeof(((GameState*)0)->joker_eternal)},
{offsetof(LegacyInventoryState,joker_sell),offsetof(GameState,joker_sell),sizeof(((LegacyInventoryState*)0)->joker_sell),sizeof(((GameState*)0)->joker_sell)},
{offsetof(LegacyInventoryState,last_consumable_kind),offsetof(GameState,last_consumable_kind),sizeof(((LegacyInventoryState*)0)->last_consumable_kind),sizeof(((GameState*)0)->last_consumable_kind)},
{offsetof(LegacyInventoryState,last_consumable_id),offsetof(GameState,last_consumable_id),sizeof(((LegacyInventoryState*)0)->last_consumable_id),sizeof(((GameState*)0)->last_consumable_id)},
{offsetof(LegacyInventoryState,gros_michel_extinct),offsetof(GameState,gros_michel_extinct),sizeof(((LegacyInventoryState*)0)->gros_michel_extinct),sizeof(((GameState*)0)->gros_michel_extinct)},
{offsetof(LegacyInventoryState,joker_uid),offsetof(GameState,joker_uid),sizeof(((LegacyInventoryState*)0)->joker_uid),sizeof(((GameState*)0)->joker_uid)},
{offsetof(LegacyInventoryState,next_joker_uid),offsetof(GameState,next_joker_uid),sizeof(((LegacyInventoryState*)0)->next_joker_uid),sizeof(((GameState*)0)->next_joker_uid)},
{offsetof(LegacyInventoryState,hands_max),offsetof(GameState,hands_max),sizeof(((LegacyInventoryState*)0)->hands_max),sizeof(((GameState*)0)->hands_max)},
{offsetof(LegacyInventoryState,discards_max),offsetof(GameState,discards_max),sizeof(((LegacyInventoryState*)0)->discards_max),sizeof(((GameState*)0)->discards_max)},
{offsetof(LegacyInventoryState,probability_normal),offsetof(GameState,probability_normal),sizeof(((LegacyInventoryState*)0)->probability_normal),sizeof(((GameState*)0)->probability_normal)},
{offsetof(LegacyInventoryState,selected_uids),offsetof(GameState,selected_uids),sizeof(((LegacyInventoryState*)0)->selected_uids),sizeof(((GameState*)0)->selected_uids)},
{offsetof(LegacyInventoryState,highlighted_count),offsetof(GameState,highlighted_count),sizeof(((LegacyInventoryState*)0)->highlighted_count),sizeof(((GameState*)0)->highlighted_count)},
{offsetof(LegacyInventoryState,ox_target_hand),offsetof(GameState,ox_target_hand),sizeof(((LegacyInventoryState*)0)->ox_target_hand),sizeof(((GameState*)0)->ox_target_hand)},
{offsetof(LegacyInventoryState,boss_uses),offsetof(GameState,boss_uses),sizeof(((LegacyInventoryState*)0)->boss_uses),sizeof(((GameState*)0)->boss_uses)},
{offsetof(LegacyInventoryState,next_card_uid),offsetof(GameState,next_card_uid),sizeof(((LegacyInventoryState*)0)->next_card_uid),sizeof(((GameState*)0)->next_card_uid)},
{offsetof(LegacyInventoryState,joker_focus),offsetof(GameState,joker_focus),sizeof(((LegacyInventoryState*)0)->joker_focus),sizeof(((GameState*)0)->joker_focus)},
{offsetof(LegacyInventoryState,joker_grab),offsetof(GameState,joker_grab),sizeof(((LegacyInventoryState*)0)->joker_grab),sizeof(((GameState*)0)->joker_grab)},
{offsetof(LegacyInventoryState,hands_played),offsetof(GameState,hands_played),sizeof(((LegacyInventoryState*)0)->hands_played),sizeof(((GameState*)0)->hands_played)},
{offsetof(LegacyInventoryState,hands_this_round),offsetof(GameState,hands_this_round),sizeof(((LegacyInventoryState*)0)->hands_this_round),sizeof(((GameState*)0)->hands_this_round)},
{offsetof(LegacyInventoryState,used_planet_mask),offsetof(GameState,used_planet_mask),sizeof(((LegacyInventoryState*)0)->used_planet_mask),sizeof(((GameState*)0)->used_planet_mask)},
{offsetof(LegacyInventoryState,discards_used),offsetof(GameState,discards_used),sizeof(((LegacyInventoryState*)0)->discards_used),sizeof(((GameState*)0)->discards_used)},
{offsetof(LegacyInventoryState,shop_pack_kind),offsetof(GameState,shop_pack_kind),sizeof(((LegacyInventoryState*)0)->shop_pack_kind),sizeof(((GameState*)0)->shop_pack_kind)},
{offsetof(LegacyInventoryState,first_shop_buffoon),offsetof(GameState,first_shop_buffoon),sizeof(((LegacyInventoryState*)0)->first_shop_buffoon),sizeof(((GameState*)0)->first_shop_buffoon)},
{offsetof(LegacyInventoryState,pack_offer_focus),offsetof(GameState,pack_offer_focus),sizeof(((LegacyInventoryState*)0)->pack_offer_focus),sizeof(((GameState*)0)->pack_offer_focus)},
{offsetof(LegacyInventoryState,pack_item_kind),offsetof(GameState,pack_item_kind),sizeof(((LegacyInventoryState*)0)->pack_item_kind),sizeof(((GameState*)0)->pack_item_kind)},
{offsetof(LegacyInventoryState,pack_cards),offsetof(GameState,pack_cards),sizeof(((LegacyInventoryState*)0)->pack_cards),sizeof(((GameState*)0)->pack_cards)},
{offsetof(LegacyInventoryState,starting_deck_size),offsetof(GameState,starting_deck_size),sizeof(((LegacyInventoryState*)0)->starting_deck_size),sizeof(((GameState*)0)->starting_deck_size)},
{offsetof(LegacyInventoryState,tarot_uses),offsetof(GameState,tarot_uses),sizeof(((LegacyInventoryState*)0)->tarot_uses),sizeof(((GameState*)0)->tarot_uses)},
{offsetof(LegacyInventoryState,joker_extra_value),offsetof(GameState,joker_extra_value),sizeof(((LegacyInventoryState*)0)->joker_extra_value),sizeof(((GameState*)0)->joker_extra_value)},
{offsetof(LegacyInventoryState,price_inflation),offsetof(GameState,price_inflation),sizeof(((LegacyInventoryState*)0)->price_inflation),sizeof(((GameState*)0)->price_inflation)},
{offsetof(LegacyInventoryState,price_discount_percent),offsetof(GameState,price_discount_percent),sizeof(((LegacyInventoryState*)0)->price_discount_percent),sizeof(((GameState*)0)->price_discount_percent)},
};
static void inventory_migrate(uint8_t*d){uint8_t f=sizeof(inventory_fields)/sizeof(inventory_fields[0]);uint16_t n,a,b,oldsize,newsize;while(f--){a=inventory_fields[f][0];b=inventory_fields[f][1];oldsize=inventory_fields[f][2];newsize=inventory_fields[f][3];n=oldsize;while(n--)d[b+n]=d[a+n];memset(d+b+oldsize,0,newsize-oldsize);}memset(d+offsetof(GameState,consumable_edition),0,sizeof(GameState)-offsetof(GameState,consumable_edition));}
uint8_t game_save(const GameState *state) CORE_BANKED {
 volatile uint8_t *ram=(volatile uint8_t *)0xA000;const uint8_t *data=(const uint8_t *)state;uint8_t meta[20],version;uint16_t size,c;
 if(!joker_history_valid(state)||!inventory_valid(state))return 0;native_rng_export(meta);version=14u;
 if(meta[0]&&(!native_rng_valid(meta)||(meta[2]|((uint16_t)meta[3]<<8))!=state->seed))return 0;
 size=sizeof(GameState)+20u;c=checksum(data,sizeof(GameState));if(version>=13u)c=checksum_meta(c,meta);
 ENABLE_RAM_MBC5;SWITCH_RAM_MBC5(0);ram[0]=0;
 ram[1]='B';ram[2]='G';ram[3]=version;ram[4]=size&255;ram[5]=size>>8;ram[6]=c&255;ram[7]=c>>8;
 copy_bytes((uint8_t *)(ram+8),data,sizeof(GameState));if(version>=13u)copy_bytes((uint8_t *)(ram+8+sizeof(GameState)),meta,20u);
 if(!pg_save(c)){DISABLE_RAM_MBC5;return 0;}ram[0]='N';DISABLE_RAM_MBC5;resume_available=1;return 1;
}
uint8_t game_load(GameState *state) CORE_BANKED {
 volatile uint8_t *ram=(volatile uint8_t *)0xA000;uint8_t *data=(uint8_t *)state;uint8_t meta[20],version,valid=0;uint16_t i,c,stored,size,core;
 memset(meta,0,20u);ENABLE_RAM_MBC5;SWITCH_RAM_MBC5(0);version=ram[3];core=version>=13u?sizeof(GameState):4659u;size=core+(version==10u?0u:20u);
 if(ram[0]=='N'&&ram[1]=='B'&&ram[2]=='G'&&(version==10u||version==12u||version==13u||version==14u)&&ram[4]==(size&255)&&ram[5]==(size>>8)){
 for(i=0;i<core;i++)data[i]=ram[8+i];stored=ram[6]|((uint16_t)ram[7]<<8);c=checksum(data,core);
 if(version!=10u){for(i=0;i<20u;i++)meta[i]=ram[8+core+i];c=checksum_meta(c,meta);}
 if(c!=stored)goto load_done;
 if(version<13u)inventory_migrate(data);if(!pg_load(state,stored,version))goto load_done;
 valid=( (version!=12u||meta[0]==1u) && (!meta[0]||(native_rng_valid(meta)&&(meta[2]|((uint16_t)meta[3]<<8))==state->seed)) && state->phase>=PHASE_TABLE && state->phase<=PHASE_BLOCKED && state->ante>=1&&state->ante<=9&&state->blind<=2&&state->hand_count<=HAND_STORAGE_CAP&&state->deck_pos<=state->deck_count&&state->deck_count>=1&&state->deck_count<=DECK_CAP&&state->joker_count<=PG_JMAX && inventory_valid(state) && hand_valid(state) && joker_history_valid(state));
 }load_done:
 DISABLE_RAM_MBC5;
 if(valid)valid=native_rng_import(meta);if(!valid){memset(meta,0,20u);native_rng_import(meta);}resume_available=valid;return valid;
}

#include <string.h>
static const uint16_t np_palette_1[75][14][4]={/* OWNER_INSTALL_SLOT:np_palette_1 */};
uint8_t native_D_lookup_1(uint8_t id,uint16_t*p,uint8_t variant) BANKED {uint8_t i;if(id>=75u||variant>=13u)return 0;for(i=0;i<4u;i++)if(p[i]!=np_palette_1[id][0][i])return 0;memcpy(p,np_palette_1[id][variant+1u],8u);return 1;}
