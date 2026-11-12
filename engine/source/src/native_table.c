#include "inventory_bounds.h"
#pragma bank 255
#include "native_table.h"
#include "native_busy.h"
#include "table_1998_data.h"
#include "raster_runtime.h"
#include "hand_preview.h"
#include "hand_api.h"
#include "frontend_public_preview.h"
#include "frontend_numeric.h"
#include "frontend_heading.h"
#include "boss_labels.h"
#include "boss.h"
#include <gb/cgb.h>
#include <string.h>
#include "a1_assets.h"
#include "a1_ui_runtime.h"
#include "a1_families.inc"

void native_plain_detail_full(uint8_t,uint8_t,uint8_t,uint8_t,const A1Text *,uint8_t,uint8_t) BANKED;
uint8_t native_table_live;

uint8_t native_scene_prepare(const GameState *) BANKED;
void frontend_legacy_receipt(const GameState *) BANKED;


/* BSS starts invalid. Only install establishes validity; render entry lends
 * it to the bank0-only consumable renderer. Maps/icons/UI are never retained. */
uint8_t native_table_bank1_owner;
void frontend_legacy_invalidate(void) BANKED;
void frontend_native_acquire(void) BANKED;
void raster_capture_inputs(void) NONBANKED;
void native_table_joker_copy_0(uint8_t,uint8_t *) BANKED;
void native_table_joker_copy_1(uint8_t,uint8_t *) BANKED;
uint8_t felt_item_copy(uint8_t,uint8_t,uint8_t *) BANKED;
#define FONT_DECL(n) void felt_font_copy_##n(uint16_t,uint8_t *) BANKED;
FONT_DECL(0) FONT_DECL(1) FONT_DECL(2) FONT_DECL(3)
FONT_DECL(4) FONT_DECL(5) FONT_DECL(6) FONT_DECL(7)
#define MAP_DEST 0x9800u
#define CURSOR_TILE 254u
#define RECEIPT_TILE 162u
#define BLANK_TILE 134u
#define ICON_BASE 192u
/* Entire cache overlays the existing144-byte exclusive raster lease.
 * Keys and summary contain only public fields, never identity/value of a back.
 * The window is derived from absolute focus, independent of core hand_page. */
#include "native_inventory_cache.h"
#include "cooperative_native_redirect.h"
static void region(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const uint8_t *ids,const uint8_t *attrs){
 VBK_REG=0;set_tiles(x,y,w,h,(uint8_t *)MAP_DEST,ids);
 VBK_REG=1;set_tiles(x,y,w,h,(uint8_t *)MAP_DEST,attrs);VBK_REG=0;raster_capture_inputs();
}
uint8_t frontend_modifier_fields(A1Text *f,uint8_t n,char lines[][32],const uint8_t *key) BANKED;
static void field(A1Text *f,const char *s,uint8_t x,uint8_t y,uint8_t font,uint8_t color){f->s=s;f->x=x;f->y=y;f->font=font;f->color=color;}
static void center_field(A1Text *f,const char *s,uint8_t x,uint8_t y,uint8_t width,uint8_t height,uint8_t font,uint8_t color){
 field(f,s,x+width/2u-a1_text_width(s,font)/2u,y+(height-(font==5u?7u:5u))/2u,font,color);
}
static uint8_t selected(const GameState *g,uint8_t index){
 uint8_t j;uint16_t delta;
 for(j=0;j<g->highlighted_count&&j<5u;j++){delta=(uint16_t)(g->selected_uids[j]^g->hand[index].uid);if(!delta)return 1;}
 return 0;
}
static void key(const GameState *g,uint8_t index,uint8_t out[4]){
 const Card *c;uint16_t delta;
 memset(out,0,4);if(index>=g->hand_count)return;c=g->hand+index;
 out[0]=c->flags&3u;if(c->flags&CARD_FACE_DOWN)return;
 if(c->rank>=2u&&c->rank<=14u&&c->suit<=3u){out[0]|=c->rank<<2;out[1]=c->suit;}
 out[1]|=(c->enhancement&15u)<<2;out[2]=(c->edition&15u)|((c->seal&15u)<<4);
 delta=(uint16_t)(g->forced_card_uid^c->uid);out[3]=g->boss_id==12u&&!g->boss_disabled&&!delta;
}
/* Same public key bytes, used only by the existing unchanged-window selection path. */
static void selection_key(const GameState *g,uint8_t index,uint8_t out[4]){
 const Card *c;uint16_t delta;
 out[1]=out[2]=out[3]=0;if(index>=g->hand_count){out[0]=0;return;}c=g->hand+index;
 out[0]=c->flags&3u;if(c->flags&CARD_FACE_DOWN)return;
 if(c->rank>=2u&&c->rank<=14u&&c->suit<=3u){out[0]|=c->rank<<2;out[1]=c->suit;}
 out[1]|=(c->enhancement&15u)<<2;out[2]=(c->edition&15u)|((c->seal&15u)<<4);
 delta=(uint16_t)(g->forced_card_uid^c->uid);out[3]=g->boss_id==12u&&!g->boss_disabled&&!delta;
}
static uint8_t summary(const GameState *g,uint8_t force){
 uint8_t i,j,k[4],changed=force,found;uint16_t delta;
 for(j=0;j<5u;j++){
  found=255u;memset(k,0,4);
  if(j<g->highlighted_count)for(i=0;i<g->hand_count;i++){
   delta=(uint16_t)(g->selected_uids[j]^g->hand[i].uid);
   if(!delta){found=i;key(g,i,k);break;}
  }
  if(memcmp(k,cache->summary[j],4)||found!=cache->summary_index[j])changed=1;
  memcpy(cache->summary[j],k,4);cache->summary_index[j]=found;
 }
 return changed;
}
static uint8_t geometry(const GameState *g,uint8_t *first,uint16_t *mask,uint16_t *expanded,uint8_t x[13]){
 uint8_t n,i,p,extra=0u,start,width=15u;
 n=g->hand_count;*first=0;*mask=0;*expanded=0;if(!n)return 0;
 if(n>13u){n=11u;width=13u;*first=g->focus>5u?g->focus-5u:0u;if(*first>g->hand_count-11u)*first=g->hand_count-11u;}
 for(i=0;i<n;i++)if(selected(g,*first+i))*mask|=1u<<i;
 p=g->focus-*first;
 if(g->hand_count<=13u&&p<n-1u){extra=15u-(n+2u);if(extra>2u)extra=2u;}
 start=(g->hand_count>13u?6u:5u)+(width-(n+2u+extra))/2u;
 for(i=0;i<n;i++){x[i]=start;start+=1u+(i==p?extra:0u);}
 *expanded=extra?((uint16_t)extra<<13)|(1u<<p):0u;return n;
}
/* Cursor position last staged by cursor(); y0 = no cursor. A cooperative
 * render can cancel after staging, so the receipt trusts this home only
 * when the visible sprite0 sits exactly on it or on one of its own leans. */
