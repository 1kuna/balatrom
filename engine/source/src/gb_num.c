#pragma bank 255
#include "gb_num.h"
#include <string.h>
static uint8_t wa[16],wb[16],wc[16],wd[16];
/* byte-granular scans and shifts. Same results as the former
 * bit-at-a-time loops; scoring spent ~half its time in them. */
static int16_t highest(const uint8_t*a){uint8_t i=16,v,b;while(i){v=a[--i];if(v){b=7;while(!(v&128u)){v<<=1;b--;}return (int16_t)((uint8_t)(i<<3)|b);}}return -1;}
static uint8_t bit(const uint8_t*a,int16_t i){return i>=0&&i<128?(a[i>>3]>>(i&7))&1:0;}
static uint8_t any_low(const uint8_t*a,int16_t n){uint8_t i,w,r;if(n<=0)return 0;if(n>128)n=128;w=(uint8_t)n>>3;r=(uint8_t)n&7;for(i=0;i<w;i++)if(a[i])return 1;return r&&(a[w]&(uint8_t)((1u<<r)-1u))?1:0;}
static void shift_left(uint8_t*a,uint8_t n){uint8_t i,w,r,c,x;if(n>=128){memset(a,0,16);return;}w=n>>3;r=n&7;if(w){for(i=15;i>=w;i--)a[i]=a[i-w];memset(a,0,w);}while(r--){c=0;for(i=0;i<16;i++){x=a[i]>>7;a[i]=(uint8_t)((a[i]<<1)|c);c=x;}}}
static void shift_right(uint8_t*a,int16_t n,uint8_t jam){uint8_t i,w,r,c,x,sticky;if(n<=0)return;sticky=jam?any_low(a,n):0;if(n>=128)memset(a,0,16);else{w=(uint8_t)n>>3;r=(uint8_t)n&7;if(w){for(i=0;i<16u-w;i++)a[i]=a[i+w];memset(a+16u-w,0,w);}while(r--){c=0;for(i=16;i!=0;){--i;x=a[i]&1u;a[i]=(uint8_t)((a[i]>>1)|c);c=x?128u:0;}}}if(sticky)a[0]|=1;}
static void plus_one(uint8_t*a){uint8_t i;for(i=0;i<16;i++)if(++a[i])break;}
static int8_t wide_cmp(const uint8_t*a,const uint8_t*b){uint8_t i;for(i=16;i!=0;){--i;if(a[i]!=b[i])return a[i]>b[i]?1:-1;}return 0;}
static void wide_sub(uint8_t*a,const uint8_t*b){uint8_t i,borrow=0;int16_t v;for(i=0;i<16;i++){v=(int16_t)a[i]-b[i]-borrow;a[i]=(uint8_t)v;borrow=v<0;}}
static uint8_t invalid(GbNum*out,uint8_t flags){memset(out,0,sizeof(*out));out->bits[6]=0xf8;out->bits[7]=0x7f;return flags|GBN_NAN;}
static uint8_t infinity(GbNum*out,uint8_t flags){memset(out,0,sizeof(*out));out->bits[6]=0xf0;out->bits[7]=0x7f;return flags|GBN_INF;}
/* 0 finite, 4 infinity, 2 NaN, 1 unsupported negative. */
static uint8_t unpack(const GbNum*n,uint8_t*m,int16_t*scale){uint16_t e;uint8_t i;memset(m,0,16);memcpy(m,n->bits,7);m[6]&=15;e=((uint16_t)(n->bits[7]&127)<<4)|(n->bits[6]>>4);*scale=e?(int16_t)e-1075:-1074;
 if(e==2047){if(highest(m)>=0)return GBN_NAN;return(n->bits[7]&128)?GBN_DOMAIN:GBN_INF;}
 if(n->bits[7]&128)return GBN_DOMAIN;
 if(e)m[6]|=16;return 0;}
static uint8_t pack(GbNum*out,uint8_t*m,int16_t scale){int16_t h=highest(m),exp,target,n;uint16_t e;uint8_t guard,sticky,odd;if(h<0){memset(out,0,sizeof(*out));return 0;}
 exp=h+scale;if(exp>1023)return infinity(out,0);target=exp>=-1022?exp-52:-1074;n=target-scale;
 if(n>0){guard=bit(m,n-1);sticky=any_low(m,n-1);odd=bit(m,n);shift_right(m,n,0);if(guard&&(sticky||odd))plus_one(m);}else if(n<0)shift_left(m,(uint8_t)-n);
 h=highest(m);if(h<0){memset(out,0,sizeof(*out));return 0;}if(h>52){shift_right(m,1,0);target++;h--;}
 e=(h<52&&target==-1074)?0:(uint16_t)(target+1075);if(e>=2047)return infinity(out,0);
 memcpy(out->bits,m,7);out->bits[6]=(out->bits[6]&15)|((e&15)<<4);out->bits[7]=e>>4;return 0;}
