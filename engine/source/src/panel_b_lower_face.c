#pragma bank 255
#include "panel_b_lower_face.h"

const uint8_t pb_lower_face_bg_ids[32] = {
    0,19,5,12, 1,19,12,3, 1,4,14,3, 0,1,17,18,
    1,13,17,0, 1,13,17,2, 1,13,17,14, 1,2,13,18
};
const uint8_t pb_lower_face_obj_ids[24] = {
    1,3,13, 0,13,17, 17,0,2, 0,14,17,
    3,19,2, 3,19,14, 0,18,19, 7,14,19
};

static const uint8_t red_seam[32] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
    0,4,0,4,0,3,0,0,0,0,0,3,0,0,0,0
};

uint8_t pb_lower_face_origin(const uint8_t *plane, uint8_t original_y,
                             uint8_t palette, uint8_t repair_needed,
                             int8_t *dy) BANKED {
    uint8_t i, bit;
    *dy = 0;
    if (!repair_needed || original_y != 96) return 0;
    if (palette == 0) {
        bit = plane[1];
        if (plane[0] || !bit || (bit & (uint8_t)(bit - 1))) return 0;
        for (i = 2; i != 32; ++i) if (plane[i]) return 0;
        *dy = -14;
        return 1;
    }
    if (palette == 3) {
        for (i = 0; i != 32; ++i) if (plane[i] != red_seam[i]) return 0;
        *dy = 2;
        return 2;
    }
    return 0;
}
