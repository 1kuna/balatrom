#pragma bank 255
#include "gb_num_format.h"
#include <string.h>
#include "decimal_power.h"
#include "gb_num_decimal.h"
/* Exact coefficient M*2^scale or M*5^(-scale), base 100 little endian.
 * IEEE binary64 worst case requires at most 767 coefficient decimal digits.
 * 384 bytes are a derived format bound, not a game-value truncation limit. */
static uint8_t decimal[384];
static uint16_t decimal_used;
static void decimal_mul(uint8_t factor,uint8_t add){uint16_t i,v;uint8_t carry=add,q;for(i=0;i<decimal_used;i++){v=(uint16_t)decimal[i]*factor+carry;q=0;while(v>=100){v-=100;q++;}decimal[i]=(uint8_t)v;carry=q;}if(carry)decimal[decimal_used++]=carry;}
static uint8_t digit(uint16_t index,uint16_t count){uint16_t offset=count-1-index;uint8_t value=decimal[offset>>1];return offset&1?value/10:value%10;}
static uint8_t append(char*out,uint8_t cap,uint8_t*used,char c){if(*used+1>=cap)return 0;out[(*used)++]=c;out[*used]=0;return 1;}
static uint8_t emit_string(char*out,uint8_t cap,const char*s){uint8_t n=(uint8_t)strlen(s);if(cap<=n){if(cap)out[0]=0;return GBN_FORMAT_SHORT;}memcpy(out,s,n+1);return 0;}
static uint8_t small_integer(const GbNum*n,uint32_t*out){int16_t exponent,i,lowest;uint16_t e;uint8_t m[7];uint32_t value=0;e=((uint16_t)(n->bits[7]&127)<<4)|(n->bits[6]>>4);if(!e)return 0;exponent=(int16_t)e-1023;if(exponent<0||exponent>31)return 0;memcpy(m,n->bits,7);m[6]=(m[6]&15)|16;lowest=52-exponent;for(i=0;i<lowest;i++)if(m[i>>3]&(1u<<(i&7)))return 0;for(i=52;i>=lowest;i--)value=(value<<1)|((m[i>>3]>>(i&7))&1);*out=value;return 1;}
static uint8_t at_least_decimal_power(const GbNum*n,int16_t exponent){uint16_t index=(uint16_t)(exponent+324);uint8_t i;const uint8_t*p=decimal_power[index];for(i=8;i!=0;){--i;uint8_t x=n->bits[i]&(i==7?127:255);if(x!=p[i])return x>p[i];}return !p[8];}
static uint8_t fast_significand(const GbNum*n,uint8_t sig,uint8_t*d,int16_t*decimal_exponent){GbNum value,factor;int16_t binary_exp,e,k,j,frac,bitpos;int32_t product;uint16_t raw_exp;uint32_t integer=0,limit=1;uint8_t i,halfbit,near_half=1;memcpy(&value,n,8);value.bits[7]&=127;raw_exp=((uint16_t)value.bits[7]<<4)|(value.bits[6]>>4);
 if(raw_exp)binary_exp=(int16_t)raw_exp-1023;else{binary_exp=-1074;for(j=51;j>=0;j--)if(value.bits[j>>3]&(1u<<(j&7))){binary_exp+=j;break;}}
 product=(int32_t)binary_exp*1233;e=product>=0?(int16_t)(product/4096):(int16_t)-((-product+4095)/4096);if(e<-324)e=-324;if(e>308)e=308;
 while(e<308&&at_least_decimal_power(&value,e+1))e++;while(e>-324&&!at_least_decimal_power(&value,e))e--;
 *decimal_exponent=e;k=(int16_t)sig-1-e;
 if(k>308){memcpy(factor.bits,decimal_power[632],8);if(gb_num_mul(&value,&value,&factor)&GBN_NAN)return 0;k-=308;}
 memcpy(factor.bits,decimal_power[(uint16_t)(k+324)],8);if(gb_num_mul(&value,&value,&factor)&GBN_NAN)return 0;
 raw_exp=((uint16_t)(value.bits[7]&127)<<4)|(value.bits[6]>>4);binary_exp=(int16_t)raw_exp-1023;if(binary_exp<0||binary_exp>20)return 0;frac=52-binary_exp;
 for(j=52;j>=frac;j--){bitpos=j;integer=(integer<<1)|(j==52?1:((value.bits[bitpos>>3]>>(bitpos&7))&1));}
 halfbit=(value.bits[(frac-1)>>3]>>((frac-1)&7))&1;
 /* Conservative guard: with factors/products rounded in binary64, scaled
    values <=1e6 have error <2^-28. Reject within 2^-21 of any half-integer,
    then use the exact decimal-coefficient path. No approximate result escapes. */
 for(j=frac-2;j>=frac-21;j--){uint8_t b=(value.bits[j>>3]>>(j&7))&1;if((halfbit&&b)||(!halfbit&&!b)){near_half=0;break;}}
 if(near_half){int8_t c=gb_num_decimal_cmp(n,integer*2+1,e-(int16_t)sig+1);if(c==2)return 0;if(c>0||(c==0&&(integer&1)))integer++;}else if(halfbit)integer++;
 for(i=0;i<sig;i++)limit*=10;if(integer>=limit){integer/=10;(*decimal_exponent)++;}
 for(i=sig;i!=0;){--i;d[i]=integer%10;integer/=10;}return 1;}
