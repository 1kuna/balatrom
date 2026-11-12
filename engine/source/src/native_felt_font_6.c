#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "native_felt_font_6_felt_font_pairs_6.inc"

void felt_font_copy_6(uint16_t index,uint8_t *out) BANKED {memcpy(out,felt_font_pairs_6+index*16u,16);}
