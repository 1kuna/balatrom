#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_number_012_ids.inc"

#include "pb_selection_lines_number_012_vectors.inc"

uint16_t pb_selection_lines_number_012(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<0u||value>=2048u||pair<85u||pair>91u)return 65535u;
 id=ids[value-0u];return vectors[id*7u+pair-85u];
}
