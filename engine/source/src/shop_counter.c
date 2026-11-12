#pragma bank 255
/* Shop Counter: the legacy shop body, moved out of render.c into its own
 * bank. Paint-only: reads GameState, calls the same name/effect/price/icon
 * APIs and the same shop154 hidden-staging writers. No GameState, RNG, input or
 * save writes; no static RAM. HUD tile ids are lent by render.c and returned. */
#include "game.h"
#include "shop.h"
#include "shop154.h"
#include "sidebar_items.h"
#include "frontend_art_loader.h"
#include "frontend_numeric.h"
#include "frontend_heading.h"
#include "frontend_phase_layout.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "frontend_phase_chrome.h"
#include "boss_labels.h"
#include "frontend_font_rows.inc"
#include "frontend_heading_rows.inc"
#include "shop_counter_tiles.inc"
void raster_capture_inputs(void) NONBANKED;
void frontend_shop_booster_label(uint8_t family,uint8_t size,char out[15]) BANKED;
void frontend_shop_name(uint8_t kind,uint8_t id,char out[33]) BANKED;
void frontend_shop_effect(uint8_t kind,uint8_t id,char out[33]) BANKED;
void frontend_shop_pack_detail(uint8_t family,uint8_t size,char out[33]) BANKED;
uint8_t shop_counter_view(const GameState *g,uint8_t focus,uint8_t hud,uint8_t *detail) BANKED;
uint8_t shop_counter_focus(const GameState *g,uint8_t old,uint8_t focus,uint8_t base,uint8_t detail) BANKED;
uint8_t ledger_legacy_view(const GameState *g,uint8_t hud) BANKED;

#define FELT_TILE 84u /* render.c PAPER_TILE: palette0 colour1 = felt */
#define P_PANEL 7u    /* 0 felt, 1 TABLE well/dark felt, 2 gold, 3 paper */
#define P_RED 1u      /* 0 felt, 1 red, 2 paper, 3 gold */
#define P_DARK 0u     /* 0 sidebar dark, 1 felt, 2 paper, 3 gold */
/* colour1 is the TABLE's own dark felt (a1 palette0 colour1,
 * 3,10,6) used by its Joker slots/DECK well and felt texture, replacing slate. */
static const palette_color_t sc_pal7[4]={RGB(4,13,9),RGB(3,10,6),RGB(30,23,7),RGB(30,29,27)};
static void sc_row(uint8_t x,uint8_t y,uint8_t w,const uint8_t *map,const uint8_t *attr); /* defined below */
/* TABLE felt texture on open felt cells (palette7 colours 0/1), screen-aligned. */
static void sc_felt(uint8_t base,uint8_t x,uint8_t y,uint8_t w){uint8_t m[15],a[15],i;
 for(i=0;i<w;i++){m[i]=base+SC_TEXAT(x+i,y);a[i]=P_PANEL;}sc_row(x,y,w,m,a);}

/* Same rule as render.c shop156_render_data: RAM source during hidden staging
 * goes through the mode-gated uploader, otherwise the fixed SDK gateway. */
static void sc_upload(uint8_t id,const uint8_t *t){
 raster_capture_inputs();VBK_REG=0;
 if(shop154_staging&&(uint16_t)t>=0x8000u)shop156_upload(id,1,t);else shop154_data(id,1,t);
}
static void sc_row(uint8_t x,uint8_t y,uint8_t w,const uint8_t *map,const uint8_t *attr){
 VBK_REG=0;shop154_tiles(x,y,w,1,map);
 if(_cpu==CGB_TYPE){VBK_REG=1;shop154_tiles(x,y,w,1,attr);VBK_REG=0;}
}
static void sc_paint(uint8_t *t,uint8_t row,uint8_t m,uint8_t c){
 t[row*2]=(uint8_t)((t[row*2]&~m)|((c&1u)?m:0u));t[row*2+1]=(uint8_t)((t[row*2+1]&~m)|((c&2u)?m:0u));
}
static void sc_fill(uint8_t *t,uint8_t bg){uint8_t r;for(r=0;r<8u;r++){t[r*2]=(bg&1u)?255u:0u;t[r*2+1]=(bg&2u)?255u:0u;}}
/* Shop-local 3x5 punctuation. The shared font draws its "?" fallback
 * [7,1,2,0,2] for + , ( ) < > = % & # ; * ", so effect text showed "16?",
 * "?12 MULT", "2?3?4?5". Only when the shared glyph IS that fallback is the
 * local glyph used, so every real shared glyph and every other screen is
 * unchanged. Rows are 3-bit masks, bit2 = left column. */
