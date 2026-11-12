#include "banking.h"
#ifndef GB_NUM_DECIMAL_H
#define GB_NUM_DECIMAL_H
#include "gb_num.h"
/* Internal exact comparison to odd * 10^power / 2. 2 means unresolved.
 * Inputs must be in RAM when the function is banked. */
int8_t gb_num_decimal_cmp(const GbNum*n,uint32_t odd,int16_t power) CORE_BANKED;
#endif
