#include "paged_inventory.h"
#include "inventory_bounds.h"
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "raster_runtime.h"
#include "native_felt_items_felt_item_tiles.inc"
const uint16_t native_item_palettes[208]={/* OWNER_INSTALL_SLOT:native_item_palettes */};

uint8_t felt_item_copy(uint8_t kind,uint8_t id,uint8_t *out) BANKED {uint8_t k;if(kind==1u&&id<12u)k=id;else if(kind==2u&&id<22u)k=12u+id;else if(kind==3u&&id<18u)k=34u+id;else return 0;memcpy(out,felt_item_tiles+(uint16_t)k*96u,96);return 1;}
void native_table_joker_copy_0(uint8_t,uint8_t *) BANKED;
void native_table_joker_copy_1(uint8_t,uint8_t *) BANKED;
void native_table_palette_copy_0(uint8_t,uint8_t *) BANKED;
void native_table_palette_copy_1(uint8_t,uint8_t *) BANKED;
static uint8_t source_id(uint8_t tile,uint8_t row,uint8_t x){uint8_t lo,hi,bit=7u-x,offset=tile*16u+row*2u;lo=raster_pixels[offset];hi=raster_pixels[offset+1u];lo>>=bit;hi>>=bit;return (lo&1u)|((hi&1u)<<1u);}
const uint8_t native_concealed_tiles[96]={0,127,127,128,127,191,127,175,127,183,127,186,127,189,127,186,0,254,254,1,254,253,254,189,254,93,254,237,254,245,254,237,127,183,127,175,127,183,127,186,127,189,127,186,127,183,127,175,254,93,254,189,254,93,254,237,254,245,254,237,254,93,254,189,127,183,127,186,127,189,127,186,127,183,127,191,127,128,0,127,254,93,254,237,254,245,254,237,254,93,254,253,254,1,0,254};
const uint16_t native_concealed_palette[4]={9636,29663,3172,5273};
/* Exact two-color BG base plus two-color OBJ overlay. No quantization. */
uint8_t native_D_colors(uint16_t*p,uint8_t n,uint8_t edition,uint8_t phase) BANKED;
uint8_t native_D_joker_palette(uint8_t id,uint16_t*p,uint8_t edition,uint8_t phase) BANKED;
void native_D_cache_icon(uint8_t,const uint16_t *,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t) BANKED;
static const uint16_t native_negative_item_palettes[208]={/* OWNER_INSTALL_SLOT:native_negative_item_palettes */};
static const uint8_t native_bit_counts[256]={0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};
#include "cooperative_native_redirect.h"
uint8_t native_hybrid_icon(uint8_t kind,uint8_t id,uint8_t slot,uint8_t overlay,uint8_t x,uint8_t edition,uint8_t phase) BANKED {
 uint8_t t,y,px,c,b0=0,b1=1,o0=2,o1=3,k,col,row,bg,obj,index,lo,hi,ink[4];uint16_t count[4]={0,0,0,0},p[4],op[4];
 if(kind==4u){memcpy(raster_pixels,native_concealed_tiles,96u);memcpy(raster_pixels+96u,native_concealed_palette,8u);}
 else if(kind==0u){if(id>=150u)return 0;if(id<75u){native_table_joker_copy_0(id,raster_pixels);native_table_palette_copy_0(id,raster_pixels+96u);}else{native_table_joker_copy_1(id-75u,raster_pixels);native_table_palette_copy_1(id-75u,raster_pixels+96u);}}
 else {if(!felt_item_copy(kind,id,raster_pixels))return 0;k=kind==1u?id:kind==2u?12u+id:34u+id;memcpy(raster_pixels+96u,native_item_palettes+(uint16_t)k*4u,8u);}
 for(k=0;k<4u;k++){p[k]=raster_pixels[96u+k*2u];p[k]|=(uint16_t)raster_pixels[97u+k*2u]<<8u;}
 if(kind==0u&&!native_D_joker_palette(id,p,edition,phase))return 0;if(kind>=1u&&kind<=3u&&edition){if(edition!=4u)return 0;t=kind==1u?id:kind==2u?12u+id:34u+id;for(k=0;k<4u;k++)p[k]=native_negative_item_palettes[(uint16_t)t*4u+k];}
 if(overlay){
  for(t=0;t<48u;t++){
   lo=raster_pixels[t*2u];hi=raster_pixels[t*2u+1u];
   count[0]+=native_bit_counts[(uint8_t)~(lo|hi)];count[1]+=native_bit_counts[lo&(uint8_t)~hi];
   count[2]+=native_bit_counts[(uint8_t)~lo&hi];count[3]+=native_bit_counts[lo&hi];
  }
  for(k=1;k<4u;k++)if(count[k]>count[b0])b0=k;
  b1=b0?0u:1u;for(k=0;k<4u;k++)if(k!=b0&&count[k]>count[b1])b1=k;
  index=0;for(k=0;k<4u;k++)if(k!=b0&&k!=b1){if(!index)o0=k;else o1=k;index++;}
  op[0]=0;op[1]=p[o0];op[2]=p[o1];op[3]=0;set_sprite_palette(1u,1u,op);
  op[3]=p[b0];p[3]=p[b1];p[2]=op[3];op[3]=0;p[0]=9636u;p[1]=6467u;
 }
 if(kind==0u)native_D_cache_icon(id,(const uint16_t *)(raster_pixels+96u),slot,overlay,b0,b1,o0,o1,edition);
 set_bkg_palette(overlay?0u:slot,1u,p);
 if(!overlay){
  /* Source color indices and every pixel are unchanged in ordinary slots. */
  VBK_REG=1;set_bkg_data(192u+(slot-1u)*6u,6u,raster_pixels);VBK_REG=0;
 }else for(t=0;t<6u;t++){
  for(y=0;y<8u;y++){
   lo=raster_pixels[t*16u+y*2u];hi=raster_pixels[t*16u+y*2u+1u];
   ink[0]=~(lo|hi);ink[1]=lo&~hi;ink[2]=~lo&hi;ink[3]=lo&hi;
   raster_planes[y*2u]=ink[b1];raster_planes[y*2u+1u]=ink[b0]|ink[b1];
   raster_planes[16u+y*2u]=ink[o0];raster_planes[17u+y*2u]=ink[o1];
  }
  VBK_REG=1;set_bkg_data(192u+t,1u,raster_planes);VBK_REG=0;
  col=t&1u;row=t>>1;set_sprite_data(228u+col*4u+row,1u,raster_planes+16u);
 }
 if(overlay){memset(raster_planes,0,16u);set_sprite_data(231u,1u,raster_planes);set_sprite_data(235u,1u,raster_planes);for(col=0;col<2u;col++)for(row=0;row<3u;row++){index=6u+col*3u+row;set_sprite_tile(index,228u+col*4u+row);set_sprite_prop(index,1u);move_sprite(index,x+col*8u+8u,24u+row*8u);}}
 for(row=0;row<3u;row++){for(col=0;col<2u;col++)raster_planes[col]=192u+(overlay?0u:slot-1u)*6u+row*2u+col;VBK_REG=0;set_bkg_tiles(x/8u,1u+row,2u,1u,raster_planes);raster_planes[0]=raster_planes[1]=8u|(overlay?0u:slot);VBK_REG=1;set_bkg_tiles(x/8u,1u+row,2u,1u,raster_planes);VBK_REG=0;}
 return 1;
}

