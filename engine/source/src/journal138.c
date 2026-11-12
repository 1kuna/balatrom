#include "journal138.h"
uint16_t pub138_bucket(uint8_t bank,uint16_t address) PUB138_API {
 if((bank==5u||bank==6u)&&address>=0xd000u&&address<0xe000u)
  return (uint16_t)(bank-5u)*256u+(address-0xd000u)/16u;
 if(bank==7u&&address>=0xd200u&&address<0xda00u)
  return 512u+(address-0xd200u)/16u;
 if(bank==3u&&address>=0xd300u&&address<0xd900u)
  return 640u+(address-0xd300u)/16u;
 return 65535u;
}
uint8_t pub138_cost_admits(uint8_t count,uint8_t blocks) PUB138_API {
 /* the cost model (fast-M GDMA 16/block,
  * 48/job; PUBLICATION_BUDGET196 rows equal it) is the time limit. The
  * block ceiling is only a sanity bound: cost<=1986 admits at most 61
  * blocks in one job; 48 keeps all plans within measured territory. */
 if(!count||count>14u||!blocks||blocks>48u)return 0;
 return 956u+(uint16_t)count*48u+(uint16_t)blocks*16u<=1986u;
}
static uint8_t marked(const uint8_t *bits,uint16_t i){return !!(bits[i/8u]&(1u<<(i&7u)));}
/* budget-driven gap bridging. Exact runs are tried first;
 * an admissible exact plan is emitted unchanged. Only when the exact plan
 * would fail (and coop would fall back to the LCD-off full transfer) are
 * adjacent runs merged, smallest gap first: each merge removes one job (48)
 * and adds one or two unjournaled blocks (16 each), so modelled VBlank cost
 * strictly falls. Unjournaled staging equals visible VRAM while coop is
 * resident (cancel restores journaled blocks; full transfer copies all), so
 * bridged copies are pixel-identical. The bank6 tile segment holding the
 * directly uploaded WAIT patterns is never bridged. All caps and the cost
 * model are unchanged. Run scratch is static: main-only, nonreentrant. */
static uint16_t run_start[24];static uint8_t run_len[24];
static uint16_t segment_end(uint16_t i){return i<256u?256u:i<512u?512u:i<576u?576u:640u;}
uint8_t pub138_make_jobs(const uint8_t *bits,uint8_t *out,uint8_t *count,uint8_t *total) PUB138_API {
 uint16_t i=0,start,end,source,dest,b=0;uint8_t n,bank,vbk,k=0,j,best,gap,best_gap;
 *count=*total=0;
 while(i<640u){
  /* A zero byte cannot contain a job; all segment limits align to8. */
  if(!bits[i>>3]){i=(i+8u)&~7u;continue;}
  if(!marked(bits,i)){i++;continue;}
  start=i;end=segment_end(i);
  n=0;do{i++;n++;}while(i<end&&n<128u&&marked(bits,i));
  if(k>=24u||n>48u)return 0;
  run_start[k]=start;run_len[k]=n;k++;b+=n;
 }
 if(b>48u)return 0;
 while(k>14u||!pub138_cost_admits(k,(uint8_t)b)){
  best=255u;best_gap=3u;
  for(j=0;j+1u<k;j++){
   start=run_start[j];end=segment_end(start);
   if(end==512u||run_start[j+1u]>=end)continue;
   gap=(uint8_t)(run_start[j+1u]-start-run_len[j]);
   if(gap<best_gap&&run_len[j]+gap+run_len[j+1u]<=48u&&b+gap<=48u){best_gap=gap;best=j;}
  }
  if(best==255u)return 0;
  run_len[best]+=best_gap+run_len[best+1u];b+=best_gap;
  for(j=best+1u;j+1u<k;j++){run_start[j]=run_start[j+1u];run_len[j]=run_len[j+1u];}
  k--;
 }
 for(j=0;j<k;j++){
  start=run_start[j];n=run_len[j];
  if(start<512u){vbk=start>=256u;bank=5u+vbk;source=0xd000u+(start&255u)*16u;dest=0x8000u+(start&255u)*16u;}
  else{vbk=start>=576u;bank=7u;source=0xd200u+(start-512u)*16u;dest=0x9800u+((start-512u)&63u)*16u;}
  out[0]=source;out[1]=source>>8;out[2]=dest;out[3]=dest>>8;out[4]=vbk;out[5]=bank;out[6]=n-1u;
  out+=7u;
 }
 *count=k;*total=(uint8_t)b;return 1;
}
