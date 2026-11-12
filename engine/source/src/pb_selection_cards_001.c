#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
#include "pb_selection_cards_001_records.inc"

#include "pb_selection_cards_001_starts.inc"

uint16_t pb_selection_cards_001(uint8_t pair,uint16_t left,uint16_t right) BANKED {
 uint16_t lo,hi,mid,at;uint8_t a,b,c,v;
 if(pair<129u||pair>153u||left>511u||right>511u)return 65535u;
 a=(uint8_t)left;b=(uint8_t)(((left>>8)&1u)|((right&127u)<<1));c=(uint8_t)((right>>7)&3u);
 v=pair-129u;lo=starts[v];hi=starts[v+1u];
 while(lo<hi){
  mid=lo+(hi-lo)/2u;at=mid*5u;
  if(records[at]<a){lo=mid+1u;continue;}if(records[at]>a){hi=mid;continue;}
  if(records[at+1u]<b){lo=mid+1u;continue;}if(records[at+1u]>b){hi=mid;continue;}
  if(records[at+2u]<c){lo=mid+1u;continue;}if(records[at+2u]>c){hi=mid;continue;}
  return (uint16_t)records[at+3u]|((uint16_t)records[at+4u]<<8);
 }return 65535u;
}