static const char sc_punct_keys[13]={'+',',','(',')','<','>','=','%','&','#',';','*','"'};
static const uint8_t sc_punct_rows[13][5]={
 {0,2,7,2,0},{0,0,0,2,4},{2,4,4,4,2},{2,1,1,1,2},{1,2,4,2,1},{4,2,1,2,4},{0,7,0,7,0},
 {5,1,2,4,5},{2,5,2,5,3},{5,7,5,7,5},{0,2,0,2,4},{0,5,2,5,0},{5,5,0,0,0}};
static const uint8_t *sc_glyph_rows(uint8_t c){const uint8_t *f=frontend_font_rows+(uint16_t)c*5u;uint8_t i;
 if(c!='?'&&f[0]==7u&&f[1]==1u&&f[2]==2u&&f[3]==0u&&f[4]==2u)for(i=0;i<13u;i++)if((uint8_t)sc_punct_keys[i]==c)return sc_punct_rows[i];
 return f;}
static uint8_t sc_char(const char **s,uint8_t *acute){uint8_t c=(uint8_t)**s;*acute=0;if(!c)return ' ';(*s)++;
 if(c==195u&&(uint8_t)**s==169u){(*s)++;*acute=1;return 'E';}return c>=128u?'?':c;}
/* 3x5 text, two characters per cell, chosen background/glyph colour indices. */
static void sc_text(uint8_t *hud,uint8_t x,uint8_t y,const char *s,uint8_t cells,uint8_t pal,uint8_t bg,uint8_t fg){
 uint8_t tile[16],map[15],attr[15],n,r,c,acute,mask,accent,lo,hi,fliplo,fliphi;const uint8_t *left,*right;
 lo=(bg&1u)?255u:0u;hi=(bg&2u)?255u:0u;fliplo=((bg^fg)&1u)?255u:0u;fliphi=((bg^fg)&2u)?255u:0u;
 for(n=0;n<cells&&n<15u&&*hud<255u;n++){
  if(bg==1u&&!*s){map[n]=FELT_TILE;attr[n]=pal;(*hud)++;continue;}
  c=sc_char(&s,&acute);left=sc_glyph_rows(c);accent=acute?32u:0u;
  c=sc_char(&s,&acute);right=sc_glyph_rows(c);if(acute)accent|=2u;
  for(r=0;r<8u;r++){mask=r==0u?accent:(r<=5u?(uint8_t)((left[r-1u]<<5)|(right[r-1u]<<1)):0u);tile[r*2u]=lo^(mask&fliplo);tile[r*2u+1u]=hi^(mask&fliphi);}
  sc_upload(*hud,tile);map[n]=(*hud)++;attr[n]=pal;
 }
 if(n)sc_row(x,y,n,map,attr);
}
/* Centred copy into out (cells*2 characters). */
static const char *sc_centre(const char *s,uint8_t cells,char *out){uint8_t len=strlen(s),pad;
 if(len>=cells*2u)return s;pad=(uint8_t)((cells*2u-len)/2u);memset(out,' ',pad);strcpy(out+pad,s);return out;}
