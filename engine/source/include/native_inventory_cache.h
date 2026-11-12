#ifndef NATIVE_INVENTORY_CACHE_H
#define NATIVE_INVENTORY_CACHE_H
#include "raster_runtime.h"
typedef struct {
 uint8_t first,count,focus,total_selected;
 uint16_t mask,wide;
 uint8_t cards[13][4],summary[5][4],summary_index[5];
 uint8_t resources[32],jokers[5],debuff,items[4],focus_bonus[8],detail_plain,preview_type;
 uint16_t preview_level;uint8_t joker_rule,consumable_rule;
} NativeCache;
typedef char cache_fits[(sizeof(NativeCache)==141u&&sizeof(NativeCache)<=sizeof(raster_loads))?1:-1];
#define cache ((NativeCache *)raster_loads)

#endif
