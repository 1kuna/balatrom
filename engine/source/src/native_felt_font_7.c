#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "native_felt_font_7_felt_font_pairs_7.inc"

void felt_font_copy_7(uint16_t index,uint8_t *out) BANKED {memcpy(out,felt_font_pairs_7+index*16u,16);}
