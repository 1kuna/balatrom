#include "paged_inventory.h"
#undef PG_JBYTE
#undef PG_JNUM
#undef PG_JUID
#undef PG_CBYTE
#define PG_JBYTE(g,i,f,o) (*pg_jfield(g,i,o))
#define PG_JNUM(g,i,f,o) (*(GbNum*)pg_jfield(g,i,o))
#define PG_JUID(g,i) (*(uint16_t*)pg_jfield(g,i,12u))
#define PG_CBYTE(g,i,f,o) (*pg_cfield(g,i,o))
#include "inventory_bounds.h"
#include "cooperative_native.h"
#include "joker_history.h"
#pragma bank 255
/* Own isolated replacement: actual GameState, no rule/RNG calls, no framebuffer. */
#include "game.h"
#include "result149.h"
#include "native_table.h"
#include "owner_legacy_cards.h"
#include "a1_title_runtime.h"
#include "frontend_phase_chrome.h"
#include "frontend_phase_layout.h"
#include "raster_runtime.h"
#include "boss_labels.h"
#include "hand_preview.h"
#include "frontend_public_preview.h"
#include "boss.h"
#include "boss_optional.h"
#include "hand_api.h"
#include "sidebar_dynamic.h"
#include "sidebar_jokers.h"
#include "sidebar_items.h"
#include "joker_ids.h"
#include "joker_config.h"
#include "shop.h"
#include "save.h"
#include "frontend_render.h"
#include "frontend_art_loader.h"
#include "frontend_numeric.h"
#include "gb_num_format.h"
#include "sidebar_dynamic.inc"
#include "frontend_backs.inc"
#include "frontend_font_rows.inc"
#include "frontend_heading.h"
#include <gb/gb.h>
#include <gb/cgb.h>
#include <string.h>
#include "shop154.h"
#define set_bkg_data shop156_render_data
#define set_bkg_tiles shop154_tiles
#define set_bkg_palette shop154_palette
static void shop156_render_data(uint8_t,uint8_t,const uint8_t *);
/* Private shop-only RAM-copy helpers; existing phase chrome/cache interfaces stay exact. */
uint8_t frontend_shop_joker_short(uint8_t id,char out[15]) BANKED;
void frontend_shop_booster_label(uint8_t family,uint8_t size,char out[15]) BANKED;
void frontend_shop_name(uint8_t kind,uint8_t id,char out[33]) BANKED;
void frontend_shop_effect(uint8_t kind,uint8_t id,char out[33]) BANKED;
uint8_t frontend_modifier_effect(uint8_t family,uint8_t value,char out[33]) BANKED;
uint8_t frontend_joker_now(const GameState *g,uint8_t i,char out[33]) BANKED;
uint8_t frontend_consumable_now(const GameState *g,uint8_t kind,uint8_t id,char out[33]) BANKED;
void frontend_shop_pack_detail(uint8_t family,uint8_t size,char out[33]) BANKED;
#define CARD_BASE 1u
#define MARKER_TILE 44u
#define JOKER_BASE 45u
#define BACK_BASE 75u
#define STONE_TILE 83u
#define PAPER_TILE 84u
#define DEBUFF_TILE 85u
#define LOCK_TILE 86u
#define SELECT_COUNT_TILE 87u
#define HUD_BASE 88u
/* One6-tick selection flight; no GameState storage, RNG or rule calls.
 * Uses OAM0..9 and existing bank0 BG patterns, independent OBJpalette0.
 *14 fixed bytes; no framebuffer, tile copies or additional tile patterns. */
typedef struct {uint8_t map[8],x,from,to,tick,active,focused;} SelectionMotion;
static SelectionMotion selection_motion;
static void motion_finish(void);
static void motion_tick(void);
volatile uint8_t input_queue[16],input_head,input_count,input_held;
/* Existing byte: bit0 legacy paint busy; bit7 replacement-hand transition. */
volatile uint8_t frontend_render_busy;
/* input policy: while an action's screen change is pending or
 * in progress, ACTION edges (A/B/SELECT/START, or any compound edge holding
 * one) are discarded, both live and from the queue, so a repeat cannot act on
 * a screen the player has not seen. Single-direction NAVIGATION edges are
 * never discarded: they queue in order and apply after the change. Previously
 * LEFT/RIGHT were kept live (coop_accepting) but purged from the queue, so the
 * same tap was kept or lost depending on a few frames of render timing. */
#define STALE_HAND_KEYS (J_A|J_B|J_SELECT|J_START)
uint8_t frontend_input_overflow;
static uint8_t marker_tick,score_flash,last_phase=255,legacy_irq_installed;
static uint8_t last_score_seen[8];
static void capture_input_raw(void) NONBANKED {uint8_t now=joypad(),edge=now&~input_held;input_held=now;if((frontend_render_busy&128u)&&(edge&STALE_HAND_KEYS))return;if(edge){if(input_count<16){input_queue[(input_head+input_count)&15]=edge;input_count++;}else frontend_input_overflow=1;}}
void raster_ferry_window(void) NONBANKED;
/* Main-only input polling/pop masks IRQs across the SDK joypad read and FIFO
 * write. Use the same native LYC lead window as the WRAM ferries;
 * VBlank capture remains the raw IRQ observer and never waits here. */
static void capture_input(void){raster_ferry_window();CRITICAL {capture_input_raw();}}
static void frontend_vblank_capture(void) NONBANKED {capture_input_raw();}
uint8_t frontend_read_pressed(void) BANKED {uint8_t b=0;raster_ferry_window();CRITICAL {capture_input_raw();if(input_count){b=input_queue[input_head];input_head=(input_head+1)&15;input_count--;}}if(b&&!raster_live)motion_finish();return b;}
void raster_ferry_window(void) NONBANKED;
void frontend_table_transition_begin(void) BANKED {
 uint8_t i,n,read,write,kept=0,mask;
 /* The full16 queue filter takes at most six double-speed scanlines.
  * Leave six lines before STAT32; wait with IRQs enabled. Natural main-loop
  * entry at VBlank/early frame remains admitted without another frame wait. */
 if(raster_live&&(LCDC_REG&LCDCF_ON))while(LY_REG>=26u&&LY_REG<=47u){}
 CRITICAL {
  frontend_render_busy|=128u;n=input_count;read=write=input_head;
  /* Preserve ordered inventory navigation. A compound edge containing any
   * stale hand target is ignored as a whole, never rewritten into new input. */
  for(i=0;i<n;i++){
   mask=input_queue[read];read=(read+1u)&15u;
   if(!(mask&STALE_HAND_KEYS)){input_queue[write]=mask;write=(write+1u)&15u;kept++;}
  }
  input_count=kept;capture_input_raw();
 }
}
void frontend_table_transition_cancel(void) BANKED {
 /* Includes native joypad sampling (428T observed), not only a flag store. */
 raster_ferry_window();
 CRITICAL {capture_input_raw();frontend_render_busy&=127u;}
}
void frontend_table_transition_end(void) BANKED {
 if(!(frontend_render_busy&128u))return;
 /* Mandatory render has returned. Wait for a completed replacement frame
  * before input can address it. A held button remains held across unlock. */
 wait_vbl_done();frontend_table_transition_cancel();
}

static uint8_t hud_next;
static const palette_color_t ui_palettes[12]={RGB(3,3,4),RGB(4,13,9),RGB(30,29,27),RGB(30,23,7),RGB(4,13,9),RGB(29,8,7),RGB(30,29,27),RGB(30,23,7),RGB(4,13,9),RGB(4,17,29),RGB(30,29,27),RGB(30,23,7)};
/* Score feedback owns its event-local16-tick lifetime. The independent
 * periodic marker keeps its existing phase and7/15 update cadence. No new RAM. */