static uint8_t cursor_home_x,cursor_home_y;
static void cursor(uint8_t count,uint8_t pos,uint16_t mask,const uint8_t x[13]){
 uint8_t visible;
 if(!count){move_sprite(0,0,0);cursor_home_y=0;return;}
 visible=pos==count-1u?24u:(x[pos+1u]-x[pos])*8u;
 set_sprite_tile(0,CURSOR_TILE);set_sprite_prop(0,8u);
 cursor_home_x=x[pos]*8u+visible/2u+4u;cursor_home_y=(mask&(1u<<pos))?96u:104u;
 move_sprite(0,cursor_home_x,cursor_home_y);
}
static uint8_t face(const uint8_t k[4]){
 uint8_t rank,suit;static const uint8_t suits[4]={0,2,3,1};
 if(k[0]&CARD_FACE_DOWN)return 52u;if((k[1]>>2)==ENH_STONE)return 53u;
 if(!(k[0]>>2))return 54u;rank=(k[0]>>2)==14u?0u:(k[0]>>2)-1u;suit=suits[k[1]&3u];return suit*13u+rank;
}
void native_D_hand_row(uint8_t,uint8_t,uint16_t,const uint8_t *,const uint8_t (*)[4],const uint8_t *,const uint8_t *) BANKED;
uint8_t native_D_hand_finish(uint8_t,uint8_t,uint8_t) BANKED;
void native_D_sidebar(void) BANKED;
uint8_t coop_plain_carriers_hidden(void) BANKED;
uint8_t coop_focus_only(uint8_t) BANKED;
static void hand(uint8_t first,uint8_t count,uint16_t mask,const uint8_t x[13]){
 uint8_t i,j,row,col,y,prev,ink,cell,f,off_left=0,off_right=0,active=0;
 for(i=0;i<count;i++)if(!(cache->cards[i][0]&CARD_FACE_DOWN)&&(cache->cards[i][2]&15u))active=1;
 for(i=0;i<5u;i++)if(cache->summary_index[i]!=255u){if(cache->summary_index[i]<first)off_left=1;if(cache->summary_index[i]>=first+count)off_right=1;}
 /* One row IDs+attrs30B, recipe24B and corner2B share existing128B scratch.
  * Rebuild all five lifetime rows, including stale selected rows. */
 for(row=0;row<5u;row++){
  a1_ui_map_row_copy(11u+row,raster_pixels+32u);
  for(i=0;i<count;i++){
   y=mask&(1u<<i)?0u:1u;if(row<y||row>=y+4u)continue;
   f=face(cache->cards[i]);a1_card_recipe_row_copy(((uint16_t)f*4u+row-y)*6u,raster_pixels+80u);
   for(col=0;col<3u;col++){
    cell=col;ink=0;
    if(!col&&((row-y)==0u||(row-y)==3u)){
     for(j=0;j<i;j++)if(x[i]-x[j]<3u){prev=mask&(1u<<j)?0u:1u;if((row-y)==0u?prev<=y:prev>=y)ink=1;}
    }
    if(ink){a1_card_corner_copy(f,(row-y)==0u?0u:2u,raster_pixels+104u);raster_pixels[32u+x[i]+col]=raster_pixels[104u];raster_pixels[52u+x[i]+col]=raster_pixels[105u];}
    else{raster_pixels[32u+x[i]+col]=raster_pixels[80u+cell*2u];raster_pixels[52u+x[i]+col]=raster_pixels[81u+cell*2u];}
   }
  }
  if(cache->resources[28]>13u){
   a1_edge_recipe_copy(0u,first,off_left,raster_pixels+104u);raster_pixels[37u]=raster_pixels[106u+row*2u];raster_pixels[57u]=raster_pixels[107u+row*2u];
   a1_edge_recipe_copy(1u,cache->resources[28]-first-count,off_right,raster_pixels+104u);raster_pixels[51u]=raster_pixels[106u+row*2u];raster_pixels[71u]=raster_pixels[107u+row*2u];
  }
  if(cache->resources[28]>13u){raster_pixels[57u]=(raster_pixels[57u]&248u)|(row?2u:1u);raster_pixels[71u]=(raster_pixels[71u]&248u)|(row?2u:1u);}
  /* Ownership rows are consumed only by the active palette compositor.
   * Its inactive branch never reads them, including restoring a prior active
   * hand. Avoid three bank ferries per row for ordinary resident card faces. */
  if(active)native_D_hand_row(row,count,mask,x,cache->cards,raster_pixels+37u,raster_pixels+57u);
  region(5u,11u+row,15u,1u,raster_pixels+37u,raster_pixels+57u);if(coop_active&&!coop_checkpoint())return;
 }
 /* Off-view stars lie in row10 above the hand; explicitly clear on exit. */
 a1_ui_map_row_copy(10u,raster_pixels+32u);
 if(cache->resources[28]>13u){
  a1_edge_recipe_copy(0u,first,off_left,raster_pixels+104u);raster_pixels[37u]=raster_pixels[104u];raster_pixels[57u]=raster_pixels[105u];
  a1_edge_recipe_copy(1u,cache->resources[28]-first-count,off_right,raster_pixels+104u);raster_pixels[51u]=raster_pixels[104u];raster_pixels[71u]=raster_pixels[105u];
 }
 if(cache->resources[28]>13u){raster_pixels[57u]=(raster_pixels[57u]&248u)|7u;raster_pixels[71u]=(raster_pixels[71u]&248u)|7u;}
 region(5u,10u,15u,1u,raster_pixels+37u,raster_pixels+57u);
}
static uint8_t resources(const GameState *g,uint8_t out[32]){
 memcpy(out,&g->score,8);memcpy(out+8,&g->target,8);memcpy(out+16,&g->money,4);
 out[20]=g->hands;out[21]=g->discards;out[22]=g->ante;out[23]=g->blind;
 out[24]=g->boss_id;out[25]=g->boss_disabled;out[26]=g->joker_count;
 out[27]=g->consumable_count;out[28]=g->hand_count;out[29]=g->deck_pos;out[30]=g->deck_count;out[31]=g->numeric_error;
 return memcmp(out,cache->resources,32)!=0||cache->joker_rule!=inventory_joker_limit(g)||cache->consumable_rule!=inventory_consumable_limit(g);
}
/* Public resource dependencies, read before the old32-byte key is replaced.
 * No hidden card data, new cache storage or validity is introduced. */
