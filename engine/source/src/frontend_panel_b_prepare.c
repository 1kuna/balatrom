#include "paged_inventory.h"
#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#define PANEL_B_ENTITY_VIEWS
#include "frontend_panel_b_view.inc"
#include "frontend_panel_b_internal.h"
#include "frontend_numeric.h"
#include "frontend_public_preview.h"
#include "boss_labels.h"
void frontend_boss_sidebar(uint8_t,uint8_t,uint8_t,char [15]) BANKED;
static const uint16_t panel_b_rgb[]={6307,13732,8389,14789,24071,15849,8525,23181,17008,18995,22195,20148,23317,21206,10584,21274,11995,25501,13053,27582,19326};
static const uint8_t panel_b_ui_palettes[]={0,19,18,12,1,19,12,3,1,4,19,7,1,14,19,7,1,13,17,0,1,18,17,2,1,13,17,14,1,18,17,14};
static const uint8_t panel_b_fixed_obj[]={0,2,18,0,14,18,3,0,5,3,19,2,3,19,14,0,18,19,0,13,19,2,14,19};
#include "frontend_panel_b_families.inc"
static uint8_t member(const uint8_t *p,uint8_t n,uint8_t c) {
 uint8_t i;for(i=0;i<n;i++)if(p[i]==c)return i;return 255;
}
static uint8_t width(const char *s,uint8_t font) {
    uint8_t n=strlen(s);return n?n*(font?6u:4u)-1u:0;
}
static uint8_t centered(const char *s,uint8_t font,uint8_t x,uint8_t w) {
    return x+(w-width(s,font)+1u)/2u;
}
static void glyph(Frame *f,const char *s,uint8_t font,uint8_t x,uint8_t y,uint8_t colour) {
    uint8_t row;uint16_t bit=1u<<f->glyph_count;Glyph *g=f->glyphs+f->glyph_count++;
    g->s=s;g->length=strlen(s);g->font=font;g->x=x;g->y=y;g->colour=colour;
    g->right=x+g->length*(font?6u:4u);g->bottom=y+(font?7u:5u);
    for(row=y>>4;row<9u&&row<=(g->bottom-1u)>>4;row++)f->glyph_bands[row]|=bit;
}
/* Existing fonts expose uppercase E. Keep core numeric formatting
 * unchanged and normalize only these prepared display strings. */
