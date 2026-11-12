#pragma bank 255
#include "game.h"
#include "native_table.h"
#include "raster_runtime.h"
#include "hand_preview.h"
#include "a1_ui_runtime.h"
#include <gb/cgb.h>
#include <stddef.h>
#include <string.h>
void raster_capture_inputs(void) NONBANKED;
/* Exact139-byte private public-key view; no new mutable allocation. */
typedef struct {
 uint8_t first,count,focus,total_selected;
 uint16_t mask,wide;
 uint8_t cards[13][4],summary[5][4],summary_index[5];
 uint8_t resources[32],jokers[5],debuff,items[4],focus_bonus[8],detail_plain,preview_type;
 uint16_t preview_level;
} PlainCache;
typedef char plain_cache_contract[(sizeof(PlainCache)==139u&&offsetof(PlainCache,detail_plain)==135u)?1:-1];
#define pcache ((const PlainCache *)raster_loads)
#include "R74_plain_templates.inc"
/* Zero-selection handling is deliberately narrower than the host version.
 * Both original public preview paths explicitly return valid/nonhidden when
 * selection is empty; cached preview_type254 records that result.
 * Existing formatting produced these fields before this exact call boundary. */
static uint8_t eligible(const A1Text *f,uint8_t n,uint8_t plain,uint8_t *chosen){
 const GameState *g=&game;const uint8_t *k;uint8_t rank,caption,len,i;
 if(native_table_live||_cpu!=CGB_TYPE||g->phase!=PHASE_TABLE||g->ui_mode||plain||n!=2u||
    native_table_bank1_owner!=NATIVE_BANK1_VALID||raster_live||raster_fault||(VBK_REG&1u)||
    ((SVBK_REG&7u)!=1u)||SCX_REG||SCY_REG||(LCDC_REG&LCDCF_WINON)||
    !((!(LCDC_REG&LCDCF_ON))||((LCDC_REG&LCDCF_BG9C00)&&raster_loads[143u]==20u))||
    pcache->detail_plain||pcache->preview_type!=254u||g->highlighted_count||g->numeric_error||
    !g->hand_count||g->hand_count>13u||pcache->first||pcache->count!=g->hand_count||
    g->focus>=g->hand_count||pcache->focus!=g->focus||g->joker_count>5u||
    !g->deck_count||g->deck_count>128u||g->deck_pos>g->deck_count||
    g->money>9999999L||g->money<(-999999L))return 0;
 k=pcache->cards[g->focus];rank=k[0]>>2;
 if((k[0]&3u)||rank<2u||rank>14u||k[1]>3u||k[2]||k[3])return 0;
 for(i=0;i<8u;i++)if(pcache->focus_bonus[i])return 0;
 caption=k[1]*13u+rank-2u;
 len=strlen(f[0].s);
 if(!len||len>7u||f[0].x!=141u-2u*len||f[0].y!=57u||f[0].font!=3u||f[0].color!=23u)return 0;
 len=strlen(plain_caption_names[caption]);
 if(strcmp(f[1].s,plain_caption_names[caption])||f[1].x!=85u-2u*len||f[1].y!=74u||f[1].font!=3u||f[1].color!=2u)return 0;
 *chosen=caption;return 1;
}
/* Exact original a1_ui_publish ABI. The caller retains its original stack and
 * instruction flow; fallback forwards all original arguments unchanged. */
void native_plain_detail_full(uint8_t x,uint8_t y,uint8_t w,uint8_t h,
 const A1Text *f,uint8_t n,uint8_t plain) BANKED {
 uint8_t caption,row,col,index,base_index=0u,slot;const uint8_t *tile;
 if(x!=5u||y!=5u||w!=15u||h!=5u||!eligible(f,n,plain,&caption)){
  a1_ui_publish(x,y,w,h,f,n,plain);return;
 }
 for(row=0;row<5u;row++){
  for(col=0;col<15u;col++){
   index=row*15u+col;
   if(row==2u&&col>=10u)continue;
   slot=plain_templates[10096u+index];
   if(row==4u&&col<11u)tile=plain_templates+944u+(uint16_t)caption*176u+(uint16_t)col*16u;
   else tile=plain_templates+(uint16_t)base_index++*16u;
   memcpy(raster_pixels,tile,16u);VBK_REG=0;set_bkg_data(slot,1u,raster_pixels);raster_capture_inputs();
  }
  memcpy(raster_pixels+32u,plain_templates+10096u+row*15u,15u);
  memcpy(raster_pixels+48u,plain_templates+10171u+row*15u,15u);
  VBK_REG=0;set_tiles(5u,5u+row,15u,1u,(uint8_t *)0x9800u,raster_pixels+32u);
  VBK_REG=1;set_tiles(5u,5u+row,15u,1u,(uint8_t *)0x9800u,raster_pixels+48u);VBK_REG=0;raster_capture_inputs();
 }
 /* Generic compositor destroys row scratch; all full rows are already done. */
 a1_ui_publish(15u,7u,5u,1u,f,1u,0u);
 return;
}
