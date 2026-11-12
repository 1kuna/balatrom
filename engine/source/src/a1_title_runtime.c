#pragma bank 255
#include "a1_title_runtime.h"
#include "frontend_numeric.h"
#include <gb/cgb.h>
#include <string.h>
#include "shop154.h"
#define set_bkg_data shop154_data
#define set_bkg_tiles shop154_tiles
#define set_bkg_palette shop154_palette
#include "title_A_assets.inc"
void raster_capture_inputs(void) NONBANKED;
/* Bank83 original BalatROM TITLE-only immutable assets. Bank0 HUD IDs88..209; bank1 WAIT and
 * all existing cache/scratch/WRAM/input ownership remain untouched. */
static void title_seed(uint16_t seed,uint8_t slot) {
 char number[16];uint8_t tiles[48],i,j,row,x,mask;
 frontend_format_number_width(&seed,2,number,16);
 /* Palette0 index0 is the authored BalatROM seed background. */
 for(i=0;i<48u;i+=2u){tiles[i]=0u;tiles[i+1u]=0u;}
 for(i=0;i<5u&&number[i];i++){
  const uint8_t *glyph=balatrom_title_digits+(uint16_t)(uint8_t)(number[i]-'0')*5u;
  for(row=0;row<5u;row++)for(j=0;j<3u;j++)if(glyph[row]&(4u>>j)){
   x=2u+i*4u+j;mask=0x80u>>(x&7u);
   tiles[(x>>3)*16u+(row+1u)*2u]&=~mask;
   tiles[(x>>3)*16u+(row+1u)*2u+1u]|=mask;
  }
 }
 for(i=0;i<3u;i++){raster_capture_inputs();VBK_REG=0;set_bkg_data(slot+i,1,tiles+i*16u);}
}
void a1_title_paint(uint16_t seed,uint8_t resume,uint8_t *seed_slot,uint8_t *hud,uint8_t full) BANKED {
 uint8_t row,i;uint16_t offset;
 if(full){
  *seed_slot=207u;*hud=210u;
  if(_cpu==CGB_TYPE)set_bkg_palette(0,3,title_A_palettes);
  /* Bound each transfer and retain existing input capture during full paint. */
  for(i=0;i<122u;i+=8u){raster_capture_inputs();VBK_REG=0;set_bkg_data(88u+i,i>114u?122u-i:8u,title_A_tiles+(uint16_t)i*16u);}
  for(row=0;row<18u;row++){
   raster_capture_inputs();offset=(uint16_t)row*20u;VBK_REG=0;set_bkg_tiles(0,row,20,1,title_A_map+offset);
   if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(0,row,20,1,title_A_attrs+offset);VBK_REG=0;}
  }
  if(resume)for(i=0;i<28u;i+=4u){
   offset=title_A_resume_patch[i]|((uint16_t)title_A_resume_patch[i+1u]<<8);
   VBK_REG=0;set_bkg_tiles(offset%20u,offset/20u,1,1,title_A_resume_patch+i+2u);
   if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(offset%20u,offset/20u,1,1,title_A_resume_patch+i+3u);VBK_REG=0;}
  }
 }
 /* Only these three reserved pattern slots change on a seed update. */
 title_seed(seed,*seed_slot);
}
void a1_title_request(uint8_t key) BANKED {
 uint8_t attrs[14];
 /* Existing exact START/A-valid admission gate and transition lock remain
  * authoritative. Palette2 swaps cream/gold on the chosen command row. */
 if(_cpu!=CGB_TYPE)return;set_bkg_palette(2,1,title_A_palettes+8);memset(attrs,2,sizeof(attrs));VBK_REG=1;
 set_bkg_tiles(3,key==J_A?11u:9u,14,1,attrs);VBK_REG=0;
}
