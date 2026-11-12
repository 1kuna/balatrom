#ifndef PANEL_B_LOWER_HS_H
#define PANEL_B_LOWER_HS_H
#include <stdint.h>
#include <gb/gb.h>

#define PB_LOWER_HS_PROFILES 21u
#define PB_LOWER_HS_IDS 56u

/* Preparation-only policy construction. Output is one caller-owned56-byte
 * block: BG32 followed by opaque OBJ24 semantic source IDs. Invalid candidate
 * returns0 without writing. Complete color/opacity/OAM admission is mandatory.
 * Inputs must already be sanitized public commands; no concealed identity
 * enters profile selection. Current coverage excludes ordinary-face color17.
 * No storage, bank placement, handler timing or caller stack is granted here.
 */
uint8_t pb_lower_hs_profile(uint8_t candidate, uint8_t *ids56) BANKED;

/* Five additional complete policy hints with ordinary-face color17.
 * Their admission scope is the named mixed boundary witnesses only.
 * Candidate IDs0..4 are a DISTINCT resource family from the21 HS policies;
 * keys bind the family and all56 bytes, never the ordinal alone.
 */
uint8_t pb_lower_boundary_profile(uint8_t candidate, uint8_t *ids56) BANKED;

/* Canonical bitplanes only. Apply BEFORE source-origin rules or decoding.
 * OBJ: bank0, no X/Y flip, priority or reserved flags; palette bits only.
 * BG: palette bits and VBK bit3 only; no flips/priority/reserved flags.
 * A VFLIP plan is rejected, not reinterpreted as an unflipped source.
 */
uint8_t pb_lower_obj_attribute_ok(uint8_t attribute) BANKED;
uint8_t pb_lower_bg_attribute_ok(uint8_t attribute) BANKED;

/* Small guarded preparation candidates, not a scheduler/admission result.
 * Variant0..63 chooses bounded origin alternatives for exact local
 * masks. Full attribute byte is required: noncanonical flags return255.
 * Return0 means no match,1..18 a matched rule. Output dy is caller-owned,
 * nonnull, and disjoint from the32-byte read-only plane.
 * Apply only on aligned whole-plan overflow; shift rows oppositely without
 * clipping and re-admit ALL144 lines. Keys bind variant and complete policy.
 */
uint8_t pb_lower_hs_origin(const uint8_t *plane, uint8_t original_y,
                           uint8_t screen_x, uint8_t attribute, uint8_t repair_needed,
                           uint8_t variant, int8_t *dy) BANKED;
#endif
