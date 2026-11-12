#ifndef FRONTEND_PANEL_B_OVERLAY_H
#define FRONTEND_PANEL_B_OVERLAY_H
#include <stdint.h>
#ifdef __SDCC
#include <gb/gb.h>
#define PB_NATIVE_BANKED BANKED
#else
#define PB_NATIVE_BANKED
#endif
typedef struct {uint8_t bg[32],obj[24];} PBOverlayPalettes;
#define PB_OVERLAY_INVALID 65535u
uint16_t panel_b_overlay_recipe(const PBOverlayPalettes *,const uint8_t *,uint8_t) PB_NATIVE_BANKED;
uint8_t panel_b_overlay_encode(const PBOverlayPalettes *,uint16_t,const uint8_t *,uint8_t *,uint8_t *,uint8_t *) PB_NATIVE_BANKED;
#endif
