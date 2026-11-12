#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "native_felt_font_4_felt_font_pairs_4.inc"

void felt_font_copy_4(uint16_t index,uint8_t *out) BANKED {memcpy(out,felt_font_pairs_4+index*16u,16);}