#define SB_HEADER 1u
#define SB_SCORE 2u
#define SB_HANDS 4u
#define SB_MONEY 8u
#define SB_ROUND 16u
#define SB_RAIL 32u
#define SB_FULL 128u
static uint8_t sidebar_changes(const GameState *g,const uint8_t res[32]){
 uint8_t dirty=0;
 if(g!=&game||native_table_bank1_owner!=NATIVE_BANK1_VALID||raster_live||raster_fault||
    !(LCDC_REG&LCDCF_ON)||!(LCDC_REG&LCDCF_BG8000)||
    (LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))||SCX_REG||SCY_REG||
    res[23]!=cache->resources[23]||res[24]!=cache->resources[24]||
    res[25]!=cache->resources[25]||res[31]!=cache->resources[31])return SB_FULL;
 if(memcmp(res,cache->resources,8))dirty|=SB_SCORE;
 if(memcmp(res+8,cache->resources+8,8))dirty|=SB_HEADER;
 if(memcmp(res+16,cache->resources+16,4))dirty|=SB_MONEY;
 if(res[20]!=cache->resources[20]||res[21]!=cache->resources[21])dirty|=SB_HANDS;
 if(res[22]!=cache->resources[22])dirty|=SB_ROUND;
 if(res[26]!=cache->resources[26]||res[27]!=cache->resources[27]||cache->joker_rule!=inventory_joker_limit(g)||cache->consumable_rule!=inventory_consumable_limit(g))dirty|=SB_RAIL;
 return dirty;
}
static void sidebar_publish(const GameState *g,uint8_t dirty,const A1Text *f,uint8_t n){
 if(dirty&SB_FULL){a1_ui_publish(0,0,5,16,f,n,g->blind==2u?2u:0u);a1_ui_publish(5,0,15,1,f,n,0);}
 else{
  /* Whole regions restore their ROM base and every original overlay in order.
   * Preview is still refreshed under the original sanitized-preview rules. */
  if(dirty&SB_HEADER)a1_ui_publish(0,0,5,3,f,n,g->blind==2u?2u:0u);
  if(dirty&SB_SCORE)a1_ui_publish(0,4,5,1,f,n,0);
  a1_ui_publish(0,5,5,4,f,n,0);
  if(dirty&SB_HANDS)a1_ui_publish(0,10,5,2,f,n,0);
  if(dirty&SB_MONEY)a1_ui_publish(0,12,5,2,f,n,0);
  if(dirty&SB_ROUND)a1_ui_publish(0,15,5,1,f,n,0);
  if(dirty&SB_RAIL)a1_ui_publish(5,0,15,1,f,n,0);
 }
}
static void sidebar(const GameState *g,uint8_t dirty,const HandPreview *hp){
 A1Text f[17];char v[14][16],blind[2][16];uint8_t n=0,font,round=(g->ante-1u)*3u+g->blind+1u;uint32_t magnitude;
 static const char *const names[12]={"HIGH CARD","PAIR","TWO PAIR","3 OF KIND","STRAIGHT","FLUSH","FULL HSE","4 OF KIND","STR FLUSH","5 OF KIND","FL HOUSE","FL FIVE"};
 if(dirty){
 if(g->blind==2u){char *space;strcpy(blind[0],boss_name(g->boss_id));space=blind[0];while(*space&&*space!=' ')space++;if(*space==' '){*space=0;strcpy(blind[1],space+1);}else strcpy(blind[1],"BLIND");}
 else{strcpy(blind[0],g->blind==0u?"SMALL":"BIG");strcpy(blind[1],"BLIND");}
 field(f+n++,blind[0],g->blind==2u?3u:14u,3,3,2);field(f+n++,blind[1],g->blind==2u?3u:14u,9,3,2);
 if(coop_active&&!coop_checkpoint())return;
 font=a1_number_text(&g->target,8,v[0],35,1);field(f+n++,v[0],3,font==5u?14u:15u,font,3);
 if(coop_active&&!coop_checkpoint())return;
 font=a1_number_text(&g->score,8,v[1],35,1);field(f+n++,v[1],3,font==5u?32u:33u,font,2);
 }
 strcpy(v[12],!hp->valid?"UNAVAIL":hp->hidden?"????":hp->hand_type<12u?names[hp->hand_type]:"UNKNOWN");field(f+n++,v[12],2,42,3,2);
 if(coop_active&&!coop_checkpoint())return;
 if(!hp->valid||hp->hidden)strcpy(v[2],"?");else a1_number_text(&hp->level,2,v[2],19,0);field(f+n++,v[2],18,49,3,3);
 if(!hp->valid||hp->hidden){strcpy(v[3],"?");strcpy(v[4],"?");center_field(f+n++,v[3],1,57,17,14,3,3);center_field(f+n++,v[4],23,57,16,14,3,3);}
 if(coop_active&&!coop_checkpoint())return;
 else{font=a1_number_text(&hp->chips,4,v[3],13,1);center_field(f+n++,v[3],1,57,17,14,font,3);font=a1_number_text(&hp->mult,4,v[4],12,1);center_field(f+n++,v[4],23,57,16,14,font,3);}
 if(dirty){
 if(coop_active&&!coop_checkpoint())return;
 font=a1_number_text(&g->hands,1,v[5],13,1);center_field(f+n++,v[5],1,81,17,12,font,3);
 if(coop_active&&!coop_checkpoint())return;
 font=a1_number_text(&g->discards,1,v[6],12,1);center_field(f+n++,v[6],23,81,16,12,font,3);
 magnitude=g->money<0?0u-(uint32_t)g->money:(uint32_t)g->money;
 if(coop_active&&!coop_checkpoint())return;
 v[7][0]=0;if(g->money<0)strcpy(v[7],"-");strcat(v[7],"$");font=a1_number_text(&magnitude,4,v[13],g->money<0?26u:30u,1);strcat(v[7],v[13]);font=a1_text_width(v[7],5)<=34u?5u:3u;field(f+n++,v[7],4,font==5u?101u:102u,font,3);
 if(coop_active&&!coop_checkpoint())return;
 font=a1_number_text(&g->ante,1,v[8],18,1);field(f+n++,v[8],1,120,font,3);if(g->ante>=1u&&g->ante<=8u){strcpy(v[13],"/8");field(f+n++,v[13],1+a1_text_width(v[8],font)+1u,122,3,1);}
 if(coop_active&&!coop_checkpoint())return;
 font=a1_number_text(&round,1,v[9],19,1);field(f+n++,v[9],20,120,font,3);
 if(coop_active&&!coop_checkpoint())return;
 inventory_caption(g,0,v[10]);field(f+n++,v[10],75,1,3,3);
 if(coop_active&&!coop_checkpoint())return;
 inventory_caption(g,1,v[11]);field(f+n++,v[11],strlen(v[11])>5u?159u-a1_text_width(v[11],3u):141u,1,3,3);
 }
 if(dirty)sidebar_publish(g,dirty,f,n);
 else a1_ui_publish(0,5,5,4,f,n,0);
}

