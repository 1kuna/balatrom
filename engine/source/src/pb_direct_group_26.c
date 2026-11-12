#ifdef __SDCC
#pragma bank 41
#include <gb/gb.h>
#else
#include <stdint.h>
#define BANKED
#endif
#include <string.h>
extern uint8_t raster_pixels[128],raster_planes[96];
static const uint8_t keys[60]={5,132,0,0,242,243,5,132,1,0,242,243,5,132,2,0,242,243,5,132,3,0,242,243,5,132,4,0,242,243,5,132,5,0,242,243,5,132,6,0,242,243,5,132,7,0,242,243,5,132,8,0,242,243,5,132,9,0,242,243};
static const uint8_t values[980]={0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,161,145,10,137,80,72,133,68,40,36,66,34,20,18,161,145,10,137,80,72,133,68,40,36,66,34,20,18,161,145,10,137,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,80,192,128,1,5,12,40,96,64,0,2,6,20,48,160,128,1,3,10,24,80,192,128,1,5,12,40,96,64,0,2,6,10,6,80,48,128,128,5,3,40,24,64,192,2,1,20,12,160,96,1,0,10,6,80,48,128,128,5,3,40,24,64,192,1,0,10,6,80,48,128,128,5,3,40,24,64,192,2,1,20,12,160,96,1,0,10,6,80,48,128,128,5,3,40,24,0,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,80,193,133,12,40,96,66,6,20,48,161,131,10,24,80,193,133,12,40,96,66,6,20,48,161,131,10,24,80,193,133,12,12,10,96,80,6,133,48,40,131,66,24,20,193,161,12,10,96,80,6,133,48,40,131,66,24,20,193,161,12,10,96,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,160,96,1,0,10,6,80,48,128,128,5,3,40,24,64,192,2,1,20,12,160,96,1,0,10,6,80,48,128,128,5,3,12,10,96,80,0,128,6,5,48,40,128,64,3,2,24,20,192,160,1,1,12,10,96,80,0,128,6,5,48,40,128,64,1,1,12,10,96,80,0,128,6,5,48,40,128,64,3,2,24,20,192,160,1,1,12,10,96,80,0,128,6,5,48,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const uint8_t indexes[128]={255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,0,0,0,0,12,0,196,0,18,0,38,1,6,0,98,0,48,0,16,3,30,0,234,1,42,0,174,2,36,0,76,2,54,0,114,3,24,0,136,1};
static const uint8_t displacements[64]={0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,2,0,0,0,4,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,0,0,0,0,0};

uint8_t pb_direct_read_26(uint16_t slot,uint16_t *recipe) BANKED {
 uint16_t at,offset;
 if(slot>=32u || !recipe)return 0;
 at=slot*4u;
 if(indexes[at]==255u && indexes[at+1u]==255u)return 0;
 offset=(uint16_t)indexes[at+2u]|((uint16_t)indexes[at+3u]<<8);
 *recipe=(uint16_t)values[offset]|((uint16_t)values[offset+1u]<<8);
 memcpy(raster_planes,values+offset+2u,96u);return 1;
}
uint8_t pb_direct_group_26(uint8_t length,uint16_t *recipe) BANKED {
 uint16_t h1,h2,bucket,at,displacement,slot,offset;
 if(!length || length>119u || !recipe)return 0;
 h1=(uint16_t)raster_pixels[124]|((uint16_t)raster_pixels[125]<<8);
 h2=(uint16_t)raster_pixels[126]|((uint16_t)raster_pixels[127]<<8);
 bucket=(h1^(h1>>5u))&31u;at=bucket*2u;
 displacement=(uint16_t)displacements[at]|((uint16_t)displacements[at+1u]<<8);
 slot=(h2+displacement)&31u;at=slot*4u;
 offset=(uint16_t)indexes[at]|((uint16_t)indexes[at+1u]<<8);
 if(offset==65535u || keys[offset]!=length || memcmp(keys+offset+1u,raster_pixels,length))return 0;
 if(!pb_direct_read_26(slot,recipe))return 0;
 raster_pixels[120]=(uint8_t)slot;raster_pixels[121]=(uint8_t)((slot>>8)|104u);
 return 1;
}

#include "a1_ui_runtime.h"
#include "raster_runtime.h"
#include <string.h>
/* Five160-byte row keys: WRAM4 D880..DB9F, after inventory D870..D874,
 * before input epochDFD0/native RNGDFE0. Public strings only, no hash/pointer.
 * Native install resets all keys; every overlapping alternate writer denies
 * residency. No SRAM or game state field is added. */
#include "cooperative_native.h"
#include "publication143.h"
#define NRK_BASE 0xd880u
/* Prospective public row keys: bank3 D900..DC1F, above palette metadata
 * D300..D8FF and below the bank3 IRQ stack floor DFA2. Only main uses it. */
#define NRK_STAGE 0xd900u
static uint8_t nrk_staged;
static uint8_t nrk_bank(uint8_t row){return coop_active&&(nrk_staged&(1u<<row))?3u:4u;}
static uint16_t nrk_at(uint8_t row){return (nrk_bank(row)==3u?NRK_STAGE:NRK_BASE)+(uint16_t)row*160u;}
static void nrk_put(uint8_t row,const uint8_t*data,uint8_t n){if(coop_active){nrk_staged|=1u<<row;raster_put_block(3u,NRK_STAGE+(uint16_t)row*160u,data,n);}else raster_put_block(4u,NRK_BASE+(uint16_t)row*160u,data,n);}
static void nrk_publish(void){uint8_t row,buf[16],length,part;uint16_t at;
 for(row=0;row<5u;row++)if(nrk_staged&(1u<<row)){
  raster_get_block(3u,NRK_STAGE+(uint16_t)row*160u,buf,3u);
  /* Keys declare their compared length in byte2; invalidation only byte0.
   * Trailing bytes are never compared before the length/header check. */
  length=buf[0]==185u?buf[2]:1u;
  if(!length||length>160u)length=160u;
  for(at=0;at<length;at+=part){part=length-at>16u?16u:length-at;
   raster_get_block(3u,NRK_STAGE+(uint16_t)row*160u+at,buf,part);
   raster_put_block(4u,NRK_BASE+(uint16_t)row*160u+at,buf,part);
  }
 }nrk_staged=0;
}
static void nrk_clear(uint8_t row){uint8_t zero=0;nrk_put(row,&zero,1u);}
void native_ui_rows_reset(void) BANKED {uint8_t i;for(i=0;i<5u;i++)nrk_clear(i);}
uint8_t native_ui_rows(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const A1Text*f,uint8_t count,uint8_t plain) BANKED {
 uint8_t row,i,len,n,j,part,dirty=0,key[160],old[16];int16_t dy;
 if(x!=5u||y!=5u||w!=15u||h!=5u||count>9u){
  if(x<20u&&x+w>5u)for(row=5u;row<10u;row++)if(y<=row&&y+h>row)nrk_clear(row-5u);
  return 255u;
 }
 for(row=0;row<5u;row++){
  if(coop_active&&!coop_checkpoint())return 255u;
  key[0]=185u;key[1]=plain;key[2]=0;n=3u;
  for(i=0;i<count;i++){
   dy=(int16_t)f[i].y-(int16_t)(row+5u)*8;
   if(dy>=8||dy+(f[i].font==5u?7u:5u)<=0)continue;
   len=strlen(f[i].s);
   if((uint16_t)n+5u+len>160u)break;
   key[n++]=f[i].x;key[n++]=f[i].y;key[n++]=f[i].font;key[n++]=f[i].color;key[n++]=len;
   memcpy(key+n,f[i].s,len);n+=len;
  }
  if(i<count){dirty|=1u<<row;nrk_clear(row);continue;}
  key[2]=n;
  for(j=0;j<n;j+=part){
   part=n-j>16u?16u:n-j;
   raster_get_block(nrk_bank(row),nrk_at(row)+j,old,part);
   if(memcmp(key+j,old,part)){dirty|=1u<<row;break;}
  }
  if(dirty&(1u<<row))nrk_put(row,key,n);
 }
 return dirty;
}

/* Private cooperative native transaction. No rule/RNG calls and no SRAM.
 * Bank5/6 stage all256 tile slots per VBK; bank7D200..DBAF holds maps,
 * OAM, palettes and the published cache backup. SB bank7D000..D1FF survives.
 * Bank3D300..D8FF is prospective native palette/ownership metadata.
 * Only final bounded LCD-off publication replaces any visible data. */
#include "cooperative_native.h"
#include "native_table.h"
#include "native_busy.h"
#include "raster_runtime.h"
#include <gb/cgb.h>
#include <string.h>
extern uint8_t native_palette_ready,coop_lcdc_held,coop_ready_held,coop_live_held;
extern volatile uint8_t input_queue[16],input_head,input_count;
void raster_capture_inputs(void) NONBANKED;
void native_palette_stop(void) NONBANKED;
void native_palette_start(void) NONBANKED;
void frontend_native_acquire(void) BANKED;
void native_ui_rows_reset(void) BANKED;
void coop_publish_metadata(void) NONBANKED;
uint8_t coop_resident;
static uint8_t coop_old_owner,coop_old_resident,coop_journal_valid,coop_oam_touched;
static uint8_t coop_last_poll;
/* Main sets this only for a direction press on an already resident TABLE.
 * Protection never suppresses capture, generation checks or action barriers. */
uint8_t coop_navigation_protected,coop_navigation_clean,coop_navigation_burst;
static uint8_t coop_navigation_started;
#include "cursor_navigation_queue.inc"
void coop_commit_begin(void) BANKED {coop_committing=1;}
void coop_commit_end(void) BANKED {coop_committing=0;coop_navigation_protected=0;coop_navigation_clean=1;coop_navigation_burst=input_count?(coop_navigation_burst&128u):0;}
uint8_t coop_checkpoint(void) BANKED {
 if(!coop_active||coop_committing)return 1;
 /* IRQ/audio are observers. Main input/load increments generation before
  * every transaction; no GameState mutation occurs here. */
 if(coop_generation!=coop_stage_generation||game.phase!=PHASE_TABLE||game.ui_mode){coop_cancelled=1;return 0;}
 if(coop_last_poll!=(uint8_t)sys_time){coop_last_poll=(uint8_t)sys_time;raster_capture_inputs();}
 if(input_count&&(!coop_navigation_protected||!coop_live_held||
    (uint8_t)((uint8_t)sys_time-coop_navigation_started)>=24u||
    !cursor_navigation_queue(input_queue,input_head,input_count))){
  coop_cancelled=1;return 0;
 }
 return !coop_cancelled;
}
uint8_t coop_begin(const GameState *g) BANKED {
 uint16_t at;uint8_t buf[32],n;
 pub139_record_enabled=0;
 if(_cpu!=CGB_TYPE||g!=&game||g->phase!=PHASE_TABLE||g->ui_mode||raster_live||raster_fault){coop_navigation_protected=0;return 0;}
 /* Complete the backup before enabling cancellable work. */
 for(at=0;at<144u;at+=16u)raster_put_block(7u,0xdb20u+at,raster_loads+at,16u);
 coop_old_owner=native_table_bank1_owner;coop_old_resident=coop_resident;nrk_staged=0;coop_journal_valid=0;coop_oam_touched=0;
 coop_cancelled=coop_committing=0;coop_stage_generation=coop_generation;
 coop_lcdc_held=LCDC_REG;coop_ready_held=native_palette_ready;coop_live_held=coop_resident&&native_table_live&&native_table_bank1_owner==NATIVE_BANK1_VALID?1u:0u;
 coop_last_poll=coop_navigation_started=(uint8_t)sys_time;coop_accepting=coop_active=1;
 for(at=0xd300u;at<(coop_live_held?0xd400u:0xd900u);at+=32u){
  /* bank2 is virtualized to3 while active, so disable mapping for source. */
  coop_active=0;raster_get_block(2u,at,buf,32u);coop_active=1;
  raster_put_block(3u,at,buf,32u);if(!coop_checkpoint())break;
 }
 if(pub143_enabled&&!coop_cancelled){pub143_begin();coop_journal_valid=pub139_record_enabled;}
 if(!coop_cancelled&&!coop_journal_valid)coop_old_resident=0;
 return 1;
}
/* Abort must restore prospective bytes too: changed-write recording compares
 * against this image, not visible VRAM. Leaving cancelled bytes here loses
 * journal entries on the next transaction. Read only journaled blocks using
 * the SDK mode-safe byte reader, with input capture between bounded blocks. */
/* Bank41 near leaf. Each VRAM read has a <=64T critical section.
 * A busy LCD mode retries with IRQs enabled; no whole mode3 wait is masked.
 * Caller is normal main code with IRQs enabled, SVBK1 stack and VBK owned. */
/* Restore a tracked block directly from visible VRAM into its prospective
 * WRAM bank. The existing main-owned HRAM ferry byte preserves data across
 * SVBK switching; stack/IRQ access resumes only after SVBK1 is restored. */
static void coop_restore_block(uint8_t bank,uint16_t target,uint16_t source) OLDCALL __naked {
 __asm
 ldhl sp,#2
 ld a,(hl+)
 ld c,a
 ld a,(hl+)
 ld e,a
 ld a,(hl+)
 ld d,a
 ld a,(hl+)
 ld h,(hl)
 ld l,a
 ld b,#16
c186_read:
 di
 ldh a,(0x41)
 and #2
 jr nz,c186_wait
 ld a,(hl+)
 ldh (0xd2),a
 ld a,c
 ldh (0x70),a
 ldh a,(0xd2)
 ld (de),a
 ld a,#1
 ldh (0x70),a
 ei
 inc de
 dec b
 jr nz,c186_read
 ret
c186_wait:
 ei
 nop
 jr c186_read
 __endasm;
}
static void coop_restore_image(void){uint8_t bits[80],buf[16],bucket,bit,vbk,marked;uint16_t block,source,target;
 raster_get_block(7u,0xdbb0u,bits,80u);
 for(bucket=0;bucket<80u;bucket++){marked=bits[bucket];for(bit=0;marked;bit++,marked>>=1)if(marked&1u){
  block=(uint16_t)(bucket&31u)*8u+bit;
  if(bucket<64u){vbk=bucket>=32u;source=0x8000u+block*16u;target=0xd000u+block*16u;}
  else{block=(uint16_t)(bucket-64u)*8u+bit;vbk=block>=64u;source=0x9800u+(block&63u)*16u;target=0xd200u+block*16u;}
  VBK_REG=vbk;coop_restore_block(bucket<64u?(vbk?6u:5u):7u,target,source);VBK_REG=0;raster_capture_inputs();
 }}
 /* Ordinary cursor staging touches only sprite0. Alternate inventory writers
  * retain the full rollback; untouched OAM requires no copy. */
 if(coop_oam_touched==1u){memcpy(buf,(uint8_t*)0xc000u,4u);raster_put_block(7u,0xda00u,buf,4u);}
 else if(coop_oam_touched)for(block=0;block<160u;block+=16u){memcpy(buf,(uint8_t*)0xc000u+block,16u);raster_put_block(7u,0xda00u+block,buf,16u);}
}
/* Published cache is in fixed WRAM C1xx; backup lives in bank7 DB20.
 * Copy in the existing16-byte IRQ-safe ferry extent without HRAM bounce.
 * Restore SVBK1 before enabling IRQs, input polling, or touching the stack. */
static void coop_restore_cache(void) OLDCALL __naked {
 __asm
 ld hl,#0xdb20
 ld de,#_raster_loads
 ld b,#9
c185_chunk:
 push bc
 push de
 push hl
 call _raster_ferry_window
 pop hl
 pop de
 pop bc
 di
 ld a,#7
 ldh (0x70),a
 ld c,#16
c185_byte:
 ld a,(hl+)
 ld (de),a
 inc de
 dec c
 jr nz,c185_byte
 ld a,#1
 ldh (0x70),a
 ei
 push bc
 push de
 push hl
 call _raster_capture_inputs
 pop hl
 pop de
 pop bc
 dec b
 jr nz,c185_chunk
 ret
 __endasm;
}
static void coop_restore(void){
 coop_navigation_protected=coop_navigation_clean=coop_navigation_burst=0;
 pub139_record_enabled=0;
 if(coop_old_resident&&coop_journal_valid)coop_restore_image();
 coop_restore_cache();
 /* Visible VRAM and its saved cache were not modified by cancelled staging. */
 native_table_bank1_owner=coop_old_owner;coop_resident=coop_old_resident;coop_active=0;nrk_staged=0;if(!coop_old_resident)native_ui_rows_reset();
}
#include "coop_blank_vblank.inc"
/* felt curtain. A cold (non-resident) commit used to turn
 * the LCD off for its 640-block full transfer (black in capture, white on a
 * CGB panel), then show black frames. Instead every BG/OBJ palette becomes
 * the TABLE felt colour, so the screen is one solid felt field while the LCD
 * stays on; the same staging is copied by VBlank-bounded GDMA chunks; the
 * real palettes return in one VBlank. Content, order and staging are those
 * of the original transfer. Double-speed only; otherwise the original path. */
extern __at(0xffbc) volatile uint16_t raster_address;
extern __at(0xffea) volatile uint8_t raster_ferry_bank;
extern __at(0xffe2) volatile uint16_t raster_dma_destination;
extern __at(0xffe4) volatile uint8_t raster_dma_vbk;
extern __at(0xffe5) volatile uint8_t raster_dma_blocks;
/* Same leaf as the LCD-off transfer: HRAM inputs only, no stack use while
 * the source WRAM bank is mapped; GDMA halts the CPU until complete. */
static void curtain_gdma_raw(void) __naked {
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
 ldh (0x55),a
 ld a,#1
 ldh (0x70),a
 xor a
 ldh (0x4f),a
 pop af
 ret
 __endasm;
}
/* TABLE felt RGB(4,13,9) as little-endian RGB555 bytes. Direct auto-increment
 * palette writes: the SDK palette helpers are too slow for one VBlank. */
#define FELT_LO ((uint8_t)(4u|(13u<<5)))
#define FELT_HI ((uint8_t)(((13u<<5)|(9u<<10))>>8))
static void curtain_felt_palettes(void){uint8_t n=64u;BCPS_REG=0x80u;OCPS_REG=0x80u;do{BCPD_REG=FELT_LO;BCPD_REG=FELT_HI;OCPD_REG=FELT_LO;OCPD_REG=FELT_HI;n-=2u;}while(n);}
static const uint8_t curtain_bank[6]={5u,5u,6u,6u,7u,7u},curtain_vbk[6]={0,0,1u,1u,0,1u},curtain_count[6]={128u,128u,128u,128u,64u,64u};
static const uint16_t curtain_source[6]={0xd000u,0xd800u,0xd000u,0xd800u,0xd200u,0xd600u},curtain_dest[6]={0x8000u,0x8800u,0x8000u,0x8800u,0x9800u,0x9800u};
/* Enter at the first VBlank line with IRQs masked: wait (IRQs on) for LY143,
 * then spin to LY144 inside the caller's critical block. The masked VBlank
 * callbacks run once afterwards (pending publication 0, native palettes
 * stopped, sprites felt-coloured), exactly one per frame as before. */
static void curtain_line143(void){while(LY_REG!=143u){}}
static void curtain_publish(void){
 uint8_t region=0,done=0,budget,n;
 curtain_line143();
 CRITICAL {
  while(LY_REG!=144u){}
  /* Stop native palette jobs here, with the felt write, so the old frame
   * finishes with its mid-frame palettes (a mid-frame stop drew one frame of
   * wrong colours on a TABLE->TABLE cold install, e.g. boss-29 re-debuff). */
  native_palette_stop();curtain_felt_palettes();
  LCDC_REG=coop_lcdc_held|LCDCF_ON;VBK_REG=0;SCX_REG=SCY_REG=0;
 }
 frontend_native_acquire();
 while(region<6u){
  curtain_line143();
  CRITICAL {
   while(LY_REG!=144u){}
   /* ~14 blocks per double-speed line: 120 blocks incl. per-chunk set-up
    * end by LY152, inside VBlank. */
   budget=120u;
   while(budget>8u&&region<6u){
    n=curtain_count[region]-done;if(n>budget-4u)n=budget-4u;
    raster_address=curtain_source[region]+(uint16_t)done*16u;raster_ferry_bank=curtain_bank[region];
    raster_dma_destination=curtain_dest[region]+(uint16_t)done*16u;raster_dma_vbk=curtain_vbk[region];raster_dma_blocks=n-1u;
    curtain_gdma_raw();
    budget-=n+4u;done+=n;if(done==curtain_count[region]){region++;done=0;}
   }
  }
 }
}
uint8_t coop_finish(void) BANKED {
 uint8_t buf[16],i,prepared=0,started=0;uint16_t at;
 if(!coop_active)return 1;
 if(!coop_checkpoint()){coop_restore();return 0;}
 /* staging is complete and main has not dispatched the
  * queued press, so the staged frame is the current state. A press that
  * lands during prepare no longer discards it: discarding forced the next
  * transaction to republish two steps at once, whose journal could exceed
  * the atomic VBlank budget and fall back to the LCD-off full transfer.
  * Generation/phase cannot change here (main owns dispatch). An unprepared
  * finish keeps the original cancel check. */
 if(pub143_enabled){coop_committing=1;prepared=pub143_prepare();coop_committing=0;}
 /* Prepared atomic finish waits for its own publication VBlank. */
 if(!prepared){wait_vbl_done();if(!coop_checkpoint()){coop_restore();return 0;}}
 pub139_record_enabled=0;
 coop_commit_begin();
 if(prepared&&pub143_finish()){nrk_publish();coop_resident=1;coop_commit_end();coop_accepting=0;return 1;}
 if((LCDC_REG&LCDCF_ON)&&(KEY1_REG&128u)){
  curtain_publish();
  coop_publish_metadata();
  for(at=0;at<160u;at+=16u){raster_get_block(7u,0xda00u+at,buf,16u);memcpy((uint8_t*)0xc000u+at,buf,16u);}
  /* BG 0xdaa0..0xdadf and OBJ 0xdae0..0xdb1f staging are contiguous. */
  raster_get_block(7u,0xdaa0u,raster_pixels,128u);
  native_table_live=coop_live_held;coop_active=0;
  /* Reveal in one VBlank: staged palettes in the original order, then the
   * native VBlank palette set and LYC chain, so the first lit frame is the
   * complete TABLE (no frame without its mid-frame palette jobs). */
  /* The VBlank callbacks run first (unmasked) so nothing pending can delay
   * the LY7 job; the fast writes below then finish inside the same VBlank. */
  do{wait_vbl_done();}while(LY_REG<144u||LY_REG>147u);
  CRITICAL {
   {uint8_t n;const uint8_t *q;
    /* BG first: native_vblank (inside native_palette_start) writes all 64 BG
     * bytes and OBJ palettes 1..2; staged OBJ 0 and 3..7 follow, giving the
     * same final palette RAM as the original staged-then-native order. */
    if(coop_ready_held){native_palette_start();started=1u;
     q=raster_pixels+64u;OCPS_REG=0x80u;n=8u;do{OCPD_REG=*q++;}while(--n);
     q=raster_pixels+88u;OCPS_REG=0x98u;n=40u;do{OCPD_REG=*q++;}while(--n);}
    else{q=raster_pixels;BCPS_REG=0x80u;n=128u;do{if(n==64u)OCPS_REG=0x80u;if(n>64u)BCPD_REG=*q++;else OCPD_REG=*q++;}while(--n);}}
  }
 }else{
 native_palette_stop();coop_blank_vblank();frontend_native_acquire();
 raster_transfer(5u,0xd000u,0x8000u,0u,128u);raster_transfer(5u,0xd800u,0x8800u,0u,128u);
 raster_transfer(6u,0xd000u,0x8000u,1u,128u);raster_transfer(6u,0xd800u,0x8800u,1u,128u);
 raster_transfer(7u,0xd200u,0x9800u,0u,64u);raster_transfer(7u,0xd600u,0x9800u,1u,64u);
 coop_publish_metadata();
 for(at=0;at<160u;at+=16u){raster_get_block(7u,0xda00u+at,buf,16u);memcpy((uint8_t*)0xc000u+at,buf,16u);}
 for(i=0;i<8u;i++){raster_get_block(7u,0xdaa0u+(uint16_t)i*8u,buf,8u);set_bkg_palette(i,1u,(uint16_t*)buf);raster_get_block(7u,0xdae0u+(uint16_t)i*8u,buf,8u);set_sprite_palette(i,1u,(uint16_t*)buf);}
 native_table_live=coop_live_held;coop_active=0;
 }
 LCDC_REG=coop_lcdc_held|LCDCF_ON;VBK_REG=0;SCX_REG=SCY_REG=0;
 if(coop_ready_held&&!started)native_palette_start();
 nrk_publish();coop_resident=1;coop_commit_end();coop_accepting=0;return 1;
}
void coop_data(uint8_t first,uint8_t count,const uint8_t *src) BANKED {
 uint16_t at=(uint16_t)first*16u,left=count?(uint16_t)count*16u:4096u;uint8_t bank=5u+(VBK_REG&1u),n;
 if(!coop_active){set_bkg_data(first,count,src);return;}
 while(left){if(!coop_checkpoint())return;n=left>16u?16u:left;raster_put_block(bank,0xd000u+at,src,n);src+=n;at+=n;left-=n;}
}
void coop_sprite_data(uint8_t first,uint8_t count,const uint8_t *src) BANKED {if(!coop_active){set_sprite_data(first,count,src);return;}coop_data(first,count,src);}
void coop_tiles(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t *map,const uint8_t *src) BANKED {
 uint8_t row;uint16_t at=0xd200u+(VBK_REG&1u)*1024u+(uint16_t)y*32u+x;
 if(!coop_active){set_tiles(x,y,w,h,map,src);return;}
 if(map!=(uint8_t*)0x9800u||x+w>32u||y+h>32u){coop_cancelled=1;return;}
 for(row=0;row<h;row++){if(!coop_checkpoint())return;raster_put_block(7u,at,src,w);src+=w;at+=32u;}
}
void coop_bkg_tiles(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *src) BANKED {if(!coop_active){set_bkg_tiles(x,y,w,h,src);return;}coop_tiles(x,y,w,h,(uint8_t*)0x9800u,src);}
static void coop_palette(uint8_t obj,uint8_t first,uint8_t count,const uint16_t *src){uint16_t at=0xdaa0u+obj*64u+(uint16_t)first*8u;uint8_t i;
 for(i=0;i<count;i++){if(!coop_checkpoint())return;raster_put_block(7u,at,(const uint8_t*)src,8u);src+=4u;at+=8u;}
}
void coop_bg_palette(uint8_t first,uint8_t count,const uint16_t *src) BANKED {if(!coop_active){set_bkg_palette(first,count,src);return;}coop_palette(0u,first,count,src);}
void coop_obj_palette(uint8_t first,uint8_t count,const uint16_t *src) BANKED {if(!coop_active){set_sprite_palette(first,count,src);return;}coop_palette(1u,first,count,src);}
void coop_capture_palette(uint8_t obj,uint8_t slot,uint8_t *out) BANKED {uint8_t i;
 if(coop_active){raster_get_block(7u,0xdaa0u+obj*64u+(uint16_t)slot*8u,out,8u);return;}
 for(i=0;i<8u;i++){if(obj){OCPS_REG=slot*8u+i;out[i]=OCPD_REG;}else{BCPS_REG=slot*8u+i;out[i]=BCPD_REG;}}
}
static void coop_oam(uint8_t id,uint8_t offset,uint8_t value){if(id)coop_oam_touched=2u;else if(!coop_oam_touched)coop_oam_touched=1u;raster_put_block(7u,0xda00u+(uint16_t)id*4u+offset,&value,1u);}
void coop_sprite_tile(uint8_t id,uint8_t tile) BANKED {if(!coop_active){set_sprite_tile(id,tile);return;}if(coop_checkpoint())coop_oam(id,2u,tile);}
void coop_sprite_prop(uint8_t id,uint8_t prop) BANKED {if(!coop_active){set_sprite_prop(id,prop);return;}if(coop_checkpoint())coop_oam(id,3u,prop);}
void coop_move_sprite(uint8_t id,uint8_t x,uint8_t y) BANKED {if(!coop_active){move_sprite(id,x,y);return;}if(coop_checkpoint()){coop_oam(id,0u,y);coop_oam(id,1u,x);}}
uint8_t coop_busy_begin(const GameState *g) BANKED {return coop_active?1u:native_busy_begin(g);}
void coop_busy_publish(void) BANKED {if(!coop_active)native_busy_publish();}
void coop_busy_prepare(const GameState *g) BANKED {if(!coop_active)native_busy_prepare_wait(g);}
void coop_acquire(void) BANKED {if(!coop_active)frontend_native_acquire();}

/* Private live overflow SRAM, independent of the durable save image.
 * Root4763bytes remains byte-compatible; shared typed overflow uses1682bytes.
 * No IRQ callback accesses SRAM. No RAM-bank switch beyond existing bank0.
 * Existing8/4 records remain in GameState, preserving its4735-byte ABI. */
#include "paged_inventory.h"
#include "raster_runtime.h"
#include "cooperative_native.h"
#include <gb/gb.h>
#include <string.h>
#define LIVE ((uint8_t*)PG_LIVE)
#define DURABLE ((uint8_t*)PG_DURABLE)
static uint8_t pg_hidden_count;
static void pg_enable(void){ENABLE_RAM_MBC5;SWITCH_RAM_MBC5(0);}
static uint8_t nj(uint16_t n){return n>8u?n-8u:0;}
static uint8_t nc(uint16_t n){return n>4u?n-4u:0;}
static uint16_t bytes(uint16_t j,uint16_t c){return (uint16_t)nj(j)*23u+(uint16_t)nc(c)*3u;}
uint8_t pg_fits(uint16_t j,uint16_t c) BANKED {return j<=PG_JMAX&&c<=PG_CMAX&&bytes(j,c)<=PG_PAYLOAD;}
uint8_t *pg_jptr(uint8_t i,uint8_t field) BANKED {pg_enable();return LIVE+32u+(uint16_t)(i-8u)*23u+field;}
uint8_t *pg_cptr(uint8_t i,uint8_t field) BANKED {pg_enable();return LIVE+32u+(uint16_t)LIVE[4]*23u+(uint16_t)(i-4u)*3u+field;}
void pg_reset(void) BANKED {pg_enable();memset(LIVE,0,PG_SPAN);LIVE[0]='P';LIVE[1]='G';LIVE[2]='I';LIVE[3]=1;pg_hidden_count=0;coop_resident=0;}
uint8_t pg_resize(uint8_t j,uint8_t c) BANKED {
 uint16_t oldj,newj,len;uint8_t *p;pg_enable();if(!pg_fits(j,c))return 0;
 oldj=(uint16_t)LIVE[4]*23u;newj=(uint16_t)nj(j)*23u;len=(uint16_t)LIVE[5]*3u;p=LIVE+32u;
 if(oldj!=newj){memmove(p+newj,p+oldj,len);if(newj>oldj)memset(p+oldj,0,newj-oldj);}
 if(nc(c)>LIVE[5])memset(p+newj+len,0,(uint16_t)(nc(c)-LIVE[5])*3u);
 LIVE[4]=nj(j);LIVE[5]=nc(c);return 1;
}
uint8_t pg_debuff(const GameState*g,uint8_t i) BANKED {return i<8u?((g->joker_debuff>>i)&1u):(*pg_jptr(i,22u)&1u);}
uint8_t pg_hidden(const GameState*g,uint8_t i) BANKED {return i<8u?((g->boss_flipped>>i)&1u):((*pg_jptr(i,22u)>>1)&1u);}
void pg_flag_set(GameState*g,uint8_t i,uint8_t hidden,uint8_t value) BANKED {
 uint8_t *p,mask;if(i<8u){p=hidden?&g->boss_flipped:&g->joker_debuff;mask=1u<<i;}else{p=pg_jptr(i,22u);mask=hidden?2u:1u;}
 if(hidden&&i>=8u&&!!(*p&mask)!=!!value){if(value)pg_hidden_count++;else pg_hidden_count--;}*p=(*p&~mask)|(value?mask:0u);
}
void pg_flags_clear(GameState*g,uint8_t hidden) BANKED {uint8_t i;if(hidden)g->boss_flipped=0;else g->joker_debuff=0;for(i=8;i<g->joker_count;i++)pg_flag_set(g,i,hidden,0);}
uint8_t pg_any_debuff(const GameState*g) BANKED {uint8_t i;for(i=0;i<g->joker_count;i++)if(pg_debuff(g,i))return 1;return 0;}
void pg_flags_remove(GameState*g,uint8_t index) BANKED {uint8_t i;for(i=index;i+1u<g->joker_count;i++){pg_flag_set(g,i,0,pg_debuff(g,i+1u));if(g->boss_id==28)pg_flag_set(g,i,1,pg_hidden(g,i+1u));}pg_flag_set(g,g->joker_count-1u,0,0);if(g->boss_id==28)pg_flag_set(g,g->joker_count-1u,1,0);}
void pg_flags_swap(GameState*g,uint8_t a,uint8_t b) BANKED {uint8_t x,y;x=pg_debuff(g,a);y=pg_debuff(g,b);pg_flag_set(g,a,0,y);pg_flag_set(g,b,0,x);if(g->boss_id==28){x=pg_hidden(g,a);y=pg_hidden(g,b);pg_flag_set(g,a,1,y);pg_flag_set(g,b,1,x);}}
uint32_t pg_hash(uint32_t h) BANKED {uint16_t i,n;pg_enable();n=32u+(uint16_t)LIVE[4]*23u+(uint16_t)LIVE[5]*3u;for(i=4;i<n;i++){h^=LIVE[i];h*=16777619ul;}return h;}
static void pg_hidden_recount(void){uint8_t i;pg_hidden_count=0;for(i=0;i<LIVE[4];i++)if(LIVE[32u+(uint16_t)i*23u+22u]&2u)pg_hidden_count++;}
uint8_t pg_any_hidden(const GameState*g) BANKED {return g->boss_flipped||pg_hidden_count;}
void pg_undo(uint8_t restore) BANKED {uint16_t i;uint8_t buf[16],n;coop_resident=0;pg_enable();for(i=0;i<PG_SPAN;i+=n){n=PG_SPAN-i>16u?16u:PG_SPAN-i;if(restore){raster_get_block(5u,0xd000u+i,buf,n);memcpy(LIVE+i,buf,n);}else{memcpy(buf,LIVE+i,n);raster_put_block(5u,0xd000u+i,buf,n);}}if(restore)pg_hidden_recount();}
static uint16_t crc(const uint8_t*p,uint16_t n){uint16_t c=0,i;for(i=0;i<n;i++)c=((c<<1)|(c>>15))^p[i];return c;}
uint8_t pg_save(uint16_t root) BANKED {uint16_t c,n;pg_enable();n=(uint16_t)LIVE[4]*23u+(uint16_t)LIVE[5]*3u;LIVE[6]=root;LIVE[7]=root>>8;c=crc(LIVE+32u,n);LIVE[8]=c;LIVE[9]=c>>8;memcpy(DURABLE,LIVE,PG_SPAN);return 1;}
uint8_t pg_load(const GameState*g,uint16_t root,uint8_t version) BANKED {uint16_t c,n;pg_enable();if(version!=14u){if(g->joker_count>8u||g->consumable_count>4u)return 0;pg_reset();return 1;}
 if(!pg_fits(g->joker_count,g->consumable_count)||DURABLE[0]!='P'||DURABLE[1]!='G'||DURABLE[2]!='I'||DURABLE[3]!=1u||DURABLE[4]!=nj(g->joker_count)||DURABLE[5]!=nc(g->consumable_count)||(DURABLE[6]|((uint16_t)DURABLE[7]<<8))!=root)return 0;
 n=bytes(g->joker_count,g->consumable_count);c=crc(DURABLE+32u,n);if(c!=(DURABLE[8]|((uint16_t)DURABLE[9]<<8)))return 0;memcpy(LIVE,DURABLE,PG_SPAN);pg_hidden_recount();coop_resident=0;return 1;}

#include "joker_effects.h"
#include "joker_repetition.h"
static void pg_scratch(uint8_t get,uint16_t at,void*data,uint8_t size){uint8_t n;uint8_t*p=data;while(size){n=size>16u?16u:size;if(get)raster_get_block(3u,at,p,n);else raster_put_block(3u,at,p,n);at+=n;p+=n;size-=n;}}
void pg_effect_put(uint8_t i,const void*p) BANKED {coop_resident=0;pg_scratch(0,0xd200u+(uint16_t)i*sizeof(JokerEffect),(void*)p,sizeof(JokerEffect));}
void pg_effect_get(uint8_t i,void*p) BANKED {pg_scratch(1,0xd200u+(uint16_t)i*sizeof(JokerEffect),p,sizeof(JokerEffect));}
void pg_recipe_put(uint8_t i,const void*p) BANKED {pg_scratch(0,0xd000u+(uint16_t)i*sizeof(JokerRepetitionRecipe),(void*)p,sizeof(JokerRepetitionRecipe));}
void pg_recipe_get(uint8_t i,void*p) BANKED {pg_scratch(1,0xd000u+(uint16_t)i*sizeof(JokerRepetitionRecipe),p,sizeof(JokerRepetitionRecipe));}

uint8_t *pg_jfield(const GameState*g,uint8_t i,uint8_t f) BANKED {if(i>=8u)return pg_jptr(i,f);switch(f){case 0:return (uint8_t*)&g->jokers[i];case 1:return (uint8_t*)&g->joker_state[i];case 9:return (uint8_t*)&g->joker_edition[i];case 10:return (uint8_t*)&g->joker_eternal[i];case 11:return (uint8_t*)&g->joker_sell[i];case 12:return (uint8_t*)&g->joker_uid[i];default:return (uint8_t*)&g->joker_extra_value[i];}}
uint8_t *pg_cfield(const GameState*g,uint8_t i,uint8_t f) BANKED {if(i>=4u)return pg_cptr(i,f);return (uint8_t*)(f==0?&g->consumable_kind[i]:f==1?&g->consumables[i]:&g->consumable_edition[i]);}

/* Each nested event owns an independent81UID batch, not a162byte C local.
 * WRAM6D000..DFD1 fits25batches. IRQ never reads it. Any written batch
 * revokes the prospective tile image; stack depth beyond tested paths is
 * unmeasured. The bound is physical scratch capacity, not inventory. */
static uint8_t pg_destroy_depth;
uint8_t pg_destroy_enter(void) BANKED {return pg_destroy_depth<25u?pg_destroy_depth++:255u;}
void pg_destroy_leave(void) BANKED {if(pg_destroy_depth)pg_destroy_depth--;}
void pg_destroy_put(uint8_t depth,uint8_t i,uint16_t uid) BANKED {coop_resident=0;raster_put_block(6u,0xd000u+((uint16_t)depth*81u+i)*2u,(const uint8_t*)&uid,2u);}
uint16_t pg_destroy_get(uint8_t depth,uint8_t i) BANKED {uint16_t uid;raster_get_block(6u,0xd000u+((uint16_t)depth*81u+i)*2u,(uint8_t*)&uid,2u);return uid;}

#include "writer_support140.inc"

#include "metadata_handoff139.inc"

#include "publication143_runtime.inc"

/* Skip the inactive compositor's hide loop only when its published carrier
 * ownership is absent and every carrier is already hidden. No palette or
 * tile-pattern residency is inferred from card geometry alone. */
uint8_t coop_plain_carriers_hidden(void) BANKED {
 uint8_t flag,i;
 if(!coop_active||!coop_live_held)return 0;
 raster_get_block(2u,0xd3afu,&flag,1u);if(flag)return 0;
 for(i=12u;i<40u;i++)if(shadow_OAM[i].x||shadow_OAM[i].y)return 0;
 return 1;
}

/* Protection belongs only to a focus-only transaction. Action/score/resource
 * redraws keep the original cooperative cancellation, avoiding elapsed-limit
 * restarts of a long preview paint after a navigation press. */
uint8_t coop_focus_only(uint8_t unsafe) BANKED {
 if(unsafe)coop_navigation_protected=0;
 return coop_checkpoint();
}

/* The caller lends a RAM public key. Text stays in this mapped bank while
 * formatting; the atomic focus compositor receives only RAM strings. */
#include "game.h"
#include "boss.h"
void native_focus_caption(const uint8_t *k,uint8_t count,uint8_t plain) BANKED {
 A1Text f[2];char caption[32],hidden[16];uint8_t n=0;
 static const char *const rank[15]={"?","?","TWO","THREE","FOUR","FIVE","SIX","SEVEN","EIGHT","NINE","TEN","JACK","QUEEN","KING","ACE"};
 static const char *const suit[4]={"SPADES","DIAMONDS","HEARTS","CLUBS"};
 if(!count)strcpy(caption,"NO CARDS");
 else if(k[0]&CARD_FACE_DOWN){
  strcpy(hidden,"FACE DOWN");f[n].s=hidden;f[n].x=84u-a1_text_width(hidden,3)/2u;f[n].y=66;f[n].font=3;f[n++].color=3;
  strcpy(caption,"FLIPS WHEN PLAYED");
 }else if((k[1]>>2)==ENH_STONE)strcpy(caption,"STONE CARD");
 else{strcpy(caption,rank[k[0]>>2]);strcat(caption," OF ");strcat(caption,suit[k[1]&3u]);}
 f[n].s=caption;f[n].x=plain?42u:84u-a1_text_width(caption,3)/2u;f[n].y=74;f[n].font=3;f[n++].color=2;
 if(count&&(k[0]&CARD_FACE_DOWN))a1_ui_publish_focus(5,8,11,2,f,n,0);
 else a1_ui_publish_focus(5,9,11,1,f,n,plain);
}

/* The first protected snapshot remains publishable. Drain only captured pure
 * navigation, bounded to16 edges. A no-op after draining may skip its paint
 * only while no unpublished action/cancel dirties the visible scene. */
#include "native_inventory_cache.h"
extern volatile uint8_t input_count,input_head,input_queue[16];
void frontend_table_transition_end(void) BANKED;
uint8_t queued_table_navigation(uint8_t pressed) BANKED {
 uint8_t next=0,pending=0;
 if((pressed!=J_LEFT&&pressed!=J_RIGHT)||!native_table_live||
    game.phase!=PHASE_TABLE||game.ui_mode||!game.hand_count){coop_navigation_burst=0;return 0;}
 CRITICAL {pending=input_count;if(pending)next=input_queue[input_head];}
 if(pending>=8u||!coop_navigation_clean)coop_navigation_burst|=128u;
 if((coop_navigation_burst&127u)<15u&&(next==J_LEFT||next==J_RIGHT)&&
    (pending>1u||game.focus==cache->focus||!coop_navigation_clean||(coop_navigation_burst&128u))){
  coop_navigation_burst++;return 1;
 }
 coop_navigation_burst&=128u;
 if(!pending&&coop_navigation_clean&&!coop_active&&!coop_cancelled&&!raster_fault&&
    coop_resident&&native_table_bank1_owner==NATIVE_BANK1_VALID&&
    game.focus==cache->focus){
  coop_navigation_protected=coop_accepting=0;frontend_table_transition_end();return 1;
 }
 return 0;
}
