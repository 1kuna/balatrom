#include "paged_inventory.h"
#pragma bank 48
#include <gb/gb.h>
uint8_t pb_direct_read_24(uint16_t slot,uint16_t *r) BANKED {(void)slot;(void)r;return 0;}
uint8_t pb_direct_group_24(uint8_t n,uint16_t *r) BANKED {(void)n;(void)r;return 0;}

static const uint8_t dh_physical[180]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,234,235,246,247,248};
static const uint8_t dh_suits[4]={0,2,3,1};
static const uint16_t dh_words[20][4][4]={/* OWNER_INSTALL_SLOT:dh_words */};
/* Native card compositor. All source pixels retain their RGB5 colors. */
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "raster_runtime.h"
#include "a1_assets.h"
#include "native_busy.h"


#include "cooperative_native_redirect.h"
static const uint8_t dh_rev[256]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,255,255,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,175,176,255,255,255,255,255,255,255,255,255,255,177,178,179,255,255,255,255,255,255,255};
static uint8_t dh_reverse(uint8_t n){return dh_rev[n];}
static uint8_t dh_flip(uint8_t b){b=((b&0xaau)>>1)|((b&0x55u)<<1);b=((b&0xccu)>>2)|((b&0x33u)<<2);return(b>>4)|(b<<4);}
static void dh_pixel_tile(uint8_t id,uint8_t attr,uint8_t *out){uint8_t i,j=dh_reverse(id),buf[16];if(j==255u){memset(out,0,16u);return;}a1_card_tile_copy(j,buf);for(i=0;i<8u;i++){j=(attr&64u)?7u-i:i;out[i*2u]=(attr&32u)?dh_flip(buf[j*2u]):buf[j*2u];out[i*2u+1u]=(attr&32u)?dh_flip(buf[j*2u+1u]):buf[j*2u+1u];}}
/* 75 cells each: public ownership, original ID and original attributes. */
void native_D_hand_row(uint8_t row,uint8_t count,uint16_t mask,const uint8_t *x,const uint8_t (*cards)[4],const uint8_t *ids,const uint8_t *attrs) BANKED {
 uint8_t i,c,y,f;memset(raster_planes,255u,15u);
 for(i=0;i<count;i++){y=(mask&(1u<<i))?0u:1u;if(row<y||row>=y+4u)continue;f=(cards[i][0]&2u)?0u:((cards[i][1]>>2)==6u||!(cards[i][0]>>2))?0u:dh_suits[cards[i][1]&3u];f+=((cards[i][0]&2u)?0u:(cards[i][2]&15u))*4u;if(f>=20u)f=0u;for(c=0;c<3u;c++)if(x[i]+c>=5u&&x[i]+c<20u)raster_planes[x[i]+c-5u]=f;}
 raster_put_block(2u,0xd800u+(uint16_t)row*15u,raster_planes,15u);raster_put_block(2u,0xd850u+(uint16_t)row*15u,ids,15u);raster_put_block(2u,0xd8a0u+(uint16_t)row*15u,attrs,15u);
}
uint8_t native_D_hand_finish(uint8_t large,uint8_t active,uint8_t phase) BANKED {
 uint8_t role[20],cost[20],used[32],cls[5],i,j,k,col,f,best,n=0,limit=large?5u:6u,slot,sp=12u,tile=0,src,attr,any,carrier,at,flag,obj_limit;uint16_t p[4];int8_t row;
 if(!active){raster_get_block(2u,0xd3afu,&flag,1u);if(flag){if(LCDC_REG&LCDCF_ON){native_busy_publish();wait_vbl_done();}native_palette_stop();DISPLAY_OFF;for(i=0;i<180u;i++){a1_card_tile_copy(i,raster_pixels);VBK_REG=1;set_bkg_data(dh_physical[i],1u,raster_pixels);}a1_palette_copy(raster_pixels);raster_put_block(2u,0xd394u,raster_pixels+8u,8u);raster_put_block(2u,0xd37cu,raster_pixels+24u,8u);for(i=0;i<4u;i++)raster_put_block(2u,i==0u?0xd36cu:i==1u?0xd374u:i==2u?0xd384u:0xd38cu,(const uint8_t *)dh_words[i][0],8u);native_palette_ready=2u;}flag=0;raster_put_block(2u,0xd3afu,&flag,1u);for(i=12u;i<40u;i++)move_sprite(i,0,0);VBK_REG=0;return 1;}
 if(LCDC_REG&LCDCF_ON){native_busy_publish();wait_vbl_done();}native_palette_stop();DISPLAY_OFF;
 raster_get_block(2u,0xd800u,raster_planes,75u);memset(role,0,20u);memset(cost,0,20u);memset(cls,0,5u);
 for(row=0;row<5u;row+=2u)for(col=0;col<15u;col++){f=raster_planes[row*15u+col];if(f<20u)cost[f]++;if(row<4u){j=raster_planes[(row+1u)*15u+col];if(j<20u&&j!=f)cost[j]++;}}
 /* Cover every background color class, then retain the heaviest families. */
 for(i=0;i<5u;i++){best=255u;for(f=i*4u;f<i*4u+4u;f++)if(cost[f]&&(best==255u||cost[f]>cost[best]))best=f;if(best!=255u){role[best]=255u;cls[i]=1u;n++;}}
 if(n>limit)return 0;
 while(n<limit){best=255u;for(f=0;f<20u;f++)if(cost[f]&&!role[f]&&(best==255u||cost[f]>cost[best]))best=f;if(best==255u)break;role[best]=255u;n++;}
 if(!large&&n==6u){best=255u;for(f=0;f<20u;f++)if(role[f]){any=0;for(col=0;col<15u;col++)if(raster_planes[col]==f)any=1u;if(any)continue;any=0;for(col=0;col<15u;col++)if(raster_planes[col]<20u&&raster_planes[col]/4u==f/4u)any=1u;if(any){any=0;for(j=(f/4u)*4u;j<(f/4u)*4u+4u;j++)if(j!=f&&role[j])any=1u;if(!any)continue;}if(best==255u||cost[f]>cost[best])best=f;}if(best==255u)return 0;role[best]=2u;}
 slot=1u;for(f=0;f<20u;f++)if(role[f]==255u){if(slot==2u)slot++;role[f]=slot++;}
 slot=0u;for(col=0;col<15u;col++){f=raster_planes[15u+col];if(f<20u&&cost[f]&&!role[f]){if(slot>=21u)return 0;role[f]=128u|(slot/3u+1u)|((slot%3u+1u)<<3);slot++;}}
 obj_limit=(slot+2u)/3u;
 for(i=0;i<4u;i++){
  if(coop_active&&!coop_checkpoint())return 1;
  memset(raster_pixels,0,56u);raster_put_block(2u,0xd600u+(uint16_t)i*128u+48u,raster_pixels,56u);
  for(f=0;f<20u;f++)if(cost[f]){
   memcpy(p,dh_words[f][i],8u);
   if(!(role[f]&128u))raster_put_block(2u,0xd600u+(uint16_t)i*128u+(uint16_t)(role[f]-1u)*8u,(const uint8_t *)p,8u);
   else raster_put_block(2u,0xd600u+(uint16_t)i*128u+48u+(uint16_t)((role[f]&7u)-1u)*8u+(uint16_t)((role[f]>>3)&3u)*2u,(const uint8_t *)(p+3u),2u);
  }
  {if(large||n<6u){a1_palette_copy(raster_pixels);raster_put_block(2u,0xd600u+(uint16_t)i*128u+8u,raster_pixels+24u,8u);}p[0]=0;p[1]=6467u;p[2]=28606u;p[3]=7934u;raster_put_block(2u,0xd600u+(uint16_t)i*128u+96u,(const uint8_t *)p,8u);}
 }
 raster_get_block(2u,0xd600u+(uint16_t)phase*128u,raster_pixels,104u);
 raster_put_block(2u,0xd394u,raster_pixels,8u);raster_put_block(2u,0xd37cu,raster_pixels+8u,8u);raster_put_block(2u,0xd36cu,raster_pixels+16u,8u);raster_put_block(2u,0xd374u,raster_pixels+24u,8u);raster_put_block(2u,0xd384u,raster_pixels+32u,8u);raster_put_block(2u,0xd38cu,raster_pixels+40u,8u);
 raster_put_block(2u,0xd4c0u,raster_pixels+48u,16u);set_sprite_palette(3u,5u,(uint16_t *)(raster_pixels+64u));
 /* Restore precisely the original BG tiles used by this hand before eviction. */
 memset(used,0,32u);for(i=130u;i<141u;i++)used[i>>3]|=1u<<(i&7u);for(i=192u;i<242u;i++)used[i>>3]|=1u<<(i&7u);used[31]|=192u;
 for(row=0;row<5u;row++){if(coop_active&&!coop_checkpoint())return 1;
  raster_get_block(2u,0xd850u+(uint16_t)row*15u,raster_pixels,15u);raster_get_block(2u,0xd8a0u+(uint16_t)row*15u,raster_pixels+15u,15u);
  for(col=0;col<15u;col++){f=raster_planes[row*15u+col];if(f<20u){src=raster_pixels[col];used[src>>3]|=1u<<(src&7u);}}
 }
 for(i=0;i<180u;i++){src=dh_physical[i];if(used[src>>3]&(1u<<(src&7u))){a1_card_tile_copy(i,raster_pixels);VBK_REG=1;set_bkg_data(src,1u,raster_pixels);}}
 /* Pair adjacent physical slots which the current scene does not reference. */
 for(slot=1u;slot<=obj_limit;slot++)for(col=0;col<15u;col++){
  row=1;f=raster_planes[col];if(f<20u&&(role[f]&128u)&&(role[f]&7u)==slot){raster_get_block(2u,0xd850u+col,&src,1u);raster_get_block(2u,0xd8a0u+col,&attr,1u);dh_pixel_tile(src,attr,raster_pixels+64u);any=0;for(j=0;j<8u;j++)any|=raster_pixels[64u+j*2u]&raster_pixels[65u+j*2u];if(any){row=0;f=raster_planes[15u+col];if(f>=20u||!(role[f]&128u)||(role[f]&7u)!=slot)row=-1;}}
  for(;row<5;row+=2){any=0;for(k=0;k<2u;k++)if(row+k>=0&&row+k<5){f=raster_planes[(row+k)*15u+col];any|=f<20u&&(role[f]&128u)&&(role[f]&7u)==slot;}if(!any)continue;
  while(tile<254u&&((used[tile>>3]&(1u<<(tile&7u)))||(used[(tile+1u)>>3]&(1u<<((tile+1u)&7u)))))tile+=2u;if(tile>=254u)return 0;used[tile>>3]|=1u<<(tile&7u);used[(tile+1u)>>3]|=1u<<((tile+1u)&7u);
  memset(raster_pixels+64u,0,32u);
  for(k=0;k<2u;k++)if(row+k>=0&&row+k<5){f=raster_planes[(row+k)*15u+col];if(f>=20u||!(role[f]&128u)||(role[f]&7u)!=slot)continue;at=(row+k)*15u+col;raster_get_block(2u,0xd850u+at,&src,1u);raster_get_block(2u,0xd8a0u+at,&attr,1u);dh_pixel_tile(src,attr,raster_pixels+64u+k*16u);for(j=0;j<8u;j++){carrier=raster_pixels[64u+k*16u+j*2u]&raster_pixels[65u+k*16u+j*2u];raster_pixels[64u+k*16u+j*2u]=((role[f]>>3)&1u)?carrier:0u;raster_pixels[65u+k*16u+j*2u]=((role[f]>>4)&1u)?carrier:0u;}}
  any=0;for(j=0;j<32u;j++)any|=raster_pixels[64u+j];if(!any){used[tile>>3]&=~(1u<<(tile&7u));used[(tile+1u)>>3]&=~(1u<<((tile+1u)&7u));continue;}if(sp>=(large?38u:40u))return 0;
  at=104+(int16_t)row*8;if(row==3){any=0;for(j=0;j<16u;j++)any|=raster_pixels[64u+j];if(!any){memcpy(raster_pixels+64u,raster_pixels+80u,16u);memset(raster_pixels+80u,0,16u);at+=8u;}}
  VBK_REG=1;set_sprite_data(tile,2u,raster_pixels+64u);set_sprite_tile(sp,tile);set_sprite_prop(sp,8u|slot);move_sprite(sp,(col+5u)*8u+8u,at);sp++;tile+=2u;
 }}
 for(i=sp;i<40u;i++)move_sprite(i,0,0);
 /* Constant color0 BG under each transparent OBJ source-color0 pixel. */
 memset(raster_pixels,0,16u);VBK_REG=0;set_bkg_data(242u,1u,raster_pixels);
 for(row=0;row<5u;row++){if(coop_active&&!coop_checkpoint())return 1;
  raster_get_block(2u,0xd850u+(uint16_t)row*15u,raster_pixels,15u);raster_get_block(2u,0xd8a0u+(uint16_t)row*15u,raster_pixels+15u,15u);
  for(col=0;col<15u;col++){f=raster_planes[row*15u+col];if(f>=20u)continue;if(!(role[f]&128u))raster_pixels[15u+col]=(raster_pixels[15u+col]&248u)|role[f];else{carrier=255u;for(j=(f/4u)*4u;j<(f/4u)*4u+4u;j++)if(role[j]&&!(role[j]&128u)&&(row||role[j]!=2u)){carrier=role[j];break;}if(carrier==255u)return 0;raster_pixels[15u+col]=(raster_pixels[15u+col]&248u)|carrier;}}
  if(large&&row==0u){raster_pixels[0]=242u;raster_pixels[14]=242u;raster_pixels[15]=raster_pixels[29]=0u;}
  VBK_REG=0;set_bkg_tiles(5u,11u+row,15u,1u,raster_pixels);VBK_REG=1;set_bkg_tiles(5u,11u+row,15u,1u,raster_pixels+15u);
 }
 if(large){for(i=0;i<2u;i++){raster_get_block(2u,0xd850u+(i?14u:0u),&src,1u);raster_get_block(2u,0xd8a0u+(i?14u:0u),&attr,1u);a1_edge_tile_copy(src-130u,raster_pixels);for(j=0;j<8u;j++){k=(attr&64u)?7u-j:j;raster_pixels[64u+j*2u]=(attr&32u)?dh_flip(raster_pixels[k*2u]):raster_pixels[k*2u];raster_pixels[65u+j*2u]=(attr&32u)?dh_flip(raster_pixels[k*2u+1u]):raster_pixels[k*2u+1u];}memset(raster_pixels+80u,0,16u);VBK_REG=0;set_sprite_data(238u+i*2u,2u,raster_pixels+64u);set_sprite_tile(38u+i,238u+i*2u);set_sprite_prop(38u+i,7u);move_sprite(38u+i,i?160u:48u,104u);}}
 raster_put_block(2u,0xd3b0u,&phase,1u);flag=large?3u:1u;raster_put_block(2u,0xd3afu,&flag,1u);VBK_REG=0;native_palette_ready=2u;return 1;
}