void native_D_idle(void) BANKED;
void frontend_animation_tick(void) BANKED {
 if(result149_active){capture_input();return;}
 if(raster_live||native_table_live){capture_input();if(native_table_live)native_D_idle();return;}
 uint8_t marker[16]={0};capture_input();motion_tick();
 if(++marker_tick==15){marker_tick=0;marker[0]=marker[1]=0xDB;}
 else if(marker_tick!=7)goto flash;
 VBK_REG=0;set_bkg_data(MARKER_TILE,1,marker);
 flash:
 if(score_flash&&_cpu==CGB_TYPE){
  if(--score_flash!=8u&&score_flash)return;
  palette_color_t p[4];memcpy(p,ui_palettes+8,sizeof(p));
  if(score_flash)p[1]=RGB(16,23,27);
  set_bkg_palette(2,1,p);
 }
}
static const char *const hand_names[]={"HIGH CARD","PAIR","TWO PAIR","THREE OF A KIND","STRAIGHT","FLUSH","FULL HOUSE","FOUR OF A KIND","STRAIGHT FLUSH","FIVE OF A KIND","FLUSH HOUSE","FLUSH FIVE"};

static void map_rect(uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint8_t tile,uint8_t pal){uint8_t row[20],i;for(i=0;i<w;i++)row[i]=tile;VBK_REG=0;while(h--){capture_input();set_bkg_tiles(x,y,w,1,row);if(_cpu==CGB_TYPE){VBK_REG=1;for(i=0;i<w;i++)row[i]=pal;set_bkg_tiles(x,y,w,1,row);for(i=0;i<w;i++)row[i]=tile;VBK_REG=0;}y++;}}
static void motion_positions(uint8_t y){uint8_t j;for(j=0;j<8;j++)move_sprite(j,selection_motion.x+8+(j&1)*8,y+16+(j>>1)*8);if(selection_motion.focused){move_sprite(8,selection_motion.x+8,y+8);move_sprite(9,selection_motion.x+16,y+8);}}
static void motion_finish(void){uint8_t attrs[8],j;if(!selection_motion.active)return;VBK_REG=0;set_bkg_tiles(selection_motion.x>>3,selection_motion.to>>3,2,4,selection_motion.map);if(_cpu==CGB_TYPE){for(j=0;j<8;j++)attrs[j]=(get_sprite_prop(j)&104u)|1u;VBK_REG=1;set_bkg_tiles(selection_motion.x>>3,selection_motion.to>>3,2,4,attrs);VBK_REG=0;}if(selection_motion.focused)map_rect(selection_motion.x>>3,(selection_motion.to>>3)-1,2,1,MARKER_TILE,1);selection_motion.active=0;HIDE_SPRITES;}
static void motion_tick(void){static const uint8_t ease[6]={0,1,3,5,7,8};uint8_t d;if(!selection_motion.active)return;if(++selection_motion.tick>=6){motion_finish();return;}d=ease[selection_motion.tick];motion_positions(selection_motion.to<selection_motion.from?selection_motion.from-d:selection_motion.from+d);}
static void motion_begin(uint8_t x,uint8_t target,uint8_t focused,const uint8_t *map,const uint8_t *attrs){uint8_t j;selection_motion.x=x;selection_motion.to=target;selection_motion.from=target==88?96:88;selection_motion.focused=focused;selection_motion.tick=0;selection_motion.active=1;memcpy(selection_motion.map,map,8);set_sprite_palette(0,1,ui_palettes+4);SPRITES_8x8;for(j=0;j<8;j++){set_sprite_tile(j,map[j]);set_sprite_prop(j,attrs[j]&104u);}for(j=8;j<10;j++){set_sprite_tile(j,MARKER_TILE);set_sprite_prop(j,0);move_sprite(j,0,0);}motion_positions(selection_motion.from);SHOW_SPRITES;/* Preserve old BG until default VBlank DMA stages these sprites. */wait_vbl_done();map_rect(x>>3,10,2,6,0,1);}
static void text(uint8_t x,uint8_t y,const char *s,uint8_t cells,uint8_t pal){uint8_t tile[16],map[16],attrs[16],n,row,j,c,acute,paper=pal==1||pal==2||(x>=4&&y<16);const uint8_t *glyph;for(n=0;n<cells&&n<16&&hud_next<255;n++){if(!*s){map[n]=paper?PAPER_TILE:0;attrs[n]=pal&7u;continue;}memset(tile,0,16);if(paper)for(row=0;row<8;row++)tile[row*2]=255;for(j=0;j<2;j++){acute=0;c=*s;if(c)s++;if(c==195u&&(uint8_t)*s==169u){s++;c='E';acute=1;}if(c>=128)c='?';glyph=frontend_font_rows+(uint16_t)(c?c:' ')*5;for(row=0;row<5;row++){uint8_t mask=j?glyph[row]<<1:glyph[row]<<5;tile[(row+1)*2+1]|=mask;tile[(row+1)*2]&=~mask;}if(acute){uint8_t mask=j?2u:32u;tile[1]|=mask;tile[0]&=~mask;}}
  /* pal bit7 = gold glyph (colour3), TABLE value colour; shop sidebar only. */
  if(pal&0x80u)for(row=0;row<8;row++)tile[row*2]|=tile[row*2+1];
  capture_input();set_bkg_data(hud_next,1,tile);map[n]=hud_next++;attrs[n]=pal&7u;}if(!n)return;VBK_REG=0;set_bkg_tiles(x,y,n,1,map);if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,n,1,attrs);VBK_REG=0;}}

/* Five-pixel hierarchy font; six-pixel advance, generated into the same
 * bounded HUD tiles. No framebuffer/new WRAM/extra palette. */
/* Fixed code retains the calling renderer bank for font/capture_input. */
static void heading(uint8_t x,uint8_t y,const char *s,uint8_t cells,uint8_t pal) NONBANKED {
 uint8_t tile[16],map[16],attrs[16],n,ch=0,offset=0,first,second,len=strlen(s);
 uint8_t paper=pal==1||pal==2||(x>=4&&y<16);
 for(n=0;n<cells&&n<16&&hud_next<255;n++){
  /* Capture bytes while the caller's ROM bank is still mapped. A missing
   * character is exactly the original blank5x7 space glyph. */
  first=ch<len?(uint8_t)s[ch]:' ';second=ch+1u<len?(uint8_t)s[ch+1u]:' ';
  frontend_heading_tile(first,second,offset,paper,tile);
  capture_input();set_bkg_data(hud_next,1,tile);map[n]=hud_next++;attrs[n]=pal;
  offset+=2u;ch++;if(offset==6u){offset=0;ch++;}
 }
 VBK_REG=0;set_bkg_tiles(x,y,n,1,map);
 if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,n,1,attrs);VBK_REG=0;}
}
/* Paint-only phase helpers: caller-owned HUD allocation, banked immutable
 * patterns and copied strings. No GameState write or public identity lookup. */
