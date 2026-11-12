#ifndef PANEL_B_LOWER_FACE_H
#define PANEL_B_LOWER_FACE_H

#include <stdint.h>
#include <gb/gb.h>

/* Semantic source IDs; resolve through the pinned canonical RGB555 table.
 * Complete lower policy, revision 1. No mutable global storage.
 * Caller must sanitize commands and prove color closure before encoding.
 * This face policy has no source Stone color 10; it is not a Stone fallback.
 */
extern const uint8_t pb_lower_face_bg_ids[32];
extern const uint8_t pb_lower_face_obj_ids[24];

/* Pure preparation helper. Caller supplies a complete 32-byte 8x16 OBJ
 * plane and writable dy. Set repair_needed only if the complete aligned
 * lower plan exceeds ten objects on a line. Returns rule number (0..2).
 * Shift plane rows by -dy with clipping rejection, then admit the WHOLE
 * plan on all 144 lines. This helper alone does not admit or publish.
 * Original Y and palette are screen coordinates and indices of this policy.
 */
uint8_t pb_lower_face_origin(const uint8_t *plane, uint8_t original_y,
                             uint8_t palette, uint8_t repair_needed,
                             int8_t *dy) BANKED;

#endif
