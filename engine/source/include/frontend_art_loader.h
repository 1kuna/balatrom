#ifndef FRONTEND_ART_LOADER_H
#define FRONTEND_ART_LOADER_H
#include <gb/gb.h>
/* Fixed-bank loader restores caller's renderer bank before returning. */
void frontend_load_icon(uint8_t x,uint8_t y,uint8_t slot,uint8_t family,uint8_t id) NONBANKED;
#endif