uint8_t inventory_icons(const GameState*,uint8_t) BANKED;

static void token(const uint8_t k[4],char out[5]){
 uint8_t rank=k[0]>>2;static const char suits[4]={'S','D','H','C'};out[0]=out[1]=out[2]=out[3]=out[4]=0;
 if(k[0]&CARD_FACE_DOWN){strcpy(out,"??");return;}
 if(k[1]>>2==ENH_STONE){strcpy(out,"ST");return;}
 if(rank==14u)out[0]='A';else if(rank==13u)out[0]='K';else if(rank==12u)out[0]='Q';else if(rank==11u)out[0]='J';
 else if(rank==10u){out[0]='1';out[1]='0';}else out[0]=rank>=2u?'0'+rank:'?';
 out[strlen(out)]=suits[k[1]&3u];
}
/* Default focus caption matches A1; only real public modifiers, off-view
 * selections or number overflow activate additional quiet detail lines. */
void native_focus_caption(const uint8_t *,uint8_t,uint8_t) BANKED;
static uint8_t caption_reusable(uint8_t pos,uint8_t dirty){
 return dirty==2u&&!(cache->detail_plain&2u)&&
  (!cache->detail_plain||!cache->count||!(cache->cards[pos][0]&CARD_FACE_DOWN));
}
static void details(const GameState *g,uint8_t pos,uint8_t dirty,const HandPreview *hp){
 A1Text f[9];char lines[6][32],deck[16],a[16],b[16],t[5],money_full[16];uint32_t magnitude;uint8_t i,n=0,off=0,plain=0,money_overflow=0,remaining=g->deck_count-g->deck_pos;const uint8_t *k=cache->cards[pos];
 static const char *const rank[15]={"?","?","TWO","THREE","FOUR","FIVE","SIX","SEVEN","EIGHT","NINE","TEN","JACK","QUEEN","KING","ACE"};
 static const char *const suit[4]={"SPADES","DIAMONDS","HEARTS","CLUBS"};
 static const char *const enh[9]={"","BONUS","MULT","WILD","GLASS","STEEL","STONE","GOLD","LUCKY"};
 static const char *const ed[5]={"","FOIL","HOLO","POLY","NEG"};static const char *const seal[5]={"","GOLD","RED","BLUE","PURPLE"};
 /* A cache bit records a transformed (preview/kept-value) caption. Focus changes
  * with identical effects/bonuses may update only its real caption. */

 /* The caller handles reusable captions; only general details
  * require clearing every line before conditional construction. */
 memset(lines,0,sizeof(lines));
 for(i=0;i<g->highlighted_count&&i<5u;i++)if(cache->summary_index[i]<cache->first||cache->summary_index[i]>=cache->first+cache->count)off++;
 magnitude=g->money<0?0u-(uint32_t)g->money:(uint32_t)g->money;money_full[0]=0;if(g->money<0)strcpy(money_full,"-");strcat(money_full,"$");frontend_format_number(&magnitude,4,a);strcat(money_full,a);money_overflow=a1_text_width(money_full,3)>34u;
 if(off){if(!money_overflow)strcpy(lines[0],"SEL ");for(i=0;i<g->highlighted_count&&i<5u;i++){token(cache->summary[i],t);strcat(lines[0],t);if(!money_overflow&&i+1u<g->highlighted_count)strcat(lines[0]," ");}}
 if(money_overflow){if(off)strcat(lines[0]," ");else strcpy(lines[0],"MONEY ");strcat(lines[0],money_full);}
 if(off||money_overflow)field(f+n++,lines[0],42,34,3,3);
 if(dirty&17u)a1_ui_publish(5,4,15,1,f,n,0);n=0;
 frontend_format_number(&remaining,1,deck);strcat(deck,"/");frontend_format_number(&g->deck_count,1,a);strcat(deck,a);field(f+n++,deck,140-a1_text_width(deck,3)/2u,57,3,23);
 if(!cache->count)strcpy(lines[5],"NO CARDS");
 else if(k[0]&CARD_FACE_DOWN){strcpy(lines[1],"FACE DOWN");field(f+n++,lines[1],84-a1_text_width(lines[1],3)/2u,66,3,3);strcpy(lines[5],"FLIPS WHEN PLAYED");}
 else{
  if(k[1]>>2==ENH_STONE)strcpy(lines[5],"STONE CARD");else{strcpy(lines[5],rank[k[0]>>2]);strcat(lines[5]," OF ");strcat(lines[5],suit[k[1]&3u]);}
  if((k[1]>>2)||k[2]||k[3]||(k[0]&CARD_DEBUFF)){
   plain=1;if(k[1]>>2)strcat(lines[1],k[1]>>2<9u?enh[k[1]>>2]:"ENH?");if(k[2]&15u){strcat(lines[1],"/");strcat(lines[1],(k[2]&15u)<5u?ed[k[2]&15u]:"ED?");}if(k[2]>>4){strcat(lines[1],"/");strcat(lines[1],(k[2]>>4)<5u?seal[k[2]>>4]:"SEAL?");}if(k[3])strcat(lines[1]," BELL");if(k[0]&CARD_DEBUFF)strcat(lines[1]," DEBUFF");field(f+n++,lines[1],42,42,3,2);
  }
  /* Permanent bonus is public only after face-down sanitization. */
  for(i=0;i<8u;i++)if(cache->focus_bonus[i])break;
  if(i<8u){plain=1;dirty|=128u;strcpy(lines[2],"KEPT +");frontend_format_number(cache->focus_bonus,8,a);strcat(lines[2],a);field(f+n++,lines[2],42,50,3,2);}
 }
 /* Fuller reference-adapter outputs accompany compact sidebar numbers.
  * Suppressed preview remains suppressed, including hidden selections. */
 frontend_format_number(&g->target,8,a);frontend_format_number(&g->score,8,b);
 if(a1_text_width(a,3)>35u){plain=1;strcpy(lines[3],"TGT ");strcat(lines[3],a);field(f+n++,lines[3],42,58,3,2);}
 if(a1_text_width(b,3)>35u){plain=1;strcpy(lines[4],"SC ");strcat(lines[4],b);field(f+n++,lines[4],42,66,3,2);}
 if(hp->valid&&!hp->hidden){frontend_format_number(&hp->chips,4,a);frontend_format_number(&hp->mult,4,b);if(a1_text_width(a,3)>16u||a1_text_width(b,3)>15u){plain=1;dirty|=128u;strcpy(lines[5],"CH ");strcat(lines[5],a);strcat(lines[5]," MU ");strcat(lines[5],b);}}
 if(plain&&cache->count&&(k[0]&CARD_FACE_DOWN)){
  strcpy(lines[1],"FACE DOWN / FLIPS WHEN PLAYED");f[1].x=42u;f[1].y=42u;f[1].color=2u;
  /* The merged public header already gives the flip cue. Preserve a real
   * preview-overflow line; clear only the identical lower duplicate. */
  if(!strcmp(lines[5],"FLIPS WHEN PLAYED"))lines[5][0]=0;
 }
 /* Modifier effect lines: frontend_shop_labels.c fills free y50/58/66 slots. */
 if(cache->count&&!(k[0]&CARD_FACE_DOWN))n=frontend_modifier_fields(f,n,lines,k);
 field(f+n++,lines[5],plain?42u:84u-a1_text_width(lines[5],3)/2u,74,3,2);
 /* A public overflow/modifier pane explicitly replaces the decorative deck
  * spot; its ordinary path restores the complete ROM template. */
 if(plain){f[0]=f[n-1u];n--;}
 cache->detail_plain=plain|((dirty&128u)?2u:0u);
 /* A nonzero public selection count always rejects the zero-selection
  * template gateway. Retain the original direct compositor call for that
  * known fallback, avoiding an unnecessary bank round trip and eligibility
  * walk on normal selected-hand focus. No new admission or input policy. */
 if(dirty==2u&&!plain)a1_ui_publish_focus(5,8,11,2,f,n,0);
 else if(g->highlighted_count)a1_ui_publish(5,5,15,5,f,n,plain);
 else native_plain_detail_full(5,5,15,5,f,n,plain);
 /* Public deck count moves into the otherwise empty sidebar row only while
  * the conditional pane owns its usual decorative spot. */
 if(dirty!=2u||plain){strcpy(a,"D");strcat(a,deck);if(!plain)a[0]=0;field(f,a,1,129,3,2);a1_ui_publish(0,16,5,1,f,1,0);}
}
/* Only selection count changed in an unchanged <=13-card window. The five
 * variable status cells are complete ROM variants; HAND/range text, map IDs
 * and palette attributes already reside unchanged in this exclusive lease. */
