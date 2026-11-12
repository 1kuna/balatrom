#pragma bank 255
#include "gb_num.h"
#include <string.h>

/* Every uint32 is exactly representable in binary64. Place its bits directly
 * at mantissa bit52, then insert the biased exponent. No rounding, mutable
 * cache or shared arithmetic scratch is needed. Keep the public banked ABI. */
uint8_t gb_num_from_u32(GbNum *out,uint32_t value) CORE_BANKED {
 uint8_t high,exponent=0,shift,index,part;uint16_t biased;
 memset(out,0,sizeof(*out));if(!value)return GBN_OK;
 high=(uint8_t)(value>>24);
 if(high)exponent=24u;
 else if((high=(uint8_t)(value>>16)))exponent=16u;
 else if((high=(uint8_t)(value>>8)))exponent=8u;
 else high=(uint8_t)value;
 while(high>>1){high>>=1;exponent++;}
 shift=52u-exponent;index=shift>>3;shift&=7u;
 while(value){
  part=(uint8_t)value;out->bits[index]|=(uint8_t)(part<<shift);
  if(shift&&index<6u)out->bits[index+1u]|=part>>(8u-shift);
  value>>=8;index++;
 }
 biased=1023u+exponent;
 out->bits[6]=(out->bits[6]&15u)|((biased&15u)<<4);
 out->bits[7]=biased>>4;return GBN_OK;
}

static const uint16_t de_sources[214]={/* OWNER_INSTALL_SLOT:de_sources */};
static const uint16_t de_words[214][13]={/* OWNER_INSTALL_SLOT:de_words */};
uint8_t native_D_colors(uint16_t*p,uint8_t n,uint8_t edition,uint8_t phase) BANKED {uint16_t lo,hi,mid;uint8_t i,index;if(!edition)return 1;if(edition>4u)return 0;index=edition==4u?12u:(edition-1u)*4u+(phase&3u);for(i=0;i<n;i++){lo=0;hi=214;while(lo<hi){mid=(lo+hi)>>1;if(de_sources[mid]<p[i])lo=mid+1u;else hi=mid;}if(lo>=214||de_sources[lo]!=p[i])return 0;p[i]=de_words[lo][index];}return 1;}

static const uint8_t de_fix_keys[61][2]={/* OWNER_INSTALL_SLOT:de_fix_keys */};
static const uint16_t de_fix_words[61][4]={/* OWNER_INSTALL_SLOT:de_fix_words */};
uint8_t native_D_lookup_0(uint8_t,uint16_t*,uint8_t) BANKED;
uint8_t native_D_lookup_1(uint8_t,uint16_t*,uint8_t) BANKED;
uint8_t native_D_joker_palette(uint8_t id,uint16_t*p,uint8_t edition,uint8_t phase) BANKED {uint8_t k;if(!edition)return 1;if(edition>4u||id>=150u)return 0;k=edition==4u?12u:(edition-1u)*4u+(phase&3u);return id<75u?native_D_lookup_0(id,p,k):native_D_lookup_1(id-75u,p,k);}

#include "raster_runtime.h"
extern uint8_t native_palette_ready;
void native_D_cache_icon(uint8_t id,const uint16_t *source,uint8_t slot,uint8_t overlay,uint8_t b0,uint8_t b1,uint8_t o0,uint8_t o1,uint8_t edition) BANKED {
 uint8_t phase;uint16_t q[4],p[4],at;
 if(!edition||(!overlay&&(slot<2u||slot>5u)))return;
 for(phase=0;phase<4u;phase++){
  memcpy(q,source,8u);native_D_joker_palette(id,q,edition,phase);at=0xd400u+(uint16_t)phase*48u;
  if(overlay){p[0]=9636u;p[1]=6467u;p[2]=q[b0];p[3]=q[b1];raster_put_block(2u,at,(const uint8_t *)p,8u);p[0]=0;p[1]=q[o0];p[2]=q[o1];p[3]=0;raster_put_block(2u,at+40u,(const uint8_t *)p,8u);}
  else raster_put_block(2u,at+(uint16_t)(slot-1u)*8u,(const uint8_t *)q,8u);
 }
}
void native_D_idle(void) BANKED {
 uint8_t meta[3],phase,i;uint16_t at;
 if(native_palette_ready!=1u)return;
 raster_get_block(2u,0xd3adu,meta,3u);phase=(sys_time>>5)&3u;if((!meta[0]&&!meta[2])||phase==meta[1])return;
 raster_get_block(2u,0xd400u+(uint16_t)phase*48u,raster_pixels,48u);
 /* Publish only after VBlank, leaving the entire visible palette schedule
  * intact. The next VBlank consumes all new staging words together. */
 wait_vbl_done();
 /* Icon BG2 is consumed at line7, so publish after the entire icon band
  * has ended. Otherwise BG2 advances a frame ahead of the VBlank slots. */
 while(LY_REG<33u||LY_REG>63u){}
 for(i=0;i<5u;i++)if(meta[0]&(1u<<i)){at=i==0u?0xd300u:i==1u?0xd348u:0xd308u+(uint16_t)i*8u;raster_put_block(2u,at,raster_pixels+(uint16_t)i*8u,8u);}
 if(meta[0]&1u)raster_put_block(2u,0xd340u,raster_pixels+40u,8u);
 meta[1]=phase;raster_put_block(2u,0xd3aeu,meta+1u,1u);
}
