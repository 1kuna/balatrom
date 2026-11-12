#include "cooperative_native.h"
#include "raster_runtime.h"
extern uint8_t coop_live_held;
extern __at(0xffbc) volatile uint16_t raster_address;
extern __at(0xffbe) volatile uint8_t raster_value;
extern volatile uint8_t pub139_record_enabled,pub139_record_busy;
#define PUB138_API BANKED
#include "journal138.c"
/* Classification remains under the existing recording/owner guards, but
 * does not take a nested BANKED call just to compute a physical bucket. */
uint8_t pub139_tracks(uint8_t bank,uint16_t address) BANKED {
 if(!pub139_record_enabled||pub139_record_busy||!coop_active||!coop_live_held)return 0;
 return ((bank==5u||bank==6u)&&address>=0xd000u&&address<0xe000u)||
        (bank==7u&&address>=0xd200u&&address<0xda00u)||
        (bank==3u&&address>=0xd300u&&address<0xd900u);
}
/* Near call in bank41: no stack access while bank7 is selected. Inputs use
 * the existing HRAM ferry address/value. This only ORs one journal byte,
 * never its rendered payload, descriptors, GameState or RNG. */
void raster_ferry_window(void) NONBANKED;
static void pub144_journal_or_raw(void) __naked {
 __asm
 push af
 push bc
 push hl
 ldh a,(0xbc)
 ld l,a
 ldh a,(0xbd)
 ld h,a
 ldh a,(0xbe)
 ld c,a
 ld a,#7
 ldh (0x70),a
 ld a,(hl)
 or c
 ld (hl),a
 ld a,#1
 ldh (0x70),a
 pop hl
 pop bc
 pop af
 ret
 __endasm;
}
void pub139_mark(uint8_t bank,uint16_t address) BANKED {
 uint16_t at;uint8_t mask;
 if((bank==5u||bank==6u)&&address>=0xd000u&&address<0xe000u)
  at=0xdbb0u+(address-0xd000u)/128u+(uint16_t)(bank-5u)*32u;
 else if(bank==7u&&address>=0xd200u&&address<0xda00u)at=0xdbf0u+(address-0xd200u)/128u;
 else if(bank==3u&&address>=0xd300u&&address<0xd900u)at=0xdc70u+(address-0xd300u)/128u;
 else return;
 mask=1u<<((address>>4)&7u);pub139_record_busy=1;
 raster_ferry_window();
 CRITICAL {raster_address=at;raster_value=mask;pub144_journal_or_raw();}
 pub139_record_busy=0;
}

/* BANKED entry and return use the SDK with SVBK1. No CALL or stack access
 * occurs during the temporary payload or journal bank selections. */
void pub139_put_changed_raw(void) __naked BANKED {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ldh a,(0xeb)
 ld b,a
 ld c,#0xd2
 ld l,#0
 ldh a,(0xea)
 ldh (0x70),a
pub145_put_loop:
 ld a,(de)
 ld h,a
 ldh a,(c)
 cp h
 jr z,pub145_put_same
 ld l,#1
pub145_put_same:
 ld (de),a
 inc de
 inc c
 dec b
 jr nz,pub145_put_loop
 ld a,#1
 ldh (0x70),a
 ld a,l
 ld (#_pub139_changed),a
 or a
 jp z,pub145_put_end
 /* Original physical address, not the advanced copy cursor. */
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ldh a,(0xea)
 cp #5
 jr z,pub145_pattern5
 cp #6
 jr z,pub145_pattern6
 cp #7
 jr z,pub145_maps
 cp #3
 jr z,pub145_metadata
 jp pub145_put_end
pub145_pattern5:
 ld b,#0xb0
 jr pub145_patterns
pub145_pattern6:
 ld b,#0xd0
pub145_patterns:
 ld a,d
 cp #0xd0
 jp c,pub145_put_end
 cp #0xe0
 jp nc,pub145_put_end
 sub #0xd0
 ld c,#0xdb
 jr pub145_bucket
pub145_maps:
 ld a,d
 cp #0xd2
 jp c,pub145_put_end
 cp #0xda
 jp nc,pub145_put_end
 sub #0xd2
 ld b,#0xf0
 ld c,#0xdb
 jr pub145_bucket
pub145_metadata:
 ld a,d
 cp #0xd3
 jp c,pub145_put_end
 cp #0xd9
 jp nc,pub145_put_end
 sub #0xd3
 ld b,#0x70
 ld c,#0xdc
pub145_bucket:
 add a,a
 bit 7,e
 jr z,pub145_bucket_low
 inc a
pub145_bucket_low:
 add a,b
 ld l,a
 ld h,c
 /* Mask lookup in the current ROM bank, before selecting bank7. */
 ld b,h
 ld c,l
 ld a,e
 rrca
 rrca
 rrca
 rrca
 and #7
 ld e,a
 ld d,#0
 ld hl,#pub145_masks
 add hl,de
 ld a,(hl)
 ld e,a
 ld h,b
 ld l,c
 ld a,#7
 ldh (0x70),a
 ld a,(hl)
 or e
 ld (hl),a
 ld a,#1
 ldh (0x70),a
pub145_put_end:
 pop hl
 pop de
 pop bc
 pop af
 ret
pub145_masks:
 .db 1,2,4,8,16,32,64,128
 __endasm;
}

/* Journal snapshot uses existing unbanked renderer scratch only after the
 * retained renderer has finished staging. Two40-byte leaves keep the IRQ
 * mask bounded; no bank1 local/stack access while bank7 is selected. */
static void pub147_read40_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ldh a,(0xbe)
 ld l,a
 ld h,#0
 ld bc,#_raster_pixels
 add hl,bc
 ld b,#40
 ld a,#7
 ldh (0x70),a
pub147_read40_loop:
 ld a,(de)
 ld (hl+),a
 inc de
 dec b
 jr nz,pub147_read40_loop
 ld a,#1
 ldh (0x70),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void pub147_read_journal(void){
 raster_ferry_window();
 CRITICAL {raster_address=0xdbb0u;raster_value=0;pub147_read40_raw();}
 raster_ferry_window();
 CRITICAL {raster_address=0xdbd8u;raster_value=40u;pub147_read40_raw();}
}
