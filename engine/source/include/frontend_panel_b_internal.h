#ifndef FRONTEND_PANEL_B_INTERNAL_H
#define FRONTEND_PANEL_B_INTERNAL_H
#include "frontend_panel_b.h"
#define PANEL_B_TYPES_ONLY
#include "../src/frontend_panel_b_view.inc"
#undef PANEL_B_TYPES_ONLY
typedef struct {const char *s;uint8_t x,y,font,colour,length,right,bottom;} Glyph;
typedef struct {
    const GameState *g;
    PanelBPage page;
    PanelBCard cards[8]; PanelBJoker jokers[5];
    uint8_t icons[5][96], jids[20], bg[32], obj[24], obj_used;
    uint16_t jwords[20];
    char target[16],score[16],chips[16],mult[16],money[16],label[31],level[8];
    uint8_t label_x,label_font,level_x,membership[41],glyph_count;
    Glyph glyphs[15];char stats[4][4],money_tail[9],blind_label[6];
    uint8_t family;
    uint16_t glyph_bands[9];
} Frame;

void panel_b_paint_card(const PanelBCard *,uint8_t,uint8_t,uint8_t *) NONBANKED;
extern uint8_t panel_b_recipe_active;
extern uint16_t panel_b_recipe_bytes;
extern uint8_t panel_b_dirty_active,panel_b_stage_count,panel_b_band_load[9];
void panel_b_cache_reset(void) BANKED;
uint8_t panel_b_dirty_begin(const Frame *,const HandPreview *) BANKED;
uint8_t panel_b_pair_changed(const Frame *,uint8_t,uint8_t) BANKED;
void panel_b_cache_save(const Frame *,const HandPreview *) BANKED;
uint8_t panel_b_stage_begin(uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,const uint8_t *) NONBANKED;
void panel_b_stage_object(uint8_t,const uint8_t *) NONBANKED;
void panel_b_stage_finish(uint8_t *) NONBANKED;
void panel_b_stage_flush(uint8_t *) NONBANKED;
void panel_b_prepare(Frame *,const GameState *,const HandPreview *) BANKED;

void panel_b_pair_positive(uint8_t,uint8_t) NONBANKED;
uint8_t panel_b_pair_phase(uint8_t,uint8_t) NONBANKED;

void panel_b_choose_family(Frame *,uint8_t) BANKED;
uint8_t panel_b_cached_family(void) NONBANKED;
void panel_b_prepare_numbered(Frame *,const GameState *,const HandPreview *,const char numbers[32]) BANKED;
#endif
