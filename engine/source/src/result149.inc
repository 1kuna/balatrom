/* Private native result staging. Main owns all generation and input dispatch.
 * No GameState/SRAM/RNG writes. Immutable glyph/font/layout APIs are reused.
 * The only VRAM/palette/LCDC writes belong to the bounded VBlank publisher. */
#include "result149.h"
#include "publication143.h"
#include "cooperative_native.h"
#include "native_table.h"
#include "raster_runtime.h"
#include "frontend_numeric.h"
#include "frontend_heading.h"
#include "frontend_phase_layout.h"
#include "frontend_phase_chrome.h"
#include "frontend_font_rows.inc"
#include <gb/cgb.h>
#include <string.h>
#include "boss_labels.h"
#include "result_ledger_tiles.inc"
extern volatile uint8_t input_count;
extern uint8_t native_palette_ready;
void raster_capture_inputs(void) NONBANKED;
void raster_ferry_window(void) NONBANKED;
void frontend_legacy_invalidate(void) BANKED;
void frontend_table_transition_cancel(void) BANKED;
void native_busy_wait_invalidate(void) BANKED;
uint8_t result149_active;
static uint8_t rphase,rtick,rpending,rstage,rfailed;
static uint16_t rgeneration,rtiles,rnext,rclock;
static uint8_t rchips,rmult,rtotal,routcome,rcontinue,rhand,rtarget;
static uint8_t rfinalchips,rfinalmult,rfinaltotal,remphasis,rcomparison,rstamp,rpulse,rmultemphasis;
static uint8_t rjobs[98],rjobcount,rblocks,rused[32];
static char rchiptext[16],rmulttext[16],rtotaltext[16],rtargettext[16];
static const char *const rnames[]={"HIGH CARD","PAIR","TWO PAIR","THREE OF A KIND","STRAIGHT","FLUSH","FULL HOUSE","FOUR OF A KIND","STRAIGHT FLUSH","FIVE OF A KIND","FLUSH HOUSE","FLUSH FIVE"};
/* the TABLE's slate sidebar, felt, chip blue and mult red.
 * Palette0 must match render.c ui_palettes palette0 (shop157 borrows sidebar
 * glyphs: colours0/2/3 only). Cohesion: palette1 colour3 is the TABLE dark felt
 * (a1 palette0 colour1, 3,10,6) for the felt texture; its only other user, the
 * RUN LOST plate border, deliberately becomes a muted felt edge. */
static const palette_color_t rpal[12]={RGB(3,3,4),RGB(4,13,9),RGB(30,29,27),RGB(30,23,7),RGB(4,13,9),RGB(29,8,7),RGB(30,29,27),RGB(3,10,6),RGB(4,13,9),RGB(4,17,29),RGB(30,29,27),RGB(30,23,7)};
static uint8_t rvalid(void){raster_capture_inputs();return result149_active&&rgeneration==coop_generation&&game.phase==rphase&&!input_count&&!raster_fault;}
void result149_cancel(void) BANKED {raster_ferry_window();CRITICAL {if(pub143_pending>=2u)pub143_pending=0;}result149_active=rpending=0;}
void result149_input(uint8_t pressed) BANKED {if((rphase==PHASE_WIN&&(pressed&(J_A|J_START)))||(rphase==PHASE_LOSS&&(pressed&J_START)))result149_cancel();}
/* Result-only bounded32-byte ferry. Existing IRQ observers never write the
 * fixed raster_pixels scratch during this exclusive result preparation lease.
 * Source is copied while SVBK1 is mapped; naked leaf never uses stack in5/7. */
