#pragma bank 255
#include "frontend_panel_b_overlay.h"
#include "pb_lower_rom_cache.h"
/* Exact generic ordering. Walk each half directly, reuse its zero-layer BG,
 * and reject non-improving BG costs before testing OBJ coverage. */
static const uint16_t bits[21]={1,2,4,8,16,32,0,64,0,0,8192,0,128,256,512,0,0,1024,2048,4096,0};
static uint16_t palette_mask(const uint8_t *p,uint8_t n){uint8_t i;uint16_t m=0;for(i=0;i<n;i++){if(p[i]>20u||!bits[p[i]])return 0;m|=bits[p[i]];}return m;}
static uint8_t pop(uint16_t v){uint8_t n=0;while(v){v&=v-1u;n++;}return n;}
static uint16_t generic_lower_recipe(const PBOverlayPalettes *f,const uint8_t *pixels,uint8_t maximum) {
 uint16_t masks[2],om[8],missing[2][8],bm,un;
 const uint16_t *mp;
 const uint8_t *cp;
 uint8_t costs[2][8],zero[2],i,t,a,j,k,layers,choice[2],cost[2],total,best=255,ba=0,bb=0,bj=0,bk=0,found;
 for(t=0;t<2u;t++){
  bm=0;
  for(i=0;i<64u;i++,pixels++){a=*pixels;if(a>20u||!bits[a])return 65535u;bm|=bits[a];}
  masks[t]=bm;
 }
 for(a=0;a<8u;a++){
  bm=palette_mask(f->bg+a*4u,4);om[a]=palette_mask(f->obj+a*3u,3);if(!bm||!om[a])return 65535u;
  for(t=0;t<2u;t++){missing[t][a]=masks[t]&~bm;costs[t][a]=pop(missing[t][a]);}
 }
 for(t=0;t<2u;t++){zero[t]=255;mp=missing[t];for(a=0;a<8u;a++,mp++)if(!*mp){zero[t]=a;break;}}
 if(zero[0]!=255u&&zero[1]!=255u)return (uint16_t)zero[0]|((uint16_t)zero[1]<<3);
 for(layers=1;layers<=maximum;layers++){
  found=0;best=255;
  for(j=0;j<8u;j++)for(k=layers==1u?j:j+1u;k<8u;k++){
   un=~(om[j]|om[k]);
   for(t=0;t<2u;t++){
    if(zero[t]!=255u){choice[t]=zero[t];cost[t]=0;continue;}
    choice[t]=cost[t]=255;
    mp=missing[t];cp=costs[t];
    for(a=0;a<8u;a++,mp++,cp++)if(*cp<cost[t]&&!(*mp&un)){cost[t]=*cp;choice[t]=a;}
   }
   if(choice[0]!=255u&&choice[1]!=255u){
    total=cost[0]+cost[1];
    if(!found||total<best||(total==best&&(choice[0]<ba||(choice[0]==ba&&choice[1]<bb)))){found=1;best=total;ba=choice[0];bb=choice[1];bj=j;bk=k;}
   }
   if(layers==1u)break;
  }
  if(found)return (uint16_t)ba|((uint16_t)bb<<3)|((uint16_t)bj<<6)|((uint16_t)(layers==2u?bk:0u)<<9)|((uint16_t)layers<<12);
 }
 return 65535u;
}

/* Isolated stack-lifetime canary: ROM lookup precedes generic local arrays. */
uint16_t raster_lower_recipe(const PBOverlayPalettes *f,const uint8_t *pixels,uint8_t maximum) PB_NATIVE_BANKED {
 uint16_t recipe;
 if(maximum>2u)return 65535u;
 if(pb_lower_rom_recipe(f,pixels,maximum,&recipe))return recipe;
 return generic_lower_recipe(f,pixels,maximum);
}