/* Heading font (6px advance) recoloured: glyph fg on bg. Falls back to 3x5.
 * The heading font covers ASCII 32..95 only: names are normalised to upper case
 * (mixed-case names used to show as "D??????? L???????"). fg bit7
 * adds a colour-0 drop shadow one pixel down-right. */
static void sc_head(uint8_t *hud,uint8_t x,uint8_t y,const char *src,uint8_t cells,uint8_t pal,uint8_t bg,uint8_t fg){
 uint8_t tile[16],map[15],attr[15],n,r,g,pv,sh,ch=0,off=0,len=0,a,b,k,px,apostrophe,lo,hi,fliplo,fliphi,drawn,plain=1;uint32_t tick=0;char s[33];const char *orig=src;const uint8_t *left,*right;
 /* Heading glyphs exist for space, 0-9 and A-Z only (its apostrophe is a "?"
  * glyph). Apostrophes become a space plus a locally painted tick; any other
  * punctuation sends the exact original name to the 3x5 path, which now draws
  * it (sc_glyph). Lower case and E-acute map to the heading's capitals. */
 while(*src&&len<32u){a=(uint8_t)*src++;
  if(a==195u&&(uint8_t)*src==169u){src++;a='E';}else if(a>='a'&&a<='z')a=(uint8_t)(a-32u);
  if(a==39u){tick|=(uint32_t)1u<<len;a=' ';}else if(a!=' '&&!(a>='0'&&a<='9')&&!(a>='A'&&a<='Z'))plain=0;
  s[len++]=(char)a;}
 s[len]=0;drawn=(uint8_t)(((uint16_t)len*6u+7u)/8u);lo=(bg&1u)?255u:0u;hi=(bg&2u)?255u:0u;fliplo=((bg^fg)&1u)?255u:0u;fliphi=((bg^fg)&2u)?255u:0u;
 if(!plain||(uint16_t)len*6u>(uint16_t)cells*8u){sc_text(hud,x,y,orig,cells,pal,bg,fg&3u);return;}
 for(n=0;n<cells&&n<15u&&*hud<255u;n++){
  if(bg==1u&&n>=drawn){map[n]=FELT_TILE;attr[n]=pal;(*hud)++;continue;}
  a=ch<len?(uint8_t)s[ch]:' ';b=ch+1u<len?(uint8_t)s[ch+1u]:' ';
  left=heading_rows+(uint16_t)(a-32u)*48u+(2u-(off>>1))*8u;right=heading_rows+(uint16_t)(b-32u)*48u+(5u-(off>>1))*8u;pv=0;apostrophe=0;
  if(tick)for(k=0;k<len;k++)if((tick>>k)&1u){px=(uint8_t)(k*6u+2u);if((px>>3)==n)apostrophe|=(uint8_t)(0x80u>>(px&7u));}
  for(r=0;r<8u;r++){g=*left++|*right++;
   /* 5x7 heading glyph k spans x=6k..6k+4, cap top at row1: tick at x=6k+2. */
   if(r==1u||r==2u)g|=apostrophe;
   sh=(fg&0x80u)?(uint8_t)((pv>>1)&~g):0u;pv=g;
   tile[r*2u]=(uint8_t)((lo^(g&fliplo))&~sh);tile[r*2u+1u]=(uint8_t)((hi^(g&fliphi))&~sh);}
  sc_upload(*hud,tile);map[n]=(*hud)++;attr[n]=pal;off+=2u;ch++;if(off==6u){off=0;ch++;}
 }
 if(n)sc_row(x,y,n,map,attr);
}
/* Rounded panel rows y0..y1, columns x..x+w-1, of static tile ids from base. */
static void sc_panel(uint8_t base,uint8_t x,uint8_t y0,uint8_t y1,uint8_t w,uint8_t pal){
 uint8_t m[15],a[15],i,y;
 for(y=y0;y<=y1;y++){uint8_t edge=y==y0||y==y1,flip=y==y1?0x40u:0u;
  for(i=0;i<w;i++){m[i]=base+((edge&&(i==0u||i+1u==w))?SC_CORNER:SC_FILL);a[i]=pal|flip|((edge&&i+1u==w)?0x20u:0u);}
  sc_row(x,y,w,m,a);}
}
/* Dark price tab, gold numerals, two cells centred over an icon. */
static void sc_price(uint8_t *hud,uint8_t x,uint8_t price){char v[17],t[17];v[0]='$';frontend_format_number(&price,1,v+1);sc_text(hud,x,4,sc_centre(v,2,t),2,P_DARK,0,3);}
static uint8_t sc_slot_x(uint8_t focus){return focus<2u?(uint8_t)(6u+focus*3u):(uint8_t)(14u+(focus-2u)*3u);}
static void sc_focus_bar(uint8_t base,uint8_t focus,uint8_t on){uint8_t m[2],a[2];if(focus>=4u)return;
 m[0]=m[1]=base+(on?SC_FOCUS:SC_FILL);a[0]=P_PANEL;a[1]=P_PANEL|0x20u;sc_row(sc_slot_x(focus),8,2,m,a);}
