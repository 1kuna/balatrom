#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_number_004_ids.inc"

#include "pb_selection_lines_number_004_vectors.inc"

uint16_t pb_selection_lines_number_004(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<6144u||value>=8192u||pair<85u||pair>91u)return 65535u;
 id=ids[value-6144u];return vectors[id*7u+pair-85u];
}
