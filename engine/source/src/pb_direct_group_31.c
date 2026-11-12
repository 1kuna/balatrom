#pragma bank 42
#include <gb/gb.h>
uint8_t pb_direct_read_31(uint16_t slot,uint16_t *r) BANKED {(void)slot;(void)r;return 0;}
uint8_t pb_direct_group_31(uint8_t n,uint16_t *r) BANKED {(void)n;(void)r;return 0;}
