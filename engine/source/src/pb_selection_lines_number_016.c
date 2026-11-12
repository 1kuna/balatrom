#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_number_016_ids.inc"

#include "pb_selection_lines_number_016_vectors.inc"

uint16_t pb_selection_lines_number_016(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<8192u||value>=10001u||pair<85u||pair>91u)return 65535u;
 id=ids[value-8192u];return vectors[id*7u+pair-85u];
}
