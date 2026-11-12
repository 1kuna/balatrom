#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_dispatch.h"
#include "pb_selection_tables.h"
#include "retained_caret.h"

#define SD_CARDS 15u
#define SD_LABEL 16u
#define SD_CHIPS 32u
#define SD_MULT 64u
#define SD_MISS 65535u
#define SD_ABSENT 511u

/* Fixed slot positions remain distinct when only one vertical rectangle
 * intersects. These are the old compositor's canonical horizontal spans. */
#include "pb_selection_dispatch_selection_spans.inc"


static uint8_t text_length(const char *s, uint8_t limit) {
    uint8_t n;
    for (n=0; n<=limit; ++n) if (!s[n]) return n;
    return 255u;
}

/* Only canonical unsigned decimal strings. A miss is ordinary fallback. */
static uint16_t decimal(const char *s, uint8_t n, uint8_t limit) {
    uint8_t i, digit;
    uint16_t value=0;
    if (!n || n>limit || (n>1u && s[0]=='0') || s[n]) return SD_MISS;
    for (i=0; i<n; ++i) {
        if (s[i]<'0' || s[i]>'9') return SD_MISS;
        digit=s[i]-'0';
        value=value*10u+digit;
    }
    return value;
}

static uint8_t glyph_matches(const Glyph *g, const char *s, uint8_t n,
                             uint8_t font, uint8_t x, uint8_t y,
                             uint8_t colour) {
    return g->s==s && g->length==n && g->font==font && g->x==x &&
           g->y==y && g->colour==colour &&
           g->right==x+n*(font?6u:4u) && g->bottom==y+(font?7u:5u);
}

static uint8_t intersects(const Glyph *g, uint8_t left, uint8_t right,
                          uint8_t top, uint8_t bottom) {
    uint16_t edge=(uint16_t)g->x+g->length*(g->font?6u:4u);
    return g->length && g->x<right && edge>left && g->y<bottom &&
           (uint16_t)g->y+(g->font?7u:5u)>top;
}

static uint16_t number_line(const Glyph *g, const char *s, uint8_t base) {
    uint8_t n=text_length(s,4u), x;
    uint16_t value;
    if (!n || n>4u) return SD_MISS;
    if (n==1u && s[0]=='?') value=10000u;
    else { value=decimal(s,n,4u); if (value==SD_MISS) return SD_MISS; }
    x=base+(45u-6u*n)/2u;
    if (!glyph_matches(g,s,n,1u,x,67u,19u)) return SD_MISS;
    return value;
}

static uint16_t label_line(const Frame *f, uint8_t first) {
    uint8_t n=text_length(f->label,30u), lev=text_length(f->level,7u);
    uint8_t kind, font=1u, label_width, level_width, width, x;
    uint16_t value, id;
    if (!n || n>30u || lev>7u) return SD_MISS;
    kind=pb_selection_label_kind(f->label);
    if (kind<12u) {
        if (lev<3u || lev>4u || f->level[0]!='L' || f->level[1]!='V') return SD_MISS;
        value=decimal(f->level+2u,lev-2u,2u);
        if (value==SD_MISS) return SD_MISS;
        id=(uint16_t)kind*100u+value;
    } else if (kind==12u) {
        if (lev) return SD_MISS;
        id=1200u;
    } else if (kind==13u) {
        if (lev!=3u || f->level[0]!='L' || f->level[1]!='V' || f->level[2]!='?') return SD_MISS;
        id=1300u;
    } else return SD_MISS;
    level_width=lev?lev*4u-1u:0u;
    label_width=n*6u-1u;
    width=label_width+(level_width?16u+level_width:0u);
    if (width>120u) {
        font=0u;label_width=n*4u-1u;
        width=label_width+(level_width?16u+level_width:0u);
    }
    x=94u-width/2u;
    if (!glyph_matches(f->glyphs+first,f->label,n,font,x,font?48u:49u,19u) ||
        !glyph_matches(f->glyphs+first+1u,f->level,lev,0u,x+label_width+16u,49u,12u))
        return SD_MISS;
    return id;
}

