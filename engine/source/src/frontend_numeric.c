#pragma bank 255
#include "frontend_numeric.h"
#include <string.h>
#ifdef FRONTEND_USE_GBNUM_DISPLAY
#include "num_display.h"
#include "gb_num_format.h"
#endif
void frontend_format_number(const void *value,uint8_t size,char out[16]) BANKED {const uint8_t *p=value;uint32_t n=0;char rev[10];uint8_t i,j;if(size>4){
#ifdef FRONTEND_USE_GBNUM_DISPLAY
 if(size==8){num_display(out,(const GbNum*)value);return;}
#endif
 strcpy(out,"UNSUPPORTED");return;}for(i=0;i<size;i++)n|=(uint32_t)p[i]<<(i*8);i=0;do{rev[i++]='0'+n%10;n/=10;}while(n&&i<10);for(j=0;j<i;j++)out[j]=rev[i-j-1];out[i]=0;}

void frontend_format_number_width(const void *value,uint8_t size,char out[16],uint8_t width) BANKED {frontend_format_number(value,size,out);
#ifdef FRONTEND_USE_GBNUM_DISPLAY
 if(size==8&&strlen(out)>width){GbNum copy=*(const GbNum*)value;char full[16];uint8_t sig=3,status;do{status=gb_num_format_scientific(full,16,&copy,sig);if(status){strcpy(out,"FMTERR");return;}if(strlen(full)<=width)break;}while(--sig);strncpy(out,full,11);out[11]=0;}
#endif
}
