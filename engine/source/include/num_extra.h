#include "banking.h"
#ifndef NUM_EXTRA_H
#define NUM_EXTRA_H
#include "gb_num.h"
uint8_t num_floor(GbNum *out,const GbNum *value) CORE_BANKED;
uint8_t num_to_u32_exact(uint32_t *out,const GbNum *value) CORE_BANKED;
#endif
