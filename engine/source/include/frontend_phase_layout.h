#ifndef FRONTEND_PHASE_LAYOUT_H
#define FRONTEND_PHASE_LAYOUT_H
#include <stdint.h>
#include "frontend_phase_chrome.h"

/* Pure paint vocabulary. No GameState, hardware, mutable storage, or escaped
 * bank pointers. Existing owned 3x5 font and four palette indices only.
 * The renderer owns allocation and placement of every returned pattern. */
enum { FP_LAYOUT_SPINE, FP_LAYOUT_SIDE_RULE, FP_LAYOUT_CONTENT_RULE };
void fp_layout_tile(uint8_t pattern,uint8_t out[16]) FP_CHROME_BANKED;

/* One two-character cell of a gold-edged primary-action strip. chars points
 * to a caller-owned RAM pair. column/cells define the vertical end borders.
 * All 15 cells are allocated by the renderer, including the blank tail. */
void fp_layout_action_tile(const char chars[2],uint8_t column,uint8_t cells,
                           uint8_t out[16]) FP_CHROME_BANKED;

/* text points to caller-owned RAM. Native public labels are <=32 characters. Word-wrap into caller RAM, with
 * a hard split only for words wider than width (1..32). Returns the next byte
 * offset, or 0 at end/invalid width. Output is always NUL terminated; no
 * non-space character is elided. Use 30 columns for the 120px phase body. */
uint8_t fp_layout_wrap_line(const char *text,uint8_t offset,uint8_t width,
                            char out[33]) FP_CHROME_BANKED;
#endif