static void status_selection(uint8_t total){
 uint8_t x,slot;
 a1_ui_map_row_copy(16u,raster_pixels+32u);
 for(x=15u;x<20u;x++){
  slot=raster_pixels[32u+x];a1_status_base_copy(total,x,raster_pixels);
  VBK_REG=0;set_bkg_data(slot,1,raster_pixels);raster_capture_inputs();
 }
 VBK_REG=0;
}
static void status(const GameState *g){
 A1Text f[2];char line[32],a[16];uint8_t i,off=0,last=cache->first+cache->count,first=cache->first+1u;
 if(g->hand_count>13u){frontend_format_number(&first,1,line);strcat(line,"-");frontend_format_number(&last,1,a);strcat(line,a);strcat(line,"/");frontend_format_number(&g->hand_count,1,a);strcat(line,a);for(i=0;i<g->highlighted_count&&i<5u;i++)if(cache->summary_index[i]<cache->first||cache->summary_index[i]>=last)off++;if(off){strcat(line," *");frontend_format_number(&off,1,a);strcat(line,a);}}
 else{strcpy(line,"HAND ");frontend_format_number(&g->hand_count,1,a);strcat(line,a);}
 if(g->hand_count>13u){field(f,line,42,130,3,2);a1_ui_publish(5,16,15,1,f,1,128u|g->highlighted_count);}
 else{frontend_format_number(&g->hand_count,1,a);strcpy(line,"HAND ");field(f,line,42,130,3,2);field(f+1,a,62,130,3,3);a1_ui_publish(5,16,15,1,f,2,128u|g->highlighted_count);}
}
void native_ui_rows_reset(void) BANKED;
static void install(const GameState *g){
 uint8_t x,id,prepare_wait=native_table_bank1_owner==199u&&!raster_live&&!raster_fault&&
  !(LCDC_REG&(LCDCF_BG9C00|LCDCF_WINON))&&(LCDC_REG&LCDCF_BG8000)&&!SCX_REG&&!SCY_REG;
 /* Capture genuine TITLE origin and the original foreign-owner verdict.
  * Warm installs publish first; bounded idle prepares their future WAIT.
  * An incomplete cold TITLE lease is prepared before first TABLE publication. */
 native_ui_rows_reset();wait_vbl_done();if(!native_busy_begin(g))DISPLAY_OFF;HIDE_SPRITES;HIDE_WIN;frontend_native_acquire();
 if(native_table_bank1_owner!=NATIVE_BANK1_SUSPENDED||raster_live||raster_fault){
 for(id=0;id<180u;id++){a1_card_tile_copy(id,raster_pixels);x=id<130u?id:id<147u?id+15u:id<175u?id+17u:id<177u?id+59u:id+69u;VBK_REG=1;set_bkg_data(x,1,raster_pixels);raster_capture_inputs();}
 for(id=0;id<A1_EDGE_COUNT;id++){a1_edge_tile_copy(id,raster_pixels);VBK_REG=1;set_bkg_data(130u+id,1,raster_pixels);raster_capture_inputs();}
 for(id=0;id<A1_RAIL_BACK_COUNT;id++){a1_rail_back_tile_copy(id,raster_pixels);VBK_REG=1;set_bkg_data(A1_RAIL_BACK_BASE+id,1,raster_pixels);raster_capture_inputs();}
 /* under the cooperative redirect set_*_data is BANKED and
  * reads its source after switching ROM banks, so ROM arrays from this bank
  * arrived as unrelated bank41 bytes (badge/receipt dots never drawn). Copy
  * through RAM first, as native_busy_wait_patterns already does. */
 {static const uint8_t badge[16]={0,255,66,189,36,219,24,231,24,231,36,219,66,189,0,255};memcpy(raster_pixels,badge,16);VBK_REG=1;set_bkg_data(140u,1,raster_pixels);}
 a1_cursor_copy(raster_pixels);VBK_REG=1;set_sprite_data(CURSOR_TILE,2,raster_pixels);
 {static const uint8_t dots[32]={0,0,84,0,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};memcpy(raster_pixels,dots,32);set_sprite_data(RECEIPT_TILE,2,raster_pixels);}
 }
 /* Bank0 is always rebuilt, including all176 mutable UI bases below. */
 VBK_REG=0;native_table_bank1_owner=NATIVE_BANK1_VALID;
 native_busy_install_base();if(prepare_wait)native_busy_prepare_wait(g);
 memset(cache,255,sizeof(*cache));
}
void native_table_render(const GameState *g) BANKED {
 uint8_t first,count,pos,i,k[4],old_key[4],x[13],res[32],bonus[8],detail_dirty,force=!native_table_live,hand_dirty,summary_dirty,res_dirty,icon_dirty,focus_dirty,preview_dirty,status_dirty,status_only;
 uint16_t mask,wide;HandPreview hp;
 if(coop_active&&!coop_checkpoint())return;
 if(force)install(g);if(coop_active&&!coop_checkpoint())return;
 memset(old_key,0,4);if(!force&&cache->count&&cache->focus>=cache->first&&cache->focus<cache->first+cache->count)memcpy(old_key,cache->cards[cache->focus-cache->first],4);
 count=geometry(g,&first,&mask,&wide,x);pos=count?g->focus-first:0;detail_dirty=force?63u:0u;
 hand_dirty=force||first!=cache->first||count!=cache->count||mask!=cache->mask||wide!=cache->wide;
 focus_dirty=force||cache->focus!=g->focus;
 status_dirty=force||first!=cache->first||count!=cache->count||g->hand_count!=cache->resources[28]||g->highlighted_count!=cache->total_selected;
 status_only=!force&&g->highlighted_count<=5u&&g->hand_count<=13u&&first==cache->first&&count==cache->count&&g->hand_count==cache->resources[28]&&g->highlighted_count!=cache->total_selected;
 /* Publish cursor only after any changed hand geometry is resident. */
 /* Choose the key loop once; avoid adding a branch to every general card. */
 if(status_only&&g->deck_pos==cache->resources[29]){
  for(i=0;i<count;i++){
   selection_key(g,first+i,k);if(memcmp(k,cache->cards[i],4)){hand_dirty=1;if(i==pos)focus_dirty=1;}memcpy(cache->cards[i],k,4);
  }
 }else{
  for(i=0;i<count;i++){
   key(g,first+i,k);if(memcmp(k,cache->cards[i],4)){hand_dirty=1;if(i==pos)focus_dirty=1;}memcpy(cache->cards[i],k,4);
  }
 }
 memset(bonus,0,8);if(count&&!(cache->cards[pos][0]&CARD_FACE_DOWN))memcpy(bonus,&g->hand[g->focus].permanent_bonus,8);
 if(memcmp(bonus,cache->focus_bonus,8))detail_dirty|=8u;memcpy(cache->focus_bonus,bonus,8);
 if(count&&((old_key[0]&3u)!=(cache->cards[pos][0]&3u)||(old_key[1]>>2)!=(cache->cards[pos][1]>>2)||old_key[2]!=cache->cards[pos][2]||old_key[3]!=cache->cards[pos][3]))detail_dirty|=14u;
 summary_dirty=summary(g,force);res_dirty=resources(g,res);
 if(summary_dirty||cache->total_selected!=g->highlighted_count)detail_dirty|=1u;
 if(focus_dirty)detail_dirty|=2u;
 if(first!=cache->first||count!=cache->count||g->hand_count!=cache->resources[28]||summary_dirty||cache->total_selected!=g->highlighted_count||(g->hand_count>13u&&focus_dirty))detail_dirty|=16u;
 if(!coop_focus_only(force|summary_dirty|res_dirty|status_dirty|(detail_dirty&12u)))return;
 cache->first=first;cache->count=count;cache->mask=mask;cache->focus=g->focus;cache->wide=wide;
 if(hand_dirty){uint8_t previous_total=cache->resources[28];cache->resources[28]=g->hand_count;hand(first,count,mask,x);cache->resources[28]=previous_total;}
 /* Icon keys depend on public Joker/item data and their own cache, not on
  * the sidebar resource key. Complete this unconditional check before the
  * conditional sidebar-mask helper can invalidate the compiler's g reuse. */
 icon_dirty=inventory_icons(g,force);if(coop_active&&!coop_checkpoint())return;
 if(res_dirty||force){
  /* Bit64 retains the resource-changed boolean for metadata-only changes. */
  if(!force)res_dirty=64u|sidebar_changes(g,res);
  memcpy(cache->resources,res,32);cache->joker_rule=inventory_joker_limit(g);cache->consumable_rule=inventory_consumable_limit(g);
 }
 /* Watch only the public resolved type/level. Invalid public recognition
  * conservatively refreshes; hidden or empty previews need no level lookup. */
 preview_dirty=force||summary_dirty||res_dirty||icon_dirty||cache->total_selected!=g->highlighted_count||cache->preview_type==255u||(cache->preview_type<12u&&cache->preview_level!=g->hand_levels[cache->preview_type]);
 if(preview_dirty||(detail_dirty&&!(caption_reusable(pos,detail_dirty))))frontend_public_hand_preview(g,&hp);
 if(preview_dirty){
  cache->preview_type=!hp.valid?255u:hp.hidden||!g->highlighted_count?254u:hp.hand_type;
  cache->preview_level=hp.level;sidebar(g,(force||res_dirty)?(force?SB_FULL:(res_dirty&191u)):0,&hp);
 }
 if(detail_dirty||preview_dirty){
  /* In a stable complete window, selection may change only the left preview
   * and status. No off-view summary exists. Keep the general path for every
   * focused public-key/bonus/resource change, conditional pane, invalid
   * recognition, or newly overflowing uint32 preview. Hidden preview data
   * is suppressed before this predicate and is never inspected here. */
  if(!(preview_dirty&&!force&&!focus_dirty&&!res_dirty&&!icon_dirty&&!cache->detail_plain&&g->hand_count<=13u&&!(detail_dirty&14u)&&hp.valid&&(hp.hidden||(hp.chips<10000ul&&hp.mult<10000ul))))
  {
   /* Resolve the caption-only path before allocating the general detail frame. */
   if(!preview_dirty&&caption_reusable(pos,detail_dirty))
    native_focus_caption(cache->cards[pos],cache->count,cache->detail_plain&1u);
   else details(g,pos,preview_dirty?63u:detail_dirty,&hp);
  }
 }
 if(status_dirty){if(status_only)status_selection(g->highlighted_count);else status(g);}
 if(hand_dirty||icon_dirty){uint8_t active=0;for(i=0;i<count;i++)if(!(cache->cards[i][0]&CARD_FACE_DOWN)&&(cache->cards[i][2]&15u))active=1;
  if(active||force||icon_dirty||!coop_plain_carriers_hidden())
   if(!native_D_hand_finish(g->hand_count>13u,active,(sys_time>>5)&3u)){if(!coop_cancelled)raster_fault=248u;return;}}
 /* Focus/selection republishes no money rows; keep their palette ownership.
  * Resource and full paints still restore the sidebar attributes. */
 if(force||res_dirty)native_D_sidebar();if(coop_active&&!coop_checkpoint())return;
 cursor(count,pos,mask,x);if(coop_active&&!coop_checkpoint())return;
 cache->total_selected=g->highlighted_count;
 if(force){
  a1_palette_copy(raster_pixels);set_bkg_palette(0,7,(uint16_t *)raster_pixels);set_sprite_palette(0,1,(uint16_t *)(raster_pixels+64u));

  SCX_REG=SCY_REG=0;SPRITES_8x8;native_busy_publish();set_bkg_palette(7u,1u,(uint16_t *)(raster_pixels+56u));SHOW_SPRITES;SHOW_BKG;LCDC_REG|=LCDCF_BG8000;native_table_live=1;
 }
 if(native_palette_ready==2u)native_palette_start();DISPLAY_ON;
}
#undef DISPLAY_OFF
#define DISPLAY_OFF display_off()
#undef native_table_live
#undef native_palette_stop
#undef LCDC_REG
#define LCDC_REG (*(volatile uint8_t*)0xff40u)
/* Exact pre-input action receipt; sprite0 OAM shadow only. START/B swap
 * to dots and A dips 2px onto the current card. Direction presses leave the
 * old published caret steady until the complete new card/preview/caret image
 * is published atomically. No guessed destination, separate lean or partial
 * local card-map publication. Mandatory render restores ordinary cursor.
 * A staged-but-cancelled home is still trusted only when the source sprite
 * is exactly at that home or at an existing action-receipt offset. */
