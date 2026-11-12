#include "paged_inventory.h"
#include "inventory_bounds.h"
#include "a1_title_runtime.h"
#pragma bank 255
#include <string.h>
#include <stddef.h>
#include "retained_selection.h"
#include "retained_caret.h"
#include "raster_runtime.h"
#include "frontend_numeric.h"
#include "frontend_public_preview.h"
#include "hand_api.h"
#include "save.h"
#include "frontend_render.h"
#define MODEL 0xd100u
#define UPPER (MODEL+sizeof(Frame))
extern volatile uint16_t raster_address;
extern volatile uint8_t raster_ferry_bank,raster_ferry_length;
extern uint8_t native_palette_ready;
static void selection_window(void){
 uint8_t line;
 /* This bridge transfers32 rather than16 bytes. Retain two additional
  * scanlines before STAT32 for setup/call overhead as well as the raw loop.
  * Waiting keeps interrupts enabled; only the bounded copy is critical. */
 if(raster_live&&(LCDC_REG&LCDCF_ON))while(LY_REG>=28u&&LY_REG<=47u){}
 /* Native palette IRQ has additional deadlines. The32-byte bridge needs
  * two more setup/copy guard lines than the16-byte runtime ferry. */
 if(native_palette_ready==1u&&(LCDC_REG&LCDCF_ON))do{line=LY_REG;}while((line>=2u&&line<=7u)||(line>=26u&&line<=43u)||(line>=72u&&line<=95u)||(line>=106u&&line<=127u));
}
/* Only this register-only banked code sees SVBK != 1. The caller stack and
 * normal C remain in bank1. 32-byte maximum bounds the critical section below
 * two double-speed scanlines; dedicated LY28..47 exclusion protects STAT32, including setup overhead.
 * raster_pixels is fixed WRAM and owned exclusively by this transfer phase. */
static void read_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xbc)
 ld l,a
 ldh a,(0xbd)
 ld h,a
 ld de,#_raster_pixels
 ldh a,(0xeb)
 ld b,a
 ldh a,(0xea)
 ldh (0x70),a
47000$:
 ld a,(hl+)
 ld (de),a
 inc de
 dec b
 jr nz,47000$
 ld a,#1
 ldh (0x70),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void write_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ld hl,#_raster_pixels
 ldh a,(0xeb)
 ld b,a
 ldh a,(0xea)
 ldh (0x70),a
47001$:
 ld a,(hl+)
 ld (de),a
 inc de
 dec b
 jr nz,47001$
 ld a,#1
 ldh (0x70),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
void retained_selection_get(uint8_t bank,uint16_t address,uint8_t *dest,uint8_t length) BANKED {
 uint8_t n;
 while(length){n=length>32u?32u:length;selection_window();
  CRITICAL {raster_address=address;raster_ferry_bank=bank;raster_ferry_length=n;read_raw();}
  memcpy(dest,raster_pixels,n);dest+=n;address+=n;length-=n;
 }
}
void retained_selection_put(uint8_t bank,uint16_t address,const uint8_t *source,uint8_t length) BANKED {
 uint8_t n;
 while(length){n=length>32u?32u:length;memcpy(raster_pixels,source,n);selection_window();
  CRITICAL {raster_address=address;raster_ferry_bank=bank;raster_ferry_length=n;write_raw();}
  source+=n;address+=n;length-=n;
 }
}
void retained_selection_invalidate(void) BANKED {
 uint8_t bad=255;retained_selection_put(7,MODEL+offsetof(Frame,page),(const uint8_t *)&bad,1);
}
static void frame_copy(Frame *f,uint8_t write){
 uint16_t offset;uint8_t n;
 for(offset=0;offset<sizeof(Frame);offset+=n){n=sizeof(Frame)-offset>128u?128u:sizeof(Frame)-offset;
  if(write)retained_selection_put(7,MODEL+offset,(const uint8_t *)f+offset,n);
  else retained_selection_get(7,MODEL+offset,(uint8_t *)f+offset,n);
 }
}
void retained_selection_store(Frame *f,const PBOverlayPalettes *upper) BANKED {
 uint8_t i;uint16_t at;const GameState *g=f->g;
 if(!raster_published){retained_selection_invalidate();return;}
 if(!retained_caret_mode||raster_error||raster_fault||!raster_live)return;
 /* Strings are either owned Frame bytes or immutable fixed-ROM boss labels.
  * Reject every other pointer before changing the prospective model. The
  * high bit tags fixed ROM; relative Frame offsets remain unchanged. */
 for(i=0;i<f->glyph_count;i++){
  at=(uint16_t)f->glyphs[i].s;
  if(f->glyphs[i].length>31u||(at<0x4000u?at>0x4000u-f->glyphs[i].length-1u:
      at<(uint16_t)f||at-(uint16_t)f>sizeof(Frame)-f->glyphs[i].length-1u)){
   retained_selection_invalidate();return;
  }
 }
 for(i=0;i<f->glyph_count;i++){
  at=(uint16_t)f->glyphs[i].s;
  f->glyphs[i].s=(const char *)(at<0x4000u?at|0x8000u:at-(uint16_t)f);
 }
 f->g=(const GameState *)raster_published;frame_copy(f,1);
 for(i=0;i<f->glyph_count;i++){
  at=(uint16_t)f->glyphs[i].s;
  f->glyphs[i].s=(const char *)(at&0x8000u?at&0x7fffu:(uint16_t)f+at);
 }
 f->g=g;retained_selection_put(7,UPPER,(const uint8_t *)upper,56);
}
static void rect(uint8_t *dirty,uint8_t x,uint8_t y,uint8_t w,uint8_t h){
 uint16_t right=(uint16_t)x+w,bottom=(uint16_t)y+h;uint8_t row,col,pair;
 if(x>=160u||y>=144u||!w||!h)return;
 if(right>160u)right=160;if(bottom>144u)bottom=144;
 for(row=y>>4;row<=(bottom-1u)>>4;row++)for(col=x>>3;col<=(right-1u)>>3;col++){pair=row*20u+col;dirty[pair>>3]|=1u<<(pair&7u);}
}
static uint8_t width(const char *s,uint8_t font){uint8_t n=strlen(s);return n?n*(font?6u:4u)-1u:0;}
static uint8_t centered(const char *s,uint8_t font,uint8_t x,uint8_t w){return x+(w-width(s,font)+1u)/2u;}
static void glyph(Glyph *g,const char *s,uint8_t font,uint8_t x,uint8_t y,uint8_t colour){
 g->s=s;g->length=strlen(s);g->font=font;g->x=x;g->y=y;g->colour=colour;
 g->right=x+g->length*(font?6u:4u);g->bottom=y+(font?7u:5u);
}
/* Exact public projection of pure TABLE/J_A: core hand_toggle changes only
 * selection UIDs/count. Current preview remains freshly computed by caller.
 * Live raster and generation gates reject resume, modal, page and fault paths. */
