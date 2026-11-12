#pragma bank 255
#include "frontend_panel_b_overlay.h"
/* Solve palette assignment and encode disjoint bit planes.
 * Return the pure per-pair recipe; the renderer owns complete admission. */
static const uint16_t bits[21]={1,2,4,8,16,32,0,64,0,0,8192,0,128,256,512,0,0,1024,2048,4096,0};
static uint16_t palette_mask(const uint8_t *p,uint8_t n) {
    uint16_t m=0;uint8_t i;
    for(i=0;i<n;i++){if(p[i]>20u||!bits[p[i]])return 0;m|=bits[p[i]];}
    return m;
}
static uint8_t population(uint16_t mask) {
    uint8_t n=0;while(mask){mask&=mask-1u;n++;}return n;
}
uint16_t panel_b_overlay_recipe(const PBOverlayPalettes *f,const uint8_t *pixels,uint8_t maximum) PB_NATIVE_BANKED {
    uint16_t masks[2]={0,0},bm[8],om[8],missing,union_mask;
    uint8_t i,t,a,j,k,layers,choice[2],cost[2],n,total,best_cost;
    uint8_t best_a=0,best_b=0,best_j=0,best_k=0,found;
    if(maximum>2u)return PB_OVERLAY_INVALID;
    for(i=0;i<128u;i++){
        if(pixels[i]>20u||!bits[pixels[i]]){return PB_OVERLAY_INVALID;}
        masks[i>>6]|=bits[pixels[i]];
    }
    for(i=0;i<8u;i++){
        bm[i]=palette_mask(f->bg+i*4u,4);om[i]=palette_mask(f->obj+i*3u,3);
        if(!bm[i]||!om[i]){return PB_OVERLAY_INVALID;}
    }
    for(t=0;t<2u;t++){
        choice[t]=255u;
        for(a=0;a<8u;a++)if(!(masks[t]&~bm[a])){choice[t]=a;break;}
    }
    if(choice[0]!=255u&&choice[1]!=255u){
        return (uint16_t)choice[0]|((uint16_t)choice[1]<<3);
    }
    for(layers=1;layers<=maximum;layers++){
        found=0;best_cost=255u;
        for(j=0;j<8u;j++)for(k=layers==1u?j:j+1u;k<8u;k++){
            union_mask=om[j]|om[k];
            for(t=0;t<2u;t++){
                choice[t]=cost[t]=255u;
                for(a=0;a<8u;a++){
                    missing=masks[t]&~bm[a];if(missing&~union_mask)continue;
                    n=population(missing);
                    if(n<cost[t]){cost[t]=n;choice[t]=a;}
                }
            }
            if(choice[0]!=255u&&choice[1]!=255u){
                total=cost[0]+cost[1];
                if(!found||total<best_cost||(total==best_cost&&
                   (choice[0]<best_a||(choice[0]==best_a&&choice[1]<best_b)))){
                    found=1;best_cost=total;best_a=choice[0];best_b=choice[1];best_j=j;best_k=k;
                }
            }
            if(layers==1u)break;
        }
        if(found){
            return (uint16_t)best_a|((uint16_t)best_b<<3)|
                ((uint16_t)best_j<<6)|((uint16_t)(layers==2u?best_k:0u)<<9)|((uint16_t)layers<<12);
        }
    }
    return PB_OVERLAY_INVALID;
}
static uint8_t member(const uint8_t *p,uint8_t n,uint8_t c){
    uint8_t i;for(i=0;i<n;i++)if(p[i]==c)return i;return 255u;
}
uint8_t panel_b_overlay_encode(const PBOverlayPalettes *p,uint16_t r,const uint8_t *pixels,
                          uint8_t *bg,uint8_t *first,uint8_t *second) PB_NATIVE_BANKED {
    uint8_t i,x,y,c,b,o1,o2,bit,si,di,layers=(r>>12)&3u;const uint8_t *pal;
    if(r==PB_OVERLAY_INVALID||(r&49152u)||layers>2u)return 0;
    for(i=0;i<128u;i++){
        c=pixels[i];if(c>20u||!bits[c])return 0;
        pal=p->bg+((i<64u?r:r>>3)&7u)*4u;
        if(member(pal,4,c)!=255u)continue;
        if(layers&&member(p->obj+((r>>6)&7u)*3u,3,c)!=255u)continue;
        if(layers==2u&&member(p->obj+((r>>9)&7u)*3u,3,c)!=255u)continue;
        return 0;
    }
    for(i=0;i<32u;i++)bg[i]=first[i]=second[i]=0;
    for(y=0;y<16u;y++){
        pal=p->bg+((y<8u?r:r>>3)&7u)*4u;
        for(x=0;x<8u;x++){
            si=y;si*=8u;si+=x;c=pixels[si];b=member(pal,4,c);o1=o2=0;
            if(b==255u){
                b=0;o1=member(p->obj+((r>>6)&7u)*3u,3,c);
                if(o1!=255u)o1++;
                else{o1=0;o2=member(p->obj+((r>>9)&7u)*3u,3,c)+1u;}
            }
            bit=128u>>x;di=y;di*=2u;
            if(b&1u)bg[di]|=bit;if(b&2u)bg[di+1u]|=bit;
            if(o1&1u)first[di]|=bit;if(o1&2u)first[di+1u]|=bit;
            if(o2&1u)second[di]|=bit;if(o2&2u)second[di+1u]|=bit;
        }
    }
    return 1;
}