extern uint8_t coop_navigation_protected,coop_navigation_clean,coop_navigation_burst;
void native_table_receipt(const GameState *g,uint8_t pressed) BANKED {
 uint8_t dx,dy;
 coop_navigation_protected=(pressed==J_LEFT||pressed==J_RIGHT)&&
  native_table_live&&g==&game&&g->phase==PHASE_TABLE&&!g->ui_mode&&g->hand_count;
 if(!coop_navigation_protected){coop_navigation_clean=0;coop_navigation_burst=0;}
 if(!native_table_live&&g==&game&&g->phase==PHASE_PACK&&!g->ui_mode&&(pressed==J_A||pressed==J_B||pressed==J_START)){frontend_legacy_receipt(g);return;}
 if(!native_table_live||g!=&game||g->phase!=PHASE_TABLE||g->ui_mode)return;
 if(pressed==J_START||pressed==J_B){set_sprite_tile(0,RECEIPT_TILE);return;}
 if(!cursor_home_y||shadow_OAM[0].tile!=CURSOR_TILE)return;
 dx=shadow_OAM[0].x-cursor_home_x;dy=shadow_OAM[0].y-cursor_home_y;
 if(!((!dy&&(!dx||dx==3u||dx==253u))||(!dx&&dy==2u)))return;
 if(pressed==J_A)move_sprite(0,cursor_home_x,cursor_home_y+2u);

}
void native_palette_stop(void) NONBANKED;
void native_table_leave(void) BANKED {
 coop_resident=0;if(!native_table_live)return;
 wait_vbl_done();native_palette_stop();DISPLAY_OFF;native_table_live=0;LCDC_REG&=~LCDCF_BG9C00;HIDE_SPRITES;VBK_REG=0;
 frontend_legacy_invalidate();
}
