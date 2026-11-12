#pragma bank 255
#include <string.h>
#include "retained_caret.h"
#include "retained_selection.h"
#include "raster_runtime.h"
#include "hand_api.h"
uint8_t retained_caret_mode, retained_pressed;
static uint8_t valid,first,focus,selected,merged_slot=255u,backup_generation;
static uint16_t backup_at(void){return backup_generation?0xd080u:0xd000u;}
static uint16_t plane_at(uint8_t tile){return 0xd880u+(uint16_t)(tile-40u)*16u;}
/* Register-only publication of the 64 lower palette words. The ordinary
 * stack is bank1; fixed raster_pixels is the staged source. No C/stack/IRQ
 * observes the temporarily mapped active WRAM bank. 128 iterations take
 * 5320T including return, below six double-speed scanlines. The caller
 * admits LY144..149 after the original VBlank IRQ; a late start may wrap
 * to LY0/1, but all words complete before the next STAT32 consumer.
 * No VRAM/OAM write occurs. */
static void busy_palette_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ld hl,#_raster_pixels
 ld de,#0xd280
 ld b,#128
 ldh a,(0xd0)
 ldh (0x70),a
47100$:
 ld a,(hl+)
 ld (de),a
 inc de
 dec b
 jr nz,47100$
 ld a,#1
 ldh (0x70),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void accepted_busy(void){
 uint8_t i;uint16_t colour;
 raster_get_block(raster_active_bank,0xd280u,raster_pixels,128);
 /* Halve each RGB555 component independently; this changes presentation
  * only. The next complete normal render reconstructs every source word.
  * Both BG and OBJ dim together, preserving the coherent old composition. */
 for(i=0;i<128u;i+=2u){
  colour=(uint16_t)raster_pixels[i]|((uint16_t)raster_pixels[i+1u]<<8);
  colour=(colour>>1)&0x3defu;
  raster_pixels[i]=colour;raster_pixels[i+1u]=colour>>8;
 }
 do{wait_vbl_done();}while(LY_REG<144u||LY_REG>149u);
 CRITICAL {busy_palette_raw();}
}
/* Exact normal TABLE START/B receipt, before core validation. Three small
 * dots replace the independent caret triangle: the request is being checked,
 * with no prediction of success. Descriptor/focus/selection, palettes, audio,
 * core, RNG and SRAM stay untouched. Even rejected requests may show receipt;
 * the mandatory full render reconstructs the ordinary triangle afterward.
 * Merged/unsupported carets retain the existing presentation fallback. */
void retained_caret_receipt(const GameState *g) BANKED {
 uint8_t descriptor[4],code;
 if(g!=&game||g->phase!=PHASE_TABLE||g->ui_mode||!valid||merged_slot!=255u||!retained_caret_mode||!raster_live||raster_fault||!(LCDC_REG&LCDCF_ON))return;
 raster_get_block(raster_active_bank,0xd100u,descriptor,4);
 if(descriptor[2]<40u||descriptor[2]>=120u)return;
 raster_get_block(raster_active_bank,plane_at(descriptor[2]),raster_pixels,32);
 code=(raster_pixels[22]&128u?1u:0u)|(raster_pixels[23]&128u?2u:0u);
 if(!code)return;
 memset(raster_pixels,0,32);
 raster_pixels[22]=raster_pixels[24]=(code&1u)?84u:0;
 raster_pixels[23]=raster_pixels[25]=(code&2u)?84u:0;
 /* SDK publication waits for legal VRAM access; source shadow and visible
  * tile describe the same dots. The existing descriptor/OAM row admission
  * is unchanged and the IRQ never reads plane shadows. */
 raster_put_block(raster_active_bank,plane_at(descriptor[2]),raster_pixels,32);
 wait_vbl_done();VBK_REG=0;set_sprite_data(descriptor[2],2,raster_pixels);
}
/* Acknowledge only an already accepted, certified pure TABLE mutation.
 * The complete old card/preview image remains visible until the normal
 * renderer publishes it. This independent gold caret is the interim signal:
 * horizontal position acknowledges focus; its eight-pixel rise/fall
 * acknowledges selection/deselection. No preview, identity, rule, save,
 * generation, cache key or source sprite is changed here. The source-only
 * OAM row loads must admit the caret at its prospective position. Merged
 * carets and page changes keep the complete existing renderer path.
 * The two descriptor bytes publish together through the existing bounded
 * critical ferry; its STAT exclusion and SVBK restoration remain unchanged.
 * Keep the one-use input certificate and retained caret caches unconsumed. */
