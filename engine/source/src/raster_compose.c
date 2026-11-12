#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "retained_caret.h"
#include "frontend_panel_b_internal.h"
#include "frontend_panel_b_hotpath.h"
#include "frontend_font5.inc"
#include "frontend_font_rows.inc"
#define PANEL_B_RUNTIME
#include "frontend_panel_b_assets.inc"
extern uint8_t raster_pixels[128];
#define pixels raster_pixels
static void strip_put(uint8_t dx,uint8_t dy,uint8_t colour) {
 uint8_t index;
 if(dx>=8u||dy>=16u)return;
 index=dy;index*=8u;index+=dx;pixels[index]=colour;
}
static void draw_glyph(const Glyph *g,uint8_t x0,uint8_t y0) {
    uint8_t ch,px,py,x,y,bits,col,advance=g->font?6u:4u,height=g->font?7u:5u;
    if(y0>=g->y+height||y0+16u<=g->y||x0>=g->x+g->length*advance||x0+8u<=g->x)return;
    for(ch=0;ch<g->length;ch++) {
        x=g->x+ch*advance;if(x>=x0+8u||x+advance<=x0)continue;
        col=g->s[ch];if(col<32u||col>95u)col='?';
        for(py=0;py<height;py++) {
            y=g->y+py;if(y<y0||y>=y0+16u)continue;
            bits=g->font?font5[col-32u][py]:frontend_font_rows[(uint16_t)col*5u+py];
            bits=panel_b_glyph_row_mask(bits,g->font,(int8_t)(x-x0));
            for(px=0;bits;px++,bits<<=1)if(bits&128u)strip_put(px,y-y0,g->colour);
        }
    }
}
/* Runtime text chooses prepared invariant patterns and patches only glyph ink.
 * Cards still use sanitized strip composition; owned Joker palettes use solver. */
static void patch(const uint8_t *src,uint8_t w,uint8_t h,uint8_t ox,uint8_t oy,uint8_t x0,uint8_t y0,uint8_t selected) {
    uint8_t left,right,top,bottom,x,y,c;
    if(x0>=ox+w||x0+8u<=ox||y0>=oy+h||y0+16u<=oy)return;
    left=x0>ox?x0:ox;right=x0+8u<ox+w?x0+8u:ox+w;
    top=y0>oy?y0:oy;bottom=y0+16u<oy+h?y0+16u:oy+h;
    for(y=top;y<bottom;y++)for(x=left;x<right;x++) {
        c=src[(uint16_t)(y-oy)*w+x-ox];if(c==1u||c==255u)continue;
        if(selected&&x==ox&&(y==oy||y==oy+29u))continue;
        else if(selected&&(c==13u||(y-oy==28u&&x>ox&&x<ox+18u)))c=18;
        strip_put(x-x0,y-y0,c);
    }
}
void raster_compose(const Frame *f,uint8_t x0,uint8_t y0) BANKED {
    uint8_t i,x,y,dx,dy,c,j,id,span;uint16_t glyph_mask;const PanelBCard *v;const PanelBJoker *q;

    panel_b_load_static_pair((y0>>4)*20u+(x0>>3),pixels);
    for(i=0;i<f->g->joker_count&&i<5u;i++) {
        q=f->jokers+i;if(x0>=q->x+20u||x0+8u<=q->x||y0>=35u)continue;
        patch(panel_b_joker_body,20,28,q->x,7,x0,y0,0);
        for(y=1;y<=24u;y++)if(7u+y>=y0&&7u+y<y0+16u)for(x=1;x<=16u;x++)if(q->x+x>=x0&&q->x+x<x0+8u) {
            if(q->kind==PANEL_B_HIDDEN)c=((x+y)&2u)?13u:2u;
            else if(q->kind==PANEL_B_FACE) {
                dx=x-1u;dy=y-1u;j=(dy>>3)*2u+(dx>>3);
                /* Keep both icon planes in explicit byte operations. The frozen
                 * compiler drops the low-plane dereference in the compound
                 * indexed shift expression. j stays within bytes0..94. */
                j*=16u;j+=(dy&7u)*2u;
                id=f->icons[i][j];c=f->icons[i][j+1u];
                id>>=7u-(dx&7u);c>>=7u-(dx&7u);
                id=(id&1u)|((c&1u)<<1);
                c=f->jids[i*4u+id];
            }else c=13;
            strip_put(q->x+x-x0,7u+y-y0,c);
        }
    }
    for(i=0,glyph_mask=f->glyph_bands[y0>>4];glyph_mask;i++,glyph_mask>>=1)if(glyph_mask&1u)draw_glyph(f->glyphs+i,x0,y0);
    span=y0>=80u?panel_b_card_span(x0>>3):0u;
    for(i=span&15u;i<f->page.count&&i<(span>>4);i++) {
        v=f->cards+i;if(x0>=v->x+20u||x0+8u<=v->x||y0>=v->y+31u||y0+16u<=v->y)continue;
        panel_b_paint_card(v,x0,y0,pixels);
        if((v->cues&PANEL_B_DEBUFF)&&v->y+27u>=y0&&v->y+27u<y0+16u)for(x=3;x<10u;x++)if(v->x+x>=x0&&v->x+x<x0+8u)strip_put(v->x+x-x0,v->y+27u-y0,14);
        /* The public Bell lock sits inside the 14 visible columns. Following
         * cards begin at x+14, so they cannot cover this four-pixel cue. */
        if((v->cues&PANEL_B_BELL)&&v->x+11u>=x0&&v->x+11u<x0+8u)for(y=24;y<28u;y++)if(v->y+y>=y0&&v->y+y<y0+16u)strip_put(v->x+11u-x0,v->y+y-y0,18);
    }
    for(i=span&15u;!retained_caret_mode&&i<f->page.count&&i<(span>>4);i++){v=f->cards+i;if(v->cues&PANEL_B_FOCUSED)patch(panel_b_caret,7,4,v->x+5u,v->y-5u,x0,y0,0);}
}
