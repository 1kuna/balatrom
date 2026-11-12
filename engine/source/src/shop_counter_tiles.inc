/* Shop Counter static tiles. Ids 0..3 are native frame and focus geometry.
 * Colour indices are palette-relative:
 * pal7 0 felt 1 TABLE well/dark felt 2 gold 3 paper; pal1 0 felt 1 red 2 paper 3 gold.
 * Ids 4..7: TABLE felt texture, byte-identical to a1_ui_tiles 40,41,47,48
 * (colour1 = palette7 dark felt). */
#define SC_CORNER 0u
#define SC_FILL 1u
#define SC_FOCUS 2u
#define SC_BULB 3u
#define SC_TEX 4u
/* TABLE base map parity: cell (odd,odd)=40, (even,odd)=41, (odd,even)=47, (even,even)=48. */
#define SC_TEXAT(x,y) (uint8_t)(SC_TEX+(((y)&1u)?0u:2u)+(((x)&1u)?0u:1u))
#define SC_TILE_COUNT 8u
static const uint8_t sc_tiles[128]={
63,0,127,0,255,0,255,0,255,0,255,0,255,0,255,0,
255,0,255,0,255,0,255,0,255,0,255,0,255,0,255,0,
255,0,255,0,224,31,192,63,192,63,224,31,255,0,255,0,
255,0,255,0,255,24,231,60,231,60,255,24,255,0,255,0,
128,0,128,0,128,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,1,0,2,0,4,0,8,0,32,0,
0,0,2,0,8,0,16,0,32,0,64,0,128,0,128,0,
128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
