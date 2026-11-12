/* Private live-sidebar cache. No state/RNG/SRAM writes. A real WIN scene
 * supplies glyph pixels; exact current-state key and narrow integer display
 * scope must match before the hidden SHOP map can borrow them. */
static uint8_t shop157_ready,shop157_key_target[8],shop157_key_score[8];
static int32_t shop157_key_money;
static uint8_t shop157_key_hands,shop157_key_discards,shop157_key_ante,shop157_key_blind;
static uint8_t shop157_maps[90],shop157_attrs[90],shop157_lookup[128];
static void shop157_tile_read(uint8_t,uint8_t *);
static void shop157_tile_write(uint8_t,const uint8_t *);
static uint8_t shop157_integer(const void*value){const uint8_t*p=value;uint16_t e,frac;uint8_t i;if(p[7]&128u)return 0;e=((uint16_t)p[7]<<4)|(p[6]>>4);if(!e){for(i=0;i<8;i++)if(p[i])return 0;return 1;}if(e<1023u||e>1038u||(p[0]|p[1]|p[2]|p[3]|(p[4]&15u)))return 0;frac=((uint16_t)(p[6]&15u)<<12)|((uint16_t)p[5]<<4)|(p[4]>>4);return !(frac&(0xffffu>>(e-1023u)));}
static void shop157_record(const GameState*g){shop157_ready=0;memcpy(shop157_key_target,&g->target,8);memcpy(shop157_key_score,&g->score,8);shop157_key_money=g->money;shop157_key_hands=g->hands;shop157_key_discards=g->discards;shop157_key_ante=g->ante;shop157_key_blind=g->blind;}
uint8_t shop157_sidebar(const GameState*g,uint8_t first,uint8_t*next) BANKED {uint8_t x,y,i,id,out,zero,pixels[16];uint8_t *map,*attr;
 if(!shop157_ready||!shop154_staging||g!=&game||g->phase!=PHASE_SHOP||g->joker_count||!shop157_integer(&g->target)||!shop157_integer(&g->score)||memcmp(shop157_key_target,&g->target,8)||memcmp(shop157_key_score,&g->score,8)||shop157_key_money!=g->money||shop157_key_hands!=g->hands||shop157_key_discards!=g->discards||shop157_key_ante!=g->ante||shop157_key_blind!=g->blind)return 0;
 /* Maps are recorded from the real result staging writes before publication. */
 memset(shop157_lookup,255,sizeof(shop157_lookup));shop157_lookup[0]=0;out=first;
 /* Preflight all identifiers before writing any hidden pixel or map. The
  * source sidebar must live in positive signed slots in VRAM0 only. */
 for(i=0;i<90;i++){id=shop157_maps[i];if(id>=128u||(shop157_attrs[i]&0xf8u))return 0;if(shop157_lookup[id]==255u){shop157_lookup[id]=254u;out++;}}
 if(out>200u)return 0;memset(shop157_lookup,255,sizeof(shop157_lookup));shop157_lookup[0]=0;out=first;VBK_REG=0;
 for(y=0;y<18;y++){VBK_REG=0;map=shop157_maps+y*5u;attr=shop157_attrs+y*5u;for(x=0;x<5;x++){id=map[x];if(shop157_lookup[id]==255u){shop157_tile_read(id,pixels);zero=1;for(i=0;i<16;i++)if(pixels[i]){zero=0;break;}if(zero)shop157_lookup[id]=0;else{shop157_lookup[id]=out;shop157_tile_write(out++,pixels);}}map[x]=shop157_lookup[id];attr[x]&=7u;}VBK_REG=0;shop154_tiles(0,y,5,1,map);VBK_REG=1;shop154_tiles(0,y,5,1,attr);}
 VBK_REG=0;shop157_ready=0;*next=out;return 1;
}
