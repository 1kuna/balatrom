/* Cash-Out Ledger tiles, result ids 5..16, 2bpp.
 * Ids 5..12 are native rounded plates, lips and bars.
 * Ids 13..16 are the TABLE felt texture: source tiles a1_ui_tiles 40,41,47,48
 * (colour index1 only), re-expressed as colour3 for rpal palette1, which carries
 * the TABLE dark felt (3,10,6) in colour3. Palette roles:
 * pal0 0 slate 1 felt 2 paper 3 gold; pal1 0 felt 1 red 2 paper 3 dark felt;
 * pal2 0 felt 1 blue 2 paper 3 gold. */
#define RL_LIP_END 5u
#define RL_LIP_MID 6u
#define RL_PLATE_CORNER 7u
#define RL_PLATE_EDGE 8u
#define RL_PLATE_SIDE 9u
#define RL_BAR_CAP 10u
#define RL_BAR_EMPTY 11u
#define RL_BAR_FULL 12u
#define RL_TEX 13u
/* TABLE base map parity: cell (odd,odd)=40, (even,odd)=41, (odd,even)=47, (even,even)=48. */
#define RL_TEXAT(x,y) (uint8_t)(RL_TEX+(((y)&1u)?0u:2u)+(((x)&1u)?0u:1u))
#define RL_TILE_COUNT 12u
static const uint8_t rledger_tiles[192]={
0,0,0,0,0,0,0,0,0,0,0,0,63,0,127,0,
0,0,0,0,0,0,0,0,0,0,0,0,255,0,255,0,
0,0,0,0,0,0,0,0,7,7,15,8,31,16,31,16,
0,0,0,0,0,0,0,0,255,255,255,0,255,0,255,0,
31,16,31,16,31,16,31,16,31,16,31,16,31,16,31,16,
255,0,255,0,224,0,192,0,192,0,224,0,255,0,255,0,
255,0,255,0,0,0,0,0,0,0,0,0,255,0,255,0,
255,0,255,0,255,255,255,255,255,255,255,255,255,0,255,0,
128,128,128,128,128,128,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,1,1,2,2,4,4,8,8,32,32,
0,0,2,2,8,8,16,16,32,32,64,64,128,128,128,128,
128,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
