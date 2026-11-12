#ifndef RASTER_RUNTIME_H
#define RASTER_RUNTIME_H
#include <gb/gb.h>
#include <stdint.h>
#include "game.h"
#include "hand_preview.h"
#ifndef RASTER_RUNTIME_IMPLEMENTATION
#define raster_live (*(volatile uint8_t *)0xffd1)
#define raster_fault (*(volatile uint8_t *)0xffb5)
#define raster_active_bank (*(volatile uint8_t *)0xffd0)
#define raster_build_bank (*(volatile uint8_t *)0xffbf)
#endif
extern uint8_t raster_error, raster_upper_count, raster_lower_count, raster_peak;
extern volatile uint16_t raster_published;
extern uint8_t raster_dirty_pairs;
extern uint8_t raster_pixels[128], raster_planes[96], raster_loads[144];
void raster_put(uint16_t,uint8_t) NONBANKED;
uint8_t raster_get(uint16_t) NONBANKED;
void raster_put_block(uint8_t,uint16_t,const uint8_t *,uint8_t) NONBANKED;
void raster_get_block(uint8_t,uint16_t,uint8_t *,uint8_t) NONBANKED;
void raster_begin_publish(void) NONBANKED;
void raster_end_publish(void) NONBANKED;
uint8_t raster_transfer(uint8_t,uint16_t,uint16_t,uint8_t,uint8_t) NONBANKED;
uint8_t raster_publish_maps(uint8_t) NONBANKED;
uint8_t raster_publish_jobs(uint8_t) NONBANKED;
void raster_stage_bg(uint8_t,const uint8_t *) BANKED;
uint8_t raster_publish_prepared(const uint8_t *) BANKED;
void raster_stop(void) NONBANKED;
void raster_capture_inputs(void) NONBANKED;
void raster_legacy_reset(void) BANKED;
/* Preview hint retained for ABI compatibility; fresh core preview comes
 * from the stable authoritative GameState before public commands/cache. */
uint8_t raster_render(const GameState *, const HandPreview *) BANKED;
#endif