uint8_t retained_selection_load(Frame *f,PBOverlayPalettes *upper,const GameState *g,const HandPreview *p,uint8_t pressed,uint8_t *dirty) BANKED {
 uint8_t header[6],first,mask,count,i,j,n=0,changes,len,lev,w,row;Glyph old;char oldtext[32];uint16_t bits,at;
 if(pressed!=J_A||!retained_caret_mode||!raster_live||raster_fault||!(LCDC_REG&LCDCF_ON)||g->phase!=PHASE_TABLE||g->ui_mode)return 0;
 retained_selection_get(7,MODEL,header,6);
 if(header[2]==255u||((uint16_t)header[0]|((uint16_t)header[1]<<8))!=raster_published)return 0;
 count=hand_page(g,&first,&mask);
 if(!count||count!=header[3]||first!=header[2])return 0;
 changes=mask^header[4];if(g->focus<first||g->focus>=first+count||changes!=(changes?(1u<<(g->focus-first)):0u))return 0;
 frame_copy(f,0);f->g=g;
 if(f->glyph_count<4u||f->glyph_count>15u)return 0;
 for(i=0;i<f->glyph_count;i++){
  at=(uint16_t)f->glyphs[i].s;
  if(at&0x8000u){
   at&=0x7fffu;
   if(at>=0x4000u||f->glyphs[i].length>=0x4000u-at)return 0;
   f->glyphs[i].s=(const char *)at;
  }else{
   if(at>=sizeof(Frame)||f->glyphs[i].length>=sizeof(Frame)-at)return 0;
   f->glyphs[i].s=(const char *)((uint16_t)f+at);
  }
 }
 retained_selection_get(7,UPPER,(uint8_t *)upper,56);memset(dirty,0,23);
 for(i=0;i<count;i++){
  if(changes&(1u<<i))rect(dirty,f->cards[i].x,f->cards[i].y,20,31);
  f->cards[i].cues&=~(PANEL_B_SELECTED|PANEL_B_FOCUSED);
  if(mask&(1u<<i)){n++;f->cards[i].cues|=PANEL_B_SELECTED;f->cards[i].y=92;}else f->cards[i].y=100;
  if(g->focus==first+i)f->cards[i].cues|=PANEL_B_FOCUSED;
  if(changes&(1u<<i))rect(dirty,f->cards[i].x,f->cards[i].y,20,31);
 }
 if(n>g->highlighted_count||g->highlighted_count-n!=header[5])return 0;
 /* Exact hand_toggle result: Bell and full-five no-ops remain legal. */
 i=g->focus-first;j=0;for(len=0;len<count;len++)if(header[4]&(1u<<len))j++;
 j+=header[5];mask=1u<<i;
 if(header[4]&mask){if(changes!=((f->cards[i].cues&PANEL_B_BELL)?0u:mask))return 0;}
 else if(changes!=(j<5u?mask:0u))return 0;
 mask=header[4]^changes;
 f->page.selected_mask=mask;f->page.offpage_selected=g->highlighted_count-n;
 if(p->hidden||frontend_public_preview_hidden(g)){strcpy(f->chips,"?");strcpy(f->mult,"?");strcpy(f->label,"????");strcpy(f->level,"LV?");}
 else {
  frontend_format_number_width(&p->chips,4,f->chips,9);frontend_format_number_width(&p->mult,4,f->mult,9);
  if(!g->highlighted_count){strcpy(f->label,"SELECT CARDS");f->level[0]=0;}
  else {for(i=0;i<15u&&p->label[i]&&!(p->label[i]==' '&&p->label[i+1u]=='L'&&p->label[i+2u]=='V');i++)f->label[i]=p->label[i];f->label[i]=0;strcpy(f->level,"LV");frontend_format_number(&p->level,2,f->level+2);}
 }
 len=strlen(f->label);lev=width(f->level,0);f->label_font=1;w=len*6u-1u+(lev?16u+lev:0u);
 if(w>120u){f->label_font=0;w=len*4u-1u+(lev?16u+lev:0u);}
 f->label_x=94u-w/2u;f->level_x=f->label_x+width(f->label,f->label_font)+16u;
 i=f->glyph_count-4u;
 glyph(f->glyphs+i,f->label,f->label_font,f->label_x,f->label_font?48u:49u,19);
 glyph(f->glyphs+i+1u,f->level,0,f->level_x,49,12);
 j=width(f->chips,1)<=39u;glyph(f->glyphs+i+2u,f->chips,j,centered(f->chips,j,46,43),j?67u:68u,19);
 j=width(f->mult,1)<=39u;glyph(f->glyphs+i+3u,f->mult,j,centered(f->mult,j,104,43),j?67u:68u,19);
 for(;i<f->glyph_count;i++){
  retained_selection_get(7,MODEL+offsetof(Frame,glyphs)+(uint16_t)i*sizeof(Glyph),(uint8_t *)&old,sizeof(Glyph));
  if(old.length>31u||(uint16_t)old.s+old.length>=sizeof(Frame))return 0;
  retained_selection_get(7,MODEL+(uint16_t)old.s,(uint8_t *)oldtext,old.length+1u);
  if(memcmp((uint8_t *)&old+2u,(uint8_t *)(f->glyphs+i)+2u,7)||memcmp(oldtext,f->glyphs[i].s,old.length+1u)){
   rect(dirty,old.x,old.y,old.right-old.x,old.bottom-old.y);
   rect(dirty,f->glyphs[i].x,f->glyphs[i].y,f->glyphs[i].right-f->glyphs[i].x,f->glyphs[i].bottom-f->glyphs[i].y);
  }
 }
 memset(f->glyph_bands,0,sizeof(f->glyph_bands));
 for(i=0,bits=1;i<f->glyph_count;i++,bits<<=1)for(row=f->glyphs[i].y>>4;row<9u&&row<=(f->glyphs[i].bottom-1u)>>4;row++)f->glyph_bands[row]|=bits;
 return 1;
}
void retained_selection_save_keys(const Frame *f) BANKED {
 uint8_t i;const Glyph *g;
 for(i=0;i<8u;i++)retained_selection_put(4,0xd000u+i*9u,(const uint8_t *)(f->cards+i)+2u,9);
 retained_selection_put(4,0xd050u,(const uint8_t *)&f->page,4);
 for(i=f->glyph_count-4u;i<f->glyph_count;i++){
  g=f->glyphs+i;retained_selection_put(4,0xd400u+(uint16_t)i*40u,(const uint8_t *)g+2u,7);
  retained_selection_put(4,0xd407u+(uint16_t)i*40u,(const uint8_t *)g->s,g->length+1u);
 }
}
/* The 132-cell VRAM proposal is a guard, not an assumption about bitplanes.
 * Any changed exterior encoding rejects the partial path before publication. */
