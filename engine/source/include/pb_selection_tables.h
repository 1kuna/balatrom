#ifndef PB_SELECTION_TABLES_H
#define PB_SELECTION_TABLES_H
#include <gb/gb.h>
#define PB_SELECTION_ABSENT 511u
#define PB_SELECTION_HIDDEN_CARD 52u
#define PB_SELECTION_STONE_CARD 53u
#define PB_SELECTION_NUMBER_HIDDEN 10000u
#define PB_SELECTION_ROLE_CHIPS 0u
#define PB_SELECTION_ROLE_MULT 1u
#define PB_SELECTION_LABEL_SELECT 12u
#define PB_SELECTION_LABEL_HIDDEN 13u
#define PB_SELECTION_LABEL_UNKNOWN 255u
#define PB_SELECTION_LINE_SELECT 1200u
#define PB_SELECTION_LINE_HIDDEN 1300u
uint16_t pb_selection_cards_record(uint8_t profile,uint8_t pair,uint16_t left,uint16_t right) BANKED;
uint16_t pb_selection_label_record(uint8_t profile,uint16_t line,uint8_t pair) BANKED;
uint16_t pb_selection_number_record(uint8_t profile,uint8_t role,uint16_t value,uint8_t pair) BANKED;
uint8_t pb_selection_label_kind(const char *s) BANKED;
#endif
