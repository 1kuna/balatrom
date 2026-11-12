#pragma bank 255
#include "gb_num_decimal.h"
#include <string.h>
#include "decimal_interval.h"
static uint8_t lower[15],upper[15];
static int16_t highbit(const uint8_t*a,uint8_t size){int16_t i=(int16_t)size*8-1;while(i>=0&&!(a[i>>3]&(1u<<(i&7))))i--;return i;}
static int8_t compare(const uint8_t*a,int16_t scale,const GbNum*n){uint8_t m[7];uint16_t raw;int16_t ns,h,nh,i,ai,ni;memcpy(m,n->bits,7);m[6]&=15;raw=((uint16_t)(n->bits[7]&127)<<4)|(n->bits[6]>>4);ns=raw?(int16_t)raw-1075:-1074;if(raw)m[6]|=16;h=highbit(a,15);nh=highbit(m,7);if(nh<0)return h<0?0:1;if(h<0)return -1;if(h+scale!=nh+ns)return h+scale>nh+ns?1:-1;
 for(i=h+scale;i>=(scale<ns?scale:ns);i--){ai=i-scale;ni=i-ns;uint8_t ab=ai>=0&&ai<120?((a[ai>>3]>>(ai&7))&1):0;uint8_t nb=ni>=0&&ni<56?((m[ni>>3]>>(ni&7))&1):0;if(ab!=nb)return ab>nb?1:-1;}return 0;}
int8_t gb_num_decimal_cmp(const GbNum*n,uint32_t odd,int16_t power) CORE_BANKED {int16_t shift=-1,scale;uint16_t index,v;uint8_t i,j,carry,ob[3],exact;int8_t low,up;
 while(power<0&&odd%5==0){odd/=5;power++;shift--;}
 if(power<-329||power>308)return 2;index=(uint16_t)(power+329);scale=(int16_t)(decimal_interval[index][12]|((uint16_t)decimal_interval[index][13]<<8));scale+=shift;exact=decimal_interval[index][14];
 ob[0]=(uint8_t)odd;ob[1]=(uint8_t)(odd>>8);ob[2]=(uint8_t)(odd>>16);memset(lower,0,15);
 for(j=0;j<3;j++){carry=0;for(i=0;i<12;i++){v=(uint16_t)decimal_interval[index][i]*ob[j]+lower[i+j]+carry;lower[i+j]=(uint8_t)v;carry=(uint8_t)(v>>8);}i=12+j;while(carry&&i<15){v=(uint16_t)lower[i]+carry;lower[i++]=(uint8_t)v;carry=(uint8_t)(v>>8);}}
 low=compare(lower,scale,n);if(low>0)return -1;if(!low)return exact?0:-1;if(exact)return 1;memcpy(upper,lower,15);carry=0;for(i=0;i<15;i++){v=(uint16_t)upper[i]+(i<3?ob[i]:0)+carry;upper[i]=(uint8_t)v;carry=(uint8_t)(v>>8);}up=compare(upper,scale,n);if(up<=0)return 1;return 2;}
