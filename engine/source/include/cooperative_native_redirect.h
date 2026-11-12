#include "cooperative_native.h"
#undef DISPLAY_OFF
#define DISPLAY_OFF coop_display_off()
#undef LCDC_REG
#define LCDC_REG (*coop_lcdc_ptr())
#define native_palette_ready (*coop_ready_ptr())
#define native_table_live (*coop_live_ptr())
#define native_palette_stop coop_palette_stop
#define native_palette_start coop_palette_start
#define set_bkg_data coop_data
#define set_sprite_data coop_sprite_data
#define set_tiles coop_tiles
#define set_bkg_tiles coop_bkg_tiles
#define set_bkg_palette coop_bg_palette
#define set_sprite_palette coop_obj_palette
#define set_sprite_tile coop_sprite_tile
#define set_sprite_prop coop_sprite_prop
#define move_sprite coop_move_sprite
#ifndef COOP_BUSY_IMPLEMENTATION
#define native_busy_begin coop_busy_begin
#define native_busy_publish coop_busy_publish
#define native_busy_prepare_wait coop_busy_prepare
#endif
#define frontend_native_acquire coop_acquire