/* Focus detail: same strings as the legacy shop_detail, in the lower panel. */
static void sc_detail(const GameState *g,uint8_t focus,uint8_t *hud){
 char name[33],eff[33],line[33];uint8_t i,off=0,row;
 name[0]=eff[0]=0;
 if(focus<2u){
  if(g->shop[focus]==255u){strcpy(name,"SOLD");strcpy(eff,"OFFER PURCHASED");}
  else{frontend_shop_name(g->shop_kind[focus],g->shop[focus],name);frontend_shop_effect(g->shop_kind[focus],g->shop[focus],eff);}
 }else if(focus<4u){i=focus-2u;
  frontend_shop_booster_label(g->shop_pack_kind[i],g->pack_size[i],name);
  if(g->pack_bought[i])strcpy(eff,"PACK OPENED");else frontend_shop_pack_detail(g->shop_pack_kind[i],g->pack_size[i],eff);
 }
 sc_head(hud,6,11,name,13,P_PANEL,1,2);
 off=fp_layout_wrap_line(eff,0,26,line);sc_text(hud,6,12,line,13,P_PANEL,1,3);
 if(off)fp_layout_wrap_line(eff,off,26,line);else line[0]=0;sc_text(hud,6,13,line,13,P_PANEL,1,3);
 (void)row;(void)i;
}

