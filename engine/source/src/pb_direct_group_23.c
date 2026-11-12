#include "paged_inventory.h"
#ifdef __SDCC
#pragma bank 43
#include <gb/gb.h>
#else
#include <stdint.h>
#define BANKED
#endif
#include <string.h>
extern uint8_t raster_pixels[128],raster_planes[96];
static const uint8_t keys[60]={5,130,0,0,242,243,5,130,1,0,242,243,5,130,2,0,242,243,5,130,3,0,242,243,5,130,4,0,242,243,5,130,5,0,242,243,5,130,6,0,242,243,5,130,7,0,242,243,5,130,8,0,242,243,5,130,9,0,242,243};
static const uint8_t values[980]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,80,48,128,128,5,3,40,24,64,192,2,1,20,12,160,96,1,0,10,6,80,48,128,128,5,3,40,24,64,192,2,1,12,10,96,80,0,128,6,5,48,40,128,64,3,2,24,20,192,160,1,1,12,10,96,80,0,128,6,5,48,40,128,64,1,1,12,10,96,80,0,128,6,5,48,40,128,64,3,2,24,20,192,160,1,1,12,10,96,80,0,128,6,5,48,40,0,0,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,85,204,170,153,85,51,170,102,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,182,109,109,219,219,182,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,40,24,66,193,20,12,161,96,10,6,80,48,133,131,6,12,48,96,131,6,24,48,193,131,12,24,96,193,6,12,48,96,131,6,24,48,193,131,12,24,96,193,6,12,48,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,34,160,144,1,129,10,9,80,72,128,64,5,4,40,36,64,32,2,2,20,18,160,144,1,129,10,9,80,72,128,64,5,4,6,12,48,96,128,0,3,6,24,48,192,128,1,3,12,24,96,192,0,1,6,12,48,96,128,0,3,6,24,48,192,128,0,1,6,12,48,96,128,0,3,6,24,48,192,128,1,3,12,24,96,192,0,1,6,12,48,96,128,0,3,6,24,48};
static const uint8_t indexes[128]={255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,255,255,0,0,0,0,0,0,6,0,98,0,24,0,136,1,30,0,234,1,48,0,16,3,54,0,114,3,36,0,76,2,42,0,174,2,12,0,196,0,18,0,38,1};
static const uint8_t displacements[64]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,7,0,0,0,0,0,0,0,0,0,0,0};

uint8_t pb_direct_read_23(uint16_t slot,uint16_t *recipe) BANKED {
 uint16_t at,offset;
 if(slot>=32u || !recipe)return 0;
 at=slot*4u;
 if(indexes[at]==255u && indexes[at+1u]==255u)return 0;
 offset=(uint16_t)indexes[at+2u]|((uint16_t)indexes[at+3u]<<8);
 *recipe=(uint16_t)values[offset]|((uint16_t)values[offset+1u]<<8);
 memcpy(raster_planes,values+offset+2u,96u);return 1;
}
uint8_t pb_direct_group_23(uint8_t length,uint16_t *recipe) BANKED {
 uint16_t h1,h2,bucket,at,displacement,slot,offset;
 if(!length || length>119u || !recipe)return 0;
 h1=(uint16_t)raster_pixels[124]|((uint16_t)raster_pixels[125]<<8);
 h2=(uint16_t)raster_pixels[126]|((uint16_t)raster_pixels[127]<<8);
 bucket=(h1^(h1>>5u))&31u;at=bucket*2u;
 displacement=(uint16_t)displacements[at]|((uint16_t)displacements[at+1u]<<8);
 slot=(h2+displacement)&31u;at=slot*4u;
 offset=(uint16_t)indexes[at]|((uint16_t)indexes[at+1u]<<8);
 if(offset==65535u || keys[offset]!=length || memcmp(keys+offset+1u,raster_pixels,length))return 0;
 if(!pb_direct_read_23(slot,recipe))return 0;
 raster_pixels[120]=(uint8_t)slot;raster_pixels[121]=(uint8_t)((slot>>8)|92u);
 return 1;
}

