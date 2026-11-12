#ifndef NATIVE_RUN_RNG_H
#define NATIVE_RUN_RNG_H
#include <stdint.h>
#include "banking.h"
uint8_t native_rng_valid(const uint8_t[20]) CORE_BANKED;
void native_rng_export(uint8_t[20]) CORE_BANKED;
uint8_t native_rng_import(const uint8_t[20]) CORE_BANKED;
uint8_t native_rng_begin(uint16_t) CORE_BANKED;
uint8_t native_rng_mode(void) CORE_BANKED;
uint8_t native_rng_mode_available(uint8_t) CORE_BANKED;
void native_rng_pc_warning(char[33]) CORE_BANKED;
uint8_t native_rng_step(uint8_t[20],uint16_t,uint16_t*) CORE_BANKED;
uint8_t native_rng_step_count(uint8_t[20],uint16_t,uint16_t*,uint32_t*) CORE_BANKED;
uint8_t native_rng_roll(uint16_t,uint16_t*) CORE_BANKED;
uint16_t game_random_index(uint16_t) CORE_BANKED;
#endif