uint8_t shop_counter_view(const GameState *g,uint8_t focus,uint8_t hud,uint8_t *detail) BANKED {
 static const uint8_t pack_ids[5][3]={{28,30,31},{8,12,14},{0,4,6},{16,18,19},{20,24,26}};
 uint8_t base,i,x,pack,price,m[11],a[11];char t[16];const char *hint;
 if(hud>(uint8_t)(255u-SC_TILE_COUNT)){*detail=hud;return hud;}
 if(_cpu==CGB_TYPE)shop154_palette(7,1,sc_pal7);
 base=hud;raster_capture_inputs();VBK_REG=0;shop154_data(hud,SC_TILE_COUNT,sc_tiles);hud+=SC_TILE_COUNT;
 /* TABLE felt texture on every open felt cell (render.c cleared them to
  * flat PAPER_TILE): marquee flanks, gutter column12, row9 and row15. */
 for(i=0;i<3u;i++){sc_felt(base,5,i,2);sc_felt(base,18,i,2);}
 for(i=3;i<9u;i++)sc_felt(base,12,i,1);
 sc_felt(base,5,9,15);sc_felt(base,5,15,15);
 /* Rows0..2: SHOP marquee, red sign with gold bulbs, columns7..17. */
 for(i=0;i<11u;i++){m[i]=base+((i==0u||i==10u)?SC_CORNER:(i&1u)?SC_BULB:SC_FILL);a[i]=P_RED|(i==10u?0x20u:0u);}
 sc_row(7,0,11,m,a);for(i=0;i<11u;i++)a[i]|=0x40u;sc_row(7,2,11,m,a);
 for(i=0;i<11u;i++){m[i]=base+((i==0u||i==10u)?SC_BULB:SC_FILL);a[i]=P_RED;}sc_row(7,1,11,m,a);
 sc_head(&hud,11,1,"SHOP",3,P_RED,1,3);
 /* Rows3..8: Joker counter (5..11) and pack counter (13..19), felt gutter12;
  * row9 stays felt so the counters read as separate containers. */
 sc_panel(base,5,3,8,7,P_PANEL);sc_panel(base,13,3,8,7,P_PANEL);
 for(i=0;i<2u;i++){
  x=6u+i*3u;
  if(g->shop[i]!=255u){frontend_load_icon(x,5,i,g->shop_kind[i]==ITEM_JOKER?0u:g->shop_kind[i],g->shop[i]);price=shop_item_price(g,i);sc_price(&hud,x,price);}
  else sc_text(&hud,x,6,"SOLD",2,P_PANEL,1,3);
  x=14u+i*3u;pack=g->pack_size[i]<3u?g->pack_size[i]:0u;
  if(!g->pack_bought[i]){frontend_load_icon(x,5,2u+i,SIDEBAR_ITEM_BOOSTER,pack_ids[g->shop_pack_kind[i]<5u?g->shop_pack_kind[i]:1u][pack]);price=shop_pack_price(g,i);sc_price(&hud,x,price);}
  else sc_text(&hud,x,6,"OPENED",3,P_PANEL,1,3);
 }
 sc_focus_bar(base,focus,1);
 /* Row15 navigation hint (same strings); render.c may overwrite with errors. */
 hint=g->joker_count?(g->consumable_count?"DOWN JOKERS  UP ITEMS":"DOWN JOKERS"):g->consumable_count?"UP ITEMS":"";
 if(*hint)sc_text(&hud,5,15,hint,(uint8_t)((strlen(hint)+1u)/2u),P_DARK,1,2); /* text cells only: texture stays around it */
 /* Rows16..17: the existing actions as buttons (same words, same inputs). */
 sc_text(&hud,5,16,sc_centre("A BUY",6,t),6,P_PANEL,2,1);
 sc_text(&hud,11,16,sc_centre("START NEXT",9,t),9,P_RED,1,2);
 sc_text(&hud,5,17,sc_centre("SEL REROLL",6,t),6,P_PANEL,1,3);
 sc_text(&hud,11,17,sc_centre("B SELL LAST",9,t),9,P_PANEL,1,3);
 /* Rows10..14: focus detail panel, reallocated from *detail on focus moves. */
 sc_panel(base,5,10,14,15,P_PANEL);
 *detail=hud;sc_detail(g,focus,&hud);
 return hud;
}
/* Focus-only redraw: move the gold bar and rebuild the detail tiles in place.
 * base is the view's first HUD id (render.c sidebar_hud_end), detail its mark. */
uint8_t shop_counter_focus(const GameState *g,uint8_t old,uint8_t focus,uint8_t base,uint8_t detail) BANKED {
 if(base<=(uint8_t)(255u-SC_TILE_COUNT)){sc_focus_bar(base,old,0);sc_focus_bar(base,focus,1);}
 sc_detail(g,focus,&detail);return detail;
}

/* Not compiled. Results with owned Jokers now go through the native ledger
 * (result149); this painter ran inside the legacy LCD-off window and made
 * it 9 frames longer. Kept for reference. */
#ifdef SC_LEGACY_LEDGER_PAINTER
#include "result_ledger_tiles.inc"
/* ===== Legacy owned-Joker end of round: static Cash-Out Ledger =====
 * result149 declines when Jokers are owned (their icons hold palettes3..7), so
 * render.c's legacy WIN/LOSS path paints this settled ledger instead. It uses
 * only palettes0..2 (render.c ui_palettes == result149 rpal), the native
 * ledger's own tiles, and the same GameState facts/formatters. No animation:
 * the legacy path publishes in place, so this is the native ledger's t54 frame.
 * Rows1..3 keep render.c's owned-Joker icons. */
