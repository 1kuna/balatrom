#include <stdint.h>
typedef uint16_t palette_color_t;
#include "../../engine/source/src/title_A_assets.inc"
#include "../../engine/source/src/frontend_font_rows.inc"
#include "../../engine/source/generated/frontend_font5_font5.inc"
#include "../../engine/source/generated/a1_font_3_a1_font3_masks.inc"
#include "../../engine/source/generated/a1_font_5_a1_font5_masks.inc"
#include "../../engine/source/generated/a1_ui_assets_a1_ui_tiles.inc"
#include "../../engine/source/generated/a1_ui_assets_a1_status_tiles.inc"
_Static_assert(sizeof(a1_ui_tiles)==1536, "base UI ABI");
_Static_assert(sizeof(a1_status_tiles)==1440, "status UI ABI");
_Static_assert(sizeof(title_A_tiles)==1952, "title ABI");
_Static_assert(sizeof(title_A_map)==360, "map ABI");
_Static_assert(sizeof(title_A_attrs)==360, "attributes ABI");
_Static_assert(sizeof(title_A_resume_patch)==28, "resume ABI");
_Static_assert(sizeof(title_A_palettes)==24, "palette ABI");
_Static_assert(sizeof(balatrom_title_digits)==50, "seed ABI");
_Static_assert(sizeof(frontend_font_rows)==640, "compact ABI");
_Static_assert(sizeof(font5)==448, "regular ABI");
_Static_assert(sizeof(a1_font3_masks)==7680, "compact masks ABI");
_Static_assert(sizeof(a1_font5_masks)==7680, "regular masks ABI");
int main(void){return title_A_map[272]!=207 || title_A_map[273]!=208 || title_A_map[274]!=209;}
