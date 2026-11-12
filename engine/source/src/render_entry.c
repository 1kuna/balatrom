#include "game.h"
#include "shop154.h"
#include "publication143.h"
#include "result149.h"
#include "cooperative_native.h"
#include "native_table.h"
#include "native_modal.h"
#include "hand_preview.h"
#include "raster_runtime.h"
#include <gb/cgb.h>
void raster_cache_legacy(uint8_t) BANKED;
void render_game_banked(const GameState *,uint8_t,uint8_t) BANKED;
#ifdef QA_RASTER_GUARD
void raster_guard_init(void) BANKED;
#endif
void bank1_suspend(const GameState *g) BANKED;
void render_game(const GameState *g,uint8_t focus,uint8_t selected) {
#ifdef QA_RASTER_GUARD
 raster_guard_init();
#endif
 if(_cpu==CGB_TYPE&&g->phase==PHASE_TABLE){
  if(!g->ui_mode){if(coop_begin(g)){if(!coop_cancelled)native_table_render(g);coop_finish();}else native_table_render(g);return;}
  if(g->ui_mode==1u){coop_resident=0;native_modal_render(g);return;}
 }
 if(result149_begin(g))return;
 shop154_start(g);
 coop_resident=0;native_table_leave();
 raster_cache_legacy(g->phase==PHASE_TABLE&&(g->ui_mode==1u||g->ui_mode==2u));
 if(raster_live||raster_fault){native_table_bank1_owner=NATIVE_BANK1_INVALID;raster_stop();raster_legacy_reset();}
 render_game_banked(g,focus,selected);
 bank1_suspend(g);
}
/* Publish ownership after legacy writes. native_table_leave already hands
 * VBK0 to first modal entry; legacy writers preserve that bank on later paints.
 * Keep bookkeeping out of the LCD-on pre-render timing path. */