extern uint8_t raster_pixels[128];
extern __at(0xffbc) volatile uint16_t raster_address;
extern __at(0xffea) volatile uint8_t raster_ferry_bank;
extern __at(0xffeb) volatile uint8_t raster_ferry_length;
static void rfast_raw(void) __naked {
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
r152_copy:
 ld a,(hl+)
 ld (de),a
 inc de
 dec b
 jr nz,r152_copy
 ld a,#1
 ldh (0x70),a
 pop hl
 pop de
 pop bc
 pop af
 ret
 __endasm;
}
static void rfast_put(uint8_t bank,uint16_t at,const uint8_t *s,uint8_t n){uint8_t part;while(n){part=n>32?32:n;memcpy(raster_pixels,s,part);raster_ferry_window();CRITICAL {raster_address=at;raster_ferry_bank=bank;raster_ferry_length=part;rfast_raw();}at+=part;s+=part;n-=part;}}
/* Exclusive result lease: IRQ observers do not write raster_pixels. */
static void rfast_current(uint8_t bank,uint16_t at,uint8_t n){raster_ferry_window();CRITICAL {raster_address=at;raster_ferry_bank=bank;raster_ferry_length=n;rfast_raw();}}
static void rput(uint16_t tile,const uint8_t *p){rfast_put(5u,0xd000u+tile*16u,p,16u);}
static void rmap(uint8_t x,uint8_t y,uint16_t tile,uint8_t pal){uint8_t v=tile&127u,a=pal|((tile>=128u)?8u:0u);rfast_put(7u,0xd200u+(uint16_t)y*32u+x,&v,1);rfast_put(7u,0xd600u+(uint16_t)y*32u+x,&a,1);}
static void rtext_at(uint16_t base,uint8_t x,uint8_t y,const char *s,uint8_t cells,uint8_t pal,uint8_t heading,uint8_t action,uint8_t maps){uint8_t n,j,row,c,*tile,map[16],attrs[16],a,b,ch=0,off=0,buffered=0,len=strlen(s);char pair[2];
 for(n=0;n<cells;n++){
  tile=raster_pixels+buffered*16u;
  if(heading){a=ch<len?s[ch]:' ';b=ch+1u<len?s[ch+1u]:' ';frontend_heading_tile(a,b,off,x>=4u&&y<16u,tile);off+=2;ch++;if(off==6){off=0;ch++;}}
  else if(action){for(j=0;j<2;j++){pair[j]=0;if(!n&&!j)pair[j]=' ';else if(*s)pair[j]=*s++;}fp_layout_action_tile(pair,n,cells,tile);}
  else{const uint8_t *left,*right;uint8_t *out;memset(tile,0,16);if(x>=4u&&y<16u)for(row=0;row<8;row++)tile[row*2]=255;if(*s){c=*s++;if(c>=128)c='?';left=frontend_font_rows+(uint16_t)c*5u;c=*s;if(c)s++;if(c>=128)c='?';right=frontend_font_rows+(uint16_t)(c?c:' ')*5u;out=tile+2;for(row=0;row<5;row++){uint8_t m=(*left++<<5)|(*right++<<1);out[1]=m;out[0]&=~m;out+=2;}}}
  /* Ledger gold (pal bit7): paper glyph -> gold, slate drop shadow down-right. */
  if(pal&0x80u){uint8_t gm,pv=0,sh;for(row=0;row<8;row++){gm=tile[row*2+1];sh=(uint8_t)(pv>>1)&(uint8_t)~gm;tile[row*2]=(uint8_t)~sh;pv=gm;}}
  /* pal bit6 = gold glyph keeping the background (TABLE sidebar values). */
  if(pal&0x40u)for(row=0;row<8;row++)tile[row*2]|=tile[row*2+1];
  buffered++;if(buffered==3u||n+1u==cells){rfast_current(5u,0xd000u+(base+n+1u-buffered)*16u,buffered*16u);buffered=0;}if(maps){if(rstage<4u)rused[(base+n)>>3]|=1u<<((base+n)&7u);map[n]=(base+n)&127;attrs[n]=(pal&7u)|((base+n>=128)?8:0);}if(!rvalid()){rfailed=1;return;}
 }
 if(maps){if(x<5u&&x+cells<=5u){memcpy(shop157_maps+(uint16_t)y*5u+x,map,cells);memcpy(shop157_attrs+(uint16_t)y*5u+x,attrs,cells);}rfast_put(7,0xd200+(uint16_t)y*32+x,map,cells);rfast_put(7,0xd600+(uint16_t)y*32+x,attrs,cells);}
}
static uint8_t rtext(uint8_t x,uint8_t y,const char*s,uint8_t cells,uint8_t pal,uint8_t heading,uint8_t action){uint8_t base;if((rtiles&127u)+cells>128u)rtiles=(rtiles+127u)&~127u;base=rtiles;if(rtiles+cells>256u){rfailed=1;return 0;}rtiles+=cells;rtext_at(base,x,y,s,cells,pal,heading,action,1);return base;}
/* Reserve the same field geometry, but initial blank padding uses paper tile1.
 * Mutable glyphs are generated only when their real formatted text is drawn. */
