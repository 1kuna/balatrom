#pragma bank 255
#include "native_busy.h"
#include "native_modal.h"
#include "native_table.h"
#include "game.h"
#include "raster_runtime.h"
#include "a1_assets.h"
#include <gb/cgb.h>
#include <string.h>
void raster_capture_inputs(void) NONBANKED;
#define COOP_BUSY_IMPLEMENTATION
#include "cooperative_native_redirect.h"
/* 0..18 prepare one inactive row per idle iteration;19 ready;20 visible.
 * Slots242..245 are outside all198 retained and42 mutable native patterns.
 * Legacy modal owns bank0/9800. Native TABLE publishes into9800 as well. */
/* Tail byte143 is outside the139-byte NativeCache. Access only while
 * acquiring or holding the native exclusive144-byte raster_loads lease.
 * Foreign raster owners may overwrite it; install/publish reinitialize. */
#define progress raster_loads[143]
static const uint8_t patterns[64]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,0,10,0,14,0,14,0,10,0,0,0,0,0,0,0,78,0,164,0,228,0,164,0,174,0,0,0,0,0,0,0,224,0,64,0,64,0,64,0,64,0,0,0,0};
/* Keep the ROM read in this bank. coop_data is BANKED, so its source must
 * remain addressable after the callee changes ROM banks. Caller-stack RAM is
 * owned for the entire synchronous upload; no shared scratch lease is used. */
static void native_busy_wait_patterns(void){
 uint8_t upload[64];
 memcpy(upload,patterns,sizeof(upload));
 set_bkg_data(242u,4u,upload);
}
/* WAIT has its own four patterns and inactive map. Readiness does not lend
 * partially resident cards. Foreign scene/register owners deny preparation. */
static uint8_t safe_wait(const GameState *g){
 return _cpu==CGB_TYPE&&g==&game&&g->phase==PHASE_TABLE&&g->ui_mode<=1u&&
  !raster_live&&!raster_fault&&!(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))&&
  (LCDC_REG&LCDCF_BG8000)&&!SCX_REG&&!SCY_REG&&
  native_table_bank1_owner>=NATIVE_BANK1_VALID&&native_table_bank1_owner<=198u;
}
/* the legacy felt curtain borrows the inactive 0x9C00 map;
 * drop WAIT readiness so TABLE idle rebuilds it before a modal uses it as the
 * window cover (otherwise the cover showed the curtain's tiles under TABLE
 * palettes: a white frame). Never while WAIT is visible (progress 20). */
void native_busy_wait_invalidate(void) BANKED {if(progress!=20u)progress=0;}
void native_busy_prepare_wait(const GameState *g) BANKED {
 uint8_t row;
 if(progress==20u)return; /* Already visible: never edit the WAIT map. */
 if(!safe_wait(g)){progress=0;return;}
 if(progress==19u)return;
 progress=0;VBK_REG=1;native_busy_wait_patterns();VBK_REG=0;
 for(row=0;row<18u;row++){
  if(!safe_wait(g)){progress=0;VBK_REG=0;return;}
  memset(raster_pixels,242,20u);
  if(row==8u){raster_pixels[8]=243u;raster_pixels[9]=244u;raster_pixels[10]=245u;}
  set_tiles(0,row,20u,1u,(uint8_t *)0x9c00u,raster_pixels);
  memset(raster_pixels,15u,20u);VBK_REG=1;
  set_tiles(0,row,20u,1u,(uint8_t *)0x9c00u,raster_pixels);VBK_REG=0;
  raster_capture_inputs();
 }
 progress=19u;
}
/* State199 belongs only to TITLE's inactive WAIT preparation. It is never
 * accepted as card residency. A real TITLE A-resume may transfer a complete
 * WAIT lease; the following TABLE install still loads all198. */
