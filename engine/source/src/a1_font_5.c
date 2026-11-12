#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "a1_assets.h"
#include "a1_font_5_a1_font5_masks.inc"

void a1_font5_mask_copy(uint8_t ch,int8_t dx,int8_t dy,uint8_t *out) BANKED {
 uint16_t offset;uint8_t first,n;
 if(ch>=97u&&ch<=122u)ch-=32u;if(ch<32u||ch>95u)ch=63u;
 memset(out,0,8u);if(dx < -7 || dx > 7 || dy < -7 || dy > 7)return;
 offset=((uint16_t)(ch-32u)*15u+(uint8_t)(dx+7))*8u;
 /* Shift/clipping is a contiguous bounded copy of the original mask. */
 first=dy>0?dy:0u;n=dy>0?8u-dy:8u+dy;
 memcpy(out+first,a1_font5_masks+offset+(dy<0?-dy:0u),n);
}