static uint8_t rfield(uint8_t x,uint8_t y,const char*s,uint8_t cells){uint8_t base,drawn=(strlen(s)+1u)/2u;if(drawn>cells)drawn=cells;if((rtiles&127u)+cells>128u)rtiles=(rtiles+127u)&~127u;base=rtiles;if(rtiles+cells>256u){rfailed=1;return 0;}rtiles+=cells;rtext_at(base,x,y,s,drawn,0,0,0,1);return base;}
/* Exact IEEE binary64 subset: nonnegative integers 0..65535 only.
 * Retained formatter owns all other values and any narrower width. */
static uint8_t rsmall(const void*v,char out[16]){const uint8_t*p=v;uint16_t e,frac,n;uint8_t i=0,j;char rev[5];if(p[7]&128u)return 0;e=((uint16_t)p[7]<<4)|(p[6]>>4);if(!e){for(j=0;j<8;j++)if(p[j])return 0;out[0]='0';out[1]=0;return 1;}if(e<1023u||e>1038u||(p[0]|p[1]|p[2]|p[3]|(p[4]&15u)))return 0;e-=1023u;frac=((uint16_t)(p[6]&15u)<<12)|((uint16_t)p[5]<<4)|(p[4]>>4);if(frac&(0xffffu>>e))return 0;n=(uint16_t)1u<<e;if(e)n|=frac>>(16u-e);do{rev[i++]='0'+n%10u;n/=10u;}while(n);for(j=0;j<i;j++)out[j]=rev[i-j-1u];out[i]=0;return 1;}
static void rformat(const void*v,uint8_t size,char out[16],uint8_t width){if(size==8u&&rsmall(v,out)&&strlen(out)<=width)return;frontend_format_number_width(v,size,out,width);}
static void rnumber(uint8_t x,uint8_t y,const void*v,uint8_t size,uint8_t cells,uint8_t heading,uint8_t pal){char s[16];rformat(v,size,s,heading&&cells==4?8:heading?cells*8u/6u:cells*2u);if(heading&&cells==4&&strlen(s)>5)heading=0;rtext(x,y,s,cells,pal,heading,0);}
/* Decimal presentation only. Scale the formatted mantissa, retaining exponent.
 * No binary64 helpers/shared numeric scratch; step==end copies exact final text. */
static void rscale(const char *s,uint8_t step,uint8_t end,char out[16]){uint8_t i,n=0,digits=0,at[15],q;uint16_t carry=0,v;strcpy(out,s);if(step>=end)return;for(i=0;s[i]&&s[i]!='e'&&s[i]!='E';i++)if(s[i]>='0'&&s[i]<='9')at[n++]=i;for(i=n;i;i--){q=at[i-1];v=(s[q]-'0')*step+carry;out[q]='0'+v%10;carry=v/10;}for(i=0;i<n;i++){q=at[i];v=carry*10+(out[q]-'0');out[q]='0'+v/end;carry=v%end;digits++;}(void)digits;}
static uint8_t rappend(uint8_t bank,uint16_t source,uint16_t dest,uint8_t vbk,uint8_t blocks){uint8_t *j;if(!blocks||rjobcount>=14u||rblocks+blocks>31u)return 0;j=rjobs+rjobcount*7u;j[0]=source;j[1]=source>>8;j[2]=dest;j[3]=dest>>8;j[4]=vbk;j[5]=bank;j[6]=blocks-1;rjobcount++;rblocks+=blocks;return 1;}
static uint8_t rsubmit(uint8_t mode){if(!rvalid()||!rjobcount)return 0;rfast_put(7u,0xdc00u,rjobs,rjobcount*7u);raster_ferry_window();CRITICAL {pub143_job_count=rjobcount;pub143_status=0;pub143_pending=mode;}rpending=mode;return 1;}
static uint8_t rjob(uint8_t mode,uint8_t bank,uint16_t source,uint16_t dest,uint8_t vbk,uint8_t blocks){rjobcount=rblocks=0;return rappend(bank,source,dest,vbk,blocks)&&rsubmit(mode);}
static void rattribute(uint8_t tile,uint8_t x,uint8_t y,uint8_t n,uint8_t palette){uint8_t i,attrs[15];for(i=0;i<n;i++)attrs[i]=palette|((tile+i>=128)?8:0);rfast_put(7,0xd600+(uint16_t)y*32+x,attrs,n);rappend(7,0xd600+(uint16_t)y*32,0x9c00+(uint16_t)y*32,1,2);}
static uint8_t rglyphs(uint8_t tile,uint8_t count){return rappend(5,0xd000+(uint16_t)tile*16,0x9000+(uint16_t)(tile&127)*16,tile>>7,count);}
/* Publish exact text and its complete field map in one bounded VBlank job.
 * A shorter sample clears old trailing characters through shared paper tile1. */