static void phase_action(uint8_t x,uint8_t y,const char *s,uint8_t cells){
 uint8_t tile[16],map[15],attrs[15],n,j;char pair[2];
 if(cells>15||hud_next>255u-cells)return;
 for(n=0;n<cells;n++){
  for(j=0;j<2;j++){pair[j]=0;if(!n&&!j)pair[j]=' ';else if(*s)pair[j]=*s++;}
  fp_layout_action_tile(pair,n,cells,tile);capture_input();VBK_REG=0;
  set_bkg_data(hud_next,1,tile);map[n]=hud_next++;attrs[n]=2;
 }
 set_bkg_tiles(x,y,cells,1,map);
 if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,cells,1,attrs);VBK_REG=0;}
}
static void phase_wrap(uint8_t x,uint8_t y,const char *s,uint8_t cells,uint8_t rows){
 char line[33],label[33];uint8_t offset=0,row,done=0;
 strcpy(label,s);
 for(row=0;row<rows;row++){
  if(done)line[0]=0;else{offset=fp_layout_wrap_line(label,offset,cells*2,line);done=!offset;}
  text(x,y+row,line,cells,cells==15u?2u:0u); /* full-width effect text: blue tooltip panel */
 }
}
static void phase_rule(uint8_t x,uint8_t y,uint8_t cells){
 uint8_t tile[16];if(hud_next>=255)return;
 fp_layout_tile(FP_LAYOUT_CONTENT_RULE,tile);capture_input();VBK_REG=0;
 set_bkg_data(hud_next,1,tile);map_rect(x,y,cells,1,hud_next++,0);
}
static void phase_sidebar_chrome(void){
 uint8_t tile[16],base=hud_next;if(hud_next>253)return;
 fp_layout_tile(FP_LAYOUT_SPINE,tile);capture_input();VBK_REG=0;set_bkg_data(hud_next++,1,tile);
 fp_layout_tile(FP_LAYOUT_SIDE_RULE,tile);capture_input();VBK_REG=0;set_bkg_data(hud_next++,1,tile);
 map_rect(4,0,1,18,base,0);map_rect(0,4,4,1,base+1,0);map_rect(0,8,4,1,base+1,0);
}
/* Same renderer owns the retained phase maps, palettes and HUD generation. */
static uint8_t chrome_focus_tile,chrome_generation,compat_generation;
static void chrome_heading(const FPChromeCommand *c,uint8_t family) NONBANKED {
 uint8_t tile[16],map[16],attrs[16],n;
 for(n=0;n<c->cells&&n<16&&hud_next<255;n++){
  if(!fp_chrome_heading_tile(c->key,family,n,tile))break;
  capture_input();VBK_REG=0;set_bkg_data(hud_next,1,tile);
  map[n]=hud_next++;attrs[n]=c->palette;
 }
 if(!n)return;
 set_bkg_tiles(c->x,c->y,n,1,map);
 if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(c->x,c->y,n,1,attrs);VBK_REG=0;}
}
static void chrome_command(uint8_t view,uint8_t family,uint8_t ordinal) NONBANKED {
 FPChromeCommand c;
 if(!fp_chrome_command(view,family,ordinal,&c))return;
 if(c.style==FP_CHROME_HEADING)chrome_heading(&c,family);
 else if(c.y==16)phase_action(c.x,c.y,c.text,c.cells);
 else text(c.x,c.y,c.text,c.cells,c.palette);
}
static void chrome_focus_allocate(void) NONBANKED {
 uint8_t tiles[32];
 if(hud_next>253)return;
 fp_chrome_focus_tiles(tiles);capture_input();VBK_REG=0;
 chrome_focus_tile=hud_next;set_bkg_data(hud_next,2,tiles);hud_next+=2;
 compat_generation=chrome_generation;
}
static void chrome_focus(uint8_t x,uint8_t y) NONBANKED {
 uint8_t map[2],attrs[2]={0,0};
 if(!compat_generation||compat_generation!=chrome_generation)return;
 map[0]=chrome_focus_tile;map[1]=chrome_focus_tile+1;capture_input();VBK_REG=0;
 set_bkg_tiles(x,y,2,1,map);
 if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,2,1,attrs);VBK_REG=0;}
}
static uint8_t chrome_family(uint8_t kind) NONBANKED {
 return kind==ITEM_JOKER?FP_CHROME_BUFFOON:kind==ITEM_PLANET?FP_CHROME_CELESTIAL:
 kind==ITEM_TAROT?FP_CHROME_ARCANA:kind==ITEM_PLAYING?FP_CHROME_STANDARD:FP_CHROME_SPECTRAL;
}

static void large_number(uint8_t x,uint8_t y,const void *value,uint8_t size,uint8_t cells,uint8_t pal){char s[16];uint8_t copy[8],i;for(i=0;i<size&&i<8;i++)copy[i]=((const uint8_t *)value)[i];frontend_format_number_width(copy,size,s,cells*8/6);heading(x,y,s,cells,pal);}
/* Five binary64 fields cache exact input bits and16-byte display strings.
 *123 bytes of fixed renderer state including key mask/state pointer. Numeric
 * formatter only runs after value changes, never from VBlank or animation. */
typedef struct {uint8_t bits[8];char text[16];} NumberCache;
static NumberCache number_cache[5];
static uint8_t number_cache_valid;
static const GameState *number_state;
/* Sidebar width may be8characters: use correctly rounded precision, and
 * split at the exponent marker when needed. Never split exponent digits. */
static void compact_scientific(char *s){uint8_t i,j;for(i=0;s[i];i++)if(s[i]=='E'||s[i]=='e'){s[i]='e';j=i+1+(s[i+1]=='-'||s[i+1]=='+');while(s[j]=='0'&&s[j+1])memmove(s+j,s+j+1,strlen(s+j));return;}}
static void selection_count(uint8_t n){char label[3];uint8_t saved=hud_next;label[0]=n?'S':0;label[1]='0'+n;label[2]=0;hud_next=SELECT_COUNT_TILE;text(4,17,label,1,0);hud_next=saved;}
static void number_value(uint8_t x,uint8_t y,const void *value,uint8_t size,uint8_t cells,uint8_t pal){char s[16];const char *display=s;uint8_t len,i,copy[8],key=255;if(size>8)size=8;for(i=0;i<size;i++)copy[i]=((const uint8_t*)value)[i];if(size==8&&number_state){if(value==&number_state->target)key=0;else if(value==&number_state->score)key=1;else if(value==&number_state->last_score)key=2;else if(value==&number_state->last_chips)key=3;else if(value==&number_state->last_mult)key=4;}if(key<5){NumberCache *cache=number_cache+key;if(!(number_cache_valid&(1u<<key))||memcmp(cache->bits,copy,8)){frontend_format_number_width(copy,size,cache->text,cells*2);memcpy(cache->bits,copy,8);number_cache_valid|=1u<<key;}display=cache->text;}else frontend_format_number_width(copy,size,s,cells*2);if(display!=s)strcpy(s,display);compact_scientific(s);len=strlen(s);if(len>cells*2){char *e=s;while(*e&&*e!='e')e++;if(*e){char exponent[6];strcpy(exponent,e);*e=0;text(x,y,s,cells,pal);text(x,y+1,exponent,cells,pal);}else{text(x,y,s,cells,pal);text(x,y+1,s+cells*2,cells,pal);}}else text(x,y,s,cells,pal);}
#define number(x,y,value,cells,pal) number_value(x,y,&(value),sizeof(value),cells,pal)

static void money_value(uint8_t x,uint8_t y,int32_t value,uint8_t cells,uint8_t pal){char s[16];uint32_t mag=value<0?0u-(uint32_t)value:(uint32_t)value;uint8_t n=0;if(value<0)s[n++]='-';frontend_format_number(&mag,4,s+n);text(x,y,s,cells,pal);if(strlen(s)>cells*2)text(x,y+1,s+cells*2,cells,pal);}
static void icon(uint8_t x,uint8_t y,uint8_t slot,uint8_t id){frontend_load_icon(x,y,slot,0,id);}
static uint8_t jokers_hidden(const GameState *g){return g->boss_id==28&&g->boss_flipped!=0;}
void inventory_owned_icon(const GameState*,uint8_t,uint8_t,uint8_t,uint8_t) BANKED;

