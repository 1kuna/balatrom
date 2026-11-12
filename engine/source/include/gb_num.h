#include "banking.h"
#ifndef GB_NUM_H
#define GB_NUM_H
#include <stdint.h>
/* Exact nonnegative IEEE binary64, little endian. No native floating helpers.
   Negative operands (including negative zero) are explicitly outside this API.
   Numeric functions share a 64-byte scratch bank: nonreentrant, not for ISR use.
   Outputs may alias inputs. NaN/negative domain failures are explicit. */
typedef struct {uint8_t bits[8];} GbNum;
#define GBN_OK 0
#define GBN_DOMAIN 1
#define GBN_NAN 2
#define GBN_INF 4
#define GBN_DIVZERO 8
#define GBN_UNORDERED 2
uint8_t gb_num_from_u32(GbNum*out,uint32_t value) CORE_BANKED;
uint8_t gb_num_add(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_sub_clamp(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_mul(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_div(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
/* Negative increments reject if the resulting value would be negative.
   sub_clamp is opt-in ONLY for original effects that explicitly clamp. */
uint8_t gb_num_add_i16(GbNum*out,const GbNum*a,int16_t value) CORE_BANKED;
uint8_t gb_num_ratio(GbNum*out,uint16_t numerator,uint16_t denominator) CORE_BANKED;
/* Compute factor=round64(numerator/denominator), then round64(a*factor).
   This intentionally does NOT compute round64(a*numerator)/denominator. */
uint8_t gb_num_mul_ratio(GbNum*out,const GbNum*a,uint16_t numerator,uint16_t denominator) CORE_BANKED;
int8_t gb_num_cmp(const GbNum*a,const GbNum*b) CORE_BANKED;
#endif