/* style bit0 insets boxed text by one space; bit1 draws
 * in the heading font (total), falling back to the 3x5 font if it won't fit. */
#define RD_PAD 1u
#define RD_HEAD 2u
#define RD_CENTER 4u
#define RD_RIGHT 8u
#define RD_GOLD 16u
#define RD_TEX 32u /* Cohesion: clear unused field cells to the TABLE felt texture (palette1) */
/* Ledger: text is placed at a centred/right offset inside its fixed field; every
 * other field cell is cleared to paper tile1 in the same publication. */
static uint8_t rdraw(uint8_t tile,uint8_t x,uint8_t y,const char*s,uint8_t cells,uint8_t pal,uint8_t style){uint8_t i,drawn,off,len=strlen(s),map[15],attrs[15];char t[17];
 if((style&RD_PAD)&&len<15u&&len+1u<=cells*2u){t[0]=' ';strcpy(t+1,s);s=t;len++;}
 if((style&RD_HEAD)&&(uint8_t)((len*6u+7u)/8u)<=cells)drawn=(uint8_t)((len*6u+7u)/8u);else{style&=(uint8_t)~RD_HEAD;drawn=(len+1u)/2u;}
 if(drawn>cells)drawn=cells;if(rblocks+drawn+4u>31u||rjobcount+3u>14u)return 0;
 off=(style&RD_CENTER)?(uint8_t)((cells-drawn)/2u):(style&RD_RIGHT)?(uint8_t)(cells-drawn):0u;
 rtext_at(tile,x+off,y,s,drawn,(style&RD_GOLD)?(pal|0x80u):pal,style&RD_HEAD,0,0);if(rfailed||!rvalid())return 0;
 for(i=0;i<cells;i++){uint8_t in=i>=off&&i<off+drawn;
  if(!in&&(style&RD_TEX)){map[i]=RL_TEXAT(x+i,y);attrs[i]=1u;continue;}
  map[i]=in?(uint8_t)((tile+i-off)&127u):1u;attrs[i]=pal|((in&&tile+i-off>=128u)?8u:0u);}
 rfast_put(7u,0xd200u+(uint16_t)y*32u+x,map,cells);rfast_put(7u,0xd600u+(uint16_t)y*32u+x,attrs,cells);rappend(7u,0xd200u+(uint16_t)y*32u,0x9c00u+(uint16_t)y*32u,0u,2u);rappend(7u,0xd600u+(uint16_t)y*32u,0x9c00u+(uint16_t)y*32u,1u,2u);if(drawn)rglyphs(tile,drawn);return 1;}
/* ---- Cash-Out Ledger helpers . Staging-only writes plus bounded jobs. */
#define RBEATS rcontinue /* rcontinue is never read after begin: beat flags, 0 new RAM */
#define RB_CHIPS 1u
#define RB_MULT 2u
#define RB_MONEY 4u
#define RB_BAR 8u
static uint8_t rcentre(uint8_t cells){return 5u+(uint8_t)((15u-cells)/2u);}
static uint8_t rhandcells(void){return game.last_hand<12u?(uint8_t)((strlen(rnames[game.last_hand])*6u+7u)/8u):0u;}
/* Row band attributes for columns5..19; text cells keep their VRAM bank bit. */
static void rband(uint8_t y,uint8_t x0,uint8_t cells,uint8_t tile,uint8_t pal){uint8_t i,c,attrs[15];for(i=0;i<15u;i++){c=5u+i;attrs[i]=pal|((c>=x0&&c<x0+cells&&tile+(c-x0)>=128u)?8u:0u);}rfast_put(7u,0xd600u+(uint16_t)y*32u+5u,attrs,15u);}
/* Rounded token lips (rows3,5) over a 7-cell chips/mult field at x. */
static uint8_t rlips(uint8_t x,uint8_t pal){uint8_t i,m[7],a[7],y;if(rjobcount+4u>14u||rblocks+8u>31u)return 0;
 for(y=3u;y<=5u;y+=2u){uint8_t flip=y==5u?0x40u:0u;for(i=0;i<7u;i++){m[i]=(i==0u||i==6u)?RL_LIP_END:RL_LIP_MID;a[i]=pal|flip|(i==6u?0x20u:0u);}
  rfast_put(7u,0xd200u+(uint16_t)y*32u+x,m,7u);rfast_put(7u,0xd600u+(uint16_t)y*32u+x,a,7u);
  rappend(7u,0xd200u+(uint16_t)y*32u,0x9c00u+(uint16_t)y*32u,0u,2u);rappend(7u,0xd600u+(uint16_t)y*32u,0x9c00u+(uint16_t)y*32u,1u,2u);}
 return 1;}
