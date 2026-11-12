/* Original generated SFX. Presentation observer only; no rule/RNG/save calls. */
#include "audio_sfx.h"
/* The pending-cue update disables IRQs. Keep it outside split-row consumers. */
void raster_ferry_window(void) NONBANKED;
typedef struct {uint8_t frames;uint16_t period;uint8_t volume,poly,noise;} Step;
static const Step sounds[7][4]={
 {{2,0,0,0x35,8},{2,0,0,0x57,4},{0,0,0,0,0},{0,0,0,0,0}},
 {{3,1750,9,0,0},{3,1820,7,0,0},{0,0,0,0,0},{0,0,0,0,0}},
 {{2,0,0,0x45,8},{2,0,0,0x67,4},{0,0,0,0,0},{0,0,0,0,0}},
 {{4,1797,10,0,0},{5,1881,8,0,0},{0,0,0,0,0},{0,0,0,0,0}},
 {{8,1797,10,0,0},{8,1849,10,0,0},{12,1881,9,0,0},{0,0,0,0,0}},
 {{8,1797,9,0,0},{8,1750,8,0,0},{12,1660,7,0x63,5},{0,0,0,0,0}},
 {{3,1450,6,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}}
};
static const uint8_t priorities[7]={1,2,1,3,4,4,3};
static volatile uint8_t active,step,left,pending,priority;
volatile uint8_t audio_sfx_last_cue;
volatile uint8_t audio_sfx_ticks;
void audio_sfx_tick_end(void) NONBANKED {audio_sfx_ticks++;}
static uint8_t before_phase,before_hands,before_discards,before_selected,before_mode,buttons;
static int32_t before_money;
static void cut(void){NR22_REG=0;NR42_REG=0;}
static void start_step(void){const Step *s=&sounds[active-1][step];left=s->frames;if(!left){cut();active=priority=0;return;}
 if(s->volume){NR21_REG=0x80;NR22_REG=s->volume<<4;NR23_REG=(uint8_t)s->period;NR24_REG=0x80|(s->period>>8);}else NR22_REG=0;
 if(s->noise){NR41_REG=0;NR42_REG=s->noise<<4;NR43_REG=s->poly;NR44_REG=0x80;}else NR42_REG=0;
}
void audio_sfx_tick(void) NONBANKED {if(pending){active=pending;pending=0;step=0;audio_sfx_last_cue=active;start_step();}else if(active&&! --left){step++;start_step();}audio_sfx_tick_end();}
void audio_sfx_emit(uint8_t cue) NONBANKED {if(cue<1||cue>7)return;raster_ferry_window();CRITICAL {if(priorities[cue-1]>=priority){pending=cue;priority=priorities[cue-1];}}}
void audio_sfx_init(void) NONBANKED {NR52_REG=0x80;NR50_REG=0x77;NR51_REG=0xAA;cut();active=step=left=pending=priority=0;CRITICAL {add_VBL(audio_sfx_tick);}}
void audio_sfx_before(const GameState *g,uint8_t pressed) NONBANKED {before_phase=g->phase;before_hands=g->hands;before_discards=g->discards;before_selected=g->highlighted_count;before_mode=g->ui_mode;before_money=g->money;buttons=pressed;}
void audio_sfx_after(const GameState *g) NONBANKED {
 if(g->phase==PHASE_BLOCKED&&before_phase!=PHASE_BLOCKED){audio_sfx_emit(SFX_DENIED);return;}
 if(g->phase==PHASE_LOSS&&before_phase!=PHASE_LOSS){audio_sfx_emit(SFX_LOSS);return;}
 if((g->phase==PHASE_WIN||g->phase==PHASE_COMPLETE)&&g->phase!=before_phase){audio_sfx_emit(SFX_WIN);return;}
 if(before_phase==PHASE_TABLE&&!before_mode){
  if(g->hands<before_hands){audio_sfx_emit(SFX_PLAY);return;}
  if(g->discards<before_discards){audio_sfx_emit(SFX_DISCARD);return;}
  if((buttons&J_A)&&g->highlighted_count!=before_selected){audio_sfx_emit(SFX_SELECT);return;}
 }
 if(before_phase==PHASE_SHOP&&!before_mode&&(buttons&J_A)&&g->money<before_money)audio_sfx_emit(SFX_BUY);
}
