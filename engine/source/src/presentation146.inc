#pragma bank 255
#include "presentation146.h"
#include <string.h>
/* No input handler here. The existing game_input dispatch must precede any
 * presentation work, and phase exit cancels this screen-local state. */
static uint8_t step(uint16_t tick,uint8_t begin,uint8_t end){
 if(tick<=begin)return 0;
 if(tick>=end)return end-begin;
 return tick-begin;
}
void present146_end(Present146 *p) BANKED {memset(p,0,sizeof(*p));}
uint8_t present146_begin(Present146 *p,const GameState *g) BANKED {
 present146_end(p);
 if(g->phase!=PHASE_WIN&&g->phase!=PHASE_LOSS)return 0;
 p->phase=g->phase;p->last_hand=g->last_hand;
 memcpy(&p->chips,&g->last_chips,sizeof(GbNum));
 memcpy(&p->mult,&g->last_mult,sizeof(GbNum));
 memcpy(&p->last_total,&g->last_score,sizeof(GbNum));
 memcpy(&p->run_total,&g->score,sizeof(GbNum));
 memcpy(&p->target,&g->target,sizeof(GbNum));
 p->active=1;return 1;
}
void present146_sample(Present146 *p,const GameState *g,uint16_t tick,Present146Frame *f) BANKED {
 memset(f,0,sizeof(*f));
 if(!p->active)return;
 if(g->phase!=p->phase){present146_end(p);return;}
 f->valid=1;f->continue_visible=p->phase==PHASE_WIN;f->stable=tick>=54;
 f->hand_emphasis=tick<8;
 f->chips_step=step(tick,6,16);f->mult_step=step(tick,12,20);
 f->total_step=step(tick,20,32);f->comparison_step=step(tick,28,40);
 f->outcome_step=step(tick,40,48);f->pulse_step=step(tick,48,54);
}