void native_D_sidebar(void) BANKED {uint8_t flag,attrs[5],i,row;raster_get_block(2u,0xd3afu,&flag,1u);for(row=12u;row<14u;row++){a1_ui_map_row_copy(row,raster_pixels);for(i=0;i<5u;i++)attrs[i]=(raster_pixels[20u+i]&248u)|7u;VBK_REG=1;set_bkg_tiles(0u,row,5u,1u,attrs);}VBK_REG=0;}

#undef DISPLAY_OFF
#define DISPLAY_OFF display_off()
#undef LCDC_REG
#define LCDC_REG (*(volatile uint8_t*)0xff40u)
#undef native_palette_ready
#undef native_table_live
#undef native_palette_stop
#undef native_palette_start
#undef set_bkg_data
#undef set_sprite_data
#undef set_tiles
#undef set_bkg_tiles
#undef set_bkg_palette
#undef set_sprite_palette
#undef set_sprite_tile
#undef set_sprite_prop
#undef move_sprite
#undef native_busy_begin
#undef native_busy_publish
#undef native_busy_prepare_wait
#undef frontend_native_acquire
/* Exact-byte preserved prototype/font data: no runtime banked pointer escapes. */
#include "sidebar_dynamic_sidebar_glyph3.inc"
#include "sidebar_dynamic_sidebar_card_palette.inc"
#include "sidebar_dynamic_sidebar_proto_joker_tiles.inc"
#include "sidebar_dynamic_sidebar_proto_joker_palettes.inc"

