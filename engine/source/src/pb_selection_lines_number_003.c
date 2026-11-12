#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_lines_number_003_ids.inc"

#include "pb_selection_lines_number_003_vectors.inc"

uint16_t pb_selection_lines_number_003(uint16_t value,uint8_t pair) BANKED {
 uint16_t id;
 if(value<4096u||value>=6144u||pair<85u||pair>91u)return 65535u;
 id=ids[value-4096u];return vectors[id*7u+pair-85u];
}