#define PAPER 84u /* render.c PAPER_TILE: solid colour1 */
static const char *const lg_hands[12]={"HIGH CARD","PAIR","TWO PAIR","THREE OF A KIND","STRAIGHT","FLUSH","FULL HOUSE","FOUR OF A KIND","STRAIGHT FLUSH","FIVE OF A KIND","FLUSH HOUSE","FLUSH FIVE"};
static void lg_row(uint8_t x,uint8_t y,uint8_t w,uint8_t tile,uint8_t attr){uint8_t m[15],a[15],i;for(i=0;i<w;i++){m[i]=tile;a[i]=attr;}sc_row(x,y,w,m,a);}
static void lg_centre(uint8_t *hud,uint8_t x0,uint8_t w,uint8_t y,const char *s,uint8_t pal,uint8_t bg,uint8_t fg){
 uint8_t c=(uint8_t)((strlen(s)*6u+7u)/8u);if(c>w)c=w;sc_head(hud,(uint8_t)(x0+(w-c)/2u),y,s,c,pal,bg,fg);}
/* Same exact comparison / conservative lower bound as result149 rfill. */
static uint8_t lg_fill(const GameState *g){const uint8_t *s=g->score.bits,*t=g->target.bits;uint8_t i=8u,d;uint16_t es,et,ms,mt,q;
 if((s[7]|t[7])&128u)return 0;
 while(i&&s[i-1u]==t[i-1u])i--;if(!i||s[i-1u]>t[i-1u])return 13u;
 es=((uint16_t)s[7]<<4)|(s[6]>>4);et=((uint16_t)t[7]<<4)|(t[6]>>4);if(!es||!et||et<es)return 0;
 if(et-es>7u)return 0;d=(uint8_t)(et-es);
 ms=256u+(uint16_t)(((s[6]&15u)<<4)|(s[5]>>4));mt=257u+(uint16_t)(((t[6]&15u)<<4)|(t[5]>>4));
 q=(uint16_t)(13u*ms/mt)>>d;return q>12u?12u:(uint8_t)q;}
/* Rounded token: lips rows y-1/y+1, solid colour row y, centred heading value. */
static void lg_token(uint8_t *hud,uint8_t lb,uint8_t x,uint8_t y,uint8_t pal,const char *v){uint8_t m[7],a[7],i;
 for(i=0;i<7u;i++){m[i]=lb+((i==0u||i==6u)?RL_LIP_END:RL_LIP_MID)-RL_LIP_END;a[i]=pal|(i==6u?0x20u:0u);}
 sc_row(x,y-1u,7,m,a);for(i=0;i<7u;i++)a[i]|=0x40u;sc_row(x,y+1u,7,m,a);
 lg_row(x,y,7,PAPER,pal);lg_centre(hud,x,7,y,v,pal,1,2);}