#define TITLE_WAIT_OWNER 199u
static uint8_t safe_title(void){
 return _cpu==CGB_TYPE&&game.phase==PHASE_TITLE&&!native_table_live&&
  !raster_live&&!raster_fault&&(LCDC_REG&LCDCF_ON)&&
  !(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))&&(LCDC_REG&LCDCF_BG8000)&&!SCX_REG&&!SCY_REG;
}
void native_busy_title_resume(void) BANKED {
 /* Preserve genuine TITLE origin even before its WAIT lease is complete.
  * State199 marks origin only; it says nothing about card or WAIT readiness. */
 if(safe_title()){
  if(native_table_bank1_owner!=TITLE_WAIT_OWNER||progress>19u)progress=0;
  native_table_bank1_owner=TITLE_WAIT_OWNER;
 }else {native_table_bank1_owner=NATIVE_BANK1_INVALID;progress=0;}
}
void native_busy_idle(void) BANKED {
 uint8_t row,unit,title=game.phase==PHASE_TITLE;
 if(title){
  if(!safe_title())return;
  if(native_table_bank1_owner!=TITLE_WAIT_OWNER){native_table_bank1_owner=TITLE_WAIT_OWNER;progress=0;}
 }else{
  if(native_table_bank1_owner>=NATIVE_BANK1_COLD_PREPARING)native_modal_prime_idle();
  if(_cpu!=CGB_TYPE||game.phase!=PHASE_TABLE||raster_live||raster_fault)return;
  if(!((!game.ui_mode&&native_table_live&&native_table_bank1_owner==NATIVE_BANK1_VALID)||
       (game.ui_mode==1u&&!native_table_live&&native_table_bank1_owner==NATIVE_BANK1_SUSPENDED)))return;
  if(!(LCDC_REG&LCDCF_ON)||(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))||!(LCDC_REG&LCDCF_BG8000)||SCX_REG||SCY_REG)return;
 }
 if(progress>=19u)return;
 if(!progress){VBK_REG=1;native_busy_wait_patterns();VBK_REG=0;progress=1;}
 /* TITLE uses one inactive row per slice and samples input after each row. */
 for(unit=0;unit<(title?1u:4u)&&progress<19u;unit++){
 row=progress-1u;memset(raster_pixels,242,20u);
 if(row==8u){raster_pixels[8]=243u;raster_pixels[9]=244u;raster_pixels[10]=245u;}
 set_tiles(0,row,20u,1u,(uint8_t *)0x9c00u,raster_pixels);
 memset(raster_pixels,15u,20u);VBK_REG=1;
 set_tiles(0,row,20u,1u,(uint8_t *)0x9c00u,raster_pixels);VBK_REG=0;
 progress++;raster_capture_inputs();
 }
}
/* Caller has waited for VBlank. No register/window/scroll foreign owner. */
uint8_t native_busy_begin(const GameState *g) BANKED {
 /* A cold modal may be displayed before WAIT is ready. Prepare its inactive
  * map while that modal remains visible, inside this new input transaction.
  * Never extend a completed modal publication's stale-hand lock. */
 if(g==&game&&g->phase==PHASE_TABLE&&!g->ui_mode&&progress<19u&&
    (native_table_bank1_owner==NATIVE_BANK1_SUSPENDED||
     (native_table_bank1_owner>=NATIVE_BANK1_COLD_PREPARING&&native_table_bank1_owner<=198u))&&
    !native_table_live&&!raster_live&&!raster_fault&&(LCDC_REG&LCDCF_ON)&&
    !(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))&&(LCDC_REG&LCDCF_BG8000)&&!SCX_REG&&!SCY_REG){
  native_busy_prepare_wait(g);
  if(progress==19u)wait_vbl_done();
 }
 /* A completed TITLE WAIT survives the normal initial LCD-off TABLE build.
  * TITLE is not a modal scene, so it never lends WAIT as a visible cover. */
 if(g==&game&&g->phase==PHASE_TABLE&&!g->ui_mode&&progress==19u&&
    native_table_bank1_owner==TITLE_WAIT_OWNER&&!raster_live&&!raster_fault&&
    (LCDC_REG&LCDCF_ON)&&!(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))&&
    (LCDC_REG&LCDCF_BG8000)&&!SCX_REG&&!SCY_REG)return 0;

 if(g!=&game||g->phase!=PHASE_TABLE||g->ui_mode||progress!=19u||
    !(native_table_bank1_owner==NATIVE_BANK1_SUSPENDED||
      (native_table_bank1_owner>=NATIVE_BANK1_COLD_PREPARING&&native_table_bank1_owner<=198u))||
    raster_live||raster_fault||!(LCDC_REG&LCDCF_ON)||
    (LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))||!(LCDC_REG&LCDCF_BG8000)||SCX_REG||SCY_REG){progress=0;return 0;}
 a1_palette_copy(raster_pixels);set_bkg_palette(7u,1u,(uint16_t *)(raster_pixels+32u));
 HIDE_SPRITES;HIDE_WIN;LCDC_REG|=LCDCF_BG8000|LCDCF_BG9C00;progress=20u;return 1;
}
void native_busy_install_base(void) BANKED {
 uint8_t id,y;
 VBK_REG=0;
 for(id=0;id<A1_UI_COUNT;id++){a1_ui_tile_copy(id,raster_pixels);set_bkg_data(id,1,raster_pixels);raster_capture_inputs();}
 for(y=0;y<18u;y++){
  a1_ui_map_row_copy(y,raster_pixels);
  set_tiles(0,y,20u,1u,(uint8_t *)0x9800u,raster_pixels);
  VBK_REG=1;set_tiles(0,y,20u,1u,(uint8_t *)0x9800u,raster_pixels+20u);VBK_REG=0;
  raster_capture_inputs();
 }
}
void native_busy_publish(void) BANKED {
 if(progress==20u)wait_vbl_done();
 LCDC_REG&=~LCDCF_BG9C00;progress=progress>=19u?19u:0u;
}

