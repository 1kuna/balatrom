#include <gb/gb.h>
#include <gb/cgb.h>
#include <gb/isr.h>
#include "cooperative_native.h"
#include "publication143.h"
#define RASTER_RUNTIME_IMPLEMENTATION
#include "raster_runtime.h"
void raster_stat(void) __naked NONBANKED;
void raster_vblank(void) __naked NONBANKED;
ISR_VECTOR(VECTOR_STAT,raster_stat)

__at(0xffb5) volatile uint8_t raster_fault;
__at(0xffb6) volatile uint8_t raster_last_line;
__at(0xffb7) volatile uint8_t raster_epoch;
__at(0xffb8) volatile uint16_t raster_frames;
__at(0xffba) volatile uint8_t raster_delay_47;
__at(0xffbc) volatile uint16_t raster_address;
__at(0xffbe) volatile uint8_t raster_value;
__at(0xffbf) volatile uint8_t raster_build_bank;
__at(0xffd0) volatile uint8_t raster_active_bank;
__at(0xffd1) volatile uint8_t raster_live;
__at(0xffd2) volatile uint8_t raster_ferry[16];
__at(0xffea) volatile uint8_t raster_ferry_bank;
__at(0xffeb) volatile uint8_t raster_ferry_length;
__at(0xffe2) volatile uint16_t raster_dma_destination;
__at(0xffe4) volatile uint8_t raster_dma_vbk;
__at(0xffe5) volatile uint8_t raster_dma_blocks;
__at(0xffe6) volatile uint8_t raster_dma_jobs;
__at(0xffe7) volatile uint8_t raster_map_recipe;
__at(0xffe8) volatile uint8_t raster_map_skip_ids;
uint8_t raster_error,raster_upper_count,raster_lower_count,raster_peak;
uint8_t raster_dirty_pairs;
volatile uint16_t raster_published;
uint8_t raster_pixels[128],raster_planes[96],raster_loads[144];
static uint8_t installed;

/* LCD-off GDMA only. Inputs are HRAM, so no caller stack or C local is read
 * while its bank is unmapped. GDMA halts the CPU until the whole transfer
 * completes; restore both banks before popping the bank1 caller stack. */
static void transfer_raw(void) __naked NONBANKED {
 __asm
 push af
 ldh a,(0xea)
 ldh (0x70),a
 ldh a,(0xe4)
 ldh (0x4f),a
 ldh a,(0xbd)
 ldh (0x51),a
 ldh a,(0xbc)
 ldh (0x52),a
 ldh a,(0xe3)
 ldh (0x53),a
 ldh a,(0xe2)
 ldh (0x54),a
 ldh a,(0xe5)
 _raster_single_gdma_start::
 ldh (0x55),a
 _raster_single_gdma_return::
 ld a,#1
 ldh (0x70),a
 xor a
 ldh (0x4f),a
 pop af
 ret
 __endasm;
}
uint8_t raster_transfer(uint8_t bank,uint16_t source,uint16_t destination,uint8_t vbk,uint8_t blocks) NONBANKED {
 if((LCDC_REG&LCDCF_ON)||bank<2u||bank>7u||!blocks||blocks>128u||vbk>1u||
    (source&15u)||(destination&15u)||source<0xd000u||source>0xdff0u||destination<0x8000u||destination>0x9ff0u||
    source+(uint16_t)blocks*16u>0xe000u||destination+(uint16_t)blocks*16u>0xa000u)return 0;
 CRITICAL {
  raster_address=source;raster_ferry_bank=bank;raster_dma_destination=destination;
  raster_dma_vbk=vbk;raster_dma_blocks=blocks-1u;transfer_raw();
 }
 return 1;
}

/* Six-byte jobs in bank7 DB00..DF43. All span validation and descriptor
 * construction happens while the old LCD/audio remain live. No C call,
 * stack access or IRQ occurs while bank7/source banks are mapped here. */
