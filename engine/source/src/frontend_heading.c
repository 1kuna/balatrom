#pragma bank 255
#include "frontend_heading.h"
#include "frontend_heading_rows.inc"
void frontend_heading_tile(uint8_t first,uint8_t second,uint8_t offset,uint8_t paper,uint8_t out[16]) BANKED {
 const uint8_t *left,*right;uint8_t row,mask,phase=offset>>1;
 if(first<32u||first>95u)first='?';
 if(second<32u||second>95u)second='?';
 left=heading_rows+(uint16_t)(first-32u)*48u+(2u-phase)*8u;
 right=heading_rows+(uint16_t)(second-32u)*48u+(5u-phase)*8u;
 for(row=0;row<8u;row++){
  mask=*left++|*right++;*out++=paper?(uint8_t)~mask:0u;*out++=mask;
 }
}