static void display_exponent(char text[16]) {
 uint8_t i;for(i=0;i<16u&&text[i];i++)if(text[i]=='e')text[i]='E';
}
static void prepare_frame(Frame *f,const GameState *g,const HandPreview *p,const char *numbers) {
    uint8_t i,j,k,n=0,len,w,lev,hidden=0,stone=0,catalog=0,all_hidden=1;uint16_t val;uint32_t magnitude;
    memset(f,0,sizeof(*f));f->g=g;panel_b_page(g,&f->page);
    for(i=0;i<8u;i++)panel_b_card_view(g,&f->page,i,f->cards+i);
    memcpy(f->bg,panel_b_ui_palettes,32);f->bg[21]=13; /* normal black family */
    memset(f->obj,255,24);if(!g->joker_count){memcpy(f->obj,panel_b_fixed_obj,24);f->obj_used=8;}
    for(i=0;i<5u;i++) {
        panel_b_joker_view(g,i,f->jokers+i);
        if(i<g->joker_count&&PG_JBYTE(f,i,jokers,0u).kind!=PANEL_B_HIDDEN)all_hidden=0;
        if(i>=g->joker_count||PG_JBYTE(f,i,jokers,0u).kind!=PANEL_B_FACE)continue;
        catalog=1;
        if(!panel_b_fetch_icon(PG_JBYTE(f,i,jokers,0u).id,f->icons[i],f->jwords+i*4u))continue;
        for(j=0;j<4u;j++) {
            val=f->jwords[i*4u+j]&0x7fffu;k=0;
            while(k<21u&&panel_b_rgb[k]!=val)k++;
            if(k==21u){k=0;while(k<n&&(f->jwords[k]&0x7fffu)!=val)k++;
                /* Hidden slots leave zeroed holes in jwords. A new word owns
                 * its actual visible source slot, rather than the preceding
                 * search limit, which may point into such a hole. */
                if(k==n)k=i*4u+j;k+=21u;}
            f->jids[i*4u+j]=k;n=i*4u+j+1u;
            f->bg[(3u+i)*4u+j]=k;
        }
    }
    /* Palette policy sees only sanitized public commands. It never reads
     * concealed ranks, enhancements, editions or Joker identities. */
    for(i=0;i<f->page.count;i++){hidden+=f->cards[i].kind==PANEL_B_HIDDEN;stone+=f->cards[i].kind==PANEL_B_STONE;}
    if(!catalog) {
        if(stone)f->family=stone==f->page.count?6u:5u;
        else if(hidden)f->family=1u;
        else if(g->joker_count&&g->joker_count<=5u&&all_hidden)f->family=g->highlighted_count?1u:4u;
    }
    if(f->family){memcpy(f->bg,panel_b_family_palettes[f->family-1u],32);memcpy(f->obj,panel_b_family_palettes[f->family-1u]+32,24);f->obj_used=8;}
    if(numbers){memcpy(f->target,numbers,16);memcpy(f->score,numbers+16,16);}
    else{frontend_format_number_width(&g->target,8,f->target,8);frontend_format_number_width(&g->score,8,f->score,8);}
    display_exponent(f->target);display_exponent(f->score);
    if(p->hidden||frontend_public_preview_hidden(g)){strcpy(f->chips,"?");strcpy(f->mult,"?");strcpy(f->label,"????");strcpy(f->level,"LV?");}
    else {
        frontend_format_number_width(&p->chips,4,f->chips,9);
        frontend_format_number_width(&p->mult,4,f->mult,9);
        if(!g->highlighted_count){strcpy(f->label,"SELECT CARDS");f->level[0]=0;}
        else {
            for(i=0;i<15u&&p->label[i]&&!(p->label[i]==' '&&p->label[i+1u]=='L'&&p->label[i+2u]=='V');i++)f->label[i]=p->label[i];
            f->label[i]=0;strcpy(f->level,"LV");frontend_format_number(&p->level,2,f->level+2);
        }
    }
    f->money[0]='$';i=1;if(g->money<0)f->money[i++]='-';
    magnitude=g->money<0?0u-(uint32_t)g->money:(uint32_t)g->money;
    frontend_format_number(&magnitude,4,f->money+i);
    len=strlen(f->label);lev=width(f->level,0);f->label_font=1;
    w=len*6u-1u+(lev?16u+lev:0u);
    if(w>120u){f->label_font=0;w=len*4u-1u+(lev?16u+lev:0u);}
    f->label_x=94u-w/2u;f->level_x=f->label_x+width(f->label,f->label_font)+16u;
    if(g->joker_count&&!f->family)for(i=0;i<41u;i++){f->membership[i]=0;for(j=0;j<8u;j++)if(member(f->bg+j*4u,4,i)!=255u)f->membership[i]|=1u<<j;}
    strcpy(f->blind_label,g->blind==0u?"SMALL":g->blind==1u?"BIG":"BOSS");
    if(g->blind==2u){
        /* All twelve public hand-type labels are <=14 characters. Reserve
         * label[16..30] for the copied boss sidebar name; preview updates
         * preserve that tail and the final-four-glyph contract. */
        frontend_boss_sidebar(g->boss_id,g->boss_disabled,g->ox_target_hand,f->label+16);
        glyph(f,f->label+16,0,centered(f->label+16,0,0,36),5,19);
    }else glyph(f,f->blind_label,0,g->blind==0u?9u:13u,5,19);
    j=strlen(f->target)<=5u;glyph(f,f->target,j,centered(f->target,j,0,36),24,18);
    j=strlen(f->score)<=5u;glyph(f,f->score,j,centered(f->score,j,0,36),51,19);
    for(i=0;i<4u;i++){val=i==0u?g->hands:i==1u?g->discards:i==2u?g->ante:g->blind+1u;frontend_format_number(&val,1,f->stats[i]);j=i<2u&&val<10u;glyph(f,f->stats[i],j,i<2u?33u-width(f->stats[i],j):i==2u?26u:24u,i==0u?69u:i==1u?81u:i==2u?117u:127u,i<2u?19u:12u);}
    if(strlen(f->money)>8u){strcpy(f->money_tail,f->money+8);f->money[8]=0;glyph(f,f->money,0,2,97,18);glyph(f,f->money_tail,0,2,104,18);}
    else {j=strlen(f->money)<=5u;glyph(f,f->money,j,centered(f->money,j,0,36),97,18);}
    /* Signed int32 money needs at most12 characters plus its terminator.
     * Own its two spare trailing bytes for the invariant X. Keep this glyph
     * before the final four preview glyphs used by retained_selection.
     * Explicit text makes old static-X atlas keys inapplicable without
     * modifying any dictionary record, helper or physical publication path. */
    f->money[14]='X';f->money[15]=0;
    glyph(f,f->money+14,0,96,68,19);
    /* Fixed-ROM boss strings remain mapped across all BANKED calls. Active
     * bosses show their full rule; disabled bosses show their full name,
     * paired with the explicit DISABLED sidebar label. Existing muted gold13
     * fits felt in upper BG2 after row32; no new palette/OBJ ownership. */
    if(g->blind==2u)glyph(f,g->boss_disabled?boss_name(g->boss_id):boss_rule(g->boss_id),0,40,41,13);
    glyph(f,f->label,f->label_font,f->label_x,f->label_font?48u:49u,19);glyph(f,f->level,0,f->level_x,49,12);
    j=width(f->chips,1)<=39u;glyph(f,f->chips,j,centered(f->chips,j,46,43),j?67u:68u,19);
    j=width(f->mult,1)<=39u;glyph(f,f->mult,j,centered(f->mult,j,104,43),j?67u:68u,19);
}

/* All six profiles preserve the semantic RGB words. Admission chooses a
 * representation only; this never changes a visible command or GameState. */
void panel_b_choose_family(Frame *f,uint8_t family) BANKED {
 if(!family||family>6u)return;
 f->family=family;memcpy(f->bg,panel_b_family_palettes[family-1u],32);
 memcpy(f->obj,panel_b_family_palettes[family-1u]+32,24);f->obj_used=8;
}

/* Original standalone ABI retained; actual raster entry formats its two
 * GbNum values before allocating the large Frame, then supplies owned bytes. */
void panel_b_prepare(Frame *f,const GameState *g,const HandPreview *p) BANKED {prepare_frame(f,g,p,0);}
void panel_b_prepare_numbered(Frame *f,const GameState *g,const HandPreview *p,const char numbers[32]) BANKED {prepare_frame(f,g,p,numbers);}
