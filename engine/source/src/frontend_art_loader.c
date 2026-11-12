#include "frontend_art_loader.h"
#include "sidebar_items.h"
#include "sidebar_jokers.h"
#include "frontend_resolve.h"
#include <gb/cgb.h>
#include "shop154.h"
#define set_bkg_data shop154_data
#define set_bkg_tiles shop154_tiles
#define set_bkg_palette shop154_palette
void frontend_load_icon(uint8_t x,uint8_t y,uint8_t slot,uint8_t family,uint8_t id) NONBANKED {uint8_t i,saved,bank,base,map[6],attrs[6];const uint8_t *tiles;const uint16_t *palette;SidebarJokerRef ref;if(slot>=5||!frontend_resolve_icon(family,id,&ref))return;bank=ref.bank;tiles=ref.tiles;palette=ref.palette;saved=CURRENT_BANK;base=45+slot*6;SWITCH_ROM(bank);set_bkg_data(base,6,tiles);if(_cpu==CGB_TYPE)set_bkg_palette(3+slot,1,palette);SWITCH_ROM(saved);for(i=0;i<6;i++){map[i]=base+i;attrs[i]=3+slot;}VBK_REG=0;set_bkg_tiles(x,y,2,3,map);if(_cpu==CGB_TYPE){VBK_REG=1;set_bkg_tiles(x,y,2,3,attrs);VBK_REG=0;}}
