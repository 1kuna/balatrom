#pragma bank 45
#include <gb/gb.h>
uint8_t pb_direct_read_29(uint16_t slot,uint16_t *r) BANKED {(void)slot;(void)r;return 0;}
uint8_t pb_direct_group_29(uint8_t n,uint16_t *r) BANKED {(void)n;(void)r;return 0;}

#include <string.h>
static const uint16_t np_palette_0[75][14][4]={/* OWNER_INSTALL_SLOT:np_palette_0 */};
uint8_t native_D_lookup_0(uint8_t id,uint16_t*p,uint8_t variant) BANKED {uint8_t i;if(id>=75u||variant>=13u)return 0;for(i=0;i<4u;i++)if(p[i]!=np_palette_0[id][0][i])return 0;memcpy(p,np_palette_0[id][variant+1u],8u);return 1;}