static uint8_t joker_detail_hud_start,rail_first,rail_grab;
static void joker_rail_detail(const GameState *g){uint8_t i;char label[33];
 if(g->joker_focus<g->joker_count){i=g->joker_focus;
  if(boss_joker_hidden(g,i)){text(5,9,"HIDDEN JOKER",15,0);text(5,10,"EFFECT HIDDEN",15,0);text(5,12,"VALUE HIDDEN",15,0);text(5,13,"",15,0);}
  else{frontend_shop_name(ITEM_JOKER,PG_JBYTE(g,i,jokers,0u),label);text(5,9,label,15,0);
   frontend_shop_effect(ITEM_JOKER,PG_JBYTE(g,i,jokers,0u),label);phase_wrap(5,10,label,15,2);
   if(frontend_joker_now(g,i,label))text(5,13,label,15,0);else text(5,13,"",15,0);
   if(PG_DEBUFF(g,i))text(5,8,"DEBUFFED - EFFECT OFF",15,0);
   if(PG_JBYTE(g,i,joker_eternal,10u))text(5,12,"ETERNAL - CANNOT SELL",15,0);
   else{text(5,12,"SELL $",3,0);{GbNum sell;if(joker_history_sell_value(g,i,&sell))number(8,12,sell,7,0);else text(8,12,"INVALID",7,0);}}
  }
 }
 text(5,14,g->joker_grab?"A RELEASE  L/R REORDER":"A GRAB  L/R FOCUS",15,0);
 chrome_command(FP_CHROME_JOKERS,0,1);chrome_command(FP_CHROME_JOKERS,0,2);
}
static void joker_rail(const GameState *g){uint8_t i,j,first=inventory_joker_first(g);
 chrome_command(FP_CHROME_JOKERS,0,0);chrome_focus_allocate();
 for(i=0;i+first<g->joker_count&&i<5;i++){j=i+first;inventory_owned_icon(g,5+i*3,4,i,j);if(j==g->joker_focus)chrome_focus(5+i*3,3);}
 joker_detail_hud_start=hud_next;joker_rail_detail(g);
 rail_first=first;rail_grab=g->joker_grab;
}


#ifdef PHASE_PACK

static void item_icon(uint8_t x,uint8_t y,uint8_t slot,uint8_t kind,uint8_t id){frontend_load_icon(x,y,slot,kind,id);}


static void inventory(const GameState *g){uint8_t i,j,first=inventory_consumable_first(g);char label[33];
 chrome_command(FP_CHROME_INVENTORY,0,0);phase_rule(5,3,15);chrome_focus_allocate();
 for(i=0;i+first<g->consumable_count&&i<2;i++){
  j=i+first;item_icon(6+i*7,6,i,PG_CBYTE(g,j,consumable_kind,0u),PG_CBYTE(g,j,consumables,1u));
  if(j==g->consumable_focus)chrome_focus(6+i*7,5);
 }
 if(g->consumable_focus<g->consumable_count){
  i=g->consumable_focus;if(PG_CBYTE(g,i,consumable_edition,2u)==ED_NEGATIVE)text(5,9,"NEGATIVE",15,0);frontend_shop_name(PG_CBYTE(g,i,consumable_kind,0u),PG_CBYTE(g,i,consumables,1u),label);text(5,10,label,15,0);
  frontend_shop_effect(PG_CBYTE(g,i,consumable_kind,0u),PG_CBYTE(g,i,consumables,1u),label);phase_wrap(5,11,label,15,2);
  if(frontend_consumable_now(g,PG_CBYTE(g,i,consumable_kind,0u),PG_CBYTE(g,i,consumables,1u),label))text(5,13,label,15,0);else text(5,13,"",15,0);
 }else text(5,8,"NO CONSUMABLES",15,0);
 if(game_action_error)text(5,14,game_action_error==2?"UNSUPPORTED LIMIT":"NOT APPLICABLE",15,0);
 chrome_command(FP_CHROME_INVENTORY,0,1);chrome_command(FP_CHROME_INVENTORY,0,2);
}
/* SHOP and PACK detail patterns share one phase-local tail allocation. */
static uint8_t pack_detail_hud_start,pack_focus_only;
/* the shop body lives in banked shop_counter.c. The HUD
 * cursor is lent and returned; the compat cache stays armed as before. */
uint8_t shop_counter_view(const GameState *g,uint8_t focus,uint8_t hud,uint8_t *detail) BANKED;
uint8_t shop_counter_focus(const GameState *g,uint8_t old,uint8_t focus,uint8_t base,uint8_t detail) BANKED;
static void shop_view(const GameState *g,uint8_t focus){compat_generation=chrome_generation;hud_next=shop_counter_view(g,focus,hud_next,&pack_detail_hud_start);}
static const char *const rank_labels[15]={"","","2","3","4","5","6","7","8","9","10","JACK","QUEEN","KING","ACE"};
static const char *const suit_labels[4]={"SPADES","DIAMONDS","HEARTS","CLUBS"};
static void pack_card_icon(uint8_t x,uint8_t y,const Card *card){uint8_t map[8],attrs[8],j,rank;static const uint8_t suits[4]={0,3,1,2};rank=card->rank==14?0:card->rank-1;
 owner_legacy_card_map(card->enhancement==ENH_STONE?53u:(uint8_t)(suits[card->suit]*13u+rank),map,attrs);
 VBK_REG=0;set_bkg_tiles(x,y,2,4,map);if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,2,4,attrs);VBK_REG=0;}}
static void pack_view(const GameState *g,uint8_t focus){uint8_t i,kind;const Card *card;char label[33];
 if(!pack_focus_only){
 chrome_command(FP_CHROME_PACK,chrome_family(g->pack_kind),0);
 text(5,3,"CHOICES",5,0);number(10,3,g->pack_choices,4,0);phase_rule(5,4,15);
 for(i=0;i<g->pack_count&&i<5;i++){kind=g->pack_item_kind[i];if(g->pack_kind==ITEM_PLAYING)pack_card_icon(5+i*3,7,&g->pack_cards[i]);else if(kind==ITEM_JOKER)icon(5+i*3,7,i,g->pack_ids[i]);else item_icon(5+i*3,7,i,kind,g->pack_ids[i]);}
 chrome_focus_allocate();if(focus<g->pack_count&&focus<5)chrome_focus(5+focus*3,6);
 pack_detail_hud_start=hud_next;
 }
 if(focus<g->pack_count){kind=g->pack_item_kind[focus];if(g->pack_kind==ITEM_PLAYING){card=&g->pack_cards[focus];text(5,12,rank_labels[card->rank],6,0);text(11,12,suit_labels[card->suit],8,0);{uint8_t r=13,m;for(m=0;m<3u;m++)if(frontend_modifier_effect(m,m==0u?card->enhancement:m==1u?card->edition:card->seal,label))text(5,r++,label,15,0);if(r==13)text(5,r++,"PLAIN CARD",15,0);while(r<16)text(5,r++,"",15,0);}}
 else{frontend_shop_name(kind,g->pack_ids[focus],label);text(5,12,label,15,0);frontend_shop_effect(kind,g->pack_ids[focus],label);phase_wrap(5,13,label,15,2);}}
 if(game_action_error)text(5,15,game_action_error==2?"UNSUPPORTED LIMIT":"SELECT TARGETS / NO ROOM",15,0);chrome_command(FP_CHROME_PACK,0,1);if(g->hand_count&&(g->pack_kind==ITEM_TAROT||g->pack_kind==ITEM_SPECTRAL))text(5,17,"UP TARGET CARDS",15,0);}

