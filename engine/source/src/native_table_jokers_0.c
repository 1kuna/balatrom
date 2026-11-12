#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
const uint8_t table_1998_jokers_0[7200]={/* OWNER_INSTALL_SLOT:table_1998_jokers_0 */};
const uint16_t native_record_palettes_0[300]={/* OWNER_INSTALL_SLOT:native_record_palettes_0 */};
void native_table_joker_copy_0(uint8_t id,uint8_t *out) BANKED {memcpy(out,table_1998_jokers_0+(uint16_t)id*96u,96);}
void native_table_palette_copy_0(uint8_t id,uint8_t *out) BANKED {memcpy(out,native_record_palettes_0+(uint16_t)id*4u,8);}
