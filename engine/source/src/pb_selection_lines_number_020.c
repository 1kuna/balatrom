#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_number_020_ids.inc"

#include "pb_selection_lines_number_020_vectors.inc"

uint16_t pb_selection_lines_number_020(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<6144u||value>=8192u||pair<93u||pair>99u)return 65535u;
 id=ids[value-6144u];return vectors[id*7u+pair-93u];
}