#endif
/* Condensed rules below follow boss.c; optional bosses need a rule
 * adapter before they get a detailed description. */
static HandPreview current_preview;
static uint8_t preview_hud_start,action_hud_start;
static void action_buttons(uint8_t selected,uint8_t discards){heading(4,16,"PLAY",7,selected?2:0);text(8,16,"START",3,selected?2:0);heading(11,16,"DISCARD",9,selected&&discards?1:0);text(18,16,"B",2,selected&&discards?1:0);}
/* Apply the same public recognition policy in the legacy TABLE fallback.
 * Core recognition and current GameState remain untouched. Fixed code keeps
 * this bounded bridge outside the crowded legacy renderer bank. */
static void conceal_joker_preview(const GameState *g,HandPreview *p) NONBANKED {
 if(p->hidden||frontend_public_preview_hidden(g)){
  p->hidden=1;strcpy(p->label,"???? LV?");
 }
}
static void preview_view(const GameState *g){char label[17];uint8_t i;frontend_public_hand_preview(g,&current_preview);conceal_joker_preview(g,&current_preview);if(!current_preview.valid){heading(5,7,"PREVIEW UNAVAILABLE",15,0);return;}if(!g->highlighted_count){heading(5,7,"SELECT CARDS",11,0);text(16,7,"",4,0);}else{for(i=0;i<16&&current_preview.label[i]&&!(current_preview.label[i]==' '&&current_preview.label[i+1]=='L'&&current_preview.label[i+2]=='V');i++)label[i]=current_preview.label[i];label[i]=0;heading(5,7,label,11,0);text(16,7,"LV",1,0);if(current_preview.hidden)text(17,7,"?",3,0);else number(17,7,current_preview.level,3,0);}text(5,8,"CHIPS",6,2);text(13,8,"MULT",6,1);if(current_preview.hidden){heading(5,9,"?",6,2);heading(13,9,"?",6,1);}else{large_number(5,9,&current_preview.chips,4,6,2);large_number(13,9,&current_preview.mult,4,6,1);}text(11,9," X",2,0);}
static void sidebar_number(uint8_t y,const void *value,uint8_t size){char s[16];uint8_t copy[8],i;for(i=0;i<size;i++)copy[i]=((const uint8_t *)value)[i];frontend_format_number_width(copy,size,s,8);if(strlen(s)<=5)heading(0,y,s,4,0);else number_value(0,y,value,size,4,0);}
static void sidebar(const GameState *g){phase_sidebar_chrome();heading(0,1,"TARGET",5,0);sidebar_number(3,&g->target,8);heading(0,6,"SCORE",4,0);sidebar_number(7,&g->score,8);text(0,10,"HANDS",3,0);number(3,10,g->hands,1,0);text(0,11,"DISC",3,0);number(3,11,g->discards,1,0);text(0,12,"MONEY",4,0);{uint8_t gp=g->phase==PHASE_SHOP?0x80u:0u; /* TABLE gold values, shop only */
 text(0,13,"$",1,gp);money_value(1,13,g->money,3,gp);text(0,15,"ANTE",3,0);number(3,15,g->ante,1,gp);text(0,16,"RND",3,0);{uint8_t round=g->blind+1;number(3,16,round,1,gp);}}}

static uint8_t cached_phase=255,cached_hands,cached_discards,cached_count,renderer_initialized;
static uint8_t cached_score[8],cached_target[8],cached_last[24],cached_ante,cached_blind,cached_last_hand,cached_boss_id,cached_boss_disabled,cached_boss_flipped,cached_joker_debuff,cached_jokers[5],cached_joker_count;
static int32_t cached_money;
static uint16_t cached_forced_uid;
/* Full redraw/phase/UI keys separate these lifetimes. Normal shop/pack never
 * reads hand keys; table and pack-target rendering never reads offer keys. */
static union {
 struct {uint8_t rank[8],suit[8],flags[8],enhancement[8];uint16_t uid[8];} hand;
 struct {uint8_t shop[10],pack[38];} phase;
} legacy_keys;
#define cached_rank legacy_keys.hand.rank
#define cached_suit legacy_keys.hand.suit
#define cached_flags legacy_keys.hand.flags
#define cached_enhancement legacy_keys.hand.enhancement
#define cached_card_uid legacy_keys.hand.uid
#define compat_shop_key legacy_keys.phase.shop
#define compat_pack_key legacy_keys.phase.pack
static uint8_t cached_focus=255,cached_selected,hand_dirty=1,cached_page=255;
static uint8_t visible_first,visible_count,visible_selected;
#ifdef PHASE_PACK
static uint8_t cached_ui_mode,cached_consumables,sidebar_hud_end;
/* Exact displayed offer keys; separate from native game/save state. */
static uint8_t compat_focus;
static void compat_remember(const GameState *g) {
 uint8_t i;const Card *c;
 if(g->ui_mode)return;
 if(g->phase==PHASE_SHOP){memcpy(compat_shop_key,g->shop,2);memcpy(compat_shop_key+2,g->shop_kind,2);memcpy(compat_shop_key+4,g->shop_pack_kind,2);memcpy(compat_shop_key+6,g->pack_size,4);}
 if(g->phase==PHASE_PACK){compat_pack_key[0]=g->pack_kind;compat_pack_key[1]=g->pack_count;compat_pack_key[2]=g->pack_choices;memcpy(compat_pack_key+3,g->pack_ids,5);memcpy(compat_pack_key+8,g->pack_item_kind,5);for(i=0;i<5;i++){c=g->pack_cards+i;compat_pack_key[13+i*5]=c->rank;compat_pack_key[14+i*5]=c->suit;compat_pack_key[15+i*5]=c->enhancement;compat_pack_key[16+i*5]=c->edition;compat_pack_key[17+i*5]=c->seal;}}
 compat_focus=g->focus;
}
/* Renderer-bank cache path owns map_rect and phase detail lifetimes.
 * UI/phase changes and key misses use the existing full redraw. */
