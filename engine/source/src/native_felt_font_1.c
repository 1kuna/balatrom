#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "native_felt_font_1_felt_font_pairs_1.inc"

void felt_font_copy_1(uint16_t index,uint8_t *out) BANKED {memcpy(out,felt_font_pairs_1+index*16u,16);}