static uint8_t format_value(char*out,uint8_t cap,const GbNum*n,uint8_t sig,uint8_t force) {uint8_t m[7],negative=n->bits[7]>>7,nonzero=0,i,d[6],guard,sticky=0,used=0;uint16_t e,count,k;int16_t scale,b,exponent;uint32_t integer;char reverse[10];uint8_t rev=0;
 if(!cap)return GBN_FORMAT_SHORT;out[0]=0;if(sig<1||sig>6)return GBN_FORMAT_BAD;e=((uint16_t)(n->bits[7]&127)<<4)|(n->bits[6]>>4);memcpy(m,n->bits,7);m[6]&=15;for(i=0;i<7;i++)nonzero|=m[i];
 if(e==2047){if(nonzero)return emit_string(out,cap,"NaN");return emit_string(out,cap,negative?"-Inf":"Inf");}
 if(!e&&!nonzero)return emit_string(out,cap,negative?"-0":"0");
 if(!force&&small_integer(n,&integer)){if(negative&&!append(out,cap,&used,'-'))return GBN_FORMAT_SHORT;do{reverse[rev++]=(char)('0'+integer%10);integer/=10;}while(integer);while(rev)if(!append(out,cap,&used,reverse[--rev]))return GBN_FORMAT_SHORT;return 0;}
 if(fast_significand(n,sig,d,&exponent))goto emit_scientific;
 scale=e?(int16_t)e-1075:-1074;if(e)m[6]|=16;memset(decimal,0,sizeof(decimal));decimal_used=1;
 for(b=52;b>=0;b--)decimal_mul(2,(m[b>>3]>>(b&7))&1);
 if(scale>=0){for(b=0;b<scale;b++)decimal_mul(2,0);}else for(b=0;b<-scale;b++)decimal_mul(5,0);
 count=2*(decimal_used-1)+(decimal[decimal_used-1]>=10?2:1);exponent=(int16_t)count+(scale<0?scale:0)-1;
 for(i=0;i<sig;i++)d[i]=i<count?digit(i,count):0;guard=sig<count?digit(sig,count):0;for(k=sig+1;k<count;k++)sticky|=digit(k,count)!=0;
 if(guard>5||(guard==5&&(sticky||(d[sig-1]&1)))){i=sig;while(i){--i;if(++d[i]<10)break;d[i]=0;}if(!i&&!d[0]){d[0]=1;exponent++;}}
 emit_scientific:
 if(negative&&!append(out,cap,&used,'-'))return GBN_FORMAT_SHORT;if(!append(out,cap,&used,'0'+d[0]))return GBN_FORMAT_SHORT;if(sig>1){if(!append(out,cap,&used,'.'))return GBN_FORMAT_SHORT;for(i=1;i<sig;i++)if(!append(out,cap,&used,'0'+d[i]))return GBN_FORMAT_SHORT;}
 if(!append(out,cap,&used,'e')||!append(out,cap,&used,exponent<0?'-':'+'))return GBN_FORMAT_SHORT;integer=exponent<0?(uint16_t)-exponent:(uint16_t)exponent;rev=0;do{reverse[rev++]=(char)('0'+integer%10);integer/=10;}while(integer);while(rev)if(!append(out,cap,&used,reverse[--rev]))return GBN_FORMAT_SHORT;return 0;}
uint8_t gb_num_format(char*out,uint8_t cap,const GbNum*n,uint8_t sig) CORE_BANKED {return format_value(out,cap,n,sig,0);}
uint8_t gb_num_format_scientific(char*out,uint8_t cap,const GbNum*n,uint8_t sig) CORE_BANKED {return format_value(out,cap,n,sig,1);}
uint8_t num_format(char out[GBN_FORMAT_CAP],const GbNum*n) CORE_BANKED {return gb_num_format(out,GBN_FORMAT_CAP,n,3);}
