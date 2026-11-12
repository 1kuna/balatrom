#include "paged_inventory.h"
#pragma bank 255
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include <stddef.h>
#include "retained_caret.h"
#include "retained_selection.h"
#include "pb_direct_compiler.h"
#include "pb_selection_dispatch.h"
#include "raster_runtime.h"
#include "frontend_panel_b_internal.h"
#include "frontend_panel_b_overlay.h"
#include "frontend_panel_b_hotpath.h"
#include "pb_lower_rom_cache.h"
#include "frontend_numeric.h"
#include "frontend_public_preview.h"
uint16_t raster_lower_recipe(const PBOverlayPalettes *,const uint8_t *,uint8_t) BANKED;
uint8_t pb_atlas(const Frame *,const PBOverlayPalettes *,uint8_t,uint16_t *) BANKED;
uint8_t retained_input_take(const GameState *) BANKED;
uint8_t retained_preview_same(const GameState *,const HandPreview *,uint8_t) BANKED;
uint8_t retained_slots_begin(uint8_t *,uint8_t *) BANKED;
void retained_slots_record(uint8_t,uint8_t,const uint8_t *,const uint8_t *) BANKED;
void retained_slots_finish(uint8_t,const uint8_t *,const uint8_t *,uint8_t) BANKED;
void retained_slots_commit(void) BANKED;
void retained_slots_caret_begin(void) BANKED;
void retained_slots_caret_end(void) BANKED;
void retained_slots_materialize(void) BANKED;
void retained_slots_repack_build(void) BANKED;
/* Explicit public ownership projection. Restore presentation cues before
 * returning; independently-owned caret payloads never bake into BG/OBJ base. */