static uint8_t compat_focus_redraw(const GameState *g,uint8_t focus) {
 uint8_t i;const Card *c;
 /* a refused action (game_action_error) needs the full view so
  * its existing message is drawn; the focus-only path used to hide it. */
 if(!compat_generation||compat_generation!=chrome_generation||g->phase!=cached_phase||g->ui_mode||cached_ui_mode||game_action_error)return 0;
 if(g->phase==PHASE_SHOP){if(memcmp(compat_shop_key,g->shop,2)||memcmp(compat_shop_key+2,g->shop_kind,2)||memcmp(compat_shop_key+4,g->shop_pack_kind,2)||memcmp(compat_shop_key+6,g->pack_size,4))return 0;
  if(_cpu==CGB_TYPE)set_bkg_palette(0,3,ui_palettes);
  hud_next=shop_counter_focus(g,compat_focus,focus,sidebar_hud_end,pack_detail_hud_start);
  return 1;
 }
 if(g->phase!=PHASE_PACK||compat_pack_key[0]!=g->pack_kind||compat_pack_key[1]!=g->pack_count||compat_pack_key[2]!=g->pack_choices||memcmp(compat_pack_key+3,g->pack_ids,5)||memcmp(compat_pack_key+8,g->pack_item_kind,5))return 0;
 for(i=0;i<5;i++){c=g->pack_cards+i;if(compat_pack_key[13+i*5]!=c->rank||compat_pack_key[14+i*5]!=c->suit||compat_pack_key[15+i*5]!=c->enhancement||compat_pack_key[16+i*5]!=c->edition||compat_pack_key[17+i*5]!=c->seal)return 0;}
 if(_cpu==CGB_TYPE)set_bkg_palette(0,3,ui_palettes);
 if(compat_focus<g->pack_count&&compat_focus<5)map_rect(5+compat_focus*3,6,2,1,PAPER_TILE,0);
 if(focus<g->pack_count&&focus<5)chrome_focus(5+focus*3,6);
 /* Footer patterns follow variable-length details, so regenerate both. */
 map_rect(5,12,15,6,0,0);map_rect(5,12,15,4,PAPER_TILE,0);
 hud_next=pack_detail_hud_start;pack_focus_only=1;pack_view(g,focus);pack_focus_only=0;
 return 1;
}
#endif
static void remember(const GameState *g){uint8_t i;cached_page=visible_first;cached_phase=g->phase;cached_hands=g->hands;cached_discards=g->discards;cached_count=g->hand_count;memcpy(cached_score,&g->score,sizeof(g->score));memcpy(cached_target,&g->target,8);memcpy(cached_last,&g->last_score,24);cached_money=g->money;cached_forced_uid=g->forced_card_uid;cached_ante=g->ante;cached_blind=g->blind;cached_last_hand=g->last_hand;cached_boss_id=g->boss_id;cached_boss_disabled=g->boss_disabled;cached_boss_flipped=g->boss_flipped;cached_joker_count=g->joker_count;cached_joker_debuff=frontend_public_jokers_key(g,cached_jokers);
#ifdef PHASE_PACK
cached_ui_mode=g->ui_mode;cached_consumables=g->consumable_count;
compat_remember(g);
#endif
for(i=0;i<visible_count;i++){cached_rank[i]=g->hand[visible_first+i].rank;cached_suit[i]=g->hand[visible_first+i].suit;cached_card_uid[i]=g->hand[visible_first+i].uid;cached_flags[i]=g->hand[visible_first+i].flags;cached_enhancement[i]=g->hand[visible_first+i].enhancement;}}
static void draw_hand(const GameState *g,uint8_t focus,uint8_t selected){uint8_t i,j,rank,map[8],attrs[8],top,animate,delta=selected^cached_selected;static const uint8_t suit_map[4]={0,3,1,2};selected=visible_selected;delta=selected^cached_selected;focus=visible_count?focus-visible_first:0;if(hand_dirty)map_rect(4,10,16,6,0,1);
  for(i=0;i<visible_count;i++){if(!hand_dirty&&i!=focus&&i!=cached_focus&&!((selected^cached_selected)&(1u<<i)))continue;animate=_cpu==CGB_TYPE&&!hand_dirty&&delta&&!(delta&(delta-1))&&(delta&(1u<<i));if(!hand_dirty&&!animate)map_rect(4+i*2,10,2,6,0,1);top=selected&(1u<<i)?11:12;if(g->hand[visible_first+i].flags&CARD_FACE_DOWN)owner_legacy_card_map(52u,map,attrs);else if(g->hand[visible_first+i].enhancement==ENH_STONE)owner_legacy_card_map(53u,map,attrs);else{if(g->hand[visible_first+i].rank<2||g->hand[visible_first+i].rank>14||g->hand[visible_first+i].suit>3)continue;rank=g->hand[visible_first+i].rank==14?0:g->hand[visible_first+i].rank-1;owner_legacy_card_map((uint8_t)(suit_map[g->hand[visible_first+i].suit]*13u+rank),map,attrs);}if(g->boss_id==12&&!g->boss_disabled&&g->forced_card_uid==g->hand[visible_first+i].uid){map[7]=LOCK_TILE;attrs[7]=1;}else if(!(g->hand[visible_first+i].flags&CARD_FACE_DOWN)&&(g->hand[visible_first+i].flags&CARD_DEBUFF)){map[7]=DEBUFF_TILE;attrs[7]=1;}if(animate)motion_begin(32+i*16,top*8,i==focus,map,attrs);else{set_bkg_tiles(4+i*2,top,2,4,map);if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(4+i*2,top,2,4,attrs);VBK_REG=0;}if(i==focus)map_rect(4+i*2,top-1,2,1,MARKER_TILE,1);}}
 if(hand_dirty&&g->hand_count>8){uint8_t page_no=visible_first/8+1,page_total=(g->hand_count+7)/8;number(10,17,page_no,1,0);text(11,17,"/",1,0);number(12,17,page_total,1,0);}

 cached_focus=focus;cached_selected=selected;hand_dirty=0;
}
/* legacy PACK press feedback. On A/B/START in the pack the
 * gold focus arrow becomes three dots ("checking", as on the TABLE) within a
 * frame; Tarot/Spectral use and pack close can take ~1s of game logic. Built
 * from the real arrow art (its background code + its ink code). The next
 * render restores the arrow first, so a refused action never leaves dots. */
