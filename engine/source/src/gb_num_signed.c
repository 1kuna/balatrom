#pragma bank 255
#include "gb_num_signed.h"
#include <string.h>
static uint8_t sign(const GbNum*n){return n->bits[7]>>7;}
static void magnitude(GbNum*out,const GbNum*in){memcpy(out,in,8);out->bits[7]&=127;}
static uint8_t zero(const GbNum*n){uint8_t i;for(i=0;i<7;i++)if(n->bits[i])return 0;return !(n->bits[7]&127);}
uint8_t gb_num_from_i32(GbNum*out,int32_t value) CORE_BANKED {uint8_t negative=value<0,s;uint32_t magnitude_value=negative?0UL-(uint32_t)value:(uint32_t)value;s=gb_num_from_u32(out,magnitude_value);if(negative)out->bits[7]|=128;return s;}
uint8_t gb_num_add_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {GbNum x,y;uint8_t sa=sign(a),sb=sign(b),s;int8_t cmp;magnitude(&x,a);magnitude(&y,b);
 if(sa==sb){s=gb_num_add(out,&x,&y);if(!(s&GBN_NAN)&&sa)out->bits[7]|=128;return s;}
 cmp=gb_num_cmp(&x,&y);if(cmp==GBN_UNORDERED)return gb_num_add(out,&x,&y);
 if(cmp>=0){s=gb_num_sub_clamp(out,&x,&y);if(cmp>0&&!(s&GBN_NAN)&&sa)out->bits[7]|=128;}
 else{s=gb_num_sub_clamp(out,&y,&x);if(!(s&GBN_NAN)&&sb)out->bits[7]|=128;}return s;}
uint8_t gb_num_sub_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {GbNum opposite;memcpy(&opposite,b,8);opposite.bits[7]^=128;return gb_num_add_signed(out,a,&opposite);}
uint8_t gb_num_mul_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {GbNum x,y;uint8_t negative=sign(a)^sign(b),s;magnitude(&x,a);magnitude(&y,b);s=gb_num_mul(out,&x,&y);if(!(s&GBN_NAN)&&negative)out->bits[7]|=128;return s;}
uint8_t gb_num_div_signed(GbNum*out,const GbNum*a,const GbNum*b) CORE_BANKED {GbNum x,y;uint8_t negative=sign(a)^sign(b),s;magnitude(&x,a);magnitude(&y,b);s=gb_num_div(out,&x,&y);if(!(s&GBN_NAN)&&negative)out->bits[7]|=128;return s;}
uint8_t gb_num_add_i16_signed(GbNum*out,const GbNum*a,int16_t value) CORE_BANKED {GbNum b;gb_num_from_i32(&b,value);return gb_num_add_signed(out,a,&b);}
int8_t gb_num_cmp_signed(const GbNum*a,const GbNum*b) CORE_BANKED {GbNum x,y;int8_t cmp;uint8_t sa=sign(a),sb=sign(b);magnitude(&x,a);magnitude(&y,b);cmp=gb_num_cmp(&x,&y);if(cmp==GBN_UNORDERED)return cmp;if(zero(a)&&zero(b))return 0;if(sa!=sb)return sa?-1:1;return sa?-cmp:cmp;}
