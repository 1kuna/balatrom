#pragma bank 255
#include "num_extra.h"
#include <string.h>
uint8_t num_floor(GbNum *out,const GbNum *value) CORE_BANKED {uint16_t exp;int16_t e,low;uint8_t i;
 exp=((uint16_t)(value->bits[7]&127)<<4)|(value->bits[6]>>4);
 if(value->bits[7]&128)return GBN_DOMAIN;
 if(exp==2047){if((value->bits[6]&15)|value->bits[0]|value->bits[1]|value->bits[2]|value->bits[3]|value->bits[4]|value->bits[5])return GBN_NAN;*out=*value;return GBN_INF;}
 e=(int16_t)exp-1023;if(e<0){memset(out,0,8);return GBN_OK;}*out=*value;if(e>=52)return GBN_OK;low=52-e;
 for(i=0;i<(low>>3);i++)out->bits[i]=0;if(low&7)out->bits[low>>3]&=(uint8_t)(0xff<<(low&7));return GBN_OK;
}

uint8_t num_to_u32_exact(uint32_t *out,const GbNum *v) CORE_BANKED {uint16_t raw=((uint16_t)(v->bits[7]&127)<<4)|(v->bits[6]>>4);int16_t e=(int16_t)raw-1023,b,low;uint8_t i;uint32_t n;
 if(v->bits[7]&128)return GBN_DOMAIN;if(!raw){for(i=0;i<7;i++)if(v->bits[i])return GBN_DOMAIN;*out=0;return GBN_OK;}if(e<0||e>31)return GBN_DOMAIN;low=52-e;for(b=0;b<low;b++)if(v->bits[b>>3]&(1u<<(b&7)))return GBN_DOMAIN;n=1UL<<e;for(i=0;i<e;i++){b=52-e+i;if(v->bits[b>>3]&(1u<<(b&7)))n|=1UL<<i;}*out=n;return GBN_OK;}
