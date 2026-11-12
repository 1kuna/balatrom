#pragma bank 255
#include <string.h>
#include "raster_runtime.h"
#include "frontend_panel_b_internal.h"
#include "frontend_panel_b_overlay.h"
/* Bank4 owns exact public keys. No hash, UID, concealed face field or pointer
 * value is used. Its complete range is D000..D781, disjoint from banks2/3.
 * This module never publishes, writes VRAM, or bypasses whole-frame admission. */
static uint8_t valid,card_changes,joker_changes,palette_changes;
static uint16_t glyph_changes;
/* Bit1 certifies a suspended TABLE snapshot across only inventory/Joker
 * legacy presentation. Legacy owns VRAM, never these WRAM recipe shadows.
 * Fresh public keys and whole-frame admission still run on return. */
void raster_cache_legacy(uint8_t keep) BANKED {
 if(!keep||raster_fault){valid=0;return;}
 if(raster_live)valid=(valid&1u)?3u:0u;
}
static uint8_t accessible(void){
 return (valid&1u)&&!raster_fault&&
  ((raster_live&&(LCDC_REG&LCDCF_ON))||(!raster_live&&(valid&2u)));
}
void raster_cache_full_key(void) BANKED {card_changes=255;joker_changes=31;glyph_changes=32767;palette_changes=3;}
static uint8_t different(uint16_t at,const uint8_t *data,uint8_t length){
 uint8_t n;
 while(length){n=length>16u?16u:length;raster_get_block(4,at,raster_planes,n);if(memcmp(data,raster_planes,n))return 1;at+=n;data+=n;length-=n;}
 return 0;
}
static void rectangle(uint8_t *dirty,uint8_t x,uint8_t y,uint8_t width,uint8_t height){
 uint16_t right=(uint16_t)x+width,bottom=(uint16_t)y+height;uint8_t col,row,pair;
 if(x>=160u||y>=144u||!width||!height)return;
 if(right>160u)right=160;if(bottom>144u)bottom=144;
 for(row=y>>4;row<=(bottom-1u)>>4;row++)for(col=x>>3;col<=(right-1u)>>3;col++){pair=row*20u+col;dirty[pair>>3]|=1u<<(pair&7u);}
}
static void card_rect(uint8_t *dirty,const uint8_t *c){
 if(!c[2])return;rectangle(dirty,c[0],c[1],20,31);
 if(c[8]&PANEL_B_FOCUSED)rectangle(dirty,c[0]+5u,c[1]-5u,7,4);
}
uint8_t raster_cache_policy(Frame *f) BANKED {
 if(!accessible())return 0;
 raster_get_block(4,0xd700u,f->bg,56);return 1;
}
uint8_t raster_cache_dirty(const Frame *f,const PBOverlayPalettes *upper,uint8_t *dirty) BANKED {
 uint8_t i,n;const uint8_t *cur;uint8_t old[39];const Glyph *g;
 memset(dirty,255,23);card_changes=255;joker_changes=31;glyph_changes=32767;palette_changes=3;
 if(!accessible()||different(0xd700u,f->bg,56))return 0;
 memset(dirty,0,23);card_changes=joker_changes=palette_changes=0;glyph_changes=0;
 if(different(0xd740u,upper->bg,56)){memset(dirty,255,8);palette_changes|=2;} /* conservatively top64 pairs */
 /* Bind all four page bytes. Only a changed visible window invalidates the
  * whole hand; selection uses the exact old/new card and preview glyph keys. */
 if(different(0xd050u,(const uint8_t *)&f->page,4)){
  raster_get_block(4,0xd050u,old,4);
  if(old[0]!=f->page.first||old[1]!=f->page.count)rectangle(dirty,40,80,120,64);
 }
 for(i=0;i<8u;i++){
  cur=(const uint8_t *)(f->cards+i)+2u;
  if(different(0xd000u+i*9u,cur,9)){card_changes|=1u<<i;raster_get_block(4,0xd000u+i*9u,old,9);if(!memcmp(old,cur,8)&&!((old[8]^cur[8])&~PANEL_B_FOCUSED)){
   if(old[8]&PANEL_B_FOCUSED)rectangle(dirty,old[0]+5u,old[1]-5u,7,4);
   if(cur[8]&PANEL_B_FOCUSED)rectangle(dirty,cur[0]+5u,cur[1]-5u,7,4);
  }else{card_rect(dirty,old);card_rect(dirty,cur);}}
 }
 for(i=0;i<5u;i++){
  cur=(const uint8_t *)(f->jokers+i)+2u;
  if(different(0xd060u+i*6u,cur,6)||different(0xd100u+(uint16_t)i*96u,f->icons[i],96)||different(0xd2e0u+i*8u,(const uint8_t *)(f->jwords+i*4u),8)||different(0xd310u+i*4u,f->jids+i*4u,4)){
   joker_changes|=1u<<i;raster_get_block(4,0xd060u+i*6u,old,6);if(old[2])rectangle(dirty,old[0],7,20,28);if(cur[2])rectangle(dirty,cur[0],7,20,28);
  }
 }
 raster_get_block(4,0xd780u,&n,1);
 if(n!=f->glyph_count){memset(dirty,255,23);glyph_changes=32767;return 0;}
 for(i=0;i<f->glyph_count;i++){
  g=f->glyphs+i;if(g->length>31u){memset(dirty,255,23);return 0;}
  cur=(const uint8_t *)g+2u;
  if(different(0xd400u+(uint16_t)i*40u,cur,7)||different(0xd407u+(uint16_t)i*40u,(const uint8_t *)g->s,g->length+1u)){
   glyph_changes|=1u<<i;raster_get_block(4,0xd400u+(uint16_t)i*40u,old,7);
   rectangle(dirty,old[0],old[1],old[5]-old[0],old[6]-old[1]);
   rectangle(dirty,g->x,g->y,g->right-g->x,g->bottom-g->y);
  }
 }
 return 1;
}
void raster_cache_save(const Frame *f,const PBOverlayPalettes *upper) BANKED {
 uint8_t i;const Glyph *g;
 for(i=0;i<8u;i++)if(card_changes&(1u<<i))raster_put_block(4,0xd000u+i*9u,(const uint8_t *)(f->cards+i)+2u,9);
 raster_put_block(4,0xd050u,(const uint8_t *)&f->page,4);
 for(i=0;i<5u;i++)if(joker_changes&(1u<<i)){
  raster_put_block(4,0xd060u+i*6u,(const uint8_t *)(f->jokers+i)+2u,6);
  raster_put_block(4,0xd100u+(uint16_t)i*96u,f->icons[i],96);
  raster_put_block(4,0xd2e0u+i*8u,(const uint8_t *)(f->jwords+i*4u),8);
  raster_put_block(4,0xd310u+i*4u,f->jids+i*4u,4);
 }
 for(i=0;i<f->glyph_count;i++)if(glyph_changes&(1u<<i)){
  g=f->glyphs+i;if(g->length>31u){valid=0;return;}
  raster_put_block(4,0xd400u+(uint16_t)i*40u,(const uint8_t *)g+2u,7);
  raster_put_block(4,0xd407u+(uint16_t)i*40u,(const uint8_t *)g->s,g->length+1u);
 }
 if(palette_changes&1u)raster_put_block(4,0xd700u,f->bg,56);if(palette_changes&2u)raster_put_block(4,0xd740u,upper->bg,56);raster_put_block(4,0xd780u,&f->glyph_count,1);valid=1;
}