/* Private native run mode: xoshiro128**1.1, Blackman/Vigna algorithm.
 * Reference and public-domain dedication: https://prng.di.unimi.it/xoshiro128starstar.c
 * The numeric title seed has16bits; state period does not imply128bits entropy.
 * Bank4 DFE0..DFF3 is a persistent20-byte lease. All access uses the existing
 * <=16-byte bank ferry; no IRQ executes this code or sees a switched C stack. */
#include "native_run_rng.h"
#include "raster_runtime.h"
#include <string.h>
#define NATIVE_RUN_ADDRESS 0xdfe0u
static uint32_t nr_rotate(uint32_t x,uint8_t n){return(x<<n)|(x>>(32u-n));}
static uint32_t nr_word(const uint8_t *p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static void nr_write(uint8_t*p,uint32_t v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}
uint8_t native_rng_valid(const uint8_t meta[20]) CORE_BANKED {
 uint8_t i,any=0;if(meta[0]!=1u||meta[1]!=1u||!(meta[2]|meta[3]))return 0;
 for(i=4u;i<20u;i++)any|=meta[i];return any!=0;
}
void native_rng_export(uint8_t meta[20]) CORE_BANKED {raster_get_block(4u,NATIVE_RUN_ADDRESS,meta,20u);}
uint8_t native_rng_import(const uint8_t meta[20]) CORE_BANKED {
 if(meta[0]&&!native_rng_valid(meta))return 0;
 if(!meta[0]){uint8_t i;for(i=1;i<20;i++)if(meta[i])return 0;}
 raster_put_block(4u,NATIVE_RUN_ADDRESS,meta,20u);return 1;
}
uint8_t native_rng_mode(void) CORE_BANKED {uint8_t mode;raster_get_block(4u,NATIVE_RUN_ADDRESS,&mode,1u);return mode;}
uint8_t native_rng_mode_available(uint8_t mode) CORE_BANKED {return mode==1u;}
void native_rng_pc_warning(char out[33]) CORE_BANKED {strcpy(out,"PC SEED: MUCH LONGER LOAD TIMES");}
uint8_t native_rng_begin(uint16_t seed) CORE_BANKED {
 uint8_t meta[20],i;uint32_t x,z;if(!seed)seed=12345u;meta[0]=1u;meta[1]=1u;meta[2]=seed;meta[3]=seed>>8;x=(uint32_t)seed^0xa341316cul;
 /* Explicit pinned32-bit avalanche expansion; no runtime or Lua calls. */
 for(i=0;i<4u;i++){x+=0x9e3779b9ul;z=x;z=(z^(z>>16))*0x85ebca6bul;z=(z^(z>>13))*0xc2b2ae35ul;z^=z>>16;nr_write(meta+4u+i*4u,z);}
 return native_rng_import(meta);
}
static uint32_t nr_next(uint8_t meta[20]){
 uint32_t x=nr_word(meta+8u),result;uint8_t t[4],v[4],i;
 /* Constant multiply5/9 and rotate7 preserve modulo2^32 arithmetic. */
 x=(x<<2)+x;x=(x<<7)|(x>>25);result=(x<<3)+x;
 t[0]=0;t[1]=meta[8u]<<1;t[2]=(meta[9u]<<1)|(meta[8u]>>7);t[3]=(meta[10u]<<1)|(meta[9u]>>7);
 for(i=0;i<4u;i++){
  meta[12u+i]^=meta[4u+i];meta[16u+i]^=meta[8u+i];
  meta[8u+i]^=meta[12u+i];meta[4u+i]^=meta[16u+i];meta[12u+i]^=t[i];
 }
 /* Rotate11 = rotate one byte then three bits; read before writing. */
 for(i=0;i<4u;i++)v[i]=(meta[16u+((i+3u)&3u)]<<3)|(meta[16u+((i+2u)&3u)]>>5);
 memcpy(meta+16u,v,4u);return result;
}
/* Caller-owned by-value arguments may be shifted on the stack. This routine
 * never switches banks or masks interrupts. At most32 iterations. Bound is
 * nonzero after nr_sample normalization. Result is exactly x modulo bound. */
#ifdef __SDCC
static uint16_t nr_remainder(uint32_t x,uint16_t bound) __sdcccall(0) __naked {
 __asm
 ld a,#32
 push af
 ldhl sp,#8
 ld c,(hl)
 inc hl
 ld b,(hl)
 ld de,#0
54800$:
 ldhl sp,#4
 sla (hl)
 inc hl
 rl (hl)
 inc hl
 rl (hl)
 inc hl
 rl (hl)
 rl e
 rl d
 jr c,54801$
 ld a,d
 cp b
 jr c,54802$
 jr nz,54801$
 ld a,e
 cp c
 jr c,54802$
54801$:
 ld a,e
 sub c
 ld e,a
 ld a,d
 sbc a,b
 ld d,a
54802$:
 ldhl sp,#1
 dec (hl)
 jr nz,54800$
 pop af
 ret
 __endasm;
}
#else
static uint16_t nr_remainder(uint32_t x,uint16_t bound){return x%bound;}
#endif
/* Caller-owned state allows speculative draws without touching live state.
 * Bound0 requests the legacy-shaped uniform nonzero16-bit draw domain.
 * All other bounds sample the full32-bit output with exact rejection. */
static uint8_t nr_sample(uint8_t meta[20],uint16_t bound,uint16_t*out,uint32_t*draws) {
 uint32_t r,limit;uint8_t nonzero=!bound;if(!native_rng_valid(meta)||!out)return 0;if(!bound)bound=65535u;
 limit=nr_remainder(0ul-(uint32_t)bound,bound);do{r=nr_next(meta);if(draws)(*draws)++;}while(r<limit);*out=nr_remainder(r,bound)+nonzero;return 1;
}
uint8_t native_rng_step(uint8_t meta[20],uint16_t bound,uint16_t*out) CORE_BANKED {return nr_sample(meta,bound,out,0);}
uint8_t native_rng_step_count(uint8_t meta[20],uint16_t bound,uint16_t*out,uint32_t*draws) CORE_BANKED {return nr_sample(meta,bound,out,draws);}
uint8_t native_rng_roll(uint16_t bound,uint16_t*out) CORE_BANKED {
 uint8_t meta[20];native_rng_export(meta);if(!native_rng_step(meta,bound,out))return 0;return native_rng_import(meta);
}

/* Physical storage bounds are separate from retail base+Negative rule limits. */
#include "inventory_bounds.h"
uint8_t inventory_joker_limit(const GameState*g) CORE_BANKED {uint8_t i,n=5;for(i=0;i<g->joker_count;i++){if(!(i&3u)&&coop_active&&!coop_checkpoint())return n;if(PG_JBYTE(g,i,joker_edition,9u)==4)n++;}return n;}
uint16_t inventory_consumable_limit(const GameState*g) CORE_BANKED {uint8_t i;uint16_t n=2;for(i=0;i<g->consumable_count;i++){if(!(i&3u)&&coop_active&&!coop_checkpoint())return n;if(PG_CBYTE(g,i,consumable_edition,2u)==4)n++;}return n;}
uint8_t inventory_joker_room(const GameState*g,uint8_t e) CORE_BANKED {return e<=4&&pg_fits((uint16_t)g->joker_count+1u,g->consumable_count)&&g->joker_count<inventory_joker_limit(g)+(e==4);}
uint8_t inventory_consumable_room(const GameState*g,uint8_t e) CORE_BANKED {return (e==0||e==4)&&pg_fits(g->joker_count,(uint16_t)g->consumable_count+1u)&&g->consumable_count<inventory_consumable_limit(g)+(e==4);}
uint8_t inventory_valid(const GameState*g) CORE_BANKED {uint8_t i;if(!pg_fits(g->joker_count,g->consumable_count))return 0;for(i=0;i<g->consumable_count;i++)if(PG_CBYTE(g,i,consumable_kind,0u)<1||PG_CBYTE(g,i,consumable_kind,0u)>3||PG_CBYTE(g,i,consumables,1u)>=(PG_CBYTE(g,i,consumable_kind,0u)==1?12:PG_CBYTE(g,i,consumable_kind,0u)==2?22:18)||(PG_CBYTE(g,i,consumable_edition,2u)!=0&&PG_CBYTE(g,i,consumable_edition,2u)!=4))return 0;return 1;}
uint8_t inventory_joker_first(const GameState*g) CORE_BANKED {return g->joker_count>5&&g->joker_focus>=5&&g->joker_focus<g->joker_count?g->joker_focus-4:0;}
uint8_t inventory_consumable_first(const GameState*g) CORE_BANKED {return g->consumable_count>2&&g->consumable_focus>=2&&g->consumable_focus<g->consumable_count?g->consumable_focus-1:0;}
uint8_t inventory_joker_edition(const GameState*g,uint8_t i) CORE_BANKED {i+=inventory_joker_first(g);return i<g->joker_count?PG_JBYTE(g,i,joker_edition,9u):0;}
static char *pg_decimal(char*p,uint16_t n){if(n>=100u)*p++='0'+n/100u;if(n>=10u)*p++='0'+(n/10u)%10u;*p++='0'+n%10u;return p;}
void inventory_caption(const GameState*g,uint8_t kind,char*p) CORE_BANKED {p=pg_decimal(p,kind?g->consumable_count:g->joker_count);*p++='/';p=pg_decimal(p,kind?inventory_consumable_limit(g):inventory_joker_limit(g));*p=0;}
uint8_t game_generate_consumable_edition(uint8_t kind,uint8_t id,uint8_t edition) CORE_BANKED {uint8_t i=game.consumable_count;if(kind<1||kind>3||id>=(kind==1?12:kind==2?22:18)||!inventory_consumable_room(&game,edition)){game_action_error=2;return 0;}if(!pg_resize(game.joker_count,i+1u)){game_action_error=2;return 0;}PG_CBYTE(&game,i,consumable_kind,0u)=kind;PG_CBYTE(&game,i,consumables,1u)=id;PG_CBYTE(&game,i,consumable_edition,2u)=edition;game.consumable_count++;return 1;}

/* Bank4 D870..D874, disjoint policy111 bytes and renderer throughD780. */
uint8_t inventory_native_dirty(const GameState*g) CORE_BANKED {uint8_t key[5],old[5],i=inventory_consumable_first(g);key[0]=173;key[1]=inventory_joker_first(g);key[2]=i;key[3]=PG_CBYTE(g,i,consumable_edition,2u);key[4]=PG_CBYTE(g,i+1,consumable_edition,2u);raster_get_block(4,0xd870,old,5);raster_put_block(4,0xd870,key,5);return memcmp(key,old,5)!=0;}

#include "joker_repetition.h"
#include "joker_ids.h"
uint8_t inventory_shop_end_allowed(const GameState*g) CORE_BANKED {uint8_t i,n=0;JokerCopy copy;if(!g->consumable_count)return 1;for(i=0;i<g->joker_count;i++){jc_resolve(g,i,&copy);if(copy.status==JC_OK&&PG_JBYTE(g,copy.target_index,jokers,0u)==J_PERKEO)n++;}return pg_fits(g->joker_count,(uint16_t)g->consumable_count+n);}
uint8_t inventory_perkeo(GameState*g) CORE_BANKED {uint8_t i;if(g!=&game)return 0;if(!g->consumable_count)return 1;if(!inventory_consumable_room(g,ED_NEGATIVE)){game_action_error=2;return 0;}i=game_random_index(g->consumable_count);return game_generate_consumable_edition(PG_CBYTE(g,i,consumable_kind,0u),PG_CBYTE(g,i,consumables,1u),ED_NEGATIVE);}