#include "game.h"
#include "a1_assets.h"
#include "native_busy.h"
#include "frontend_public_preview.h"


const uint16_t native_scene_card_palettes[16]={/* OWNER_INSTALL_SLOT:native_scene_card_palettes */};

static void capture_bg(uint8_t slot,uint16_t at){coop_capture_palette(0u,slot,raster_planes);raster_put_block(2u,at,raster_planes,8u);}
uint8_t native_scene_prepare(const GameState *g) BANKED {
 uint8_t ids[5],debuff,i,j,id,kind,chosen=255u,slot=2u,x,phase=(sys_time>>5)&3u;
 debuff=frontend_public_jokers_key(g,ids);
 /* Other markers and hidden classes are deliberately unsupported here. */
 if(g->joker_count>PG_JMAX||g->consumable_count>PG_CMAX)return 0;
 for(i=0;i<g->joker_count&&i<5u;i++)if(ids[i]>=150u&&ids[i]!=255u)return 0;
 if(LCDC_REG&LCDCF_ON){if(LCDC_REG&LCDCF_BG9C00)native_busy_publish();wait_vbl_done();}native_palette_stop();DISPLAY_OFF;
 a1_palette_copy(raster_pixels);set_bkg_palette(0,8,(uint16_t *)raster_pixels);set_sprite_palette(0,1,(uint16_t *)(raster_pixels+64u));
 raster_put_block(2u,0xd354u,raster_pixels+56u,8u);
 raster_put_block(2u,0xd3a0u,raster_pixels,8u);
 raster_put_block(2u,0xd35cu,raster_pixels+16u,8u);raster_put_block(2u,0xd364u,raster_pixels+32u,8u);
 raster_put_block(2u,0xd37cu,raster_pixels+24u,8u);raster_put_block(2u,0xd394u,raster_pixels+8u,8u);raster_put_block(2u,0xd4d8u,raster_pixels+8u,8u);
 raster_put_block(2u,0xd36cu,(const uint8_t *)native_scene_card_palettes,8u);raster_put_block(2u,0xd374u,(const uint8_t *)(native_scene_card_palettes+4u),8u);
 raster_put_block(2u,0xd384u,(const uint8_t *)(native_scene_card_palettes+8u),8u);raster_put_block(2u,0xd38cu,(const uint8_t *)(native_scene_card_palettes+12u),8u);
 for(i=1u;i<12u;i++)move_sprite(i,0,0);
 for(i=0;i<7u;i++){if(coop_active&&!coop_checkpoint())return 1;
  if(i<5u){if(i>=g->joker_count)continue;id=ids[i];kind=id==255u?4u:0u;}
  else {j=i-5u+inventory_consumable_first(g);if(j>=g->consumable_count)continue;kind=PG_CBYTE(g,j,consumable_kind,0u);id=PG_CBYTE(g,j,consumables,1u);}
  if(chosen==255u)chosen=i;x=i<5u?40u+i*16u:128u+(i-5u)*16u;
  if(!native_hybrid_icon(kind,id,i==chosen?0u:slot,i==chosen,x,kind==0u?inventory_joker_edition(g,i):kind>=1u&&kind<=3u?PG_CBYTE(g,j,consumable_edition,2u):0u,phase))return 0;
  if(i!=chosen)slot++;
 }

 /* Existing authored X mask, UI1 contrast colors; no secret-state artwork. */
 {static const uint8_t mask[32]={0,255,66,189,36,219,24,231,24,231,36,219,66,189,0,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};static const uint16_t palette[4]={0,7398u,28606u,0};
 /* BANKED redirected writers read after a ROM bank switch;
  * pass RAM copies (the direct raster_put_block below already reads here). */
 memcpy(raster_planes,mask,32u);memcpy(raster_planes+32u,palette,8u);
 set_sprite_data(236u,2u,raster_planes);set_sprite_palette(2u,1u,(const uint16_t *)(raster_planes+32u));raster_put_block(2u,0xd4d0u,(const uint8_t *)palette,8u);
 for(i=0;i<g->joker_count&&i<5u;i++)if(ids[i]<150u&&(debuff&(1u<<i))){set_sprite_tile(1u+i,236u);set_sprite_prop(1u+i,2u);move_sprite(1u+i,56u+i*16u,40u);}}
 capture_bg(2u,0xd348u);
 /* Phase0 has the authored top bar in slot2; line7 restores icon slot2. */
 raster_get_block(2u,0xd364u,raster_planes,8u);set_bkg_palette(2u,1u,(uint16_t *)raster_planes);
 for(i=0;i<8u;i++)capture_bg(i,0xd300u+(uint16_t)i*8u);
 coop_capture_palette(1u,1u,raster_planes);raster_put_block(2u,0xd340u,raster_planes,8u);
 /* Ephemeral native-table lease, rebuilt on every modal return. Hidden
  * identities use zero so concealed edition changes cannot leak artwork. */
 for(i=0;i<5u;i++)raster_planes[i]=i<g->joker_count&&ids[i]<150u?inventory_joker_edition(g,i):0u;
 raster_put_block(2u,0xd3a8u,raster_planes,5u);
 raster_planes[0]=0;for(i=0;i<g->joker_count&&i<5u;i++)if(ids[i]<150u&&inventory_joker_edition(g,i))raster_planes[0]|=1u<<i;
 raster_planes[1]=phase;raster_put_block(2u,0xd3adu,raster_planes,2u);
 native_palette_ready=2u;return 1;
}
