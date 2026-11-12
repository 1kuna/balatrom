extern volatile unsigned char input_count;
#include <gb/gb.h>
#include <gb/cgb.h>
#include "game.h"
#include "result149.h"
#include "cooperative_native.h"
#include "publication143.h"
#include "native_table.h"
#include "native_busy.h"
#include "hand_api.h"
#include "save.h"
#include "frontend_render.h"
#include "audio_sfx.h"
#include "retained_caret.h"
void retained_input_begin(const GameState *,uint8_t) BANKED;
void retained_input_finish(const GameState *) BANKED;
void shop_background_idle(void) BANKED;
void shop_background_input(void) BANKED;
void shop_background_reset(void) BANKED;
void boot154(void) BANKED;
uint8_t queued_table_navigation(uint8_t pressed) BANKED;
void main(void){uint8_t pressed,ticked;boot154();while(1){if(coop_cancelled&&!input_count){coop_cancelled=0;render_game(&game,game.focus,game.highlighted_count);if(!coop_cancelled){frontend_table_transition_end();coop_accepting=0;}continue;}if(!input_count){native_busy_idle();if(!result149_active)shop_background_idle();wait_vbl_done();}ticked=0;pressed=frontend_read_pressed();if(!pressed){frontend_animation_tick();ticked=1;pressed=frontend_read_pressed();}if(pressed){if(result149_active)result149_input(pressed);coop_cancelled=0;coop_generation++;coop_accepting=1;shop_background_input();native_table_receipt(&game,pressed);retained_input_begin(&game,pressed);audio_sfx_before(&game,pressed);if(game.phase==PHASE_TITLE&&(pressed&J_A)&&resume_available){native_busy_title_resume();game_load(&game);}else game_input(pressed);audio_sfx_after(&game);retained_input_finish(&game);if((pressed&(J_LEFT|J_RIGHT))&&queued_table_navigation(pressed))continue;render_game(&game,game.focus,game.highlighted_count);if(!coop_cancelled){frontend_table_transition_end();coop_accepting=0;}if(!ticked&&!native_table_live)frontend_animation_tick();}else result149_idle();}}

/* Private hidden-shop transfer gateway. Callers' ROM bank remains mapped.
 * Legacy unsigned tiles8000..8FFF cannot overlap signed result9000..97FF.
 * Palette0..2 is retained until the VBlank display-ownership switch. */
#include "shop154.h"
#include <string.h>
uint8_t shop154_staging;
palette_color_t shop154_pal[12];
void shop154_data(uint8_t first,uint8_t count,const uint8_t *data) NONBANKED OLDCALL __naked {
 __asm
 ld a,(_shop154_staging)
 or a
 jp z,_set_bkg_data
 jp _set_sprite_data
 __endasm;
}
void shop154_palette(uint8_t first,uint8_t count,const palette_color_t *data) NONBANKED OLDCALL __naked {
 __asm
 ld a,(_shop154_staging)
 or a
 jp z,_set_bkg_palette
 ldhl sp,#2
 ld a,(hl+)
 or a
 jp nz,_set_bkg_palette
 ld a,(hl)
 cp #3
 jp nz,_set_bkg_palette
 push bc
 ldhl sp,#6
 ld a,(hl+)
 ld h,(hl)
 ld l,a
 ld de,#_shop154_pal
 ld b,#24
s154_palcopy:
 ld a,(hl+)
 ld (de),a
 inc de
 dec b
 jr nz,s154_palcopy
 pop bc
 ret
 __endasm;
}
static void shop154_tiles_hidden(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *data) NONBANKED OLDCALL {set_tiles(x,y,w,h,(uint8_t*)0x9800u,data);}
void shop154_tiles(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *data) NONBANKED OLDCALL __naked {
 __asm
 ld a,(_shop154_staging)
 or a
 jp z,_set_bkg_tiles
 jp _shop154_tiles_hidden
 __endasm;
}
