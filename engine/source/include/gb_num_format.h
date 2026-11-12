#include "banking.h"
#ifndef GB_NUM_FORMAT_H
#define GB_NUM_FORMAT_H
#include "gb_num.h"
#define GBN_FORMAT_CAP 16
#define GBN_FORMAT_BAD 16
#define GBN_FORMAT_SHORT 32
/* Additive v2 renderer interface; original numeric ABI is unchanged.
 * Caller must supply 16 bytes. Exact uint32 integers render as decimal;
 * other finite values use three correctly rounded significant digits.
 * Cache output when the value changes; conversion never mutates input.
 */
uint8_t num_format(char out[GBN_FORMAT_CAP],const GbNum*value) CORE_BANKED;
/* Fully capacity checked variant, 1..6 significant scientific digits. */
uint8_t gb_num_format(char*out,uint8_t capacity,const GbNum*value,uint8_t significant_digits) CORE_BANKED;
uint8_t gb_num_format_scientific(char*out,uint8_t capacity,const GbNum*value,uint8_t significant_digits) CORE_BANKED;
#endif
