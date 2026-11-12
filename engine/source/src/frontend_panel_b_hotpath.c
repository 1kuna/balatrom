/* Glyph ink is shifted once per row, independently of palette/family choice.
 * delta is glyph_x - strip_x after the existing glyph/strip intersection test.
 * The five- and three-pixel fonts contain right-aligned semantic row bits. */
#include <gb/gb.h>
#include <stdint.h>
uint8_t panel_b_glyph_row_mask(uint8_t bits,uint8_t font,int8_t delta) NONBANKED {
 uint8_t mask=font?(bits&31u)<<3:(bits&7u)<<5;
 if(delta<0)return mask<<(uint8_t)-delta;
 return mask>>(uint8_t)delta;
}

#include "frontend_panel_b_hotpath.h"

/* OLDCALL fixes a stack ABI independent of SDCC's register-argument changes.
 * Entry stack: return16, src16, lookup16, dst16. One stack byte counts rows;
 * temporary pushes use two further bytes. No static WRAM, bank or IRQ changes.
 * src contains exactly 64 bounded IDs (0..40 semantic, or 0..63 packed codes).
 * The caller supplies a lookup spanning the input domain. 255 maps to code 0;
 * all other lookup bytes contribute their low two bits, exactly like C.
 * Updating the private dst argument slot avoids allocating a global pointer.
 */
void panel_b_pack64(const uint8_t *src, const uint8_t *lookup, uint8_t *dst)
    OLDCALL NONBANKED __naked {
    src; lookup; dst;
    __asm
    add sp,#-1
    ldhl sp,#0
    ld (hl),#8
    ldhl sp,#5
    ld a,(hl+)
    ld e,a
    ld d,(hl)
    ldhl sp,#3
    ld a,(hl+)
    ld h,(hl)
    ld l,a
45000$:
    ld b,#0
    ld c,#0
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45100$
    xor a
    jr 45200$
45100$:
    dec a
45200$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45101$
    xor a
    jr 45201$
45101$:
    dec a
45201$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45102$
    xor a
    jr 45202$
45102$:
    dec a
45202$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45103$
    xor a
    jr 45203$
45103$:
    dec a
45203$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45104$
    xor a
    jr 45204$
45104$:
    dec a
45204$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45105$
    xor a
    jr 45205$
45105$:
    dec a
45205$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45106$
    xor a
    jr 45206$
45106$:
    dec a
45206$:
    rra
    rl b
    rra
    rl c
    ld a,(hl+)
    push hl
    ld l,a
    ld h,#0
    add hl,de
    ld a,(hl)
    pop hl
    inc a
    jr nz,45107$
    xor a
    jr 45207$
45107$:
    dec a
45207$:
    rra
    rl b
    rra
    rl c
    push hl
    ldhl sp,#9
    ld a,(hl+)
    ld h,(hl)
    ld l,a
    ld a,b
    ld (hl+),a
    ld a,c
    ld (hl+),a
    ld b,h
    ld c,l
    ldhl sp,#9
    ld (hl),c
    inc hl
    ld (hl),b
    ldhl sp,#2
    dec (hl)
    pop hl
    jp nz,45000$
    add sp,#1
    ret
    __endasm;
}


/* Preserve both lookup halves. 255 becomes zero for BG. For OBJ, a missing
 * BG colour selects the low two bits of its OBJ code; this also preserves
 * the original C behavior when an inadmissible lookup contains OBJ 255. */
void panel_b_pack_pair(const uint8_t *pixels,const uint8_t lookup[3][41],uint8_t *encoded,uint8_t overlay) NONBANKED {
 uint8_t half,c,map[41];
 for(half=0;half<2u;half++) {
  if(overlay){for(c=0;c<41u;c++)map[c]=lookup[half][c]==255u?(lookup[2][c]&3u):0u;
   panel_b_pack64(pixels+half*64u,map,encoded+half*16u);
  }else panel_b_pack64(pixels+half*64u,lookup[half],encoded+half*16u);
 }
}

/* Projection fixes visible card x=40+14*slot and body/shadow width20.
 * Packed first(low nibble),exclusive limit(high nibble) preserves draw order.
 * Caret x+5,width7 lies within this same interval. */
uint8_t panel_b_card_span(uint8_t col) NONBANKED {
 static const uint8_t spans[]={0,0,0,0,0,16,32,32,49,49,66,67,83,100,100,117,117,134,135,135};
 return col<20u?spans[col]:0u;
}
