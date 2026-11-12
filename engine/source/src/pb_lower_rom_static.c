#pragma bank 255
#include "pb_lower_rom_cache.h"
#include "frontend_panel_b_hotpath.h"
#include <string.h>
#include "pb_lower_rom_static_static_face_key.inc"

#include "pb_lower_rom_static_pair_map.inc"

#include "pb_lower_rom_static_offsets.inc"

#include "pb_lower_rom_static_static_recipes.inc"

#include "pb_lower_rom_static_payload.inc"

static uint8_t static_overlaps(const void *a,uint16_t alen,const void *b,uint16_t blen){
 uintptr_t aa=(uintptr_t)a,bb=(uintptr_t)b;
 return aa<=bb?bb-aa<alen:aa-bb<blen;
}
static uint8_t intersects(uint8_t x,uint8_t y,uint8_t ox,uint8_t oy,uint8_t w,uint8_t h){
 return x<(uint16_t)ox+w&&(uint16_t)x+8u>ox&&y<(uint16_t)oy+h&&(uint16_t)y+16u>oy;
}
uint8_t pb_lower_rom_static(const Frame *f,uint8_t x,uint8_t y,
                           uint8_t *planes,uint16_t *out) BANKED {
 uint8_t i,span,advance,height,n;uint16_t glyph_mask;const Glyph *g;const PanelBCard *v;
 if(!f||!planes||!out||x>152u||y<48u||y>128u||(x&7u)||(y&15u)||
    f->page.count>8u||f->glyph_count>15u||static_overlaps(out,2,planes,96)||
    static_overlaps(out,2,f,sizeof(Frame))||static_overlaps(planes,96,f,sizeof(Frame)))return 0;
 if(memcmp(f->bg,static_face_key,56))return 0;
 for(i=0,glyph_mask=f->glyph_bands[y>>4];glyph_mask;i++,glyph_mask>>=1)if(glyph_mask&1u){
  if(i>=f->glyph_count)return 0;
  g=f->glyphs+i;advance=g->font?6u:4u;height=g->font?7u:5u;
  if(intersects(x,y,g->x,g->y,g->length*advance,height))return 0;
 }
 span=y>=80u?panel_b_card_span(x>>3):0u;
 for(i=span&15u;i<f->page.count&&i<(span>>4);i++){
  v=f->cards+i;
  if(intersects(x,y,v->x,v->y,20,31))return 0;
  if((v->cues&PANEL_B_FOCUSED)&&intersects(x,y,v->x+5u,v->y-5u,7,4))return 0;
 }
 i=pair_map[(y>>4)*20u+(x>>3)-60u];
 *out=static_recipes[i];n=32u+32u*((*out>>12)&3u);
 memset(planes,0,96);memcpy(planes,payload+offsets[i],n);return 1;
}
