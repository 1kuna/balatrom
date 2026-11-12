#ifndef FRONTEND_HEADING_H
#define FRONTEND_HEADING_H
#include <gb/gb.h>
/* Character bytes are copied by the calling bank before this ROM-bank switch.
 * offset is the exact running6-pixel phase:0,2,4. Writes only caller tile16. */
void frontend_heading_tile(uint8_t first,uint8_t second,uint8_t offset,uint8_t paper,uint8_t out[16]) BANKED;
#endif
