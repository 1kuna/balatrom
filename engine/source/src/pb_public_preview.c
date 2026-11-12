#include "pb_public_preview.h"
#include <string.h>
#ifdef __SDCC
#pragma bank 255
#endif
typedef char PublicCardBytes[(sizeof(PBPPCard)==3u)?1:-1];
uint8_t pbpp_visible_role(uint8_t id) PBPP_API {
 return id==17u?1u:id==68u?2u:id==112u?4u:0u;
}
static uint8_t count3(uint8_t value){
 return (value&1u)+((value>>1)&1u)+((value>>2)&1u);
}
/* Exact hand TYPE only. Source mask order and suit preference cannot change
 * type; chips/mult/level depend solely on type and public level progression. */
static uint8_t classify(const PBPPCard *cards,uint8_t n,uint8_t profile){
 uint8_t counts[15],i,j,rank,pairs=0,trips=0,quads=0,five=0;
 uint8_t minimum=(profile&1u)?4u:5u,length=0,skipped=0,straight=0,flush=0,suit,total;
 memset(counts,0,sizeof(counts));
 for(i=0;i<n;i++)if(cards[i].enhancement!=6u)counts[cards[i].rank]++;
 for(i=2;i<15u;i++){pairs+=counts[i]==2u;trips+=counts[i]==3u;quads+=counts[i]>=4u;five|=counts[i]==5u;}
 if(n>=minimum){
  for(j=1;j<=14u;j++){
   rank=j==1u?14u:j;
   if(counts[rank]){length++;skipped=0;}
   else if((profile&2u)&&!skipped&&j!=14u)skipped=1;
   else{length=0;skipped=0;if(straight)break;}
   if(length>=minimum)straight=1;
  }
  for(suit=0;suit<4u;suit++){
   total=0;
   for(i=0;i<n;i++)if(cards[i].enhancement!=6u&&
      (cards[i].enhancement==3u||cards[i].suit==suit||
       ((profile&4u)&&((cards[i].suit==0u||cards[i].suit==3u)==(suit==0u||suit==3u)))))total++;
   if(total>=minimum){flush=1;break;}
  }
 }
 if(five&&flush)return 11;
 if(trips&&pairs&&flush)return 10;
 if(five)return 9;
 if(straight&&flush)return 8;
 if(quads)return 7;
 if(trips&&pairs)return 6;
 if(flush)return 5;
 if(straight)return 4;
 if(trips)return 3;
 if(pairs>=2u)return 2;
 return pairs?1u:0u;
}
uint8_t pbpp_consensus(const PBPPCard *cards,uint8_t n,uint8_t known,uint8_t hidden,uint8_t *type) PBPP_API {
 uint8_t i,profile,value,first=255;uintptr_t a,b;
 if(!type||n>5u||known>7u||hidden>5u||(n&&!cards))return PBPP_INVALID;
 a=(uintptr_t)cards;b=(uintptr_t)type;
 if(n&&b>=a&&b-a<(uint16_t)n*3u)return PBPP_INVALID;
 for(i=0;i<n;i++){
  if(cards[i].enhancement>8u)return PBPP_INVALID;
  if(cards[i].enhancement==6u){if(cards[i].rank||cards[i].suit)return PBPP_INVALID;}
  else if(cards[i].rank<2u||cards[i].rank>14u||cards[i].suit>3u)return PBPP_INVALID;
 }
 for(profile=0;profile<8u;profile++){
  if((profile&known)!=known||count3(profile&~known)>hidden)continue;
  value=classify(cards,n,profile);
  if(first==255u)first=value;
  else if(first!=value)return PBPP_UNKNOWN;
 }
 *type=first;return PBPP_KNOWN;
}