static void publish_jobs_raw(void) __naked NONBANKED {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xe6)
 ld b,a
 ld hl,#0xdb00
 ld a,#7
 ldh (0x70),a
 publish_job_loop:
 ld a,(hl+)
 ldh (0x52),a
 ld a,(hl+)
 ldh (0x51),a
 ld a,(hl+)
 ldh (0x54),a
 ld a,(hl+)
 ldh (0x53),a
 ld a,(hl+)
 ld e,a
 ld a,(hl+)
 ld d,a
 ld a,e
 rrca
 rrca
 rrca
 and #1
 ldh (0x4f),a
 ld a,e
 and #7
 ldh (0x70),a
 ld a,d
 _raster_gdma_start::
 ldh (0x55),a
 _raster_gdma_return::
 ld a,#7
 ldh (0x70),a
 dec b
 jr nz,publish_job_loop
 ld a,#1
 ldh (0x70),a
 xor a
 ldh (0x4f),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
uint8_t raster_publish_jobs(uint8_t count) NONBANKED {
 if((LCDC_REG&LCDCF_ON)||!count||count>182u)return 0;
 CRITICAL {raster_dma_jobs=count;publish_jobs_raw();}
 return 1;
}

/* Canonical 360 BG tile IDs and all recipe-derived attributes. Full maps are
 * written only with the LCD off, after complete admission. IDs never depend
 * on source identities. Metadata belongs to the prospective bank. */
static void publish_maps_raw(void) __naked NONBANKED {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xe8)
 or a
 jr nz,map_attrs_start
 xor a
 ldh (0x4f),a
 ld hl,#0x9800
 ld c,#18
 ld d,#128
 map_ids_row:
 ld b,#20
 map_ids_byte:
 ld a,d
 ld (hl+),a
 inc d
 dec b
 jr nz,map_ids_byte
 ld a,l
 add a,#12
 ld l,a
 jr nc,map_ids_no_carry
 inc h
 map_ids_no_carry:
 dec c
 jr nz,map_ids_row
 map_attrs_start:
 ldh a,(0xe8)
 bit 1,a
 jr z,map_attrs_full
 /* Sparse descriptors are staged in unused bank6 D800..DACF. Each carries
  * canonical map address and both row-half attributes; no C/stack access
  * occurs while bank6 is selected. All descriptors precede LCD-off entry. */
 ld a,(_raster_dirty_pairs)
 ld b,a
 or a
 jp z,map_attrs_restore
 ld a,#6
 ldh (0x70),a
 ld a,#1
 ldh (0x4f),a
 ld hl,#0xd800
 map_sparse_loop:
 ld a,(hl+)
 ld e,a
 ld a,(hl+)
 ld d,a
 ld a,(hl+)
 ld (de),a
 ld a,e
 add a,#32
 ld e,a
 ld a,d
 adc a,#0
 ld d,a
 ld a,(hl+)
 ld (de),a
 dec b
 jr nz,map_sparse_loop
 jp map_attrs_restore
 map_attrs_full:
 ldh a,(0xbf)
 ldh (0x70),a
 ld a,#1
 ldh (0x4f),a
 ld hl,#0xd300
 ld de,#0x9800
 ld c,#0
 map_attrs_row:
 ld b,#20
 map_attrs_pair:
 ld a,(hl+)
 ldh (0xe7),a
 ld a,c
 cp #136
 ldh a,(0xe7)
 jr nc,map_attrs_first_bank0
 and #7
 or #8
 jr map_attrs_first_store
 map_attrs_first_bank0:
 and #7
 map_attrs_first_store:
 ld (de),a
 ld a,e
 add a,#32
 ld e,a
 jr nc,map_attrs_second_address
 inc d
 map_attrs_second_address:
 ld a,c
 cp #120
 jr nc,map_attrs_second_bank0
 ldh a,(0xe7)
 rrca
 rrca
 rrca
 and #7
 or #8
 jr map_attrs_second_store
 map_attrs_second_bank0:
 ldh a,(0xe7)
 rrca
 rrca
 rrca
 and #7
 map_attrs_second_store:
 ld (de),a
 ld a,e
 sub #32
 ld e,a
 jr nc,map_attrs_first_address
 dec d
 map_attrs_first_address:
 inc de
 inc hl
 inc hl
 inc hl
 inc c
 dec b
 jp nz,map_attrs_pair
 ld a,e
 add a,#44
 ld e,a
 jr nc,map_attrs_next_row
 inc d
 map_attrs_next_row:
 ld a,c
 cp #180
 jr nz,map_attrs_row
 map_attrs_restore:
 ld a,#1
 ldh (0x70),a
 xor a
 ldh (0x4f),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