/* Exact comparison and a conservative (never overstated) fill from binary64 bits.
 * Nonnegative binary64 order equals unsigned byte order; no numeric scratch. */
static uint8_t rfill(void){const uint8_t *s=game.score.bits,*t=game.target.bits;uint8_t i=8u,d;uint16_t es,et,ms,mt,q;
 if((s[7]|t[7])&128u)return 0;
 while(i&&s[i-1u]==t[i-1u])i--;if(!i||s[i-1u]>t[i-1u])return 13u;
 es=((uint16_t)s[7]<<4)|(s[6]>>4);et=((uint16_t)t[7]<<4)|(t[6]>>4);if(!es||!et||et<es)return 0;
 d=(uint8_t)((et-es)>7u?8u:et-es);if(d>7u)return 0;
 ms=256u+(uint16_t)(((s[6]&15u)<<4)|(s[5]>>4));mt=257u+(uint16_t)(((t[6]&15u)<<4)|(t[5]>>4));
 q=(uint16_t)(13u*ms/mt)>>d;return q>12u?12u:(uint8_t)q;}
static uint8_t rbar(uint8_t n){uint8_t i,m[13];if(rjobcount>=14u||rblocks+2u>31u)return 0;for(i=0;i<13u;i++)m[i]=i<n?RL_BAR_FULL:RL_BAR_EMPTY;rfast_put(7u,0xd200u+10u*32u+6u,m,13u);return rappend(7u,0xd200u+10u*32u,0x9c00u+10u*32u,0u,2u);}
/* Stamp plate rows12..14 with heading text centred on row13. */
/* A 2-block job covers one 32-byte map row, so the bottom frame
 * row14 needs its own map and attribute jobs. */
static uint8_t rplate(void){uint8_t i,m[15],a[15],pal=rphase==PHASE_WIN?2u:1u;if(rjobcount+7u>14u||rblocks+22u>31u)return 0;
 for(i=0;i<15u;i++){m[i]=(i==0u||i==14u)?RL_PLATE_CORNER:RL_PLATE_EDGE;a[i]=pal|(i==14u?0x20u:0u);}
 rfast_put(7u,0xd200u+12u*32u+5u,m,15u);rfast_put(7u,0xd600u+12u*32u+5u,a,15u);
 for(i=0;i<15u;i++)a[i]|=0x40u;rfast_put(7u,0xd200u+14u*32u+5u,m,15u);rfast_put(7u,0xd600u+14u*32u+5u,a,15u);
 m[0]=m[1]=RL_PLATE_SIDE;a[0]=pal;a[1]=pal|0x20u;rfast_put(7u,0xd200u+13u*32u+5u,m,1u);rfast_put(7u,0xd600u+13u*32u+5u,a,1u);rfast_put(7u,0xd200u+13u*32u+19u,m+1,1u);rfast_put(7u,0xd600u+13u*32u+19u,a+1,1u);
 if(!rdraw(routcome,6,13,rphase==PHASE_WIN?"BLIND CLEARED":"RUN LOST",13,pal,RD_HEAD|RD_CENTER))return 0;
 rappend(7u,0xd200u+12u*32u,0x9c00u+12u*32u,0u,2u);rappend(7u,0xd600u+12u*32u,0x9c00u+12u*32u,1u,2u);
 rappend(7u,0xd200u+14u*32u,0x9c00u+14u*32u,0u,2u);rappend(7u,0xd600u+14u*32u,0x9c00u+14u*32u,1u,2u);return 1;}