#include "inventory_bounds.h"
#include "native_inventory_cache.h"
#include "frontend_public_preview.h"
#include "table_1998_data.h"
#include "a1_assets.h"
#include <gb/cgb.h>
#include <string.h>
#define MAP_DEST 0x9800u
#define ICON_BASE 192u
void raster_capture_inputs(void) NONBANKED;
void native_table_joker_copy_0(uint8_t,uint8_t*) BANKED;
void native_table_joker_copy_1(uint8_t,uint8_t*) BANKED;
uint8_t felt_item_copy(uint8_t,uint8_t,uint8_t*) BANKED;
uint8_t native_scene_prepare(const GameState*) BANKED;
#include "cooperative_native_redirect.h"
static void inventory_region(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *ids,const uint8_t *attrs){
 VBK_REG=0;set_tiles(x,y,w,h,(uint8_t *)MAP_DEST,ids);
 VBK_REG=1;set_tiles(x,y,w,h,(uint8_t *)MAP_DEST,attrs);VBK_REG=0;raster_capture_inputs();
}
uint8_t inventory_icons(const GameState *g,uint8_t force) BANKED {
 uint8_t ids[5],editions[5],debuff,i,j,id,changed=0,map[6],attrs[6],pal;
 if(inventory_native_dirty(g))force=1;debuff=frontend_public_jokers_key(g,ids);
 if(!force)raster_get_block(2u,0xd3a8u,editions,5u);
 for(i=0;i<7u;i++){
  if(i<5u){id=ids[i];pal=id<150u?table_1998_joker_families[id]:5u;
   if(!force&&id==PG_JBYTE(cache,i,jokers,0u)&&!((debuff^cache->debuff)&(1u<<i))&&editions[i]==(i<g->joker_count&&id<150u?inventory_joker_edition(g,i):0u))continue;
   if(i>=g->joker_count)id=254u;
  }else{j=i-5u+inventory_consumable_first(g);id=j<g->consumable_count?PG_CBYTE(g,j,consumables,1u):254u;pal=PG_CBYTE(g,j,consumable_kind,0u)==ITEM_TAROT?5u:6u;
   if(!force&&id==cache->items[j*2u+1u]&&PG_CBYTE(g,j,consumable_kind,0u)==cache->items[j*2u])continue;
  }
  changed=1;
  if(id==254u){for(j=0;j<3u;j++){a1_ui_map_row_copy(1u+j,raster_pixels+32u);map[j*2u]=raster_pixels[32u+(i<5u?5u+i*2u:16u+(i-5u)*2u)];map[j*2u+1u]=raster_pixels[33u+(i<5u?5u+i*2u:16u+(i-5u)*2u)];attrs[j*2u]=raster_pixels[52u+(i<5u?5u+i*2u:16u+(i-5u)*2u)];attrs[j*2u+1u]=raster_pixels[53u+(i<5u?5u+i*2u:16u+(i-5u)*2u)];}}
  else if(id==255u||id>=150u){a1_rail_back_recipe_copy(raster_pixels);for(j=0;j<6u;j++){map[j]=raster_pixels[j*2u];attrs[j]=raster_pixels[j*2u+1u];}}
  else{
   if(i<5u){if(id<75u)native_table_joker_copy_0(id,raster_pixels);else native_table_joker_copy_1(id-75u,raster_pixels);}
   else if(!felt_item_copy(PG_CBYTE(g,i-5u+inventory_consumable_first(g),consumable_kind,0u),id,raster_pixels))memset(raster_pixels,0,96);
   if(pal==7u){for(j=0;j<96u;j+=2u)raster_pixels[j+1u]&=~raster_pixels[j];pal=5u;}
   VBK_REG=1;set_bkg_data(ICON_BASE+i*6u,6,raster_pixels);VBK_REG=0;raster_capture_inputs();
   for(j=0;j<6u;j++){map[j]=ICON_BASE+i*6u+j;attrs[j]=8u|pal;}
  }
  if(i<5u&&id<150u&&(debuff&(1u<<i))){map[5]=140u;attrs[5]=8u|pal;}
  inventory_region(i<5u?5u+i*2u:16u+(i-5u)*2u,1,2,3,map,attrs);
 }
 memcpy(cache->jokers,ids,5);cache->debuff=debuff;
 for(i=0;i<2u;i++){j=i+inventory_consumable_first(g);cache->items[i*2u]=PG_CBYTE(g,j,consumable_kind,0u);cache->items[i*2u+1u]=j<g->consumable_count?PG_CBYTE(g,j,consumables,1u):254u;}
 if(changed)native_scene_prepare(g);return changed;
}

#include "frontend_art_loader.h"
#include "boss_optional.h"
#define BACK_BASE 75u
static void inventory_debuff(uint8_t x,uint8_t y){uint8_t tile=85u,attr=1u;raster_capture_inputs();VBK_REG=0;set_bkg_tiles(x,y,1,1,&tile);if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,1,1,&attr);VBK_REG=0;}}
void inventory_owned_icon(const GameState *g,uint8_t x,uint8_t y,uint8_t slot,uint8_t index) BANKED {if(boss_joker_hidden(g,index)){uint8_t j,map[6],attrs[6];for(j=0;j<6;j++){map[j]=BACK_BASE+j;attrs[j]=1;}VBK_REG=0;set_bkg_tiles(x,y,2,3,map);if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,2,3,attrs);VBK_REG=0;}}else{frontend_load_icon(x,y,slot,0,PG_JBYTE(g,index,jokers,0u));if(PG_DEBUFF(g,index))inventory_debuff(x+1,y+2);}}
