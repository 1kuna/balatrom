#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_label_011_ids.inc"

#include "pb_selection_lines_label_011_vectors.inc"

uint16_t pb_selection_lines_label_011(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<0u||value>=1301u||pair<64u||pair>79u)return 65535u;
 id=ids[value-0u];return vectors[id*16u+pair-64u];
}
