#include "banking.h"
#ifndef GB_NUM_SIGNED_H
#define GB_NUM_SIGNED_H
#include "gb_num.h"
/* Additive v2 signed wrappers. Freeze original positive core unchanged.
 * Stored representation remains 8 bytes; negative finite values and -0 are
 * supported without globally clamping effects. Canonical NaN is flagged;
 * original platform-specific NaN payload/sign propagation is not claimed.
 */
uint8_t gb_num_from_i32(GbNum*out,int32_t value) CORE_BANKED;
uint8_t gb_num_add_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_sub_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_mul_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_div_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED;
uint8_t gb_num_add_i16_signed(GbNum*out,const GbNum*a,int16_t value) CORE_BANKED;
int8_t gb_num_cmp_signed(const GbNum*a,const GbNum*b) CORE_BANKED;
#endif
