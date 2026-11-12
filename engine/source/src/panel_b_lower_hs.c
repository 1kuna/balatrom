#pragma bank 255
#include "panel_b_lower_hs.h"

static const uint8_t hud[12] = {
    0,19,5,12, 1,19,12,3, 1,4,14,3
};
static const uint8_t body[7][4] = {
    {0,1,2,13}, {1,2,13,18}, {0,1,10,13}, {1,10,13,18},
    {0,1,13,18}, {1,2,10,13}, {1,10,13,14}
};
static const uint8_t indices[21][5] = {
    {0,1,2,3,4}, {0,1,2,3,5}, {0,1,2,3,6}, {0,1,2,4,5},
    {0,1,2,4,6}, {0,1,2,5,6}, {0,1,3,4,5}, {0,1,3,4,6},
    {0,1,3,5,6}, {0,1,4,5,6}, {0,2,3,4,5}, {0,2,3,4,6},
    {0,2,3,5,6}, {0,2,4,5,6}, {0,3,4,5,6}, {1,2,3,4,5},
    {1,2,3,4,6}, {1,2,3,5,6}, {1,2,4,5,6}, {1,3,4,5,6},
    {2,3,4,5,6}
};
static const uint8_t objects[24] = {
    0,2,18, 0,14,18, 3,0,5, 3,19,2,
    3,19,14, 0,18,19, 0,7,19, 2,10,19
};

uint8_t pb_lower_hs_profile(uint8_t candidate, uint8_t *ids56) BANKED {
    uint8_t i,j;
    if (candidate >= PB_LOWER_HS_PROFILES) return 0;
    for (i=0; i!=12; ++i) ids56[i]=hud[i];
    for (i=0; i!=5; ++i)
        for (j=0; j!=4; ++j) ids56[12+i*4+j]=body[indices[candidate][i]][j];
    for (i=0; i!=24; ++i) ids56[32+i]=objects[i];
    return 1;
}
static const uint8_t mixed_boundary[5][56] = {
    {0,19,5,12, 1,19,12,3, 1,4,14,3, 0,1,13,18,
     1,2,10,13, 1,13,17,2, 1,13,17,14, 0,1,13,17,
     0,2,18, 0,14,18, 3,0,5, 3,19,2, 3,19,14,
     0,18,19, 0,7,19, 2,10,19},
    {0,19,5,12, 1,19,12,3, 1,4,14,3, 0,1,10,13,
     1,10,13,18, 1,2,10,13, 1,13,17,14, 0,1,17,18,
     17,0,2, 0,14,17, 3,0,5, 3,19,2, 3,19,14,
     0,18,19, 0,7,19, 2,10,19},
    {0,19,5,12, 1,19,12,3, 1,4,14,3, 0,1,2,13,
     1,2,13,18, 1,10,13,18, 0,1,13,18, 1,13,17,2,
     17,0,2, 0,14,17, 3,0,5, 3,19,2, 3,19,14,
     0,18,19, 0,7,19, 2,10,19},
    {0,19,5,12, 1,19,12,3, 1,4,14,3, 1,10,13,18,
     1,13,17,2, 1,13,17,14, 0,1,13,17, 0,1,17,18,
     17,0,2, 0,14,17, 3,0,5, 3,19,2, 3,19,14,
     0,18,19, 0,7,19, 2,10,19},
    {0,19,5,12, 1,19,12,3, 1,4,14,3, 1,2,13,18,
     1,10,13,18, 1,13,17,14, 0,1,13,17, 0,1,17,18,
     17,0,2, 0,14,17, 3,0,5, 3,19,2, 3,19,14,
     0,18,19, 0,7,19, 2,10,19}
};
uint8_t pb_lower_boundary_profile(uint8_t candidate, uint8_t *ids56) BANKED {
    uint8_t i;
    if (candidate>=5) return 0;
    for (i=0; i!=56; ++i) ids56[i]=mixed_boundary[candidate][i];
    return 1;
}
uint8_t pb_lower_obj_attribute_ok(uint8_t attribute) BANKED {
    return (attribute & 0xf8u) == 0;
}
uint8_t pb_lower_bg_attribute_ok(uint8_t attribute) BANKED {
    return (attribute & 0xf0u) == 0;
}