static uint8_t atlas_base(const Frame *f,const PBOverlayPalettes *p,uint8_t profile,uint8_t pair,uint16_t *recipe){
 uint8_t i,focus=0,hit;
 if(retained_caret_mode)for(i=0;i<f->page.count;i++)if(f->cards[i].cues&PANEL_B_FOCUSED){focus|=1u<<i;((Frame *)f)->cards[i].cues&=~PANEL_B_FOCUSED;}
 hit=pb_atlas_preclassified(f,p,profile,pair,recipe,0);
 for(i=0;i<f->page.count;i++)if(focus&(1u<<i))((Frame *)f)->cards[i].cues|=PANEL_B_FOCUSED;
 return hit;
}
static uint8_t lookups[3][41];
uint8_t raster_cache_policy(Frame *) BANKED;
uint8_t raster_cache_dirty(const Frame *,const PBOverlayPalettes *,uint8_t *) BANKED;
void raster_cache_save(const Frame *,const PBOverlayPalettes *) BANKED;
void raster_cache_full_key(void) BANKED;
static uint8_t changed(const uint8_t *dirty,uint8_t pair){return !!(dirty[pair>>3]&(1u<<(pair&7u)));}
#include "panel_b_lower_face.h"
#include "panel_b_narrow_native.h"
#include "panel_b_lower_hs.h"
void raster_compose(const Frame *,uint8_t,uint8_t) BANKED;
const uint16_t raster_Frame_layout[2]={sizeof(Frame),offsetof(Frame,jwords)};
#include "semantic_rgb.inc"
static const uint8_t face[56]={0,19,5,12,1,19,12,3,1,4,14,3,0,1,17,18,1,13,17,0,1,13,17,2,1,13,17,14,1,2,13,18,1,3,13,0,13,17,17,0,2,0,14,17,3,19,2,3,19,14,0,18,19,7,14,19};
static const uint8_t upper_base[32]={0,19,1,18,0,1,13,17,0,1,3,13,0,1,13,17,0,1,13,17,0,1,13,17,0,1,13,17,0,1,13,17};
static const uint8_t upper_obj[24]={1,13,17,0,1,13,0,13,17,1,3,13,0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t member(const uint8_t *p,uint8_t n,uint8_t c){uint8_t i;for(i=0;i<n;i++)if(p[i]==c)return i;return 255;}
static uint16_t word(const Frame *f,uint8_t id){return id<21u?rgb[id]:f->jwords[id-21u]&0x7fffu;}

static void pack_codes(void) __naked {
 __asm
 ld de,#_raster_planes
 ld hl,#_raster_pixels
46000$:
 ld b,#0
 ld c,#0
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rra
 rl b
 rra
 rl c
 ld a,b
 ld (de),a
 inc de
 ld a,c
 ld (de),a
 inc de
 ld a,e
 cp #<(_raster_planes+32)
 jp nz,46000$
 ld hl,#_raster_pixels
46001$:
 ld b,#0
 ld c,#0
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,b
 ld (de),a
 inc de
 ld a,c
 ld (de),a
 inc de
 ld a,e
 cp #<(_raster_planes+64)
 jp nz,46001$
 ld hl,#_raster_pixels
46002$:
 ld b,#0
 ld c,#0
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,(hl+)
 rrca
 rrca
 rrca
 rrca
 rra
 rl b
 rra
 rl c
 ld a,b
 ld (de),a
 inc de
 ld a,c
 ld (de),a
 inc de
 ld a,e
 cp #<(_raster_planes+96)
 jp nz,46002$
 ret
 __endasm;
}
static void maps(const PBOverlayPalettes *p,uint16_t r){
 uint8_t i,c,n=(r>>12)&3u;
 memset(lookups,255,82);memset(lookups[2],0,41);
 for(i=0;i<4u;i++){
  c=p->bg[(r&7u)*4u+i];if(lookups[0][c]==255u)lookups[0][c]=i;
  c=p->bg[((r>>3)&7u)*4u+i];if(lookups[1][c]==255u)lookups[1][c]=i;
 }
 if(n)for(i=0;i<3u;i++){c=p->obj[((r>>6)&7u)*3u+i];if(!lookups[2][c])lookups[2][c]=i+1u;}
 if(n==2u)for(i=0;i<3u;i++){c=p->obj[((r>>9)&7u)*3u+i];if(!lookups[2][c])lookups[2][c]=(i+1u)<<2;}
}
static uint8_t lower_encode(const PBOverlayPalettes *p,uint16_t r){
 uint8_t i,c,b,o,n=(r>>12)&3u;
 if(r==65535u||n>2u||(r&49152u))return 0;
 maps(p,r);
 for(i=0;i<128u;i++){
  c=raster_pixels[i];if(c>20u)return 0;b=lookups[i>>6][c];o=0;
  if(b==255u){b=0;o=lookups[2][c];if(!o)return 0;}
  raster_pixels[i]=b|(o<<2);
 }
 pack_codes();return 1;
}
static uint8_t upper_policy(const Frame *f,PBOverlayPalettes *p){
 uint8_t i,j,n=0,c,lo,hi,id;
 memcpy(p->bg,upper_base,32);memcpy(p->obj,upper_obj,24);
 for(i=0;i<5u;i++){
  if(PG_JBYTE(f,i,jokers,0u).kind==PANEL_B_FACE)memcpy(p->bg+(3u+i)*4u,f->jids+i*4u,4);
  else if(PG_JBYTE(f,i,jokers,0u).kind==PANEL_B_HIDDEN){p->bg[(3u+i)*4u]=0;p->bg[(3u+i)*4u+1u]=1;p->bg[(3u+i)*4u+2u]=2;p->bg[(3u+i)*4u+3u]=13;}
 }
 if(PG_JBYTE(f,0,jokers,0u).kind==PANEL_B_FACE){
  for(i=0;i<8u;i++){
   j=i*2u+16u;lo=f->icons[0][j];hi=f->icons[0][j+1u];id=(lo&1u)|((hi&1u)<<1);c=f->jids[id];
   if(member(p->obj+12u,n,c)==255u){if(n==3u)return 0;p->obj[12u+n++]=c;}
  }
 }else if(PG_JBYTE(f,0,jokers,0u).kind==PANEL_B_HIDDEN){p->obj[12]=2;p->obj[13]=13;n=2;}
 while(n<3u){p->obj[12u+n]=p->obj[12];n++;}
 return 1;
}

/* CRAM108: BG0 entry2 is1 for target ink, then5 after row32; entry3
 * changes18 to12 after row37. Row32 never consumes entry2. The next
 * whole VBlank restore reinstates both upper words from the same batch. */
static uint8_t upper_bg_member(const PBOverlayPalettes *p,uint8_t palette,uint8_t c,uint8_t y){
 if(!palette&&y>=32u){if(c==5u)return 2;if(c==1u)return 255;}
 if(!palette&&y>37u){if(c==12u)return 3;if(c==18u)return 255;}
 return member(p->bg+palette*4u,4,c);
}
/* Upper native icon BG ownership and canonical exterior opacity follow the
 * catalog palette grammar. The edge palette contains only public
 * right-column ink. Every encoded pixel is checked against its source ID. */
static uint16_t upper_encode(const Frame *f,const PBOverlayPalettes *p,uint8_t x0,uint8_t y0){
 uint8_t bp[2],needs[16],owner[2],colours[8],need,h,i,j,k,c,a,n=0,layers=0,op0=0,op1=0,found=0,x,y,b,o;uint16_t r;
 for(h=0;h<2u;h++){
  owner[h]=255;
  if(y0+h*8u>=8u&&y0+h*8u<32u)for(i=0;i<5u;i++)if(PG_JBYTE(f,i,jokers,0u).kind&&x0<=PG_JBYTE(f,i,jokers,0u).x+16u&&x0+7u>=PG_JBYTE(f,i,jokers,0u).x+1u)owner[h]=i;
  if(y0==0u&&x0==56u&&h==1u&&PG_JBYTE(f,0,jokers,0u).kind){owner[h]=255;bp[h]=1;}
  else if(owner[h]!=255u)bp[h]=3u+owner[h];
  else{
   bp[h]=1;
   for(a=x0<40u?0u:2u;a<3u;a++){
    for(i=h*64u;i<(uint16_t)(h+1u)*64u;i++)if(upper_bg_member(p,a,raster_pixels[i],y0+(i>>3))==255u)break;
    if(i==(h+1u)*64u){bp[h]=a;break;}
   }
   /* Narrow target ink spans a sidebar/felt boundary. If no BG alone fits,
    * retain its exclusive ink18 in BG0; public exterior colors use OBJ.
    * BG0 contains18 only before the scheduled row37 palette change. */
   if(a==3u&&x0<40u&&y0+h*8u<32u&&f->target[5])
    for(i=h*64u;i<(uint16_t)(h+1u)*64u;i++)if(raster_pixels[i]==18u){bp[h]=0;break;}
  }
 }
 r=(uint16_t)bp[0]|((uint16_t)bp[1]<<3);maps(p,r);
 /* Gap halves use the restored words; target ink ends at30. No source
  * pixel consumes entry2 on row32 or the later entry3 before its change. */
 for(h=0;h<2u;h++)if(!bp[h]&&y0+h*8u>=32u){lookups[h][1]=255;lookups[h][5]=2;lookups[h][18]=255;lookups[h][12]=3;}
 /* One bit owns each boolean exterior/OBJ decision. This scratch remains
  * in this encoder invocation only; it carries no recipe or physical slot. */
 memset(needs,0,16);
 for(i=0;i<128u;i++){
  h=i>>6;c=raster_pixels[i];if(c>=41u)return 65535u;x=x0+(i&7u);y=y0+(i>>3);
  need=lookups[h][c]==255u;
  if(owner[h]!=255u&&(x<PG_JBYTE(f,owner[h],jokers,0u).x+1u||x>PG_JBYTE(f,owner[h],jokers,0u).x+16u||y<8u||y>=32u))need=1;
  if(y0==0u&&x0==56u&&i>=64u&&x==56u&&PG_JBYTE(f,0,jokers,0u).kind)need=1;
  if(need)needs[i>>3]|=1u<<(i&7u);
  if(need&&member(colours,n,c)==255u){if(n>=8u)return 65535u;colours[n++]=c;}
 }
 if(n)for(layers=1;layers<=2u&&!found;layers++)for(j=0;j<5u&&!found;j++)for(k=layers==1u?j:j+1u;k<5u;k++){
  for(i=0;i<n;i++)if(member(p->obj+j*3u,3,colours[i])==255u&&member(p->obj+k*3u,3,colours[i])==255u)break;
  if(i==n){found=1;op0=j;op1=k;break;}if(layers==1u)break;
 }
 if(n){if(!found)return 65535u;--layers;}
 r|=((uint16_t)op0<<6)|((uint16_t)op1<<9)|((uint16_t)layers<<12);
 memset(lookups[2],0,41);
 if(layers)for(i=0;i<3u;i++){c=p->obj[op0*3u+i];if(!lookups[2][c])lookups[2][c]=i+1u;}
 if(layers==2u)for(i=0;i<3u;i++){c=p->obj[op1*3u+i];if(!lookups[2][c])lookups[2][c]=(i+1u)<<2;}
 for(i=0;i<128u;i++){
  c=raster_pixels[i];need=!!(needs[i>>3]&(1u<<(i&7u)));
  b=need?0u:lookups[i>>6][c];o=need?lookups[2][c]:0u;
  if(b==255u||(need&&!o))return 65535u;
  raster_pixels[i]=b|(o<<2);
 }
 pack_codes();return r;
}
static uint8_t shift(uint8_t *p,int8_t dy){
 uint8_t out[32],r;int8_t to;
 memset(out,0,32);
 for(r=0;r<16u;r++){to=(int8_t)r-dy;if(to<0||to>=16){if(p[r*2u]||p[r*2u+1u])return 0;}else{out[(uint8_t)to*2u]=p[r*2u];out[(uint8_t)to*2u+1u]=p[r*2u+1u];}}
 memcpy(p,out,32);return 1;
}
static uint8_t add_object(uint8_t pair,uint8_t layer,uint8_t palette,int8_t dy){
 uint8_t upper=pair<60u,count=upper?raster_upper_count:raster_lower_count,y=pair/20u*16u,row,x=pair%20u*8u,tile;
 int16_t top=(int16_t)y+dy;uint16_t address;
 if(count>=(upper?20u:40u)||top<0||top>128)return 0;
 if(upper&&top+16>32)return 0;
 if(!upper&&top<48)return 0;
 for(row=(uint8_t)top;row<(uint16_t)top+16u;row++){if(++raster_loads[row]>raster_peak)raster_peak=raster_loads[row];}
 tile=(upper?0u:40u)+count*2u;
 address=(upper?0xd000u:0xd100u)+(uint16_t)count*4u;
 raster_put(address,(uint8_t)top+16u);raster_put(address+1u,x+8u);raster_put(address+2u,tile);raster_put(address+3u,palette);
 if(upper)raster_upper_count++;else raster_lower_count++;
 layer;return 1;
}
/* A completed upper plan belongs to this render_frame invocation only.
 * Lower policy/origin retries cannot change its source, palettes or geometry.
 * Upper objects occupy only y0..31; lower objects start at y48 or later. */
static uint8_t upper_cached;
static uint8_t prepare(const Frame *f,const PBOverlayPalettes *upper,uint8_t repair,uint8_t family,uint8_t variant,const uint8_t *dirty,uint8_t reuse,uint8_t guarded){
 uint8_t pair,x,y,layer,n,pal,i,old0=0,old1=0,oldn=0,count,meta[4],old[4],upper_profile,lower_profile;uint16_t recipe,at,address;int8_t dy;
 upper_profile=pb_atlas_profile(upper);lower_profile=pb_atlas_profile((const PBOverlayPalettes *)f->bg);
 if(upper_cached){
  memset(raster_loads+32,0,112);raster_lower_count=0;
  raster_peak=raster_loads[0]>raster_loads[16]?raster_loads[0]:raster_loads[16];
 }else{memset(raster_loads,0,144);raster_upper_count=raster_lower_count=raster_peak=0;}
 /* FFE8 owns prospective map count until admission, then publication flags.
  * IRQs never read this scratch; failed retries reset it before reuse. */
 if(repair!=2u){
  *(volatile uint8_t *)0xffe8=0;
  if(upper_cached&&reuse&&raster_dirty_pairs<180u)
   for(i=0;i<60u;i++)*(volatile uint8_t *)0xffe8+=changed(dirty,i);
 }
 memset(raster_planes,0,96);
 if(!upper_cached){retained_selection_put(raster_build_bank,0xd000,raster_planes,96);retained_selection_put(raster_build_bank,0xd060,raster_planes,64);}
 retained_selection_put(raster_build_bank,0xd100,raster_planes,96);retained_selection_put(raster_build_bank,0xd160,raster_planes,64);
 for(pair=upper_cached?60u:0u;pair<180u;pair++){
  x=pair%20u*8u;y=pair/20u*16u;at=0xd300u+(uint16_t)pair*4u;
  if(reuse||repair==2u){retained_selection_get(repair==2u?raster_build_bank:raster_active_bank,at,old,4);oldn=(old[1]>>4)&3u;if(oldn>2u)goto failure;}
  if(repair==2u||!changed(dirty,pair)){
   recipe=(uint16_t)old[0]|((uint16_t)old[1]<<8);memcpy(meta,old,4);
   for(layer=0;layer<oldn;layer++){
    /* Retained active WRAM contains the published OBJ planes. IRQ DMA now
     * returns via JP(HL), so it never writes this bank through a stack alias. */
    address=(pair<60u?0xd600u:0xd880u)+(uint16_t)(pair<60u?old0:old1)*32u+layer*32u;
    retained_selection_get(repair==2u?raster_build_bank:raster_active_bank,address,raster_planes+32u+layer*32u,32);
   }
  }else{
   if(atlas_base(f,pair<60u?upper:(const PBOverlayPalettes *)f->bg,pair<60u?upper_profile:lower_profile,pair,&recipe)){
    /* Exact compiled canonical source; ordinary complete admission below. */
   }else if(pair>=60u&&pb_lower_rom_static(f,x,y,raster_planes,&recipe)){
    /* Exact invariant source layer, unshifted. The existing complete origin
     * normalization, clipping, staging and admission remain mandatory. */
   }else{
   raster_compose(f,x,y);
   if(pair<60u)recipe=upper_encode(f,upper,x,y);
   else{recipe=raster_lower_recipe((const PBOverlayPalettes *)f->bg,raster_pixels,2);if(recipe!=65535u&&!lower_encode((const PBOverlayPalettes *)f->bg,recipe))goto failure;}
   }
   if(recipe==65535u)goto failure;
   if(guarded&&!retained_selection_bg_guard(pair,raster_planes))goto failure;
   raster_stage_bg(pair,raster_planes);
   meta[0]=recipe;meta[1]=recipe>>8;meta[2]=meta[3]=0;
   /* Existing BG scratch is retired after staging. Reuse its first4 bytes
    * for this dirty pair's map command; all OBJ planes start at+32 and remain
    * intact for the origin repair/admission below. Bank6 endsBG atD680. */
   if(reuse&&raster_dirty_pairs<180u){
    address=0x9800u+(uint16_t)y*4u+(x>>3);
    raster_planes[0]=address;raster_planes[1]=address>>8;
    raster_planes[2]=(meta[0]&7u)|(pair<136u?8u:0u);
    raster_planes[3]=((meta[0]>>3)&7u)|(pair<120u?8u:0u);
    retained_selection_put(6,0xd800u+(uint16_t)(*(volatile uint8_t *)0xffe8)*4u,raster_planes,4);
    ++(*(volatile uint8_t *)0xffe8);
   }
  }
  if(reuse||repair==2u){if(pair<60u)old0+=oldn;else old1+=oldn;}
  n=(recipe>>12)&3u;
  for(layer=0;layer<n;layer++){
   dy=(int8_t)meta[2u+layer];pal=(recipe>>(layer?9:6))&7u;
   if(repair&&pair>=60u&&(repair==2u||changed(dirty,pair))){
    /* A completed overfull plan owns every prospective recipe and plane.
     * Normalize its prior origins before evaluating the complete replacement
     * vector. Source and destination slot counts are identical. No active
     * residency or BG staging bytes are modified by this retry. */
    if(repair==2u&&dy){if(!shift(raster_planes+32u+layer*32u,-dy))goto failure;dy=0;}
    if(!family){
     pb_lower_face_origin(raster_planes+32u+layer*32u,y,pal,1,&dy);
     pb_narrow_face_origin((const PBOverlayPalettes *)f->bg,raster_planes+32u+layer*32u,y,x,pal,&dy);
    }
    else pb_lower_hs_origin(raster_planes+32u+layer*32u,y,x,pal,1,variant,&dy);
    if(dy&&!shift(raster_planes+32u+layer*32u,dy))goto failure;
    meta[2u+layer]=(uint8_t)dy;
   }
   count=pair<60u?raster_upper_count:raster_lower_count;
   if(!add_object(pair,layer,pal,dy))goto failure;
   address=(pair<60u?0xd600u:0xd880u)+(uint16_t)count*32u;
   retained_selection_put(raster_build_bank,address,raster_planes+32u+layer*32u,32);
  }
  retained_selection_put(raster_build_bank,at,meta,4);raster_capture_inputs();
 }
 VBK_REG=0;
 if(reuse&&raster_dirty_pairs!=180u&&(*(volatile uint8_t *)0xffe8)!=raster_dirty_pairs)goto failure;
 /* Only this completed outcome permits prepared retry. Structural failure
  * returns zero above and must rebuild through the original fallback. */
 upper_cached=1;
 return raster_peak<=10u?1u:2u;
failure:
 /* No work is added to the successful per-pair loop. An early lower failure
  * can retain upper metadata/OAM/planes; a failed upper plan cannot. */
 if(pair>=60u)upper_cached=1;
 return 0;
}
/* Selection preserves the complete upper plan and palette words. It still
 * reconstructs canonical source order and validates all lower rectangles.
 * Metadata is read/written in eight-pair groups, including empty strips. */
static void selection_copy(uint16_t address,uint16_t length){
 uint8_t n;while(length){n=length>96u?96u:length;retained_selection_get(raster_active_bank,address,raster_planes,n);retained_selection_put(raster_build_bank,address,raster_planes,n);address+=n;length-=n;}
}
static const uint8_t slot_rows[180]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
static const uint8_t slot_cols[180]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
/* Retain physical payload ownership. Only dirty source strips are composed;
 * clean objects retain their slot and planes. OAM remains in source priority. */
static uint8_t prepare_selection(const Frame *f,const uint8_t *dirty){
 uint8_t order[80],used[5],meta[4],slots[2],n,i,j,pair,layer,slot,keep=0,records=0,layers,pal,row,last=255u,profile;
 uint16_t recipe,tile,address;int16_t y;
 for(pair=0;pair<60u;pair++)if(changed(dirty,pair))return 0;
 if(raster_dirty_pairs>45u)return 0;
 profile=pb_atlas_profile((const PBOverlayPalettes *)f->bg);
 n=retained_slots_begin(order,used);if(n>39u)return 0;
 /* Retire every old dirty rectangle before adding replacements. Loads always
  * represent canonical source objects, never the frozen caret's merge. */
 for(i=0;i<n;i++){
  pair=order[i*2u];slot=order[i*2u+1u]&63u;layer=order[i*2u+1u]>>6;
  if(pair<60u||pair>=180u||slot>=39u||layer>1u)return 0;
  if(changed(dirty,pair)){
   if(last!=pair){retained_selection_get(7,0xd570u+(uint16_t)pair*4u,meta,4);last=pair;}
   y=(int16_t)(slot_rows[pair])*16u+(int8_t)meta[2u+layer];if(y<48||y>128)return 0;
   for(row=y;row<y+16;row++){if(!raster_loads[row])return 0;--raster_loads[row];}
   used[slot>>3]&=~(1u<<(slot&7u));
  }else{order[keep*2u]=pair;order[keep*2u+1u]=order[i*2u+1u];keep++;}
 }
 n=keep;
 /* The retired-pair cache is dead until the unconditional OAM reset below.
  * Reuse that existing byte for admission, leaving no new live stack storage
  * above the complete original recipe fallback. Frame stays immutable here. */
 last=pb_selection_dispatch_begin(f,profile);
 for(pair=60u;pair<180u;pair++)if(changed(dirty,pair)){
  tile=pb_selection_dispatch_record(f,last,pair);
  if((tile==65535u||!pb_atlas_copy(tile,&recipe))&&!atlas_base(f,(const PBOverlayPalettes *)f->bg,profile,pair,&recipe)&&!pb_lower_rom_static(f,slot_cols[pair]*8u,slot_rows[pair]*16u,raster_planes,&recipe)){
   raster_compose(f,slot_cols[pair]*8u,slot_rows[pair]*16u);recipe=raster_lower_recipe((const PBOverlayPalettes *)f->bg,raster_pixels,2);
   if(recipe==65535u||!lower_encode((const PBOverlayPalettes *)f->bg,recipe))return 0;
  }
  if(recipe==65535u||!retained_selection_bg_guard(pair,raster_planes))return 0;
  tile=(uint16_t)(slot_rows[pair])*40u+slot_cols[pair];
  for(i=0;i<2u;i++,tile+=20u)retained_selection_put(tile<256u?5u:6u,0xd000u+(tile&255u)*16u,raster_planes+i*16u,16);
  meta[0]=recipe;meta[1]=recipe>>8;meta[2]=meta[3]=0;slots[0]=slots[1]=255u;layers=(recipe>>12)&3u;if(layers>2u)return 0;
  for(layer=0;layer<layers;layer++){
   if(n>=39u)return 0;
   for(slot=0;slot<39u;slot++)if(!(used[slot>>3]&(1u<<(slot&7u))))break;
   if(slot==39u)return 0;used[slot>>3]|=1u<<(slot&7u);slots[layer]=slot;
   retained_selection_put(raster_build_bank,0xd880u+(uint16_t)slot*32u,raster_planes+32u+layer*32u,32);
   y=(int16_t)(slot_rows[pair])*16u;if(y<48||y>128)return 0;
   for(row=y;row<y+16;row++)++raster_loads[row];
   for(j=n;j&&order[(j-1u)*2u]>pair;j--){order[j*2u]=order[(j-1u)*2u];order[j*2u+1u]=order[(j-1u)*2u+1u];}
   order[j*2u]=pair;order[j*2u+1u]=slot|(layer<<6);n++;
  }
  retained_selection_put(raster_build_bank,0xd300u+(uint16_t)pair*4u,meta,4);
  retained_slots_record(records++,pair,meta,slots);raster_capture_inputs();
 }
 raster_peak=0;for(row=0;row<144u;row++)if(raster_loads[row]>raster_peak)raster_peak=raster_loads[row];
 if(raster_peak>10u)return 2;
 memset(order+n*2u,255u,80u-n*2u);
 retained_slots_finish(n,order,used,records);
 /* Bounded40-descriptor source-order publication; no clean pattern movement. */
 memset(raster_planes,0,96);retained_selection_put(raster_build_bank,0xd100u,raster_planes,96);retained_selection_put(raster_build_bank,0xd160u,raster_planes,64);
 last=255u;
 for(i=0;i<n;i++){
  pair=order[i*2u];slot=order[i*2u+1u]&63u;layer=order[i*2u+1u]>>6;
  if(last!=pair){retained_selection_get(raster_build_bank,0xd300u+(uint16_t)pair*4u,meta,4);last=pair;}
  recipe=(uint16_t)meta[0]|((uint16_t)meta[1]<<8);pal=(recipe>>(layer?9:6))&7u;y=(int16_t)(slot_rows[pair])*16u+(int8_t)meta[2u+layer];
  if(y<48||y>128)return 0;
  j=(i&7u)*4u;raster_planes[j]=y+16u;raster_planes[j+1u]=slot_cols[pair]*8u+8u;raster_planes[j+2u]=40u+slot*2u;raster_planes[j+3u]=pal;
  if((i&7u)==7u||i+1u==n)retained_selection_put(raster_build_bank,0xd100u+(uint16_t)(i&248u)*4u,raster_planes,j+4u);
 }
 raster_lower_count=n;upper_cached=1;return 1;
}
static void palettes(const Frame *f,const PBOverlayPalettes *upper){
 uint8_t epoch,pal,code,id;uint16_t at,value;const PBOverlayPalettes *p;
 for(epoch=0;epoch<2u;epoch++){
  p=epoch?(const PBOverlayPalettes *)f->bg:upper;at=epoch?0xd280u:0xd200u;
  for(pal=0;pal<8u;pal++)for(code=0;code<4u;code++){
   id=p->bg[pal*4u+code];value=word(f,id);raster_put(at++,value);raster_put(at++,value>>8);
  }
  for(pal=0;pal<8u;pal++)for(code=0;code<4u;code++){
   value=code?word(f,p->obj[pal*3u+code-1u]):0;raster_put(at++,value);raster_put(at++,value>>8);
  }
 }
}
static void publish(const Frame *f,const PBOverlayPalettes *upper,const uint8_t *dirty){
 if(!raster_publish_prepared(dirty)){raster_error=4;raster_fault=1;return;}
 raster_cache_save(f,upper);
}
static uint8_t render_frame(const GameState *g,const HandPreview *preview,const char *numbers,uint8_t pressed) {
 Frame f;PBOverlayPalettes upper;uint8_t candidate,family=0,hidden_stone=0,i,ok=0,reuse,dirty[23],saved_policy,prior_mode=retained_caret_mode,selection;
 selection=retained_selection_load(&f,&upper,g,preview,pressed,dirty);
 retained_selection_invalidate();
 retained_caret_reset();retained_caret_mode=1;
 if(!selection){panel_b_prepare_numbered(&f,g,preview,numbers);retained_slots_materialize();}if(!upper_policy(&f,&upper)){raster_error=2;return 0;}
 for(i=0;i<f.page.count;i++)if(f.cards[i].kind==PANEL_B_HIDDEN||f.cards[i].kind==PANEL_B_STONE)hidden_stone=1;
 plan_attempt:
 upper_cached=0;
 raster_build_bank=raster_active_bank==2u?3u:2u;
 saved_policy=selection?1u:raster_cache_policy(&f);
 if(!saved_policy)memcpy(f.bg,face,56);
 family=memcmp(f.bg,face,56)?1u:0u;
 reuse=selection?1u:raster_cache_dirty(&f,&upper,dirty);
 /* A mode transition retires all integrated-caret recipes. */
 if(!prior_mode||!retained_caret_mode){reuse=0;memset(dirty,255,23);raster_cache_full_key();}
 raster_dirty_pairs=0;for(i=0;i<180u;i++)raster_dirty_pairs+=changed(dirty,i);
 ok=selection?prepare_selection(&f,dirty):prepare(&f,&upper,0,family,0,dirty,reuse,0);
 if(ok==2u&&!family&&!selection)ok=prepare(&f,&upper,2,0,0,dirty,reuse,selection);
 if(ok!=1u){selection=0;raster_cache_full_key();memcpy(f.bg,face,56);memset(dirty,255,23);raster_dirty_pairs=180;family=0;ok=prepare(&f,&upper,0,0,0,dirty,0,0);}
 if(ok==2u)ok=prepare(&f,&upper,2,0,0,dirty,0,0);
 if(ok!=1u&&hidden_stone)for(candidate=0;candidate<21u&&ok!=1u;candidate++){
  family=1;pb_lower_hs_profile(candidate,f.bg);ok=prepare(&f,&upper,0,1,0,dirty,0,0);
  if(ok!=1u&&raster_peak>10u)ok=prepare(&f,&upper,1,1,0,dirty,0,0);
 }
 if(ok!=1u)goto marker_fallback;
 if(selection)retained_slots_caret_begin();
 if(retained_caret_mode&&!retained_caret_install(f.page.first,f.obj,f.page.selected_mask,g->focus-f.page.first))goto marker_fallback;
 if(selection)retained_slots_caret_end();
 if(!selection)palettes(&f,&upper);
 if(selection){if(!retained_atomic_publish(dirty)){retained_slots_repack_build();retained_atomic_full(dirty);if(!raster_publish_prepared(dirty)){raster_error=4;raster_fault=1;}retained_atomic_reset();}else retained_slots_commit();if(!raster_error)retained_selection_save_keys(&f);}
 else {retained_atomic_full(dirty);publish(&f,&upper,dirty);retained_atomic_reset();}
 retained_selection_store(&f,&upper);family;return raster_error?0u:1u;
marker_fallback:
 if(retained_caret_mode){retained_caret_mode=0;prior_mode=0;selection=0;goto plan_attempt;}
 raster_error=3;return 0;
}

uint8_t raster_render(const GameState *g,const HandPreview *preview) BANKED {
 HandPreview authoritative_preview;uint8_t pressed;
 (void)preview;raster_error=0;pressed=retained_input_take(g);
 if(!g||g->hand_count>128u||g->joker_count>PG_JMAX||g->highlighted_count>5u){raster_error=1;return 0;}
 frontend_public_hand_preview(g,&authoritative_preview);
 /* Complete displayed public-preview comparison precedes focus reuse. The
  * source is fresh public consensus, never a private GameState/save hash. */
 if(!retained_preview_same(g,&authoritative_preview,pressed))retained_pressed=0;
 if(retained_caret_try(g))return 1;
 /* Existing planes scratch is idle until preparation begins. IRQs never use
  * it. Copy both strings into Frame before any plane encoding reuses it.
  * Keep the exact large-number formatter outside Frame's1080-byte lifetime. */
 memset(raster_planes,0,32);
 frontend_format_number_width(&g->target,8,(char *)raster_planes,8);
 frontend_format_number_width(&g->score,8,(char *)raster_planes+16,8);
 return render_frame(g,&authoritative_preview,(const char *)raster_planes,pressed);
}

uint16_t integration_rgb_word(const Frame *f,uint8_t id) BANKED {return word(f,id);}
void integration_program_palettes(const Frame *f,const PBOverlayPalettes *upper) BANKED {palettes(f,upper);}