uint8_t ledger_legacy_view(const GameState *g,uint8_t hud) BANKED {
 uint8_t lb,i,win=g->phase==PHASE_WIN,pal=win?2u:1u,n,tile[16],m[15],a[15];char v[17],w[24];uint32_t mag;
 const char *s;FPChromeCommand c;char pair[2];uint8_t j;
 if(hud>170u)return hud; /* HUD budget guard: keeps every id below 255 */
 lb=hud;for(i=0;i<8u;i++){raster_capture_inputs();VBK_REG=0;shop154_data(hud++,1,rledger_tiles+(uint16_t)i*16u);}
 /* Row0 blind plate (gold, shadow); row4 gold rule; row5 hand name. */
 lg_centre(&hud,5,15,0,boss_name(g->blind==2u?g->boss_id:g->blind),0,1,3u|0x80u);
 fp_layout_tile(FP_LAYOUT_CONTENT_RULE,tile);raster_capture_inputs();VBK_REG=0;shop154_data(hud,1,tile);lg_row(5,4,15,hud++,0);
 lg_centre(&hud,5,15,5,g->last_hand<12u?lg_hands[g->last_hand]:"",0,1,2);
 /* Rows6..8 chips (blue) X mult (red) tokens; row9 gold total. */
 frontend_format_number_width(&g->last_chips,8,v,14);lg_token(&hud,lb,5,7,2,v);
 sc_text(&hud,12,7,"X",1,0,1,2);
 frontend_format_number_width(&g->last_mult,8,v,12);lg_token(&hud,lb,13,7,1,v);
 frontend_format_number_width(&g->last_score,8,v,30);lg_centre(&hud,5,15,9,v,0,1,3u|0x80u);
 /* Rows10/11 exact SCORE and TARGET, right aligned. */
 sc_text(&hud,5,10,"SCORE",3,0,1,2);sc_text(&hud,5,11,"TARGET",3,0,1,2);
 frontend_format_number_width(&g->score,8,v,24);n=(uint8_t)((strlen(v)+1u)/2u);sc_text(&hud,(uint8_t)(20u-n),10,v,n,0,1,2);
 frontend_format_number_width(&g->target,8,v,24);n=(uint8_t)((strlen(v)+1u)/2u);sc_text(&hud,(uint8_t)(20u-n),11,v,n,0,1,2);
 /* Row12 progress track: exact comparison, never-overstated loss fill. */
 n=lg_fill(g);for(i=0;i<15u;i++){m[i]=lb+((i==0u||i==14u)?RL_BAR_CAP:(i-1u<n?RL_BAR_FULL:RL_BAR_EMPTY))-RL_LIP_END;a[i]=i==14u?0x20u:0u;}sc_row(5,12,15,m,a);
 /* Rows13..15 stamp plate, same strings as the native ledger. */
 for(i=0;i<15u;i++){m[i]=lb+((i==0u||i==14u)?RL_PLATE_CORNER:RL_PLATE_EDGE)-RL_LIP_END;a[i]=pal|(i==14u?0x20u:0u);}
 sc_row(5,13,15,m,a);for(i=0;i<15u;i++)a[i]|=0x40u;sc_row(5,15,15,m,a);
 m[0]=lb+RL_PLATE_SIDE-RL_LIP_END;a[0]=pal;sc_row(5,14,1,m,a);a[0]=pal|0x20u;sc_row(19,14,1,m,a);
 lg_row(6,14,13,PAPER,pal);lg_centre(&hud,6,13,14,win?"BLIND CLEARED":"RUN LOST",pal,1,2);
 /* Row16 existing action strip (same chrome command and tile writer). */
 if(fp_chrome_command(win?FP_CHROME_BLIND_WIN:FP_CHROME_RUN_LOSS,0,2,&c)&&c.cells<=15u&&hud<=(uint8_t)(255u-c.cells)){
  s=c.text;for(n=0;n<c.cells;n++){for(j=0;j<2u;j++){pair[j]=0;if(!n&&!j)pair[j]=' ';else if(*s)pair[j]=*s++;}
   fp_layout_action_tile(pair,n,c.cells,tile);sc_upload(hud,tile);m[n]=hud++;a[n]=2;}
  sc_row(c.x,c.y,c.cells,m,a);}
 /* Row17 true current money (gold on the dark band), no invented payout rows. */
 mag=g->money<0?0u-(uint32_t)g->money:(uint32_t)g->money;strcpy(w,g->money<0?"MONEY -$":"MONEY $");frontend_format_number(&mag,4,v);strcat(w,v);
 lg_centre(&hud,5,15,17,w,0,0,3);
 return hud;
}
#endif /* SC_LEGACY_LEDGER_PAINTER */