void retained_caret_acknowledge(const GameState *g) BANKED {
 uint8_t page,mask,count,pos,top,row,descriptor[2],ticket=retained_pressed;
 if(g!=&game||!valid||!retained_caret_mode||!raster_live||raster_fault||!(LCDC_REG&LCDCF_ON))return;
 /* Only the exact audited main-loop action can issue these tickets. Failed
  * previews/Bell/resource checks do not decrement and cannot look accepted.
  * Durable tickets always keep the original synchronous save afterward. */
 if(g->phase!=PHASE_BLOCKED&&
    (((ticket&0xe0u)==0x20u&&g->hands<(ticket&31u))||
     ((ticket&0xe0u)==0x60u&&g->discards<(ticket&31u)))){
  accepted_busy();return;
 }
 if(merged_slot!=255u||g->phase!=PHASE_TABLE||g->ui_mode)return;
 if(ticket!=(0x40u|J_LEFT)&&ticket!=(0x40u|J_RIGHT)&&ticket!=(0x80u|J_A))return;
 count=hand_page(g,&page,&mask);
 if(!count||page!=first||g->focus<page||g->focus>=page+count)return;
 pos=g->focus-page;
 if(ticket==(0x80u|J_A)){
  if(pos!=focus||(mask^selected)!=(1u<<pos))return;
 }else if(mask!=selected||pos==focus)return;
 top=(mask&(1u<<pos))?76u:84u;
 for(row=top;row<top+16u;row++)if(raster_loads[row]>=10u)return;
 descriptor[0]=top+16u;descriptor[1]=53u+pos*14u;
 raster_put_block(raster_active_bank,0xd100u,descriptor,2);
}
/* Plan metadata always describes the source without the independent caret.
 * Before a full rebuild restore only its WRAM plane shadow. Published VRAM
 * and OAM remain the old correct image while preparation proceeds. */
void retained_caret_reset(void) BANKED {
 if(merged_slot!=255u){
  /* raster_planes still owns the formatted score/target strings here. */
  raster_get_block(7,backup_at(),raster_pixels,36);
  raster_put_block(raster_active_bank,plane_at(raster_pixels[2]),raster_pixels+4,32);
  merged_slot=255u;
 }
 valid=0;
}
uint8_t retained_caret_install(uint8_t page,const uint8_t *obj,uint8_t sel,uint8_t pos) BANKED {
 uint8_t i,pal,code,slot=raster_lower_count,old[4],top=(sel&(1u<<pos))?76u:84u;
 if(slot>=40u)return 0;
 for(i=top;i<top+16u;i++)if(raster_loads[i]>=10u)return 0;
 for(pal=0;pal<8u;pal++){for(code=0;code<3u;code++)if(obj[pal*3u+code]==19u)break;if(code<3u)break;}
 if(pal==8u)return 0;
 code++;memset(raster_planes,0,32);
 for(i=0;i<4u;i++){uint8_t mask=i==0?254u:i==1?124u:i==2?56u:16u;raster_planes[22u+i*2u]=(code&1u)?mask:0;raster_planes[23u+i*2u]=(code&2u)?mask:0;}
 raster_put_block(raster_build_bank,0xd880u+(uint16_t)slot*32u,raster_planes,32);
 retained_oam_prepend(slot);
 old[0]=top+16u;old[1]=53u+pos*14u;old[2]=40u+slot*2u;old[3]=pal;
 raster_put_block(raster_build_bank,0xd100u,old,4);
 /* Loads retain the source-only baseline. Physical caret admission is added
  * here; later focus transactions can subtract/replace any source object. */
 for(i=top;i<top+16u;i++)if(raster_loads[i]+1u>raster_peak)raster_peak=raster_loads[i]+1u;
 raster_lower_count++;first=page;focus=pos;selected=sel;valid=1;return 1;
}
static void source_descriptor(uint8_t slot,uint8_t *out){
 if(slot==merged_slot)raster_get_block(7,backup_at(),out,4);
 else raster_get_block(raster_active_bank,0xd100u+(uint16_t)slot*4u,out,4);
}
static void source_planes(uint8_t slot,uint8_t tile,uint8_t *out){
 if(slot==merged_slot)raster_get_block(7,backup_at()+4u,out,32);
 else raster_get_block(raster_active_bank,plane_at(tile),out,32);
}
static uint8_t code_at(const uint8_t *p,uint8_t x,uint8_t y){
 uint8_t lo=p[y*2u],hi=p[y*2u+1u],shift=7u-x;
 lo>>=shift;hi>>=shift;
 return (lo&1u)|((hi&1u)<<1);
}
static void write_code(uint8_t *p,uint8_t x,uint8_t y,uint8_t code){
 uint8_t bit=128u>>x,inv=~bit;
 p[y*2u]=(p[y*2u]&inv)|((code&1u)?bit:0);
 p[y*2u+1u]=(p[y*2u+1u]&inv)|((code&2u)?bit:0);
}
/* Scratch: old plane0..31,new32..63,palette IDs64..87,old descriptor88..91,
 * new descriptor92..95. All are prospective until the final commit. */
