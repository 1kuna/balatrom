#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "native_felt_font_3_felt_font_pairs_3.inc"

void felt_font_copy_3(uint16_t index,uint8_t *out) BANKED {memcpy(out,felt_font_pairs_3+index*16u,16);}
