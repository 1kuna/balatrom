#pragma bank 255
#include <gb/gb.h>
#ifdef QA_RASTER_GUARD
__at(0xd900) volatile uint8_t qa_stack_guard[256];
uint8_t qa_guard_value=QA_RASTER_GUARD;
static uint8_t guard_initialized;
void raster_guard_init(void) BANKED {
 uint16_t i;if(guard_initialized)return;
 for(i=0;i<256u;i++)qa_stack_guard[i]=qa_guard_value;
 guard_initialized=1;
}
#endif