uint8_t retained_selection_bg_guard(uint8_t pair,const uint8_t *planes) BANKED {
 uint8_t half,row=pair/20u*2u,col=pair%20u;uint16_t tile;uint8_t old[16];
 for(half=0;half<2u;half++,row++){
  if((row>=11u&&row<=16u&&col>=5u)||(row==6u&&col>=4u)||((row==8u||row==9u)&&((col>=5u&&col<=11u)||(col>=13u&&col<=18u))))continue;
  tile=(uint16_t)row*20u+col;retained_selection_get(tile<256u?5u:6u,0xd000u+(tile&255u)*16u,old,16);
  if(memcmp(old,planes+half*16u,16))return 0;
 }
 return 1;
}

/* Private Rev03 canary. Inactive BG writes precede the VBlank commit;
 * frozen IRQ and bank0 OBJ identities remain unchanged. */
#define ATOMIC_PHASES 0xda90u
#define ATOMIC_CONTROL 0xdacfu
static const uint8_t atomic_cells[360]={255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,0,255,1,255,2,255,3,255,4,255,5,255,6,255,7,255,8,255,9,255,10,255,11,255,12,255,13,255,14,255,15,255,255,255,255,255,255,255,255,255,255,255,16,29,17,30,18,31,19,32,20,33,21,34,22,35,255,255,23,36,24,37,25,38,26,39,27,40,28,41,255,255,255,255,255,255,255,255,255,255,255,255,255,42,255,43,255,44,255,45,255,46,255,47,255,48,255,49,255,50,255,51,255,52,255,53,255,54,255,55,255,56,255,255,255,255,255,255,255,255,255,255,57,72,58,73,59,74,60,75,61,76,62,77,63,78,64,79,65,80,66,81,67,82,68,83,69,84,70,85,71,86,255,255,255,255,255,255,255,255,255,255,87,102,88,103,89,104,90,105,91,106,92,107,93,108,94,109,95,110,96,111,97,112,98,113,99,114,100,115,101,116,255,255,255,255,255,255,255,255,255,255,117,255,118,255,119,255,120,255,121,255,122,255,123,255,124,255,125,255,126,255,127,255,128,255,129,255,130,255,131,255};
static const uint8_t atomic_rows[180]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8};
static const uint8_t atomic_cols[180]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
/* Private LCD-on block access. SVBK stays1 throughout; IRQs run between
 * bytes and while waiting out LY28..47. Only one legal-access byte is critical.
 * Caller is normal renderer C with interrupts enabled, never an IRQ/CRITICAL
 * context. The IRQ does not consume VBK or fixed raster_pixels scratch. */
static void atomic_vread_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xbc)
 ld l,a
 ldh a,(0xbd)
 ld h,a
 ld de,#_raster_pixels
 ldh a,(0xeb)
 ld b,a
47200$:
 ldh a,(0x44)
 cp #28
 jr c,47201$
 cp #48
 jr c,47200$
47201$:
 di
