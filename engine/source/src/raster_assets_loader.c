#include "frontend_panel_b_internal.h"
#include "frontend_resolve.h"
#include <string.h>
extern const uint8_t panel_b_static_pixels[],panel_b_static_pair_map[];
BANKREF_EXTERN(panel_b_static_pixels)
void panel_b_load_static_pair(uint8_t pair,uint8_t *out) NONBANKED {
    uint8_t saved=CURRENT_BANK;
    SWITCH_ROM(BANK(panel_b_static_pixels));
    memcpy(out,panel_b_static_pixels+(uint16_t)panel_b_static_pair_map[pair]*128u,128);
    SWITCH_ROM(saved);
}
uint8_t panel_b_fetch_icon(uint8_t id,uint8_t *tiles,uint16_t *palette) NONBANKED {
    SidebarJokerRef ref;
    uint8_t saved;
    if (!frontend_resolve_icon(0,id,&ref)) return 0;
    saved=CURRENT_BANK; SWITCH_ROM(ref.bank);
    memcpy(tiles,ref.tiles,96); memcpy(palette,ref.palette,8);
    SWITCH_ROM(saved); return 1;
}
extern const uint8_t panel_b_card_grammar_0[];
BANKREF_EXTERN(panel_b_card_grammar_0)
extern const uint8_t panel_b_card_grammar_1[];
BANKREF_EXTERN(panel_b_card_grammar_1)
extern const uint8_t panel_b_card_grammar_2[];
BANKREF_EXTERN(panel_b_card_grammar_2)
extern const uint8_t panel_b_card_grammar_3[];
BANKREF_EXTERN(panel_b_card_grammar_3)
extern const uint8_t panel_b_card_grammar_4[];
BANKREF_EXTERN(panel_b_card_grammar_4)
extern const uint8_t panel_b_card_grammar_5[];
BANKREF_EXTERN(panel_b_card_grammar_5)
extern const uint8_t panel_b_card_grammar_6[];
BANKREF_EXTERN(panel_b_card_grammar_6)
extern const uint8_t panel_b_card_grammar_7[];
BANKREF_EXTERN(panel_b_card_grammar_7)
extern const uint8_t panel_b_card_grammar_8[];
BANKREF_EXTERN(panel_b_card_grammar_8)
extern const uint8_t panel_b_card_grammar_9[];
BANKREF_EXTERN(panel_b_card_grammar_9)
extern const uint8_t panel_b_card_grammar_10[];
BANKREF_EXTERN(panel_b_card_grammar_10)
void panel_b_paint_card(const PanelBCard *v,uint8_t x0,uint8_t y0,uint8_t *out) NONBANKED {
 uint8_t id,selected=!!(v->cues&PANEL_B_SELECTED),saved=CURRENT_BANK;
 uint8_t left,right,top,bottom,y,dy,a,b,index;const uint8_t *src=0;
 /* Face concealment precedes every template index and bank lookup. */
 if(v->kind==PANEL_B_HIDDEN)id=104u+selected;
 else if(v->kind==PANEL_B_STONE)id=106u+selected;
 else if(v->kind==PANEL_B_FACE)id=(v->rank-2u)*4u+v->suit+selected*52u;
 else id=108u+selected;
 switch(id/10u) {
case 0: SWITCH_ROM(BANK(panel_b_card_grammar_0)); src=panel_b_card_grammar_0; break;
case 1: SWITCH_ROM(BANK(panel_b_card_grammar_1)); src=panel_b_card_grammar_1; break;
case 2: SWITCH_ROM(BANK(panel_b_card_grammar_2)); src=panel_b_card_grammar_2; break;
case 3: SWITCH_ROM(BANK(panel_b_card_grammar_3)); src=panel_b_card_grammar_3; break;
case 4: SWITCH_ROM(BANK(panel_b_card_grammar_4)); src=panel_b_card_grammar_4; break;
case 5: SWITCH_ROM(BANK(panel_b_card_grammar_5)); src=panel_b_card_grammar_5; break;
case 6: SWITCH_ROM(BANK(panel_b_card_grammar_6)); src=panel_b_card_grammar_6; break;
case 7: SWITCH_ROM(BANK(panel_b_card_grammar_7)); src=panel_b_card_grammar_7; break;
case 8: SWITCH_ROM(BANK(panel_b_card_grammar_8)); src=panel_b_card_grammar_8; break;
case 9: SWITCH_ROM(BANK(panel_b_card_grammar_9)); src=panel_b_card_grammar_9; break;
case 10: SWITCH_ROM(BANK(panel_b_card_grammar_10)); src=panel_b_card_grammar_10; break;
 }
 src+=(uint16_t)(id%10u)*620u;
 left=x0>v->x?x0:v->x;right=x0+8u<v->x+20u?x0+8u:v->x+20u;
 top=y0>v->y?y0:v->y;bottom=y0+16u<v->y+31u?y0+16u:v->y+31u;
 for(y=top;y<bottom;y++) {
  dy=y-v->y;a=left;b=right;
  if(selected&&(dy==0u||dy==29u)&&a==v->x)a++;
  if(selected&&dy==30u&&a<v->x+2u)a=v->x+2u;
  if(dy==0u&&b>v->x+18u)b=v->x+18u;
  else if((dy==1u||dy==30u)&&b>v->x+19u)b=v->x+19u;
  if(a<b){index=y-y0;index*=8u;index+=a-x0;memcpy(out+index,src+(uint16_t)dy*20u+a-v->x,b-a);}
 }
 SWITCH_ROM(saved);
}
