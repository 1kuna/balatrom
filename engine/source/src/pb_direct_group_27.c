#ifdef __SDCC
#pragma bank 44
#include <gb/gb.h>
#else
#include <stdint.h>
#define BANKED
#endif
#include <string.h>
extern uint8_t raster_pixels[128],raster_planes[96];
static const uint8_t keys[60]={5,133,0,0,242,243,5,133,1,0,242,243,5,133,2,0,242,243,5,133,3,0,242,243,5,133,4,0,242,243,5,133,5,0,242,243,5,133,6,0,242,243,5,133,7,0,242,243,5,133,8,0,242,243,5,133,9,0,242,243};
static const uint8_t values[980]={0,16,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,160,144,1,129,10,9,80,72,128,64,5,4,40,36,64,32,2,2,20,18,160,144,1,129,10,9,80,72,128,64,5,4,10,6,80,48,128,128,5,3,40,24,64,192,2,1,20,12,160,96,1,0,10,6,80,48,128,128,5,3,40,24,64,192,1,0,10,6,80,48,128,128,5,3,40,24,64,192,2,1,20,12,160,96,1,0,10,6,80,48,128,128,5,3,40,24,0,0,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,161,145,10,137,80,72,133,68,40,36,66,34,20,18,161,145,10,137,80,72,133,68,40,36,66,34,20,18,161,145,10,137,12,10,96,80,6,133,48,40,131,66,24,20,193,161,12,10,96,80,6,133,48,40,131,66,24,20,193,161,12,10,96,80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,80,192,128,1,5,12,40,96,64,0,2,6,20,48,160,128,1,3,10,24,80,192,128,1,5,12,40,96,64,0,2,6,12,10,96,80,0,128,6,5,48,40,128,64,3,2,24,20,192,160,1,1,12,10,96,80,0,128,6,5,48,40,128,64,1,1,12,10,96,80,0,128,6,5,48,40,128,64,3,2,24,20,192,160,1,1,12,10,96,80,0,128,6,5,48,40,0,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const uint8_t indexes[128]={255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,0,0,0,0,12,0,196,0,18,0,38,1,36,0,76,2,42,0,174,2,54,0,114,3,6,0,98,0,24,0,136,1,30,0,234,1,48,0,16,3,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0};
static const uint8_t displacements[64]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0};

uint8_t pb_direct_read_27(uint16_t slot,uint16_t *recipe) BANKED {
 uint16_t at,offset;
 if(slot>=32u || !recipe)return 0;
 at=slot*4u;
 if(indexes[at]==255u && indexes[at+1u]==255u)return 0;
 offset=(uint16_t)indexes[at+2u]|((uint16_t)indexes[at+3u]<<8);
 *recipe=(uint16_t)values[offset]|((uint16_t)values[offset+1u]<<8);
 memcpy(raster_planes,values+offset+2u,96u);return 1;
}
uint8_t pb_direct_group_27(uint8_t length,uint16_t *recipe) BANKED {
 uint16_t h1,h2,bucket,at,displacement,slot,offset;
 if(!length || length>119u || !recipe)return 0;
 h1=(uint16_t)raster_pixels[124]|((uint16_t)raster_pixels[125]<<8);
 h2=(uint16_t)raster_pixels[126]|((uint16_t)raster_pixels[127]<<8);
 bucket=(h1^(h1>>5u))&31u;at=bucket*2u;
 displacement=(uint16_t)displacements[at]|((uint16_t)displacements[at+1u]<<8);
 slot=(h2+displacement)&31u;at=slot*4u;
 offset=(uint16_t)indexes[at]|((uint16_t)indexes[at+1u]<<8);
 if(offset==65535u || keys[offset]!=length || memcmp(keys+offset+1u,raster_pixels,length))return 0;
 if(!pb_direct_read_27(slot,recipe))return 0;
 raster_pixels[120]=(uint8_t)slot;raster_pixels[121]=(uint8_t)((slot>>8)|108u);
 return 1;
}

#include "shop154.h"
#include "raster_runtime.h"
#include "shop157_sidebar.inc"
#include "result149.inc"

void bank1_suspend(const GameState *g) BANKED {
 /* Suspend only an already valid/suspended native set in the actual TABLE
  * consumable modal. Cold modal resume, other states, Joker mode, foreign
  * GameState and raster fault/live paths conservatively revoke ownership. */
 if(_cpu==CGB_TYPE&&g==&game&&g->phase==PHASE_TABLE&&g->ui_mode==1u&&
    !raster_live&&!raster_fault&&
    (native_table_bank1_owner==NATIVE_BANK1_VALID||
     native_table_bank1_owner==NATIVE_BANK1_SUSPENDED))
 {VBK_REG=0;native_table_bank1_owner=NATIVE_BANK1_SUSPENDED;}
 else native_table_bank1_owner=NATIVE_BANK1_INVALID;
}
/* Suspended legacy entry owns VBK0. Its attribute/map writers restore VBK0;
 * VBlank input capture never changes VBK, and raster live/fault denies lending. */

#include "shop154.h"
uint8_t native_busy_loss_restart(const GameState *) BANKED;
extern palette_color_t shop154_pal[12];
/* felt curtain for legacy screen changes (SHOP<->PACK and
 * inventory/target views). The progressive legacy paint used to be visible,
 * including near-black slate frames. The spare 0x9C00 map is filled with
 * PAPER_TILE (palette0 colour1 = TABLE felt, uploaded once at init), shown in
 * VBlank, and the existing shop154 hidden staging paints 0x9800 unseen; the
 * unchanged shop154_commit reveals the complete screen in one VBlank. The
 * live palettes0..2 seed shop154_pal so a commit never writes stale words. */