47202$:
 ldh a,(0x41)
 and #2
 jr nz,47202$
 ld a,(hl+)
 ld (de),a
 ei
 inc de
 dec b
 jr nz,47200$
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void atomic_vwrite_raw(void) __naked {
 __asm
 push af
 push bc
 push de
 push hl
 ldh a,(0xbc)
 ld e,a
 ldh a,(0xbd)
 ld d,a
 ld hl,#_raster_pixels
 ldh a,(0xeb)
 ld b,a
47203$:
 ldh a,(0x44)
 cp #28
 jr c,47204$
 cp #48
 jr c,47203$
47204$:
 di
47205$:
 ldh a,(0x41)
 and #2
 jr nz,47205$
 ld a,(hl+)
 ld (de),a
 ei
 inc de
 dec b
 jr nz,47203$
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void atomic_vram(uint16_t address,uint8_t *bytes,uint8_t length,uint8_t write){
 if(write)memcpy(raster_pixels,bytes,length);
 raster_address=address;raster_ferry_length=length;
 if(write)atomic_vwrite_raw();else atomic_vread_raw();
 if(!write)memcpy(bytes,raster_pixels,length);
}
void retained_atomic_reset(void) BANKED {
 memset(raster_planes,0,22);
 retained_selection_put(7,ATOMIC_PHASES,raster_planes,17);
 retained_selection_put(7,ATOMIC_CONTROL,raster_planes,5);
}
/* Full fallback must restore canonical map IDs and all backing patterns. */
void retained_atomic_full(uint8_t *dirty) BANKED {
 uint8_t used;retained_selection_get(7,ATOMIC_CONTROL,&used,1);
 if(!used)return;
 memset(dirty,255,23);raster_dirty_pairs=180;raster_begin_publish();
}
/* Critical caller: transfer800 bytes in VBlank at LY144..149, then thirty
 * 16-byte chunks at fresh mode3->0 edges on lines0..29. The top48 rows
 * are identical across both plans. Restore SVBK1 before stack access and
 * re-enable IRQ before STAT32. Hardware DMA startup/tail bounds remain a
 * separate acceptance obligation; see the exact linked schedule proof. */
static uint8_t atomic_commit_raw(void) __naked {
 __asm
 ldh a,(0x44)
 cp #144
 jp c,47109$
 cp #150
 jp nc,47109$
 push bc
 push de
 push hl
 xor a
 ldh (0x4f),a
 ldh a,(0xbf)
 ldh (0x70),a
 ld a,#0xd8
 ldh (0x51),a
 ld a,#0x80
 ldh (0x52),a
 ld a,#2
 ldh (0x53),a
 ld a,#0x80
 ldh (0x54),a
 ld b,#30
 ld a,#49
 ldh (0x55),a
 ld hl,#0xdba0
 ld de,#0x85a0
47102$:
 ld a,h
 ldh (0x51),a
 ld a,l
 ldh (0x52),a
 ld a,d
 and #0x1f
 ldh (0x53),a
 ld a,e
 ldh (0x54),a
 /* A fresh mode3->0 edge gives the full HBlank, even with ten upper OBJs. */
47103$:
 ldh a,(0x41)
 and #3
 cp #3
 jr nz,47103$
47104$:
 ldh a,(0x41)
 and #3
 jr nz,47104$
 xor a
 ldh (0x55),a
 ld a,l
 add a,#16
 ld l,a
 jr nc,47106$
 inc h
47106$:
 ld a,e
 add a,#16
 ld e,a
 jr nc,47107$
 inc d
47107$:
 dec b
 jr nz,47102$
47105$:
 ld a,#1
 ldh (0x70),a
 ldh a,(0xbf)
 ldh (0xd0),a
 ldh a,(0x40)
 xor #8
 ldh (0x40),a
 pop hl
 pop de
 pop bc
 ld a,#1
 ret
47109$:
 xor a
 ret
 __endasm;
}
/* Publication owns the fixed scratch after every compiler/caret consumer:
 * pixels0..31 ferry,32..111 four map spans,112..127 union bitmap;
 * planes0..79 complete row metadata. IRQs do not consume these arrays. */
static void atomic_pattern(uint16_t tile,uint8_t cell,uint8_t phase){
 uint16_t source=0xd000u+(tile&255u)*16u;uint8_t from=tile<256u?5u:6u;
 selection_window();
 CRITICAL {raster_address=source;raster_ferry_bank=from;raster_ferry_length=16;read_raw();}
 VBK_REG=phase?0u:(tile<256u);
 raster_address=phase?0x8e80u+(uint16_t)cell*16u:0x8800u+(tile&255u)*16u;raster_ferry_length=16;
 atomic_vwrite_raw();
}
uint8_t retained_atomic_publish(const uint8_t *dirty) BANKED {
 uint8_t phases[17],row,col,bank,pair,half,cell,phase,meta,i,previous,used,first,last,width,current,ok=0,one=1;
 uint16_t active,inactive,address,target,tile;
 if(!raster_live||raster_fault||!(LCDC_REG&LCDCF_ON)||(LCDC_REG&0x20u)||!raster_lower_count||raster_lower_count>40u||raster_dirty_pairs>45u)return 0;
 active=(LCDC_REG&8u)?0x9c00u:0x9800u;inactive=active^0x400u;
 retained_selection_get(7,ATOMIC_PHASES,phases,17);
 memcpy(raster_pixels+112,dirty+7,16);
 /* Build metadata is complete after slots_begin. Reconstruct the affected
  * lower union. Cold publication first inherits the full visible map: this
  * costs fewer native cycles than reconstructing all120 lower pairs. */
 retained_selection_get(7,ATOMIC_CONTROL,&used,1);
 if(!used){
  for(bank=0;bank<2u;bank++){
   VBK_REG=bank;
   for(row=0;row<18u;row++){
    address=active+(uint16_t)row*32u;target=inactive+(uint16_t)row*32u;
    atomic_vram(address,raster_planes,20,0);atomic_vram(target,raster_planes,20,1);raster_capture_inputs();
   }
  }
 }else{
  retained_selection_get(7,0xdaa1u,&previous,1);if(previous>45u)return 0;
  retained_selection_get(7,0xdaa2u,raster_planes,previous);
  for(i=0;i<previous;i++){
   pair=raster_planes[i];
   if(pair<60u||pair>=180u)return 0;
   raster_pixels[105u+(pair>>3)]|=1u<<(pair&7u);
  }
 }
 for(row=3u;row<9u;row++){
  first=20;last=0;
  for(col=0,pair=row*20u;col<20u;col++,pair++)if(raster_pixels[105u+(pair>>3)]&(1u<<(pair&7u))){if(first==20u)first=col;last=col+1u;}
  if(first==20u)continue;
  width=last-first;pair=row*20u+first;
  retained_selection_get(raster_build_bank,0xd300u+(uint16_t)pair*4u,raster_planes,width*4u);
  for(col=0;col<width;col++,pair++){
   meta=raster_planes[col*4u];current=dirty[pair>>3]&(1u<<(pair&7u));tile=(uint16_t)row*40u+first+col;
   for(half=0;half<2u;half++,tile+=20u){
    cell=atomic_cells[(uint16_t)pair*2u+half];phase=0;
    if(cell!=255u){
     if(current)phases[cell>>3]^=1u<<(cell&7u);
     phase=!!(phases[cell>>3]&(1u<<(cell&7u)));
     if(current)atomic_pattern(tile,cell,phase);
    }
    bank=phase?0u:(tile<256u);
    raster_pixels[32u+half*40u+col]=phase?(uint8_t)(cell+232u):(uint8_t)(tile+128u);
    raster_pixels[52u+half*40u+col]=((half?(meta>>3):meta)&7u)|(bank<<3);
   }
   if(current)raster_capture_inputs();
  }
  address=inactive+(uint16_t)row*64u+first;
  VBK_REG=0;atomic_vram(address,raster_pixels+32,width,1);atomic_vram(address+32u,raster_pixels+72,width,1);
  VBK_REG=1;atomic_vram(address,raster_pixels+52,width,1);atomic_vram(address+32u,raster_pixels+92,width,1);
 }
 VBK_REG=0;if(raster_fault)return 0;
 /* IRQ completes first. Reject late windows before any visible mutation. */
 do{wait_vbl_done();CRITICAL {ok=atomic_commit_raw();}}while(!ok&&!raster_fault);
 if(!ok)return 0;
 ++raster_published;retained_selection_put(7,ATOMIC_PHASES,phases,17);
 retained_selection_put(7,ATOMIC_CONTROL,&one,1);
 i=0;for(pair=60u;pair<180u;pair++)if(dirty[pair>>3]&(1u<<(pair&7u)))raster_planes[++i]=pair;
 raster_planes[0]=i;retained_selection_put(7,0xdaa1u,raster_planes,i+1u);
 wait_vbl_done();raster_capture_inputs();return !raster_fault;
}

/* Rev03 stable lower physical-slot transaction. Canonical metadata remains
 * distinct from the frozen caret's temporary OAM/plane merge. Slots0..38
 * belong to source objects; slot39 always owns the independent caret. */
#define SLOT_META 0xd570u
#define SLOT_IDS 0xd840u
#define SLOT_LOADS 0xd9a8u
#define SLOT_USED 0xda38u
#define SLOT_ORDER 0xda40u
#define SLOT_RECORDS 0xdbb9u
#define SLOT_NEXT_ORDER 0xde95u
static void slot_copy(uint8_t from,uint16_t source,uint8_t to,uint16_t dest,uint16_t bytes){
 uint8_t n;
 while(bytes){n=bytes>32u?32u:bytes;selection_window();
  CRITICAL {raster_address=source;raster_ferry_bank=from;raster_ferry_length=n;read_raw();}
  /* Interrupts do not use raster_pixels; the second half consumes that exact
   * fixed buffer without two extra C copies or BANKED calls. */
  selection_window();
  CRITICAL {raster_address=dest;raster_ferry_bank=to;raster_ferry_length=n;write_raw();}
  source+=n;dest+=n;bytes-=n;
 }
}
/* Move backwards because the destination overlaps the source by four bytes.
 * Each complete <=32-byte read precedes its write; both window gates remain.
 * count is 0..39 from caret admission; zero must never reach a raw loop. */
void retained_oam_prepend(uint8_t count) BANKED {
 uint16_t end=(uint16_t)count*4u;uint8_t n;
 while(end){n=end>32u?32u:end;end-=n;slot_copy(raster_build_bank,0xd100u+end,raster_build_bank,0xd104u+end,n);}
}
uint8_t retained_slots_begin(uint8_t *order,uint8_t *used) BANKED {
 uint8_t valid,n=0,pair,layer,count,i,meta[32],slots[2],records,record[12];
 retained_selection_get(7,ATOMIC_CONTROL+2u,&valid,1);
 if(!valid){
  slot_copy(raster_active_bank,0xd000u,raster_build_bank,0xd000u,160);
  slot_copy(raster_active_bank,0xd200u,raster_build_bank,0xd200u,256);
  slot_copy(raster_active_bank,0xd300u,raster_build_bank,0xd300u,720);
  slot_copy(raster_active_bank,0xd600u,raster_build_bank,0xd600u,640);
  slot_copy(raster_active_bank,0xd880u,raster_build_bank,0xd880u,1280);
  slot_copy(raster_active_bank,0xd300u,7,SLOT_META,720);
  memset(order,255,80);memset(used,0,5);
  for(pair=60u;pair<180u;pair++){
   if(!((pair-60u)&7u))retained_selection_get(raster_active_bank,0xd300u+(uint16_t)pair*4u,meta,32);
   i=(pair-60u)&7u;count=(meta[i*4u+1u]>>4)&3u;if(count>2u)return 255u;
   slots[0]=slots[1]=255u;
   for(layer=0;layer<count;layer++){
    if(n>=39u)return 255u;slots[layer]=n;order[n*2u]=pair;order[n*2u+1u]=n|(layer<<6);used[n>>3]|=1u<<(n&7u);n++;
   }
   retained_selection_put(7,SLOT_IDS+(uint16_t)pair*2u,slots,2);
  }
  if(n+1u!=raster_lower_count)return 255u;
  retained_selection_put(7,SLOT_ORDER,order,80);retained_selection_put(7,SLOT_USED,used,5);
  retained_selection_put(7,SLOT_LOADS,raster_loads,144);valid=1;records=0;
  retained_selection_put(7,ATOMIC_CONTROL+1u,&records,1);retained_selection_put(7,ATOMIC_CONTROL+2u,&valid,1);retained_selection_put(7,ATOMIC_CONTROL+3u,&n,1);
 }else{
  /* The previous admitted records remain alive until this catch-up retires
   * their consumers. Clean physical payloads are never rebuilt or moved. */
  retained_selection_get(7,ATOMIC_CONTROL+1u,&records,1);if(records>45u)return 255u;
  for(i=0;i<records;i++){
   retained_selection_get(4,SLOT_RECORDS+(uint16_t)i*12u,record,12);pair=record[0];if(pair<60u||pair>=180u)return 255u;
   slot_copy(raster_active_bank,0xd300u+(uint16_t)pair*4u,raster_build_bank,0xd300u+(uint16_t)pair*4u,4);
   for(layer=0;layer<2u;layer++)if(record[5u+layer]!=255u){
    if(record[5u+layer]>=39u)return 255u;
    slot_copy(raster_active_bank,0xd880u+(uint16_t)record[5u+layer]*32u,raster_build_bank,0xd880u+(uint16_t)record[5u+layer]*32u,32);
   }
  }
  retained_selection_get(7,SLOT_ORDER,order,80);retained_selection_get(7,SLOT_USED,used,5);retained_selection_get(7,SLOT_LOADS,raster_loads,144);retained_selection_get(7,ATOMIC_CONTROL+3u,&n,1);
 }
 return n;
}
void retained_slots_record(uint8_t index,uint8_t pair,const uint8_t *meta,const uint8_t *slots) BANKED {
 uint8_t record[12];memset(record,0,12);record[0]=pair;memcpy(record+1,meta,4);memcpy(record+5,slots,2);
 retained_selection_put(4,SLOT_RECORDS+(uint16_t)index*12u,record,12);
}
void retained_slots_finish(uint8_t n,const uint8_t *order,const uint8_t *used,uint8_t records) BANKED {
 retained_selection_put(4,SLOT_NEXT_ORDER,order,80);retained_selection_put(4,0xde8du,used,5);retained_selection_put(4,0xddd5u,raster_loads,144);
 retained_selection_put(7,ATOMIC_CONTROL+1u,&records,1);retained_selection_put(7,ATOMIC_CONTROL+4u,&n,1);
}
void retained_slots_commit(void) BANKED {
 uint8_t n,records,i,record[12];
 retained_selection_get(7,ATOMIC_CONTROL+1u,&records,1);
 for(i=0;i<records;i++){
  retained_selection_get(4,SLOT_RECORDS+(uint16_t)i*12u,record,12);
  retained_selection_put(7,SLOT_META+(uint16_t)record[0]*4u,record+1,4);retained_selection_put(7,SLOT_IDS+(uint16_t)record[0]*2u,record+5,2);
 }
 slot_copy(4,SLOT_NEXT_ORDER,7,SLOT_ORDER,80);slot_copy(4,0xde8du,7,SLOT_USED,5);slot_copy(4,0xddd5u,7,SLOT_LOADS,144);
 retained_selection_get(7,ATOMIC_CONTROL+4u,&n,1);retained_selection_put(7,ATOMIC_CONTROL+3u,&n,1);
}
void retained_slots_caret_begin(void) BANKED {
 if(raster_lower_count<39u)slot_copy(raster_build_bank,0xd880u+(uint16_t)raster_lower_count*32u,6,0xd680u,32);
}
void retained_slots_caret_end(void) BANKED {
 uint8_t n=raster_lower_count-1u;
 if(n<39u){slot_copy(raster_build_bank,0xd880u+(uint16_t)n*32u,raster_build_bank,0xdd60u,32);slot_copy(6,0xd680u,raster_build_bank,0xd880u+(uint16_t)n*32u,32);}
 retained_selection_get(raster_build_bank,0xd100u,raster_planes,4);raster_planes[2]=118u;retained_selection_put(raster_build_bank,0xd100u,raster_planes,4);
}
void retained_slots_materialize(void) BANKED {
 uint8_t valid,n,i,owner[2];
 retained_selection_get(7,ATOMIC_CONTROL+2u,&valid,1);if(!valid)return;
 retained_selection_get(7,ATOMIC_CONTROL+3u,&n,1);
 /* Gather all unread physical payloads before an in-place permutation can
  * overwrite them. This leased1328-byte staging range fits39*32 bytes. */
 for(i=0;i<n;i++){
  retained_selection_get(7,SLOT_ORDER+(uint16_t)i*2u,owner,2);
  slot_copy(raster_active_bank,0xd880u+(uint16_t)(owner[1]&63u)*32u,6,0xdad0u+(uint16_t)i*32u,32);
 }
 slot_copy(6,0xdad0u,raster_active_bank,0xd880u,(uint16_t)n*32u);
 valid=0;retained_selection_put(7,ATOMIC_CONTROL+2u,&valid,1);
}
/* Canonicalize a prospective stable plan for the exact old publication path.
 * Gather before permutation; slot39 is the independent caret. */
void retained_slots_repack_build(void) BANKED {
 uint8_t n,i,owner[2],zero=0;
 retained_selection_get(7,ATOMIC_CONTROL+4u,&n,1);
 for(i=0;i<n;i++){
  retained_selection_get(4,SLOT_NEXT_ORDER+(uint16_t)i*2u,owner,2);
  slot_copy(raster_build_bank,0xd880u+(uint16_t)(owner[1]&63u)*32u,6,0xdad0u+(uint16_t)i*32u,32);
 }
 slot_copy(raster_build_bank,0xdd60u,6,0xdad0u+(uint16_t)n*32u,32);
 slot_copy(6,0xdad0u,raster_build_bank,0xd880u,(uint16_t)(n+1u)*32u);
 owner[0]=40u+n*2u;retained_selection_put(raster_build_bank,0xd102u,owner,1);
 for(i=0;i<n;i++){owner[0]=40u+i*2u;retained_selection_put(raster_build_bank,0xd106u+(uint16_t)i*4u,owner,1);}
 retained_selection_put(7,ATOMIC_CONTROL+2u,&zero,1);
 owner[0]=1;retained_selection_put(7,ATOMIC_CONTROL,owner,1);
}

uint8_t retained_preview_same(const GameState *g,const HandPreview *p,uint8_t pressed) BANKED {
 uint8_t header[6],i,n;char old[32];char *chips=(char *)raster_planes,*mult=(char *)raster_planes+16,*label=(char *)raster_planes+32,*level=(char *)raster_planes+63;
 if((pressed!=J_LEFT&&pressed!=J_RIGHT)||!raster_live||raster_fault||!retained_caret_mode||g->phase!=PHASE_TABLE||g->ui_mode)return 1;
 retained_selection_get(7,MODEL,header,6);
 if(header[2]==255u||((uint16_t)header[0]|((uint16_t)header[1]<<8))!=raster_published)return 0;
 if(p->hidden||frontend_public_preview_hidden(g)){strcpy(chips,"?");strcpy(mult,"?");strcpy(label,"????");strcpy(level,"LV?");}
 else{
  frontend_format_number_width(&p->chips,4,chips,9);frontend_format_number_width(&p->mult,4,mult,9);
  if(!g->highlighted_count){strcpy(label,"SELECT CARDS");level[0]=0;}
  else{for(i=0;i<30u&&p->label[i]&&!(p->label[i]==' '&&p->label[i+1u]=='L'&&p->label[i+2u]=='V');i++)label[i]=p->label[i];label[i]=0;strcpy(level,"LV");frontend_format_number(&p->level,2,level+2);}
 }
 n=strlen(chips)+1u;retained_selection_get(7,MODEL+offsetof(Frame,chips),(uint8_t *)old,n);if(memcmp(old,chips,n))return 0;
 n=strlen(mult)+1u;retained_selection_get(7,MODEL+offsetof(Frame,mult),(uint8_t *)old,n);if(memcmp(old,mult,n))return 0;
 n=strlen(label)+1u;retained_selection_get(7,MODEL+offsetof(Frame,label),(uint8_t *)old,n);if(memcmp(old,label,n))return 0;
 n=strlen(level)+1u;retained_selection_get(7,MODEL+offsetof(Frame,level),(uint8_t *)old,n);return !memcmp(old,level,n);
}

/* One-use public-mutation certificate, issued only by the audited main-loop
 * boundary before game_input. A raw button byte is FULL_OR_UNKNOWN. No new
 * mutable storage: retained_pressed carries the ticket until renderer entry.
 * Unknown writers/callers must cancel before mutation; absence is unknown.
 * The complete interval is every game_input->save->render iteration: all other
 * input masks invalidate the model before any core mutation. IRQ/audio/save
 * never mutate GameState. Pure certificates cannot cross a page/domain change. */
void retained_input_unknown(void) BANKED {
 retained_pressed=0;retained_selection_invalidate();
}
/* The invalidated Frame cache is scratch until the mandatory render. Store
 * only bounded public hand-target UIDs, not cards/GameState/RNG. IRQ consumers
 * never read this bank7 model. Compare both UID bytes without short circuit. */
#define TARGET_META (MODEL+8u)
typedef char target_scratch_bounds[(sizeof(Frame)>=9u+2u*HAND_STORAGE_CAP&&
 offsetof(Frame,page)+sizeof(PanelBPage)<=8u)?1:-1];
static uint8_t destructive_consumable(uint8_t kind,uint8_t id){
 return (kind==ITEM_TAROT&&id==12u)||
  (kind==ITEM_SPECTRAL&&(id<=2u||id==9u||id==15u));
}
/* Finite actionable public sets: phase/domain, owned counts and identities,
 * SHOP offers and PACK choices. Focus navigation alone is not a set change.
 * Fixed-length work; no scoring/value/RNG state or new mutable allocation. */
#define PUBLIC_TARGET_BYTES 65u
static void public_target_bytes(const GameState *g,uint8_t *b){
 uint8_t i;
 b[0]=g->phase;b[1]=g->ui_mode;b[2]=g->hand_count;b[3]=g->joker_count;
 b[4]=g->consumable_count;b[5]=g->pack_count;b[6]=g->pack_choices;
 b[7]=g->pack_bought[0];b[8]=g->pack_bought[1];
 b[9]=g->shop_kind[0];b[10]=g->shop[0];b[11]=g->shop_kind[1];b[12]=g->shop[1];
 for(i=0;i<8u;i++){b[13u+i*3u]=i<g->joker_count?(uint8_t)PG_JUID(g,i):0;b[14u+i*3u]=i<g->joker_count?(uint8_t)(PG_JUID(g,i)>>8):0;b[15u+i*3u]=i<g->joker_count&&!boss_joker_hidden(g,i)?PG_JBYTE(g,i,joker_edition,9u):0;}
 for(i=0;i<4u;i++){b[37u+i*3u]=i<g->consumable_count?PG_CBYTE(g,i,consumable_kind,0u):0;b[38u+i*3u]=i<g->consumable_count?PG_CBYTE(g,i,consumables,1u):0;b[39u+i*3u]=i<g->consumable_count?PG_CBYTE(g,i,consumable_edition,2u):0;}
 for(i=0;i<5u;i++){b[49u+i*2u]=g->pack_item_kind[i];b[50u+i*2u]=g->pack_ids[i];}
 b[59]=inventory_joker_limit(g);b[60]=inventory_consumable_limit(g);{uint32_t h=pg_hash(2166136261ul);memcpy(b+61u,&h,4u);}
}
static void public_transaction_begin(const GameState *g){
 uint8_t bytes[PUBLIC_TARGET_BYTES];
 frontend_table_transition_begin();retained_selection_invalidate();
 public_target_bytes(g,bytes);retained_selection_put(7,TARGET_META,bytes,sizeof(bytes));
 retained_pressed=0xb0u;
}
static uint8_t public_transaction_changed(const GameState *g){
 uint8_t before[PUBLIC_TARGET_BYTES],after[PUBLIC_TARGET_BYTES],i,changed=0;
 retained_selection_get(7,TARGET_META,before,sizeof(before));public_target_bytes(g,after);
 for(i=0;i<PUBLIC_TARGET_BYTES;i++)changed|=before[i]^after[i];
 return changed;
}
static void target_transaction_begin(const GameState *g){
 uint8_t bytes[32],i,n,j;
 frontend_table_transition_begin();retained_selection_invalidate();
 bytes[0]=g->hand_count;retained_selection_put(7,TARGET_META,bytes,1);
 for(i=0;i<g->hand_count;i+=n){
  n=g->hand_count-i;if(n>16u)n=16u;
  for(j=0;j<n;j++){
   bytes[j*2u]=(uint8_t)g->hand[i+j].uid;
   bytes[j*2u+1u]=(uint8_t)(g->hand[i+j].uid>>8);
  }
  retained_selection_put(7,TARGET_META+1u+(uint16_t)i*2u,bytes,n*2u);
 }
 retained_pressed=0xa0u;
}
static uint8_t target_transaction_changed(const GameState *g){
 uint8_t bytes[32],i,n,j,count,changed;uint16_t uid;
 retained_selection_get(7,TARGET_META,bytes,1);count=bytes[0];
 changed=count^g->hand_count;
 for(i=0;i<count;i+=n){
  n=count-i;if(n>16u)n=16u;
  retained_selection_get(7,TARGET_META+1u+(uint16_t)i*2u,bytes,n*2u);
  for(j=0;j<n;j++){
   uid=i+j<g->hand_count?g->hand[i+j].uid:0;
   changed|=bytes[j*2u]^(uint8_t)uid;
   changed|=bytes[j*2u+1u]^(uint8_t)(uid>>8);
  }
 }
 return changed;
}
void retained_input_begin(const GameState *g,uint8_t key) BANKED {
 retained_pressed=0;
 if(g==&game&&g->phase==PHASE_TITLE&&(key==J_START||(key==J_A&&resume_available))){
  frontend_table_transition_begin();retained_selection_invalidate();
  retained_pressed=0xc0u;a1_title_request(key);return;
 }
 if(g==&game&&g->phase==PHASE_WIN&&(key==J_START||key==J_A)){
  public_transaction_begin(g);return;
 }
 if(g==&game&&g->phase==PHASE_PACK&&!g->ui_mode&&g->pack_count<=5u&&(key==J_A||key==J_B||key==J_START)){
  if(key==J_A&&g->hand_count<=HAND_STORAGE_CAP&&g->pack_count<=5u&&g->focus<g->pack_count&&
     destructive_consumable(g->pack_item_kind[g->focus],g->pack_ids[g->focus])){
   target_transaction_begin(g);return;
  }
  public_transaction_begin(g);return;
 }
 if(g==&game&&g->hand_count<=HAND_STORAGE_CAP&&
    ((g->phase==PHASE_TABLE&&!g->ui_mode&&key==J_SELECT)||
     (g->phase==PHASE_PACK&&g->ui_mode==3u&&key==J_SELECT))){
  target_transaction_begin(g);return;
 }
 if(g==&game&&(g->phase==PHASE_TABLE||g->phase==PHASE_SHOP)&&g->ui_mode==1u&&key==J_A&&
    g->hand_count<=HAND_STORAGE_CAP&&g->consumable_count<=PG_CMAX&&
    g->consumable_focus<g->consumable_count){
  uint8_t kind=PG_CBYTE(g,g->consumable_focus,consumable_kind,0u),id=PG_CBYTE(g,g->consumable_focus,consumables,1u);
  if(destructive_consumable(kind,id)){
   target_transaction_begin(g);return;
  }
  public_transaction_begin(g);return;
 }
 if(g==&game&&
    (((g->phase==PHASE_TABLE||g->phase==PHASE_SHOP)&&g->ui_mode==2u&&key==J_B)||
     (g->phase==PHASE_SHOP&&!g->ui_mode&&(key==J_A||key==J_B||key==J_SELECT)))){
  public_transaction_begin(g);return;
 }
 /* SHOP START can replace the entire hand. The ticket cannot admit a
  * skipped save or an accepted-action dim; it only closes stale input. */
 if(g==&game&&g->phase==PHASE_SHOP&&!g->ui_mode&&key==J_START){
  frontend_table_transition_begin();retained_selection_invalidate();
  retained_pressed=0xe0u;return;
 }
 if(g==&game&&g->phase==PHASE_TABLE&&!g->ui_mode&&g->hand_count&&g->hand_count<=128u&&g->focus<g->hand_count&&g->joker_count<=PG_JMAX&&g->highlighted_count<=5u){
  if(key==J_LEFT||key==J_RIGHT){retained_pressed=0x40u|key;return;}
  if(key==J_A){retained_pressed=0x80u|key;return;}
  /* Durable action tickets carry only a bounded pre-action resource count.
   * They invalidate retained projection immediately and cannot certify a
   * skipped save. Presentation finish must observe an actual decrement. */
  if(key==J_START&&g->highlighted_count&&g->hands&&g->hands<=31u){
   frontend_table_transition_begin();retained_caret_receipt(g);
   retained_selection_invalidate();retained_pressed=0x20u|g->hands;return;
  }
  if(key==J_B&&g->highlighted_count&&g->discards&&g->discards<=31u){
   frontend_table_transition_begin();retained_caret_receipt(g);
   retained_selection_invalidate();retained_pressed=0x60u|g->discards;return;
  }
  if(key==J_START||key==J_B)retained_caret_receipt(g);
 }
 retained_selection_invalidate();
}
/* Main-loop-only finish, immediately after the unchanged game_input and
 * audio observer. The existing one-use ticket proves the pre-action normal
 * TABLE domain and exact single LEFT/RIGHT/A mask. In the bound core these
 * paths change only focus or selection UIDs/count; IRQ/audio never write
 * GameState. Check the post-domain too. No copied GameState or new mutable
 * byte is needed. Sort, compound masks, page-independent gameplay, modals,
 * economy and all unknown actions retain the existing synchronous save.
 * A power cut during pure UI intent resumes the last durable checkpoint's
 * focus/highlight; gameplay/cards/money/resources/score/RNG are not deferred.
 * This persistence certificate is separate from raster admission: a legacy
 * paint or caret miss does not turn a pure UI action into game progress. */
void retained_input_finish(const GameState *g) BANKED {
 uint8_t ticket=retained_pressed;
 if(ticket==0xc0u&&g->phase==PHASE_TITLE)frontend_table_transition_cancel();
 if(ticket==0xb0u&&!public_transaction_changed(g))frontend_table_transition_cancel();
 if(ticket==0xa0u&&!target_transaction_changed(g))frontend_table_transition_cancel();
 if(ticket==0xe0u&&g->phase==PHASE_SHOP)frontend_table_transition_cancel();
 if(g->phase==PHASE_TABLE&&
    (((ticket&0xe0u)==0x20u&&g->hands>=(ticket&31u))||
     ((ticket&0xe0u)==0x60u&&g->discards>=(ticket&31u))))
  frontend_table_transition_cancel();
 retained_caret_acknowledge(g);
 if(g->phase==PHASE_TITLE)return;
 if(g==&game&&g->phase==PHASE_TABLE&&!g->ui_mode&&g->hand_count&&g->hand_count<=128u&&g->focus<g->hand_count&&g->joker_count<=PG_JMAX&&g->highlighted_count<=5u&&
    (ticket==(0x40u|J_LEFT)||ticket==(0x40u|J_RIGHT)||ticket==(0x80u|J_A)))return;
 game_save(g);
}
uint8_t retained_input_take(const GameState *g) BANKED {
 uint8_t ticket=retained_pressed,key=0,header[6],first,count,mask,n=0,i;
 retained_pressed=0;
 if(ticket==(0x40u|J_LEFT)||ticket==(0x40u|J_RIGHT))key=ticket&3u;
 else if(ticket==(0x80u|J_A))key=J_A;
 if(!key||g!=&game||g->phase!=PHASE_TABLE||g->ui_mode||!g->hand_count||g->hand_count>128u||g->focus>=g->hand_count||g->joker_count>5u||g->highlighted_count>5u||!raster_live||raster_fault||!raster_published)goto unknown;
 retained_selection_get(7,MODEL,header,6);
 if(header[2]==255u||((uint16_t)header[0]|((uint16_t)header[1]<<8))!=raster_published)goto unknown;
 count=hand_page(g,&first,&mask);
 if(count!=header[3]||first!=header[2])goto unknown;
 if(key!=J_A){
  for(i=0;i<count;i++)if(mask&(1u<<i))n++;
  if(mask!=header[4]||n>g->highlighted_count||g->highlighted_count-n!=header[5])goto unknown;
 }
 retained_pressed=key;return key;
unknown:
 retained_selection_invalidate();return 0;
}