static uint8_t merge_candidate(uint8_t slot,uint8_t cx,uint8_t cy){
 uint8_t x,y,c,ox,oy,l=cx,r=cx+6u,t=cy,b=cy+3u,pal,oldpal,used=0,map[4],caret=0,i,j;
 uint8_t *old=raster_pixels,*next=raster_pixels+32,*ids=raster_pixels+64,*desc=raster_pixels+88;
 source_descriptor(slot,desc);
 if(!desc[0]||desc[0]<16u||desc[1]<8u||(desc[3]&248u))return 0;
 ox=desc[1]-8u;oy=desc[0]-16u;oldpal=desc[3];
 /* Bounding-box rejection avoids decoding irrelevant screen regions. */
 if(ox+15u<cx||cx+13u<ox||oy+30u<cy||cy+18u<oy)return 0;
 source_planes(slot,desc[2],old);
 for(y=0;y<16u;y++)for(x=0;x<8u;x++){
  c=code_at(old,x,y);if(!c)continue;used|=1u<<c;
  if(ox+x<l)l=ox+x;if(ox+x>r)r=ox+x;
  if(oy+y<t)t=oy+y;if(oy+y>b)b=oy+y;
 }
 if(r-l>=8u||b-t>=16u||l>152u||t>128u)return 0;
 for(i=t;i<t+16u;i++)if(raster_loads[i]-(i>=oy&&i<oy+16u)+1u>10u)return 0;
 /* Complete palette closure over all original opaque codes plus exact19. */
 for(pal=0;pal<8u;pal++){
  for(caret=1u;caret<=3u;caret++)if(ids[pal*3u+caret-1u]==19u)break;
  if(caret==4u)continue;
  for(c=1u;c<=3u;c++){
   map[c]=0;if(!(used&(1u<<c)))continue;
   for(i=1u;i<=3u;i++)if(ids[pal*3u+i-1u]==ids[oldpal*3u+c-1u]){map[c]=i;break;}
   if(!map[c])break;
  }
  if(c==4u)break;
 }
 if(pal==8u)return 0;
 /* Earlier source sprites must not cover any newly opaque caret pixel. */
 for(i=1u;i<slot;i++){
  source_descriptor(i,raster_planes);
  if(!raster_planes[0]||raster_planes[0]<16u||raster_planes[1]<8u)return 0;
  x=raster_planes[1]-8u;y=raster_planes[0]-16u;
  if(x>cx+6u||x+7u<cx||y>cy+3u||y+15u<cy)continue;
  source_planes(i,raster_planes[2],raster_planes+4);
  for(j=0;j<4u;j++)for(c=j;c<7u-j;c++)
   if(cx+c>=x&&cx+c<x+8u&&cy+j>=y&&cy+j<y+16u&&code_at(raster_planes+4,cx+c-x,cy+j-y))return 0;
 }
 memset(next,0,32);
 for(y=0;y<16u;y++)for(x=0;x<8u;x++){
  c=code_at(old,x,y);if(c)write_code(next,ox+x-l,oy+y-t,map[c]);
 }
 for(y=0;y<4u;y++)for(x=y;x<7u-y;x++)write_code(next,cx+x-l,cy+y-t,caret);
 raster_pixels[92]=t+16u;raster_pixels[93]=l+8u;raster_pixels[94]=desc[2];raster_pixels[95]=pal;
 return 1;
}
static void restore_published_merge(void){
 if(merged_slot==255u)return;
 /* The backup and both WRAM shadows were staged before the VBlank wait.
  * Only visible tile/descriptor publication belongs in this short window. */
 VBK_REG=0;set_sprite_data(raster_planes[2],2,raster_planes+4);
 raster_put_block(raster_active_bank,0xd100u+(uint16_t)merged_slot*4u,raster_planes,4);
 merged_slot=255u;
}
uint8_t retained_caret_try(const GameState *g) BANKED {
 uint8_t pos,pressed=retained_pressed,buf[2],i,top,slot=255u;uint16_t prospective;
 retained_pressed=0;
 if(!valid||(pressed!=J_LEFT&&pressed!=J_RIGHT)||!raster_live||raster_fault||!(LCDC_REG&LCDCF_ON)||g->phase!=PHASE_TABLE||g->ui_mode||g->focus/8u*8u!=first)return 0;
 pos=g->focus-first;if(pos>=8u)return 0;top=(selected&(1u<<pos))?76u:84u;
 for(i=top;i<top+16u;i++)if(raster_loads[i]>=10u)break;
 if(i<top+16u){
  raster_get_block(4,0xd720u,raster_pixels+64,24);
  for(slot=1u;slot<raster_lower_count;slot++)if(merge_candidate(slot,45u+pos*14u,top+11u))break;
  if(slot==raster_lower_count)return 0; /* No active plan changes on miss. */
  prospective=backup_generation?0xd000u:0xd080u;
  raster_put_block(7,prospective,raster_pixels+88,4);
  raster_put_block(7,prospective+4u,raster_pixels,32);
 }
 /* IRQ reads OAM/palettes, never the encoded plane shadows. Stage those
  * after successful preflight, while the complete old image is still live.
  * Keeping their slow ferries outside publication avoids crossing line40. */
 if(merged_slot!=255u){
  raster_get_block(7,backup_at(),raster_planes,36);
  raster_put_block(raster_active_bank,plane_at(raster_planes[2]),raster_planes+4,32);
 }
 if(slot!=255u)raster_put_block(raster_active_bank,plane_at(raster_pixels[94]),raster_pixels+32,32);
 /* Publish after VBlank; lower OAM DMA is at line40. All SVBK ferries
  * restore1; VRAM writes use the SDK legal-access helper. */
 wait_vbl_done();restore_published_merge();
 if(slot!=255u){
  VBK_REG=0;set_sprite_data(raster_pixels[94],2,raster_pixels+32);
  raster_put_block(raster_active_bank,0xd100u+(uint16_t)slot*4u,raster_pixels+92,4);
  buf[0]=buf[1]=0;raster_put_block(raster_active_bank,0xd100u,buf,2);
  backup_generation^=1u;merged_slot=slot;
 }else{
  buf[0]=top+16u;buf[1]=53u+pos*14u;raster_put_block(raster_active_bank,0xd100u,buf,2);
 }
 raster_get_block(4,0xd008u+(uint16_t)focus*9u,buf,1);buf[0]&=~2u;raster_put_block(4,0xd008u+(uint16_t)focus*9u,buf,1);
 raster_get_block(4,0xd008u+(uint16_t)pos*9u,buf,1);buf[0]|=2u;raster_put_block(4,0xd008u+(uint16_t)pos*9u,buf,1);
 focus=pos;raster_dirty_pairs=0;wait_vbl_done();return 1;
}