static uint8_t s154_prev_phase=255u,s154_prev_ui=255u,s154_prev_key[20];
/* Public fields whose change makes render.c repaint the whole screen
 * (focus-only moves keep the instant compat redraw and no curtain). */
static uint8_t s154_key_changed(const GameState *g){uint8_t k[20],changed;
 memcpy(k,&g->money,4u);k[4]=g->joker_count;k[5]=g->consumable_count;k[6]=g->hand_count;k[7]=g->hands;k[8]=g->discards;
 k[9]=g->shop[0];k[10]=g->shop[1];k[11]=g->shop_kind[0];k[12]=g->shop_kind[1];k[13]=g->pack_count;k[14]=g->pack_choices;
 k[15]=g->pack_bought[0];k[16]=g->pack_bought[1];k[17]=g->ante;k[18]=g->blind;k[19]=0;
 changed=memcmp(k,s154_prev_key,20u)!=0;memcpy(s154_prev_key,k,20u);return changed;}
void native_busy_wait_invalidate(void) BANKED;
static void s154_felt_curtain(void){uint8_t row[20],y,i;
 native_busy_wait_invalidate();
 memset(row,84u,20u);VBK_REG=0;for(y=0;y<18u;y++)set_tiles(0,y,20u,1u,(uint8_t*)0x9C00u,row);
 memset(row,0,20u);VBK_REG=1;for(y=0;y<18u;y++)set_tiles(0,y,20u,1u,(uint8_t*)0x9C00u,row);VBK_REG=0;
 do{wait_vbl_done();}while(LY_REG<144u||LY_REG>150u);
 CRITICAL {for(i=0;i<24u;i++){BCPS_REG=i;((uint8_t*)shop154_pal)[i]=BCPD_REG;}LCDC_REG|=LCDCF_BG9C00;}
}
void shop154_start(const GameState *g) BANKED {uint8_t title_lease=0;if(g==&game&&((game.phase==PHASE_TITLE&&rphase==PHASE_LOSS)||(game.phase==PHASE_SHOP&&rphase==PHASE_WIN))&&!result149_active&&pub143_enabled&&_cpu==CGB_TYPE&&native_table_live&&!raster_live&&!raster_fault&&(LCDC_REG&0x98u)==0x90u)title_lease=native_busy_loss_restart(g);shop154_staging=g==&game&&pub143_enabled&&_cpu==CGB_TYPE&&(game.phase==PHASE_SHOP||(game.phase==PHASE_TITLE&&rphase==PHASE_LOSS&&!result149_active))&&!native_table_live&&!raster_live&&!raster_fault&&((LCDC_REG&0x98u)==0x88u||(!result149_active&&title_lease&&(LCDC_REG&0x98u)==0x98u));
 if(!shop154_staging&&g==&game&&_cpu==CGB_TYPE&&!native_table_live&&!raster_live&&!raster_fault&&(LCDC_REG&0x98u)==0x90u&&
    (g->phase==PHASE_SHOP||g->phase==PHASE_PACK)&&(s154_prev_phase==PHASE_SHOP||s154_prev_phase==PHASE_PACK)&&
    ((g->phase!=s154_prev_phase||g->ui_mode!=s154_prev_ui||game_action_error)|s154_key_changed(g))){s154_felt_curtain();shop154_staging=1;}
 else if(g==&game)s154_key_changed(g);
 if(g==&game){s154_prev_phase=g->phase;s154_prev_ui=g->ui_mode;}
}

#include "save.h"
#include "frontend_render.h"
#include "audio_sfx.h"
void shop_background_reset(void) BANKED;
void boot154(void) BANKED {if(_cpu==CGB_TYPE)cpu_fast();pub143_boot();game_load(&game);shop_background_reset();game.phase=PHASE_TITLE;DISPLAY_ON;render_game(&game,game.focus,game.highlighted_count);audio_sfx_init();}

extern palette_color_t shop154_pal[12];
void shop154_commit(void) BANKED {if(!shop154_staging)return;while(LY_REG<144u||LY_REG>151u){wait_vbl_done();}CRITICAL {VBK_REG=0;set_bkg_palette(0u,3u,shop154_pal);LCDC_REG=(LCDC_REG|LCDCF_BG8000)&~LCDCF_BG9C00;shop154_staging=0;}}

static const uint8_t *shop155_source;
static uint16_t shop155_dest;
static uint8_t shop155_groups;
static void shop155_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ld a,(_shop155_source)
 ld l,a
 ld a,(_shop155_source+1)
 ld h,a
 ld a,(_shop155_dest)
 ld e,a
 ld a,(_shop155_dest+1)
 ld d,a
 ld a,(_shop155_groups)
 ld b,a
s155_burst:
 di
s155_wait:
 ldh a,(0x41)
 and #2
 jr nz,s155_wait
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
 ei
 dec b
 jr nz,s155_burst
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}

void shop156_upload(uint8_t first,uint8_t count,const uint8_t *data) BANKED {shop155_source=data;shop155_dest=0x8000u+(uint16_t)first*16u;shop155_groups=count*4u;shop155_raw();}

static void shop157_tile_read(uint8_t id,uint8_t*out){shop155_source=(const uint8_t*)(0x9000u+(uint16_t)id*16u);shop155_dest=(uint16_t)out;shop155_groups=4u;shop155_raw();}
static void shop157_tile_write(uint8_t id,const uint8_t*data){shop155_source=data;shop155_dest=0x8000u+(uint16_t)id*16u;shop155_groups=4u;shop155_raw();}