static uint8_t legacy_receipt_active;
void frontend_legacy_receipt(const GameState *g) BANKED {
 uint8_t tiles[32],dots[32],bg,ink,i,c,count[4]={0,0,0,0};
 if(legacy_receipt_active||!renderer_initialized||g!=&game||g->phase!=PHASE_PACK||g->ui_mode||!compat_generation||compat_generation!=chrome_generation||!(LCDC_REG&LCDCF_ON)||shop154_staging)return;
 fp_chrome_focus_tiles(tiles);
 bg=(tiles[0]>>7&1u)|((tiles[1]>>7&1u)<<1);
 for(i=0;i<128u;i++){uint8_t row=(i&63u)>>3,col=i&7u,t=i>>6;c=((tiles[t*16u+row*2u]>>(7u-col))&1u)|(((tiles[t*16u+row*2u+1u]>>(7u-col))&1u)<<1);if(c!=bg)count[c]++;}
 ink=1;for(c=1;c<4u;c++)if(count[c]>count[ink])ink=c;if(ink==bg)ink=3;
 for(i=0;i<16u;i++){dots[i*2u]=(bg&1u)?255u:0;dots[i*2u+1u]=(bg&2u)?255u:0;}
 /* dots at x=3,7 (tile0) and x=11 (tile1 col3), rows 3..4 */
 for(i=3;i<5u;i++){
  uint8_t m0=0x10u|0x01u,m1=0x10u;
  dots[i*2u]=(dots[i*2u]&~m0)|((ink&1u)?m0:0);dots[i*2u+1u]=(dots[i*2u+1u]&~m0)|((ink&2u)?m0:0);
  dots[16u+i*2u]=(dots[16u+i*2u]&~m1)|((ink&1u)?m1:0);dots[17u+i*2u]=(dots[17u+i*2u]&~m1)|((ink&2u)?m1:0);
 }
 VBK_REG=0;set_bkg_data(chrome_focus_tile,2,dots);legacy_receipt_active=1;
}
static void legacy_receipt_restore(void){uint8_t tiles[32];if(!legacy_receipt_active)return;legacy_receipt_active=0;fp_chrome_focus_tiles(tiles);VBK_REG=0;set_bkg_data(chrome_focus_tile,2,tiles);}
void render_game_banked(const GameState *g,uint8_t focus,uint8_t selected) BANKED {uint8_t i,same;uint8_t blank[16]={0},mark[16]={0};
#ifdef PHASE_PACK
 uint8_t reuse_sidebar;
#endif
 frontend_render_busy=(frontend_render_busy&128u)|1u;motion_finish();legacy_receipt_restore();number_state=g;visible_first=visible_count=visible_selected=0;if((g->phase==PHASE_TABLE&&!g->ui_mode)||(g->phase==PHASE_PACK&&g->ui_mode==3))visible_count=hand_page(g,&visible_first,&visible_selected);selected=g->highlighted_count;capture_input();
 /* TITLE has an exclusive legacy-key lifetime. The seed and resume flag
  * reuse hand-key bytes; entering/leaving/reacquiring forces a full build. */
 if(g->phase==PHASE_TITLE&&renderer_initialized&&cached_phase==PHASE_TITLE&&cached_flags[0]==resume_available){
  if(cached_card_uid[0]!=new_run_seed){a1_title_paint(new_run_seed,resume_available,&preview_hud_start,&hud_next,0);cached_card_uid[0]=new_run_seed;}
  frontend_render_busy&=128u;return;
 }
 same=g->hands==cached_hands&&g->discards==cached_discards&&!memcmp(&g->score,cached_score,sizeof(g->score))&&g->hand_count==cached_count&&g->money==cached_money&&g->forced_card_uid==cached_forced_uid&&g->ante==cached_ante&&g->blind==cached_blind&&g->last_hand==cached_last_hand&&g->boss_id==cached_boss_id&&g->boss_disabled==cached_boss_disabled&&g->boss_flipped==cached_boss_flipped&&g->joker_count==cached_joker_count&&frontend_public_jokers_same(g,cached_jokers,cached_joker_debuff)&&!memcmp(&g->target,cached_target,8)&&!memcmp(&g->last_score,cached_last,24);
#ifdef PHASE_PACK
 /* the target-selection hand row (ui_mode3) draws from column4,
  * inside the sidebar; leaving it must repaint the sidebar (residue fix). */
 reuse_sidebar=renderer_initialized&&same&&(g->phase==PHASE_SHOP||g->phase==PHASE_PACK)&&(cached_phase==PHASE_SHOP||cached_phase==PHASE_PACK)&&cached_ui_mode!=3;
 if(reuse_sidebar&&compat_focus_redraw(g,focus))goto done;
 /* A pure, ungrabbed rail focus move retains the complete resident screen.
  * Reordering, selling, changed public keys, window shifts and failed actions
  * still rebuild. Reuse the original variable-detail tile allocation so no
  * icon, sidebar or unrelated glyph can be overwritten by a longer label. */
 if(renderer_initialized&&same&&g->phase==cached_phase&&g->ui_mode==2u&&cached_ui_mode==2u&&
    !game_action_error&&!g->joker_grab&&!rail_grab&&
    inventory_joker_first(g)==rail_first&&compat_generation&&compat_generation==chrome_generation){
  map_rect(5,3,15,1,PAPER_TILE,0);
  if(g->joker_focus>=rail_first&&g->joker_focus<rail_first+5u)chrome_focus(5+(g->joker_focus-rail_first)*3u,3);
  map_rect(5,8,15,5,PAPER_TILE,0);hud_next=joker_detail_hud_start;joker_rail_detail(g);
  goto done;
 }

#endif
 same=same&&cached_page==visible_first&&g->phase==PHASE_TABLE&&cached_phase==PHASE_TABLE;
 #ifdef PHASE_PACK
 if(g->ui_mode||cached_ui_mode!=g->ui_mode||cached_consumables!=g->consumable_count)same=0;
#endif
 if(same)for(i=0;i<visible_count;i++)if(g->hand[visible_first+i].rank!=cached_rank[i]||g->hand[visible_first+i].suit!=cached_suit[i]||g->hand[visible_first+i].uid!=cached_card_uid[i]||g->hand[visible_first+i].flags!=cached_flags[i]||g->hand[visible_first+i].enhancement!=cached_enhancement[i])same=0;
 if(same){uint8_t saved=hud_next;hud_next=preview_hud_start;preview_view(g);hud_next=action_hud_start;action_buttons(selected,g->discards);hud_next=saved;draw_hand(g,focus,selected);selection_count(g->highlighted_count);remember(g);frontend_render_busy&=128u;return;}

 compat_generation=0;if(++chrome_generation==0)chrome_generation=1;
 hand_dirty=1;
 if(!renderer_initialized){if(!shop154_staging){DISPLAY_OFF;LCDC_REG=(LCDC_REG|LCDCF_BG8000)&~LCDCF_BG9C00;}HIDE_WIN;HIDE_SPRITES;VBK_REG=0;BGP_REG=0x1B;set_bkg_data(0,1,blank);set_bkg_data(CARD_BASE,SIDEBAR_CARD_TILE_COUNT,sidebar_card_tiles);owner_legacy_cards_load();mark[0]=mark[1]=0xDB;set_bkg_data(MARKER_TILE,1,mark);set_bkg_data(BACK_BASE,8,frontend_back_tiles);{uint8_t stone[16];for(i=0;i<8;i++){stone[i*2]=i&1?0xAA:0x55;stone[i*2+1]=0xFF;}set_bkg_data(STONE_TILE,1,stone);for(i=0;i<8;i++){stone[i*2]=255;stone[i*2+1]=0;}set_bkg_data(PAPER_TILE,1,stone);for(i=0;i<8;i++){stone[i*2]=i>0&&i<7?(0x80>>i)|(1u<<i):0;stone[i*2+1]=255u^stone[i*2];}set_bkg_data(DEBUFF_TILE,1,stone);for(i=0;i<8;i++){uint8_t m=i>=1&&i<=5?(i==5?0x1C:0x10):0;stone[i*2]=m;stone[i*2+1]=255u^m;}set_bkg_data(LOCK_TILE,1,stone);}renderer_initialized=1;if(!legacy_irq_installed){add_VBL(frontend_vblank_capture);legacy_irq_installed=1;}}
 if(memcmp(&g->last_score,last_score_seen,sizeof(g->last_score))){memcpy(last_score_seen,&g->last_score,sizeof(g->last_score));score_flash=16;}last_phase=g->phase;
 if(_cpu==CGB_TYPE)set_bkg_palette(0,3,ui_palettes);
#ifdef PHASE_PACK
 if(reuse_sidebar)map_rect(5,0,15,18,0,0);else
#endif
 map_rect(0,0,20,18,0,0);if(g->phase!=PHASE_TITLE)map_rect(5,0,15,16,PAPER_TILE,0);hud_next=HUD_BASE;
 #ifdef PHASE_BLOCKED
 if(g->phase==PHASE_BLOCKED){text(3,5,"NUMBER LIMIT",14,0);text(3,8,"EXACT BACKEND NEEDED",14,0);text(3,11,"RUN NOT SCORED",14,0);text(3,15,"START TITLE",14,2);goto done;}
#endif
 if(g->phase==PHASE_TITLE){a1_title_paint(new_run_seed,resume_available,&preview_hud_start,&hud_next,1);goto done;}
#ifdef PHASE_PACK
 if(reuse_sidebar)hud_next=sidebar_hud_end;else{if(!shop157_sidebar(g,hud_next,&hud_next))sidebar(g);sidebar_hud_end=hud_next;}
#else
 sidebar(g);
#endif
#ifdef PHASE_PACK
 if((g->phase==PHASE_TABLE||g->phase==PHASE_SHOP)&&g->ui_mode==2){joker_rail(g);goto done;}
 if((g->phase==PHASE_TABLE||g->phase==PHASE_SHOP)&&g->ui_mode==1){inventory(g);goto done;}
 if(g->phase==PHASE_SHOP){shop_view(g,focus);if(game_action_error==2)text(5,15,"UNSUPPORTED LIMIT",15,0);goto done;}
 if(g->phase==PHASE_PACK){if(g->ui_mode==3){text(5,1,"SELECT TARGETS",15,0);draw_hand(g,focus,selected);selection_count(g->highlighted_count);text(4,16,"A SELECT SEL SORT",16,2);text(6,17,"DOWN/START BACK",14,0);}else pack_view(g,focus);goto done;}
#endif
 if(g->phase!=PHASE_SHOP)for(i=0;i<g->joker_count&&i<5;i++)inventory_owned_icon(g,5+i*3,1,i,i+inventory_joker_first(g));
 if(g->phase==PHASE_TABLE){
 if(!g->joker_count)for(i=0;i<5;i++)map_rect(5+i*3,1,2,3,0,0);
 heading(5,5,boss_name(g->blind==2?g->boss_id:g->blind),15,0);
 if(g->blind==2){if(g->boss_id==2&&!g->boss_disabled&&g->ox_target_hand<12){text(5,6,"LOSE ALL $:",6,0);text(11,6,hand_names[g->ox_target_hand],9,0);}else text(5,6,g->boss_disabled?"BOSS DISABLED":boss_rule(g->boss_id),15,0);}else if(g->consumable_count)text(5,6,"UP ITEMS",15,0);else if(g->joker_count)text(5,6,"DOWN JOKERS",15,0);
 preview_hud_start=hud_next;preview_view(g);hud_next=preview_hud_start+44;draw_hand(g,focus,selected);
 action_hud_start=hud_next;action_buttons(selected,g->discards);selection_count(g->highlighted_count);text(6,17,"A PICK",4,0);text(13,17,"SEL SORT",7,0);
 }else if(g->phase==PHASE_SHOP){text(5,5,"SHOP",15,0);for(i=0;i<2;i++){if(g->shop[i]!=255){icon(6+i*7,7,i,g->shop[i]);{char label[33];frontend_shop_name(ITEM_JOKER,g->shop[i],label);text(5+i*7,11,label,7,0);}text(5+i*7,12,"$4",7,0);}else text(5+i*7,11,"SOLD",7,0);if(focus==i)map_rect(6+i*7,6,2,1,MARKER_TILE,0);}text(4,14,"A BUY  B SELL LAST",16,0);text(4,16,"START NEXT BLIND",16,2);
 }else if(g->phase==PHASE_WIN){phase_rule(5,5,15);phase_rule(5,12,15);chrome_command(FP_CHROME_BLIND_WIN,0,0);chrome_command(FP_CHROME_BLIND_WIN,0,1);number(6,9,g->score,12,0);chrome_command(FP_CHROME_BLIND_WIN,0,2);
 }else if(g->phase==PHASE_COMPLETE){phase_rule(5,5,15);phase_rule(5,12,15);chrome_command(FP_CHROME_ANTE8_WIN,0,0);chrome_command(FP_CHROME_ANTE8_WIN,0,1);chrome_command(FP_CHROME_ANTE8_WIN,0,2);
 }else{phase_rule(5,5,15);phase_rule(5,12,15);chrome_command(FP_CHROME_RUN_LOSS,0,0);chrome_command(FP_CHROME_RUN_LOSS,0,1);chrome_command(FP_CHROME_RUN_LOSS,0,2);}
 /* Original legacy WIN/COMPLETE/LOSS lines, kept as the fallback for
  * ineligible results; results with owned Jokers use the native ledger. */
 done:VBK_REG=0;shop154_commit();SHOW_BKG;DISPLAY_ON;remember(g);if(g->phase==PHASE_TITLE){cached_card_uid[0]=new_run_seed;cached_flags[0]=resume_available;}frontend_render_busy&=128u;
}