uint8_t result149_begin(const GameState*g) BANKED {uint16_t i;uint8_t tile[16],a;char label[16];uint32_t money;
 if(result149_active){if(g==&game&&g->phase==rphase){rgeneration=coop_generation;return 1;}result149_cancel();}
 /* owned Jokers no longer exclude the native ledger. The scene
  * maps all 18x32 cells itself, uploads only to the 0x9000 region the 8000-mode
  * TABLE never displays, publishes palettes0..2 only (Joker icons own3..7 and are
  * not referenced), hides OBJ at flip, stops native_D_idle while active, and
  * invalidates bank1 residency at commit. shop157_sidebar keeps its own Joker
  * guard. The legacy fallback path held the LCD off for its whole paint
  * (native_table_leave); this route keeps the display live. */
 if(!pub143_enabled||g!=&game||_cpu!=CGB_TYPE||(g->phase!=PHASE_WIN&&g->phase!=PHASE_LOSS)||!native_table_live||raster_live||raster_fault||!native_palette_ready||(LCDC_REG&0x98u)!=0x90u||(LCDC_REG&LCDCF_WINON)||SCX_REG||SCY_REG)return 0;
 frontend_table_transition_cancel();
 shop157_record(g);
 result149_active=1;rphase=g->phase;rgeneration=coop_generation;rpending=rfailed=rstage=rtick=0;rnext=0;rtiles=5u+RL_TILE_COUNT;rfinalchips=rfinalmult=rfinaltotal=remphasis=rcomparison=rstamp=rpulse=rmultemphasis=0;
 /* Tiles0..4 as before; 5..12 Cash-Out Ledger lips/plate/bar (all uploaded in prep). */
 memset(rused,0,sizeof(rused));rused[0]=255u;rused[1]=255u;rused[2]=1u; /* ids0..16 incl. texture13..16 */memset(tile,0,16);rput(0,tile);for(i=0;i<8;i++){tile[i*2]=255;tile[i*2+1]=0;}rput(1,tile);for(a=0;a<3;a++){fp_layout_tile(a,tile);rput(2+a,tile);}
 for(a=0;a<RL_TILE_COUNT;a++)rput(5u+a,rledger_tiles+(uint16_t)a*16u);
 /* Static ledger map: row1 gold rule, row10 empty progress track (caps at5/19). */
 for(i=0;i<18;i++){uint8_t x;uint8_t maprow[32],attrrow[32];memset(maprow,0,32);memset(attrrow,0,32);for(x=5;x<20;x++){maprow[x]=i>=16?0:i==1?4:i==10?((x==5||x==19)?RL_BAR_CAP:RL_BAR_EMPTY):i==2?1:RL_TEXAT(x,i);
  /* Cohesion: open felt carries the TABLE texture (palette1); row2 stays flat for the hand band. */
  if(i<16&&i!=1&&i!=2&&i!=10)attrrow[x]=1u;}if(i==10)attrrow[19]=0x20u;maprow[4]=2;if(i==4||i==8)for(x=0;x<4;x++)maprow[x]=3;memcpy(shop157_maps+i*5u,maprow,5u);memcpy(shop157_attrs+i*5u,attrrow,5u);rfast_put(7,0xd200+i*32,maprow,32);rfast_put(7,0xd600+i*32,attrrow,32);if(!rvalid()){result149_cancel();return 1;}}
 /* Cohesion: money/ante/round values in TABLE gold (pal bit6), same in shop157's borrowed copy. */
 rtext(0,1,"TARGET",5,0,1,0);rnumber(0,3,&g->target,8,4,1,0);rtext(0,6,"SCORE",4,0,1,0);rnumber(0,7,&g->score,8,4,1,0);
 rtext(0,10,"HANDS",3,0,0,0);rnumber(3,10,&g->hands,1,1,0,0);rtext(0,11,"DISC",3,0,0,0);rnumber(3,11,&g->discards,1,1,0,0);rtext(0,12,"MONEY",4,0,0,0);rtext(0,13,"$",1,0x40u,0,0);money=g->money<0?0u-(uint32_t)g->money:(uint32_t)g->money;if(g->money<0){label[0]='-';frontend_format_number(&money,4,label+1);}else frontend_format_number(&money,4,label);rtext(1,13,label,3,0x40u,0,0);rtext(0,15,"ANTE",3,0,0,0);rnumber(3,15,&g->ante,1,1,0,0x40u);rtext(0,16,"RND",3,0,0,0);a=g->blind+1;rnumber(3,16,&a,1,1,0,0x40u);
 /* Row0 blind plate: existing blind/boss name, gold heading, centred. */
 {const char *bn=boss_name(g->blind==2u?g->boss_id:g->blind);uint8_t c=(uint8_t)((strlen(bn)*6u+7u)/8u);if(c>15u)c=15u;rtext(rcentre(c),0,bn,c,0x80u,1,0);}
 /* Row2 hand name, centred heading on a blue band that settles to felt at tick8. */
 a=rhandcells();rhand=rtext(rcentre(a),2,g->last_hand<12?rnames[g->last_hand]:"",a,2,1,0);rband(2,rcentre(a),a,rhand,2u);
 rformat(&g->last_chips,8,rchiptext,14);rformat(&g->last_mult,8,rmulttext,12);rformat(&g->last_score,8,rtotaltext,30);
 /* Row4 tokens (lips on rows3/5 ignite later), row6 total, rows8/9 score/target. */
 rchips=rfield(8,4,"0",7);rtext(12,4,"X",1,0,0,0);rmult=rfield(16,4,"0",7); /* "0" centred in its token */rtotal=rfield(5,6,"",15);
 rtext(5,8,"SCORE",3,0,0,0);rtext(5,9,"TARGET",3,0,0,0);rformat(&g->target,8,rtargettext,24);rtarget=rfield(8,8,"",24);
 /* One reservation: stamp text routcome+0..12, money routcome+13..27. */
 routcome=rfield(6,13,"",28);
 {FPChromeCommand c;fp_chrome_command(rphase==PHASE_WIN?FP_CHROME_BLIND_WIN:FP_CHROME_RUN_LOSS,0,2,&c);rcontinue=rtext(c.x,c.y,c.text,c.cells,c.palette,0,1);}
 RBEATS=0;
 rfast_put(7,0xdaa0,(const uint8_t*)rpal,24);if(rfailed||!rvalid()){result149_cancel();return 1;}coop_resident=0;return 1;
}
void result149_idle(void) BANKED {uint16_t elapsed;uint8_t blocks,a;char s[16];
 if(!result149_active)return;if(!rvalid()){result149_cancel();return;}
 if(rpending){if(pub143_pending)return;if(pub143_status!=2u){uint8_t mode=rpending;rpending=0;rsubmit(mode);return;}if(rpending==3u){rstage=4;shop157_ready=rphase==PHASE_WIN;rclock=sys_time;rtick=0;native_table_bank1_owner=NATIVE_BANK1_INVALID;frontend_legacy_invalidate();}pub143_status=0;rpending=0;}
 /* Fill each preparation batch across glyph/map plane boundaries. The
  * same immutable sources, VBK geometry and 31-block/14-job bounds apply. */
 if(rstage<3u){rjobcount=rblocks=0;while(rstage<3u&&rblocks<31u&&rjobcount<14u){
  if(rstage==0u){uint16_t first;while(rnext<rtiles&&!(rused[rnext>>3]&(1u<<(rnext&7u))))rnext++;if(rnext==rtiles){native_busy_wait_invalidate();rstage=1u;rnext=0;continue;}first=rnext;blocks=0;while(rnext<rtiles&&(rused[rnext>>3]&(1u<<(rnext&7u)))&&blocks<31u-rblocks&&(first&127u)+blocks<128u){rnext++;blocks++;}rappend(5u,0xd000u+first*16u,0x9000u+(first&127u)*16u,first>>7,blocks);}
  else{if(rnext==36u){rstage++;rnext=0;continue;}blocks=36u-rnext;if(blocks>31u-rblocks)blocks=31u-rblocks;rappend(7u,(rstage==1u?0xd200u:0xd600u)+rnext*16u,0x9c00u+rnext*16u,rstage-1u,blocks);rnext+=blocks;}
 }if(rstage==2u&&rnext==36u){rstage=3u;rnext=0;}if(rjobcount){rsubmit(rstage==3u?3u:2u);return;}}
 if(rstage==3){rjob(3,7,0xd200,0x9c00,0,1);return;}
 if(rstage!=4u)return; /* No animation sampling before the scene commit. */
 elapsed=(uint16_t)(sys_time-rclock);if(elapsed>54)elapsed=54;if(elapsed==rtick&&!(elapsed==54&&rpulse!=2))return;rtick=elapsed;
 /* No callback/timer survives exit. Each sample paints final-field copies;
  * the 31-block budget includes simultaneous chips/mult and total/target. */
 rjobcount=rblocks=0;
 /* Cash-Out Ledger beats (see the result animation schedule). Every beat is
  * idempotent and retries next tick if the 14-job/31-block budget is full. */
 if(rtick>=8&&!remphasis&&rjobcount<14u&&rblocks+2u<=31u){a=rhandcells();rband(2,rcentre(a),a,rhand,0);rappend(7u,0xd600u+2u*32u,0x9c00u+2u*32u,1u,2u);remphasis=1;}
 if(rtick>=6&&!(RBEATS&RB_CHIPS)&&rlips(5,2u))RBEATS|=RB_CHIPS;
 if(rtick>=12&&!(RBEATS&RB_MULT)&&rlips(13,1u))RBEATS|=RB_MULT;
 if(rtick>=6&&rtick<=20){rscale(rchiptext,rtick>=16?10:rtick-6,10,s);if(rdraw(rchips,5,4,s,7,2,RD_HEAD|RD_CENTER))rfinalchips=rtick>=16;rscale(rmulttext,rtick>=20?8:rtick<12?0:rtick-12,8,s);if(rdraw(rmult,13,4,s,7,rtick>=12?1u:0u,RD_HEAD|RD_CENTER))rfinalmult=rtick>=20;}
 else if(rtick>20&&(!rfinalchips||!rfinalmult)){if(rdraw(rchips,5,4,rchiptext,7,2,RD_HEAD|RD_CENTER))rfinalchips=1;if(rdraw(rmult,13,4,rmulttext,7,1,RD_HEAD|RD_CENTER))rfinalmult=1;}
 if(rtick>=20&&!rfinaltotal){rscale(rtotaltext,rtick>=30?12:rtick-20,12,s);if(rdraw(rtotal,5,6,s,15,0,RD_HEAD|RD_CENTER|RD_GOLD|RD_TEX)&&rtick>=30)rfinaltotal=1;}
 if(rtick>=30&&!rcomparison){rformat(&game.score,8,s,24);if(rdraw(rtarget,8,8,s,12,0,RD_RIGHT|RD_TEX)&&rdraw(rtarget+12u,8,9,rtargettext,12,0,RD_RIGHT|RD_TEX))rcomparison=1;}
 if(rtick>=34&&!(RBEATS&RB_BAR)){uint8_t f=rfill(),n=rtick>=40?f:(uint8_t)((uint16_t)f*(uint8_t)(rtick-33u)/7u);if(rbar(n)&&rtick>=40)RBEATS|=RB_BAR;}
 if(rtick>=40&&!rstamp&&rplate())rstamp=1;
 if(rtick>=44&&!(RBEATS&RB_MONEY)){char m[24],v[16];uint32_t mag=game.money<0?0u-(uint32_t)game.money:(uint32_t)game.money;strcpy(m,game.money<0?"MONEY -$":"MONEY $");frontend_format_number(&mag,4,v);strcat(m,v);if(rdraw(routcome+13u,5,15,m,15,0,RD_HEAD|RD_CENTER|RD_GOLD|RD_TEX))RBEATS|=RB_MONEY;}
 if(rjobcount){rsubmit(2);return;}
 /* Stamp pulse 48..53 on the stamp's own palette: blue win, red loss. */
 if(rtick>=48&&rpulse==0){palette_color_t pal[12];memcpy(pal,rpal,sizeof(pal));if(rphase==PHASE_WIN)pal[9]=RGB(16,23,27);else pal[5]=RGB(31,18,16);rfast_put(7,0xdaa0,(const uint8_t*)pal,24);rpulse=1;rjob(4,7,0xd200,0x9c00,0,1);return;}
 if(rtick>=53&&rpulse==1){rfast_put(7,0xdaa0,(const uint8_t*)rpal,24);rpulse=2;rjob(4,7,0xd200,0x9c00,0,1);}
 (void)rcontinue;
}
