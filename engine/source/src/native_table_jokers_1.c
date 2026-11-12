#pragma bank 255
#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
const uint8_t table_1998_jokers_1[7200]={/* OWNER_INSTALL_SLOT:table_1998_jokers_1 */};
const uint16_t native_record_palettes_1[300]={/* OWNER_INSTALL_SLOT:native_record_palettes_1 */};
void native_table_joker_copy_1(uint8_t id,uint8_t *out) BANKED {memcpy(out,table_1998_jokers_1+(uint16_t)id*96u,96);}
void native_table_palette_copy_1(uint8_t id,uint8_t *out) BANKED {memcpy(out,native_record_palettes_1+(uint16_t)id*4u,8);}
