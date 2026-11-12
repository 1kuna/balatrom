#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_number_008_ids.inc"

#include "pb_selection_lines_number_008_vectors.inc"

uint16_t pb_selection_lines_number_008(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<4096u||value>=6144u||pair<93u||pair>99u)return 65535u;
 id=ids[value-4096u];return vectors[id*7u+pair-93u];
}