/* Legacy owner ROM pointer stays in this bank; no cooperative redirect. */
#undef set_bkg_data
#include "native_table.h"
#include "owner_legacy_cards.h"
#include "owner_legacy_cards.inc"
#include "shop154.h"
#include <gb/cgb.h>
void raster_capture_inputs(void) NONBANKED;
void owner_legacy_cards_load(void) BANKED {
 uint16_t i;uint8_t n,saved=VBK_REG;
 native_table_bank1_owner=NATIVE_BANK1_INVALID;VBK_REG=1;
 for(i=0;i<sizeof(owner_legacy_tiles)/16u;i+=8u){n=(sizeof(owner_legacy_tiles)/16u-i)>8u?8u:(uint8_t)(sizeof(owner_legacy_tiles)/16u-i);raster_capture_inputs();shop154_data((uint8_t)i,n,owner_legacy_tiles+i*16u);}
 VBK_REG=saved;
}
void owner_legacy_card_map(uint8_t face,uint8_t *map,uint8_t *attrs) BANKED {
 uint8_t j;const uint8_t *p;if(face>=55u)face=54u;p=owner_legacy_maps+(uint16_t)face*16u;
 for(j=0;j<8u;j++){map[j]=p[j*2u];attrs[j]=p[j*2u+1u];}
}

/* Complete existing WAIT handoff for LOSS restart or WIN Continue before result commit.
 * Tail242..245 and palette7 are reserved outside the legacy title assets.
 * The entire inactive map is rebuilt before the VBlank switch. */
void native_palette_stop(void) NONBANKED;
void frontend_legacy_invalidate(void) BANKED;
uint8_t native_busy_loss_restart(const GameState *g) BANKED {
 uint8_t row;
 if(_cpu!=CGB_TYPE||g!=&game||(game.phase!=PHASE_TITLE&&game.phase!=PHASE_SHOP)||!native_table_live||raster_live||raster_fault||native_table_bank1_owner!=NATIVE_BANK1_VALID||(LCDC_REG&0x98u)!=0x90u||!(LCDC_REG&LCDCF_ON)||(LCDC_REG&LCDCF_WINON)||SCX_REG||SCY_REG)return 0;
 /* Reuse only the complete TABLE WAIT lease for early WIN Continue.
  * Result publication revokes readiness before touching the inactive map.
  * All other paths retain the complete synchronous rebuild. */
 if(game.phase!=PHASE_SHOP||progress!=19u){
 VBK_REG=1;native_busy_wait_patterns();VBK_REG=0;
 for(row=0;row<18u;row++){
  memset(raster_pixels,242u,20u);if(row==8u){raster_pixels[8]=243u;raster_pixels[9]=244u;raster_pixels[10]=245u;}
  set_tiles(0,row,20u,1u,(uint8_t*)0x9c00u,raster_pixels);
  memset(raster_pixels,15u,20u);VBK_REG=1;set_tiles(0,row,20u,1u,(uint8_t*)0x9c00u,raster_pixels);VBK_REG=0;
 }
 }
 wait_vbl_done();native_palette_stop();HIDE_SPRITES;HIDE_WIN;LCDC_REG|=LCDCF_BG8000|LCDCF_BG9C00;native_table_live=0;progress=20u;frontend_legacy_invalidate();return 1;
}