int8_t gb_num_cmp(const GbNum*a,const GbNum*b) CORE_BANKED {uint8_t i;int16_t x,y;uint8_t as=unpack(a,wa,&x),bs=unpack(b,wb,&y);if(as&(GBN_DOMAIN|GBN_NAN)||bs&(GBN_DOMAIN|GBN_NAN))return GBN_UNORDERED;for(i=8;i!=0;){--i;if((a->bits[i]&(i==7?127:255))!=(b->bits[i]&(i==7?127:255)))return(a->bits[i]&(i==7?127:255))>(b->bits[i]&(i==7?127:255))?1:-1;}return 0;}
static uint8_t add_sub(GbNum*out,const GbNum*a,const GbNum*b,uint8_t sub){int16_t x,y,scale;uint16_t sum;uint8_t i,c=0,as=unpack(a,wa,&x),bs=unpack(b,wb,&y);
 if(as&(GBN_DOMAIN|GBN_NAN)||bs&(GBN_DOMAIN|GBN_NAN))return invalid(out,as|bs);
 if(as==GBN_INF||bs==GBN_INF){if(sub&&as==bs)return invalid(out,0);if(sub&&bs==GBN_INF){memset(out,0,8);return 0;}return infinity(out,0);}
 if(sub&&wide_cmp(wa,wb)==0&&x==y){memset(out,0,8);return 0;}
 if(sub&&(x<y||(x==y&&wide_cmp(wa,wb)<0))){/* Different subnormal/normal scales need actual magnitude. */
  uint8_t j;int8_t cmp=0;for(j=8;j!=0;){--j;if(a->bits[j]!=b->bits[j]){cmp=a->bits[j]>b->bits[j]?1:-1;break;}}if(cmp<0){memset(out,0,8);return 0;}}
 shift_left(wa,3);shift_left(wb,3);scale=(x>y?x:y)-3;if(x<y)shift_right(wa,y-x,1);else if(y<x)shift_right(wb,x-y,1);
 if(sub){if(wide_cmp(wa,wb)<0){memset(out,0,8);return 0;}wide_sub(wa,wb);}else for(i=0;i<16;i++){sum=(uint16_t)wa[i]+wb[i]+c;wa[i]=(uint8_t)sum;c=sum>>8;}
 return pack(out,wa,scale);}
uint8_t gb_num_add(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {return add_sub(out,a,b,0);}
uint8_t gb_num_sub_clamp(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {return add_sub(out,a,b,1);}
uint8_t gb_num_mul(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {int16_t x,y;uint16_t v,carry;uint8_t i,j,z,as=unpack(a,wa,&x),bs=unpack(b,wb,&y);
 if(as&(GBN_DOMAIN|GBN_NAN)||bs&(GBN_DOMAIN|GBN_NAN))return invalid(out,as|bs);
 if(as==GBN_INF||bs==GBN_INF){if((!as&&highest(wa)<0)||(!bs&&highest(wb)<0))return invalid(out,0);return infinity(out,0);}
 memset(wc,0,16);for(i=0;i<7;i++){carry=0;for(j=0;j<7;j++){v=(uint16_t)wa[i]*wb[j]+wc[i+j]+carry;wc[i+j]=(uint8_t)v;carry=v>>8;}z=i+7;while(carry){v=(uint16_t)wc[z]+carry;wc[z++]=(uint8_t)v;carry=v>>8;}}
 return pack(out,wc,x+y);}
uint8_t gb_num_div(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {int16_t x,y,i,h;uint8_t as=unpack(a,wa,&x),bs=unpack(b,wb,&y);
 if(as&(GBN_DOMAIN|GBN_NAN)||bs&(GBN_DOMAIN|GBN_NAN))return invalid(out,as|bs);
 if(as==GBN_INF&&bs==GBN_INF)return invalid(out,0);if(as==GBN_INF)return infinity(out,0);if(bs==GBN_INF){memset(out,0,8);return 0;}
 if(highest(wb)<0){if(highest(wa)<0)return invalid(out,GBN_DIVZERO);return infinity(out,GBN_DIVZERO);}if(highest(wa)<0){memset(out,0,8);return 0;}
 h=highest(wa);shift_left(wa,(uint8_t)(52-h));x-=52-h;h=highest(wb);shift_left(wb,(uint8_t)(52-h));y-=52-h;shift_left(wa,64);memset(wc,0,16);memset(wd,0,16);
 for(i=116;i>=0;i--){shift_left(wd,1);wd[0]|=bit(wa,i);if(wide_cmp(wd,wb)>=0){wide_sub(wd,wb);wc[i>>3]|=(uint8_t)(1u<<(i&7));}}
 if(highest(wd)>=0)wc[0]|=1;return pack(out,wc,x-y-64);}
uint8_t gb_num_add_i16(GbNum*out,const GbNum*a,int16_t value) CORE_BANKED {GbNum n;uint16_t magnitude=value<0?(uint16_t)(-(value+1))+1:(uint16_t)value;gb_num_from_u32(&n,magnitude);if(value<0){if(gb_num_cmp(a,&n)<0)return invalid(out,GBN_DOMAIN);return gb_num_sub_clamp(out,a,&n);}return gb_num_add(out,a,&n);}
uint8_t gb_num_ratio(GbNum*out,uint16_t numerator,uint16_t denominator) CORE_BANKED {GbNum n,d;gb_num_from_u32(&n,numerator);gb_num_from_u32(&d,denominator);return gb_num_div(out,&n,&d);}
uint8_t gb_num_mul_ratio(GbNum*out,const GbNum*a,uint16_t numerator,uint16_t denominator) CORE_BANKED {GbNum factor;uint8_t s=gb_num_ratio(&factor,numerator,denominator);return gb_num_mul(out,a,&factor)|s;}
