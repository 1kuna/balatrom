#pragma bank 255
#include <gb/gb.h>
#include "pb_direct_static_static_records.inc"

uint16_t pb_direct_static(uint8_t profile,uint8_t target,uint8_t pair) BANKED {
 uint16_t at;if(profile>=11u||target>1u||pair>=180u)return 65535u;
 at=((uint16_t)profile*360u+(uint16_t)target*180u+pair)*2u;
 return (uint16_t)static_records[at]|((uint16_t)static_records[at+1u]<<8);
}