static const uint8_t local_masks[7][32] = {
    {0,0,0,0,0,0,0,0,0,0,0,84,0,168,0,84,
     0,168,0,84,0,168,0,84,0,168,0,84,0,168,0,84},
    {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,
     0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1},
    {0,0,0,0,0,0,0,0,0,0,0,64,0,128,0,64,
     0,128,0,64,0,128,0,64,0,128,0,64,0,128,0,64},
    {0,0,0,0,0,0,0,0,0,0,0,152,0,48,0,100,
     0,204,0,152,0,48,0,100,0,204,0,152,0,48,0,100},
    {0,0,0,0,0,0,0,0,0,0,0,128,0,0,0,64,
     0,192,0,128,0,0,0,64,0,192,0,128,0,0,0,64},
    {0,15,0,7,0,2,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     240,240,240,240,0,0,0,0,0,0,0,0,0,0,0,1}
};
static uint8_t matches(const uint8_t *plane, uint8_t mask) {
    uint8_t i;
    for (i=0; i!=32; ++i) if (plane[i]!=local_masks[mask][i]) return 0;
    return 1;
}
static const uint8_t boundary_masks[9][32] = {
    {0,0,0,0,0,0,0,0,0,0,96,96,192,192,144,144,
     48,48,96,96,192,192,144,144,48,48,96,96,192,192,144,144},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,
     0,4,0,4,0,3,0,0,0,0,0,3,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,252,0,252,0,252,0,
     252,0,252,0,252,0,252,0,252,0,252,0,252,0,252,0},
    {0,64,0,64,0,64,0,64,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {240,4,240,4,240,4,240,4,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {127,0,120,0,120,0,125,0,101,0,96,0,112,0,125,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,255,192,16,64,16,64,16,192,16,192,16},
    {0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,127,
     0,0,0,0,255,0,16,0,16,0,16,0,16,0,16,0}
};
static const uint8_t boundary_y[9] = {96,96,96,96,96,112,112,112,112};
static const uint8_t boundary_palette[9] = {0,1,0,0,0,0,0,7,1};
static const int8_t boundary_dy[9] = {1,2,4,-12,-7,-7,9,1,4};
uint8_t pb_lower_hs_origin(const uint8_t *plane, uint8_t original_y,
                           uint8_t screen_x, uint8_t attribute, uint8_t repair_needed,
                           uint8_t variant, int8_t *dy) BANKED {
    uint8_t i,j,bit;
    *dy=0;
    if (!pb_lower_obj_attribute_ok(attribute) || variant>=64) return 255;
    if (!repair_needed) return 0;
    if (variant & 32u) {
        for (j=0; j!=9; ++j) {
            if (original_y!=boundary_y[j] || attribute!=boundary_palette[j]) continue;
            for (i=0; i!=32; ++i) if (plane[i]!=boundary_masks[j][i]) break;
            if (i==32) { *dy=boundary_dy[j]; return 10+j; }
        }
    }
    if (original_y==112 && attribute==1 && (variant & 16u)) {
        if (matches(plane,6)) { *dy=1; return 9; }
        if (plane[30] || !plane[31]) return 0;
        for (i=0; i!=30; ++i) if (plane[i]) return 0;
        *dy=1; return 8;
    }
    if (original_y!=96) return 0;
    if (attribute==2) {
        bit=plane[0];
        if (plane[1] || !bit || (bit & (uint8_t)(bit-1))) return 0;
        for (i=2; i!=32; ++i) if (plane[i]) return 0;
        *dy=(variant & 1u) ? -15 : -14;
        return 1;
    }
    if (attribute==7) {
        if (matches(plane,0)) { *dy=(variant & 2u) ? 2 : 1; return 2; }
        if (matches(plane,1)) { *dy=1; return 3; }
        if (matches(plane,2)) {
            /* Last8-card overlap column of the pinned x40+14*i geometry.
             * This is a geometric mask class, never a hidden identity key. */
            *dy=(variant & 16u) ? (screen_x==136 ? 3 : 2) : ((variant & 4u) ? 3 : 2);
            return 4;
        }
    }
    if (attribute==0) {
        if (matches(plane,3)) { *dy=(variant & 8u) ? 2 : 1; return 5; }
        if (matches(plane,4)) { *dy=2; return 6; }
    }
    if (attribute==3 && matches(plane,5)) { *dy=-13; return 7; }
    return 0;
}
