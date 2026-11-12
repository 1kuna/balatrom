#ifndef PB_SELECTION_DISPATCH_H
#define PB_SELECTION_DISPATCH_H
#include "frontend_panel_b_internal.h"

/* Invocation-local admission flags plus profile10 bit only. Public Frame supplies
 * identities on demand; no descriptor remains live above ordinary fallback. */
uint8_t pb_selection_dispatch_begin(const Frame *, uint8_t) BANKED;
uint16_t pb_selection_dispatch_record(const Frame *, uint8_t,
                                     uint8_t) BANKED;
#endif