uint8_t pb_selection_dispatch_begin(const Frame *f, uint8_t profile) BANKED {
    uint8_t i, row, cards=1u, first, flags=0u, count=f->page.count;
    uint16_t line, bit, valid_bits;
    const PanelBCard *card;
    const Glyph *g;
    if (!retained_caret_mode || (profile!=0u && profile!=10u) || !count || count>8u ||
        f->glyph_count<4u || f->glyph_count>15u) return 0u;
    for (i=0;i<count;++i) {
        card=f->cards+i;
        if (card->x!=40u+14u*i ||
            card->y!=((card->cues&PANEL_B_SELECTED)?92u:100u) ||
            (card->cues&~(PANEL_B_SELECTED|PANEL_B_FOCUSED|PANEL_B_DEBUFF|PANEL_B_BELL))) {
            cards=0u;break;
        }
        /* Public kind is checked before either visible face field is read. */
        if (card->kind==PANEL_B_FACE) {
            if (card->rank<2u || card->rank>14u || card->suit>3u) {cards=0u;break;}
        } else if (card->kind!=PANEL_B_HIDDEN && card->kind!=PANEL_B_STONE) {cards=0u;break;}
    }
    if (cards) flags=SD_CARDS;
    first=f->glyph_count-4u;
    line=label_line(f,first);
    if (line!=SD_MISS && (f->glyph_bands[3]&(1u<<first)) &&
        (!f->glyphs[first+1u].length || (f->glyph_bands[3]&(1u<<(first+1u))))) flags|=SD_LABEL;
    line=number_line(f->glyphs+first+2u,f->chips,46u);
    if (line!=SD_MISS && (f->glyph_bands[4]&(1u<<(first+2u)))) flags|=SD_CHIPS;
    line=number_line(f->glyphs+first+3u,f->mult,104u);
    if (line!=SD_MISS && (f->glyph_bands[4]&(1u<<(first+3u)))) flags|=SD_MULT;
    valid_bits=(1u<<f->glyph_count)-1u;
    if (f->glyph_bands[3]&~valid_bits) flags&=~SD_LABEL;
    if (f->glyph_bands[4]&~valid_bits) flags&=~(SD_CHIPS|SD_MULT);
    for (row=5u;row<9u;++row) if (f->glyph_bands[row]&~valid_bits) flags&=~(1u<<(row-5u));
    /* The old key's glyph-band mask is authoritative. Extra contributors
     * reject the affected region; none are silently omitted from a vector. */
    for (i=0,bit=1u;i<f->glyph_count;++i,bit<<=1) {
        g=f->glyphs+i;
        if (i!=first && i!=first+1u && (f->glyph_bands[3]&bit) &&
            intersects(g,40u,160u,48u,64u)) flags&=~SD_LABEL;
        if ((f->glyph_bands[4]&bit)) {
            if (i!=first+2u && intersects(g,40u,96u,64u,80u)) flags&=~SD_CHIPS;
            if (i!=first+3u && intersects(g,104u,160u,64u,80u)) flags&=~SD_MULT;
        }
        for (row=5u;row<9u;++row) if ((f->glyph_bands[row]&bit) &&
            intersects(g,40u,160u,row*16u,(row+1u)*16u)) flags&=~(1u<<(row-5u));
    }
    return flags | (profile==10u?128u:0u);
}

/* Admission has already checked exact geometry and all glyph contributors.
 * Recover only the short identity from the same immutable public Frame. */
static uint16_t label_identity(const Frame *f) {
    uint8_t kind=pb_selection_label_kind(f->label), n;
    uint16_t value;
    if (kind==12u) return 1200u;
    if (kind==13u) return 1300u;
    if (kind>=12u || f->level[0]!='L' || f->level[1]!='V') return SD_MISS;
    n=text_length(f->level,7u);
    if (n<3u || n>4u) return SD_MISS;
    value=decimal(f->level+2u,n-2u,2u);
    return value==SD_MISS?SD_MISS:(uint16_t)kind*100u+value;
}
static uint16_t number_identity(const char *s) {
    uint8_t n=text_length(s,4u);
    if (n==1u && s[0]=='?') return 10000u;
    return decimal(s,n,4u);
}

uint16_t pb_selection_dispatch_record(const Frame *f, uint8_t admission,
                                     uint8_t pair) BANKED {
    uint8_t profile=(admission&128u)?10u:0u, flags=admission&127u;
    uint8_t col, row, span, first, end, i, y;
    uint16_t left=SD_ABSENT, right=SD_ABSENT, code;
    const PanelBCard *card;
    if (!flags || profile>=11u || f->page.count>8u || pair<65u || pair>=180u) return SD_MISS;
    col=pair%20u;row=pair/20u;
    if (col<5u) return SD_MISS; /* Sidebar dependencies stay with old compiler. */
    if (row==3u) {
        if (!(flags&SD_LABEL)) return SD_MISS;
        return pb_selection_label_record(profile,label_identity(f),pair);
    }
    if (row==4u) {
        if (col>=5u && col<=11u && (flags&SD_CHIPS))
            return pb_selection_number_record(profile,0u,number_identity(f->chips),pair);
        if (col>=13u && (flags&SD_MULT))
            return pb_selection_number_record(profile,1u,number_identity(f->mult),pair);
        return SD_MISS; /* In particular, multiplication Glyph strip92. */
    }
    if (row<5u || !(flags&(1u<<(row-5u)))) return SD_MISS;
    span=selection_spans[col];first=span&15u;end=span>>4;
    for (i=first;i<end && i<f->page.count;++i) {
        card=f->cards+i;
        /* Hidden/Stone identities never inspect rank or suit. */
        if (card->kind==PANEL_B_FACE) {
            if (card->rank<2u || card->rank>14u || card->suit>3u) return SD_MISS;
            code=(card->rank-2u)*4u+card->suit;
        } else if (card->kind==PANEL_B_HIDDEN) code=52u;
        else if (card->kind==PANEL_B_STONE) code=53u;
        else return SD_MISS;
        if (card->cues&PANEL_B_SELECTED) code|=64u;
        if (card->cues&PANEL_B_DEBUFF) code|=128u;
        if (card->cues&PANEL_B_BELL) code|=256u;
        y=(code&64u)?92u:100u;
        if (row*16u>=y+31u || (row+1u)*16u<=y-5u) continue;
        if (i==first) left=code;else right=code;
    }
    return pb_selection_cards_record(profile,pair,left,right);
}