void raster_ferry_window(void) NONBANKED;
void raster_capture_inputs(void) NONBANKED {raster_ferry_window();CRITICAL {capture_input_raw();}}
void raster_legacy_reset(void) BANKED {motion_finish();renderer_initialized=0;cached_phase=255;last_phase=255;compat_generation=0;}

/* Native Felt UI leases bank0. A complete legacy rebuild is mandatory after
 * that lease; no prior cached HUD/map/pattern residency remains valid. */
void frontend_legacy_invalidate(void) BANKED {renderer_initialized=0;cached_phase=255u;compat_generation=0;}

/* Retire legacy motion before the native bank0 pattern lease begins. */
void frontend_native_acquire(void) BANKED {motion_finish();}

/* Modal publication gateways execute from HOME with the renderer ROM bank
 * explicitly selected. Original static helpers keep their original ABI. */
BANKREF(render_game_banked)
BANKREF_EXTERN(render_game_banked)
/* crash fix: inside render.c,
 * set_bkg_data is shop156_render_data, a static in this banked unit, and the
 * tile sources live here too. This NONBANKED entry is called from the modal
 * bank, so without mapping the renderer bank the first call jumped into an
 * unrelated bank (TABLE item modal: tile garbage, then white screen). Map the
 * renderer bank exactly as frontend_modal_body does. */
void frontend_modal_static(void) NONBANKED {
 uint8_t i,blank[16]={0},mark[16]={0},previous_bank;
 if(renderer_initialized)return;
 previous_bank=CURRENT_BANK;SWITCH_ROM(BANK(render_game_banked));
 LCDC_REG|=LCDCF_BG8000;HIDE_SPRITES;VBK_REG=0;BGP_REG=0x1B;set_bkg_data(0,1,blank);set_bkg_data(CARD_BASE,SIDEBAR_CARD_TILE_COUNT,sidebar_card_tiles);mark[0]=mark[1]=0xDB;set_bkg_data(MARKER_TILE,1,mark);set_bkg_data(BACK_BASE,8,frontend_back_tiles);{uint8_t stone[16];for(i=0;i<8;i++){stone[i*2]=i&1?0xAA:0x55;stone[i*2+1]=0xFF;}set_bkg_data(STONE_TILE,1,stone);for(i=0;i<8;i++){stone[i*2]=255;stone[i*2+1]=0;}set_bkg_data(PAPER_TILE,1,stone);for(i=0;i<8;i++){stone[i*2]=i>0&&i<7?(0x80>>i)|(1u<<i):0;stone[i*2+1]=255u^stone[i*2];}set_bkg_data(DEBUFF_TILE,1,stone);for(i=0;i<8;i++){uint8_t m=i>=1&&i<=5?(i==5?0x1C:0x10):0;stone[i*2]=m;stone[i*2+1]=255u^m;}set_bkg_data(LOCK_TILE,1,stone);}renderer_initialized=1;if(!legacy_irq_installed){add_VBL(frontend_vblank_capture);legacy_irq_installed=1;}
 SWITCH_ROM(previous_bank);
}
void frontend_modal_body(const GameState *g) NONBANKED {
 uint8_t previous_bank=CURRENT_BANK;
 SWITCH_ROM(BANK(render_game_banked));
 frontend_render_busy=(frontend_render_busy&128u)|1u;motion_finish();number_state=g;
 visible_first=visible_count=visible_selected=0;compat_generation=0;
 if(++chrome_generation==0)chrome_generation=1;hand_dirty=1;
 if(memcmp(&g->last_score,last_score_seen,sizeof(g->last_score))){memcpy(last_score_seen,&g->last_score,sizeof(g->last_score));score_flash=16;}
 last_phase=g->phase;if(_cpu==CGB_TYPE)set_bkg_palette(0,3,ui_palettes);
 hud_next=HUD_BASE;sidebar(g);sidebar_hud_end=hud_next;inventory(g);
 VBK_REG=0;SHOW_BKG;remember(g);frontend_render_busy&=128u;
 SWITCH_ROM(previous_bank);
}

#undef set_bkg_palette
extern palette_color_t shop154_pal[12];

#undef set_bkg_tiles

/* RAM glyphs lend their source across the banked writer call. ROM data keeps
 * the fixed unsigned SDK gateway so caller-bank ownership is never lost. */
static void shop156_render_data(uint8_t first,uint8_t count,const uint8_t *data){if(!shop154_staging||!count||count>63u||(uint16_t)data<0x8000u){shop154_data(first,count,data);return;}shop156_upload(first,count,data);}
