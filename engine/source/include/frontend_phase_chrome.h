#ifndef FRONTEND_PHASE_CHROME_H
#define FRONTEND_PHASE_CHROME_H
#include <stdint.h>
#ifdef __SDCC
#define FP_CHROME_BANKED __banked
#else
#define FP_CHROME_BANKED
#endif

/* Presentation vocabulary, deliberately independent of game.h values. */
enum { FP_CHROME_SHOP, FP_CHROME_PACK, FP_CHROME_BLIND_WIN,
       FP_CHROME_RUN_LOSS, FP_CHROME_ANTE8_WIN, FP_CHROME_INVENTORY,
       FP_CHROME_JOKERS };
enum { FP_CHROME_BUFFOON, FP_CHROME_CELESTIAL, FP_CHROME_ARCANA,
       FP_CHROME_STANDARD, FP_CHROME_SPECTRAL };
enum { FP_CHROME_TEXT, FP_CHROME_HEADING };
enum { FP_KEY_SHOP_TITLE=1, FP_KEY_SHOP_BUY, FP_KEY_SHOP_SECONDARY,
       FP_KEY_PACK_TITLE, FP_KEY_PACK_ACTIONS,
       FP_KEY_BLIND_TITLE, FP_KEY_BLIND_SCORE_LABEL, FP_KEY_BLIND_ACTIONS,
       FP_KEY_LOSS_TITLE, FP_KEY_LOSS_MESSAGE, FP_KEY_LOSS_ACTIONS,
       FP_KEY_ANTE8_TITLE, FP_KEY_ANTE8_MESSAGE, FP_KEY_ANTE8_ACTIONS,
       FP_KEY_ITEMS_TITLE, FP_KEY_ITEMS_ACTIONS, FP_KEY_ITEMS_SECONDARY,
       FP_KEY_JOKERS_TITLE, FP_KEY_JOKERS_ACTIONS, FP_KEY_JOKERS_SECONDARY };

/* Exactly 31 bytes. The caller owns this temporary, preferably on its stack.
 * Strings are copied into RAM; no pointer into a switched ROM bank escapes.
 * cells is the map width: HEADING allocates that many HUD patterns; the existing
 * TEXT writer reuses its fill pattern after the string ends. Footer commands
 * keep a 15-cell band inside the 120px phase body. All commands occupy existing right text
 * rows. A renderer which
 * retains those rows must invalidate their former extents before replacing them.
 */
typedef struct {
    uint8_t key,x,y,cells,palette,style;
    char text[25];
} FPChromeCommand;

/* Zero-based ordinal. Returns 1 for a command or 0 and a zeroed output for end/
 * invalid view. Family is consulted only for PACK; unknown means SPECTRAL.
 * Caller maps game phases/item kinds explicitly. No hardware or state access.
 */
uint8_t fp_chrome_command(uint8_t view,uint8_t family,uint8_t ordinal,
                         FPChromeCommand *out) FP_CHROME_BANKED;

/* Two adjacent 8x8 BG patterns, 32 copied bytes. Teal index 1 background with
 * a steady gold index 3, 7x4 downward caret centred in a 16x8 marker footprint.
 * Caller allocates both patterns through its existing HUD allocator and owns
 * map placement/attributes/cache keys. No reserved IDs or palette writes.
 * Intended for public SHOP/PACK focus only, not card/forced-Bell cues.
 */
void fp_chrome_focus_tiles(uint8_t out[32]) FP_CHROME_BANKED;

/* Pre-rasterized existing 5px font for HEADING commands. Copies one 16-byte
 * pattern to a caller buffer. column is 0..command.cells-1. Valid -> 1; invalid
 * key/column -> 0 with zeroed output. PACK title additionally uses family.
 * This avoids the legacy heading routine's per-pixel division at render time.
 * Caller retains ownership of HUD pattern IDs, maps, attributes and palettes.
 */
uint8_t fp_chrome_heading_tile(uint8_t key,uint8_t family,uint8_t column,
                               uint8_t out[16]) FP_CHROME_BANKED;
#endif
