#ifndef PANEL_B_NARROW_NATIVE_H
#define PANEL_B_NARROW_NATIVE_H
#include <gb/gb.h>
#include "frontend_panel_b_overlay.h"
/* Face-policy and pixel-plane rules for narrow overlay repair.
 * Caller first rejects the aligned whole plan, regenerates original planes,
 * shifts by -dy with opaque clipping rejection and re-admits all144 loads.
 * Zero dy can override an older generic rule. This never admits or publishes. */
uint8_t pb_narrow_face_origin(const PBOverlayPalettes *,const uint8_t *,uint8_t,uint8_t,uint8_t,int8_t *) BANKED;
#endif
