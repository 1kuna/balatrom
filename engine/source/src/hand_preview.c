#pragma bank 255
#include "hand_preview.h"
#include "hand_api.h"
#include "joker_missing.h"
#include "boss.h"
#include <string.h>
static const char *const names[12]={"HIGH CARD","PAIR","TWO PAIR","THREE OF A KIND","STRAIGHT","FLUSH","FULL HOUSE","FOUR OF A KIND","STRAIGHT FLUSH","FIVE OF A KIND","FLUSH HOUSE","FLUSH FIVE"};
void hand_preview(const GameState *g,HandPreview *o) CORE_BANKED {
 JokerContext c;JokerHandShapes shapes;uint8_t counts[15]={0},i,rank,five=0,n;uint16_t level;char rev[5];
 static const uint8_t bc[12]={/* OWNER_INSTALL_SLOT:bc */},bm[12]={/* OWNER_INSTALL_SLOT:bm */},lc[12]={/* OWNER_INSTALL_SLOT:lc */},lm[12]={/* OWNER_INSTALL_SLOT:lm */};
 memset(o,0,sizeof(*o));if(!g)return;memset(&c,0,sizeof(c));c.played_count=hand_selected_play_order(g,c.played_index);if(!c.played_count){o->valid=1;return;}
 for(i=0;i<c.played_count;i++){const Card *card=&g->hand[c.played_index[i]];if(card->flags&CARD_FACE_DOWN){o->hidden=o->valid=1;strcpy(o->label,"???? LV?");return;}if(card->enhancement!=ENH_STONE){rank=card->rank;if(rank<2||rank>14)return;counts[rank]++;}}
 for(i=2;i<15;i++){if(counts[i]==2)c.pairs++;if(counts[i]==3)c.trips++;if(counts[i]>=4)c.quads++;if(counts[i]==5)five=1;}
 joker_hand_shapes(g,&c,&shapes);if(!shapes.implemented)return;c.straight=shapes.straight_mask!=0;c.flush=shapes.flush_mask!=0;
 if(five&&c.flush)c.hand_type=11;else if(c.trips&&c.pairs&&c.flush)c.hand_type=10;else if(five)c.hand_type=9;else if(c.straight&&c.flush)c.hand_type=8;else if(c.quads)c.hand_type=7;else if(c.trips&&c.pairs)c.hand_type=6;else if(c.flush)c.hand_type=5;else if(c.straight)c.hand_type=4;else if(c.trips)c.hand_type=3;else if(c.pairs>=2)c.hand_type=2;else if(c.pairs)c.hand_type=1;
 level=g->hand_levels[c.hand_type];if(!level)return;o->level=level;o->hand_type=c.hand_type;o->chips=bc[c.hand_type]+(uint32_t)(level-1)*lc[c.hand_type];o->mult=bm[c.hand_type]+(uint32_t)(level-1)*lm[c.hand_type];strcpy(o->label,names[c.hand_type]);strcat(o->label," LV");n=0;do{rev[n++]='0'+level%10;level/=10;}while(level);i=strlen(o->label);while(n)o->label[i++]=rev[--n];o->label[i]=0;o->valid=1;
}
