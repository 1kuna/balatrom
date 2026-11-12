#ifndef FRONTEND_NUMERIC_H
#define FRONTEND_NUMERIC_H
#include <gb/gb.h>
/* Replace this isolated adapter when core fields become GbNum binary64.
 * Output <=11 ASCII bytes plus NUL; never feeds values back into game rules. */
void frontend_format_number(const void *value,uint8_t size,char out[16]) BANKED;
void frontend_format_number_width(const void *value,uint8_t size,char out[16],uint8_t width) BANKED;
#endif