uint8_t raster_publish_maps(uint8_t skip_ids) NONBANKED {
 if((LCDC_REG&LCDCF_ON)||raster_build_bank<2u||raster_build_bank>3u)return 0;
 CRITICAL {raster_map_skip_ids=skip_ids;publish_maps_raw();}
 return 1;
}

/* C and its stack are always in SVBK1. These small critical ferries alone
 * access the prospective bank. Their register save and return happen in 1.
 * All source/value operands are HRAM; no C local is read while remapped. */
static void put_raw(void) __naked NONBANKED {
 __asm
 push hl
 ldh a,(0xbf)
 ldh (0x70),a
 ldh a,(0xbc)
 ld l,a
 ldh a,(0xbd)
 ld h,a
 ldh a,(0xbe)
 ld (hl),a
 ld a,#1
 ldh (0x70),a
 pop hl
 ret
 __endasm;
}
static void get_raw(void) __naked NONBANKED {
 __asm
 push hl
 ldh a,(0xbf)
 ldh (0x70),a
 ldh a,(0xbc)
 ld l,a
 ldh a,(0xbd)
 ld h,a
 ld a,(hl)
 ldh (0xbe),a
 ld a,#1
 ldh (0x70),a
 pop hl
 ret
 __endasm;
}
extern __at(0xfff0) uint8_t native_palette_ready;
void raster_ferry_window(void) NONBANKED {
 uint8_t line;
 if(raster_live&&(LCDC_REG&LCDCF_ON))while(LY_REG>=30u&&LY_REG<=47u){}
 /* every native LYC job line (7,31,32,43,77,79,83,87,95,
  * 111,127; see native_stat) keeps the same three-line lead the former
  * cluster bands gave each cluster's first job. Longest guarded critical
  * body is the 48-byte result ferry (~2.3 double-speed lines), so a start
  * at LY<=N-4 still ends before line N. Idle gaps inside former clusters
  * (33..39, 88..91, 112..123) no longer stall WRAM ferries. */
 if(native_palette_ready==1u&&(LCDC_REG&LCDCF_ON))do{line=LY_REG;}while((line>=4u&&line<=7u)||(line>=28u&&line<=32u)||(line>=40u&&line<=43u)||(line>=74u&&line<=87u)||(line>=92u&&line<=95u)||(line>=108u&&line<=111u)||(line>=124u&&line<=127u));
}
void raster_put(uint16_t address,uint8_t value) NONBANKED {
 raster_ferry_window();
 CRITICAL {raster_address=address;raster_value=value;put_raw();}
}
uint8_t raster_get(uint16_t address) NONBANKED {
 uint8_t value;
 raster_ferry_window();
 CRITICAL {raster_address=address;get_raw();value=raster_value;}
 return value;
}
#define PUB138_API BANKED
#include "journal138.h"
#include "writer_hook139.inc"
static void put_block_raw(void) __naked NONBANKED {
 __asm
 push af
 push bc
 push de
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ldh a,(0xeb)
 ld b,a
 ld c,#0xd2
 ldh a,(0xea)
 ldh (0x70),a
 ferry_put_loop:
 ldh a,(c)
 ld (de),a
 inc de
 inc c
 dec b
 jr nz,ferry_put_loop
 ld a,#1
 ldh (0x70),a
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void get_block_raw(void) __naked NONBANKED {
 __asm
 push af
 push bc
 push de
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ldh a,(0xeb)
 ld b,a
 ld c,#0xd2
 ldh a,(0xea)
 ldh (0x70),a
 ferry_get_loop:
 ld a,(de)
 ldh (c),a
 inc de
 inc c
 dec b
 jr nz,ferry_get_loop
 ld a,#1
 ldh (0x70),a
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
void raster_put_block(uint8_t bank,uint16_t address,const uint8_t *source,uint8_t length) NONBANKED {
 uint8_t i,n;
 if(coop_active&&bank==2u)bank=3u;
 while(length){
  uint8_t tracked=0;
  tracked=pub139_record_enabled;
  n=length>16u?16u:length;
  if(tracked&&n>16u-(address&15u))n=16u-(address&15u);
  for(i=0;i<n;i++)raster_ferry[i]=source[i];
  raster_ferry_window();
  CRITICAL {raster_address=address;raster_ferry_bank=bank;raster_ferry_length=n;
   if(tracked)pub139_put_changed_raw();else put_block_raw();}
  source+=n;address+=n;length-=n;if(coop_active)coop_checkpoint();
 }
}
void raster_get_block(uint8_t bank,uint16_t address,uint8_t *dest,uint8_t length) NONBANKED {
 uint8_t i,n;
 if(coop_active&&bank==2u)bank=3u;
 while(length){
  n=length>16u?16u:length;
  raster_ferry_window();
  CRITICAL {raster_address=address;raster_ferry_bank=bank;raster_ferry_length=n;get_block_raw();}
  for(i=0;i<n;i++)dest[i]=raster_ferry[i];
  dest+=n;address+=n;length-=n;if(coop_active)coop_checkpoint();
 }
}
extern __at(0xfff0) uint8_t native_palette_ready;
void raster_begin_publish(void) NONBANKED {
 /* Input remains serviced while waiting for the last visible lines. The
  * helper owns its existing held mask and queue; it never dispatches core. */
 if(LCDC_REG&LCDCF_ON){while(LY_REG<144u){raster_capture_inputs();} }
 raster_capture_inputs();
 CRITICAL {
  native_palette_ready=0;raster_live=0;STAT_REG=0;IE_REG&=~LCD_IFLAG;
  LCDC_REG=0x07;DISABLE_OAM_DMA;
 }
 SVBK_REG=1;
}
void raster_end_publish(void) NONBANKED {
 uint8_t i;
 /* JP(HL) returns entirely through registers. No stack access occurs
  * while the IRQ owns bank2/3, preserving resident OBJ plane bytes. */
 static const uint8_t tail[8]={0xe0,0x46,0x3e,40,0x3d,0x20,0xfd,0xe9};
 if(!installed){
  for(i=0;i<8u;i++)((volatile uint8_t *)0xffc0)[i]=tail[i];
  add_VBL(raster_vblank);installed=1;
 }
 CRITICAL {
  raster_active_bank=raster_build_bank;
  raster_fault=raster_delay_47=0;
  raster_live=1;
  raster_vblank();
  VBK_REG=0;SCX_REG=SCY_REG=0;
  IF_REG&=~LCD_IFLAG;LYC_REG=32;STAT_REG=STATF_LYC;
  IE_REG|=VBL_IFLAG|LCD_IFLAG;
  LCDC_REG=0x87;++raster_published;
 }
}
void raster_stop(void) NONBANKED {
 raster_begin_publish();
 CRITICAL {raster_fault=0;ENABLE_OAM_DMA;}
 VBK_REG=0;
}


__at(0xfff0) uint8_t native_palette_ready;
void native_palette_stop(void) NONBANKED {CRITICAL {native_palette_ready=0;STAT_REG=0;IE_REG&=~LCD_IFLAG;}}
void native_palette_start(void) NONBANKED {
 uint8_t i;static const uint8_t tail[8]={0xe0,0x46,0x3e,40,0x3d,0x20,0xfd,0xe9};
 if(!installed){for(i=0;i<8u;i++)((volatile uint8_t *)0xffc0)[i]=tail[i];add_VBL(raster_vblank);installed=1;}
 CRITICAL {native_palette_ready=1;raster_vblank();IF_REG&=~LCD_IFLAG;LYC_REG=7;STAT_REG=STATF_LYC;IE_REG|=VBL_IFLAG|LCD_IFLAG;}
}

/* Fixed-WRAM source/destination, bank0 code, no IRQ mask or bank switch.
 * Stack ABI(first,last,color); clipped span at most8 rows; color low2 bits.
 * Per-row OR/AND is exactly the original C mask blend. */
void a1_blend_mask(uint8_t first,uint8_t last,uint8_t color) __sdcccall(0) __naked NONBANKED {
 __asm
 ldhl sp,#2
 ld a,(hl)
 ld e,a
 ld d,#0
 inc hl
 ld a,(hl)
 sub e
 ld b,a
 ret z
 inc hl
 ld a,(hl)
 and #3
 ld c,a
 ld a,e
 add a,a
 ld hl,#_raster_pixels
 add a,l
 ld l,a
 ld a,h
 adc a,#0
 ld h,a
 push hl
 ld hl,#_raster_pixels+16
 add hl,de
 ld e,l
 ld d,h
 pop hl
 ld a,c
 or a
 jr z,54900$
 dec a
 jr z,54901$
 dec a
 jr z,54902$
54903$:
 ld a,(de)
 inc de
 ld c,a
 or (hl)
 ld (hl+),a
 ld a,c
 or (hl)
 ld (hl+),a
 dec b
 jr nz,54903$
 ret
54900$:
 ld a,(de)
 inc de
 cpl
 ld c,a
 and (hl)
 ld (hl+),a
 ld a,c
 and (hl)
 ld (hl+),a
 dec b
 jr nz,54900$
 ret
54901$:
 ld a,(de)
 inc de
 ld c,a
 or (hl)
 ld (hl+),a
 ld a,c
 cpl
 and (hl)
 ld (hl+),a
 dec b
 jr nz,54901$
 ret
54902$:
 ld a,(de)
 inc de
 ld c,a
 cpl
 and (hl)
 ld (hl+),a
 ld a,c
 or (hl)
 ld (hl+),a
 dec b
 jr nz,54902$
 ret
 __endasm;
}

/* Cooperative lease controls: fixed WRAM, no saved game-state fields. */
volatile uint8_t coop_active,coop_cancelled,coop_accepting,coop_committing;
volatile uint16_t coop_generation,coop_stage_generation;
uint8_t coop_lcdc_held,coop_ready_held,coop_live_held;
extern uint8_t native_table_live;
volatile uint8_t *coop_lcdc_ptr(void) NONBANKED {return coop_active?&coop_lcdc_held:(volatile uint8_t*)0xff40u;}
uint8_t *coop_ready_ptr(void) NONBANKED {return coop_active?&coop_ready_held:&native_palette_ready;}
uint8_t *coop_live_ptr(void) NONBANKED {return coop_active?&coop_live_held:&native_table_live;}
void coop_palette_stop(void) NONBANKED {if(coop_active)coop_ready_held=0;else native_palette_stop();}
void coop_palette_start(void) NONBANKED {if(coop_active)coop_ready_held=1;else native_palette_start();}
void coop_publish_metadata(void) __naked NONBANKED {
 __asm
 push af
 push bc
 push de
 push hl
 di
 ld de,#0xd300
55900$:
 ld a,#3
 ldh (0x70),a
 ld hl,#_raster_pixels
 ld b,#8
55901$:
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 ld a,(de)
 ld (hl+),a
 inc de
 dec b
 jr nz,55901$
 ld a,e
 sub #64
 ld e,a
 ld a,d
 sbc a,#0
 ld d,a
 ld a,#2
 ldh (0x70),a
 ld hl,#_raster_pixels
 ld b,#8
55902$:
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
 dec b
 jr nz,55902$
 ld a,d
 cp #0xd9
 jr nz,55900$
 ld a,#1
 ldh (0x70),a
 pop hl
 pop de
 pop bc
 pop af
 ei
 ret
 __endasm;
}

void coop_display_off(void) NONBANKED {if(coop_active)coop_lcdc_held&=127u;else display_off();}

/* Standalone component: NOT installed as a VBlank vector.
 * Arming requires double speed, SVBK1 stack, LCD/scroll/native owners unchanged,
 * live resident TABLE, no generic raster, C0 ordinary OAM base, at most14
 * checked jobs/31 blocks at bank7DC00. Main keeps the old display until commit.
 * __shadow_OAM_base stays zero until main finishes copying new OAM to C000.
 */
#include <gb/gb.h>
extern unsigned char coop_lcdc_held,coop_ready_held,coop_live_held;
extern __at(0xfff0) uint8_t native_palette_ready;
extern unsigned char native_table_live;
void pub143_vblank(void) __naked NONBANKED {
 __asm
 push af
 push bc
 push de
 push hl
 ld a,(#_pub143_enabled)
 or a
 jp z,p138_exit
 ldh a,(0xed)
 or a
 jp z,p138_exit
 ldh a,(0x4d)
 and #0x80
 jp z,p138_deny
 ldh a,(0x44)
 cp #144
 jp nz,p138_deny
 ldh a,(0xef)
 or a
 jp z,p138_deny
 cp #15
 jp nc,p138_deny
 ld b,a
 ld hl,#0xdc00
 ld a,#7
 ldh (0x70),a
p138_job:
 ld a,(hl+)
 ldh (0x52),a
 ld a,(hl+)
 ldh (0x51),a
 ld a,(hl+)
 ldh (0x54),a
 ld a,(hl+)
 ldh (0x53),a
 ld a,(hl+)
 ldh (0x4f),a
 ld a,(hl+)
 ld e,a
 ld a,(hl+)
 ld d,a
 ld a,e
 ldh (0x70),a
 ld a,d
 ldh (0x55),a
 ld a,#7
 ldh (0x70),a
 dec b
 jr nz,p138_job
 ldh a,(0xed)
 cp #1
 jp z,p138_table
 cp #2
 jp z,r149_stage_done
 cp #3
 jp z,r149_flip
 cp #4
 jp z,r149_flip
p138_table:
 xor a
 ldh (0x4f),a
 ld a,#3
 ldh (0x70),a
 ldh (0xec),a
 ld a,(#0xd3ae)
 ld (#0xd3b0),a
 ld hl,#0xd300
 ld c,#0x69
 ld a,#0x80
 ldh (0x68),a
 ld b,#8
p141_bg_loop:
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 dec b
 jr nz,p141_bg_loop
 ld c,#0x6b
 ld a,#0x80
 ldh (0x6a),a
 ld a,#7
 ldh (0x70),a
 ld hl,#0xdae0
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,#3
 ldh (0x70),a
 ld hl,#0xd340
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld hl,#0xd4d0
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,#7
 ldh (0x70),a
 ld hl,#0xdaf8
 ld b,#5
p141_obj_loop:
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 ld a,(hl+)
 ldh (c),a
 dec b
 jr nz,p141_obj_loop
 ld a,#0xda
 ld hl,#p138_dma_return
 jp 0xffc0
p138_dma_return:
 ld a,#1
 ldh (0x70),a
 xor a
 ldh (0x92),a
 ld a,(#_coop_live_held)
 ld (#_native_table_live),a
 ld a,(#_coop_ready_held)
 ld (#_native_palette_ready),a
 ld a,(#_coop_lcdc_held)
 or #0x80
 and #0xfb
 ldh (0x40),a
 xor a
 ldh (0x42),a
 ldh (0x43),a
 ldh (0xed),a
 ld a,#7
 ldh (0x45),a
 ld a,#1
 ldh (0xee),a
 jp p138_exit
r149_flip:
 xor a
 ldh (0x4f),a
 ldh a,(0xed)
 cp #4
 jr z,r149_palette_only
 xor a
 ldh (0x41),a
 ldh a,(0xff)
 and #0xfd
 ldh (0xff),a
r149_palette_only:
 ld a,#7
 ldh (0x70),a
 ld hl,#0xdaa0
 ld c,#0x69
 ld a,#0x80
 ldh (0x68),a
 ld b,#24
r149_palette:
 ld a,(hl+)
 ldh (c),a
 dec b
 jr nz,r149_palette
 ldh a,(0xed)
 cp #4
 jp z,r149_stage_done
 ld a,#1
 ldh (0x70),a
 xor a
 ldh (0xf0),a
 ld (#_native_table_live),a
 ld a,#0xc9
 ldh (0x40),a
r149_stage_done:
 xor a
 ldh (0x4f),a
 ldh (0xed),a
 inc a
 ldh (0x70),a
 inc a
 ldh (0xee),a
 jp p138_exit
p138_deny:
 xor a
 ldh (0xed),a
 ld a,#3
 ldh (0xee),a
p138_exit:
 pop hl
 pop de
 pop bc
 pop af
 /* Return to the unchanged default callback dispatcher. */
 ret
 __endasm;
}


/* Private controller fields occupy previously unused HRAM EC..F0. */
__at(0xffec) volatile uint8_t pub143_metadata_bank;
__at(0xffed) volatile uint8_t pub143_pending;
__at(0xffee) volatile uint8_t pub143_status;
__at(0xffef) volatile uint8_t pub143_job_count;
volatile uint8_t pub143_enabled;
