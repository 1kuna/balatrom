#pragma bank 255
#include "native_modal.h"
#include "native_table.h"
#include "native_busy.h"
#include "raster_runtime.h"
#include "a1_assets.h"
#include <gb/cgb.h>
#include <string.h>
void frontend_modal_static(void) NONBANKED;
void frontend_modal_body(const GameState *) NONBANKED;
void render_game_banked(const GameState *,uint8_t,uint8_t) BANKED;
void raster_cache_legacy(uint8_t) BANKED;
void raster_legacy_reset(void) BANKED;
void frontend_legacy_invalidate(void) BANKED;
void raster_capture_inputs(void) NONBANKED;
/* Cold preparation states3..198 in the existing ownership byte are not validity.
 * Only its completed 196 units establish all 198 immutable native tiles. */
#define progress raster_loads[143]
static uint8_t safe_modal(const GameState *g){
 return _cpu==CGB_TYPE&&g==&game&&g->phase==PHASE_TABLE&&g->ui_mode==1u&&
  !raster_live&&!raster_fault&&(LCDC_REG&LCDCF_ON)&&
  !(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))&&(LCDC_REG&LCDCF_BG8000)&&!SCX_REG&&!SCY_REG;
}
static void clear_hidden_map(void){
 uint8_t y;
 memset(raster_pixels,0,40u);memset(raster_pixels+5u,84u,15u);
 for(y=0;y<18u;y++){
  if(y==16u)memset(raster_pixels+5u,0,15u);
  VBK_REG=0;set_tiles(0,y,20u,1u,(uint8_t *)0x9800u,raster_pixels);
  VBK_REG=1;set_tiles(0,y,20u,1u,(uint8_t *)0x9800u,raster_pixels+20u);
  VBK_REG=0;raster_capture_inputs();
 }
}
static void publish_legacy_owner(const GameState *g){
 if(safe_modal(g)&&!native_table_live){
  if(native_table_bank1_owner==NATIVE_BANK1_VALID||native_table_bank1_owner==NATIVE_BANK1_SUSPENDED)
   native_table_bank1_owner=NATIVE_BANK1_SUSPENDED;
  else if(native_table_bank1_owner<NATIVE_BANK1_COLD_PREPARING||native_table_bank1_owner==199u){
   if(native_table_bank1_owner!=199u)progress=0;
   native_table_bank1_owner=NATIVE_BANK1_COLD_PREPARING;
  }
  /* Visible legacy publication returns immediately; inactive WAIT work belongs
   * to idle or a later modal-to-TABLE admission, never the unlock tail. */
 }else native_table_bank1_owner=NATIVE_BANK1_INVALID;
 VBK_REG=0;
}
void native_modal_render(const GameState *g) BANKED {
 uint8_t old_window,old_wx,old_wy,old_owner;
 if(!safe_modal(g)||progress!=19u||
   !((native_table_live&&native_table_bank1_owner==NATIVE_BANK1_VALID)||
     (!native_table_live&&native_table_bank1_owner>=NATIVE_BANK1_SUSPENDED&&native_table_bank1_owner<=198u))){
  /* Cold/unprepared/foreign scene follows the original complete renderer. */
  native_table_leave();raster_cache_legacy(1u);
  if(raster_live||raster_fault){native_table_bank1_owner=NATIVE_BANK1_INVALID;raster_stop();raster_legacy_reset();}
  render_game_banked(g,g->focus,g->highlighted_count);publish_legacy_owner(g);return;
 }
 wait_vbl_done();
 /* Recheck before a foreign map, window, scroll or raster could be hidden. */
 if(!safe_modal(g)){
  native_table_leave();raster_cache_legacy(1u);render_game_banked(g,g->focus,g->highlighted_count);publish_legacy_owner(g);return;
 }
 old_owner=native_table_bank1_owner;old_window=LCDC_REG&(LCDCF_WINON|LCDCF_WIN9C00);old_wx=WX_REG;old_wy=WY_REG;
 HIDE_SPRITES;WY_REG=0;WX_REG=7;LCDC_REG|=LCDCF_WIN9C00|LCDCF_WINON;
 a1_palette_copy(raster_pixels);set_bkg_palette(7u,1u,(uint16_t *)(raster_pixels+32u));
 if(native_table_live)frontend_legacy_invalidate();
 native_table_live=0;VBK_REG=0;raster_cache_legacy(1u);
 /* Every bank0 map/pattern write now occurs behind bank1 WINDOW. No edit to
  * render_game_banked or its visible PACK/SHOP instruction stream is needed. */
 frontend_modal_static();clear_hidden_map();frontend_modal_body(g);
 wait_vbl_done();
 LCDC_REG=(LCDC_REG&~(LCDCF_WINON|LCDCF_WIN9C00))|old_window;WX_REG=old_wx;WY_REG=old_wy;
 a1_palette_copy(raster_pixels);set_bkg_palette(7u,1u,(uint16_t *)(raster_pixels+56u));
 VBK_REG=0;native_table_bank1_owner=old_owner>=NATIVE_BANK1_COLD_PREPARING?old_owner:NATIVE_BANK1_SUSPENDED;progress=19u;
}
void native_modal_prime_idle(void) BANKED {
 uint8_t id,x,unit;
 static const uint8_t badge[16]={0,255,66,189,36,219,24,231,24,231,36,219,66,189,0,255};
 static const uint8_t dots[32]={0,0,84,0,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
 if(!safe_modal(&game)||native_table_live){native_table_bank1_owner=NATIVE_BANK1_INVALID;return;}
 /* Bound each idle slice to four complete tiles; input is captured after
  * every unit. Preparing states remain invalid for native admission. */
 for(unit=0;unit<4u;unit++){
 id=native_table_bank1_owner-NATIVE_BANK1_COLD_PREPARING;
 /* Extra cards are one four-tile slice including retained final tile176. */
 if(id==176u&&unit)return;VBK_REG=1;
 if(id<177u){a1_card_tile_copy(id,raster_pixels);x=id<130u?id:id<147u?id+15u:id<175u?id+17u:id+59u;set_bkg_data(x,1u,raster_pixels);}
 else if(id<187u){a1_edge_tile_copy(id-177u,raster_pixels);set_bkg_data(130u+id-177u,1u,raster_pixels);}
 else if(id<193u){a1_rail_back_tile_copy(id-187u,raster_pixels);set_bkg_data(236u+id-187u,1u,raster_pixels);}
 else if(id==193u)set_bkg_data(140u,1u,badge);
 else if(id==194u){a1_cursor_copy(raster_pixels);set_sprite_data(254u,2u,raster_pixels);}
 else set_sprite_data(162u,2u,dots);
 if(id==176u){
 for(x=177u;x<180u;x++){a1_card_tile_copy(x,raster_pixels);set_bkg_data(x+69u,1u,raster_pixels);raster_capture_inputs();}
 VBK_REG=0;raster_capture_inputs();native_table_bank1_owner++;return;
 }
 VBK_REG=0;raster_capture_inputs();
 if(id==195u){native_table_bank1_owner=NATIVE_BANK1_SUSPENDED;return;}
 else native_table_bank1_owner++;
 }
}
