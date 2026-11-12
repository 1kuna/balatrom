#pragma bank 255
#include "panel_b_narrow_native.h"
#include <string.h>
typedef struct {uint8_t tag,x,y,attribute;int8_t dy;uint8_t plane[32];} Local;
typedef char LocalBytes[(sizeof(Local)==37u)?1:-1];
static const uint8_t face_key[56]={0,19,5,12,1,19,12,3,1,4,14,3,0,1,17,18,1,13,17,0,1,13,17,2,1,13,17,14,1,2,13,18,1,3,13,0,13,17,17,0,2,0,14,17,3,19,2,3,19,14,0,18,19,7,14,19};
static const Local local[4]={
{0,48,96,3,2,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,0,192,0,128,0,0,0,0}},
{0,128,96,0,0,{0,128,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
{0,136,96,3,1,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,3,0,1,0,1,0,1,0,1,0,3,0,0,0,0}},
{0,144,96,0,0,{0,64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
};
uint8_t pb_narrow_face_origin(const PBOverlayPalettes *p,const uint8_t *plane,
                            uint8_t y,uint8_t x,uint8_t attribute,int8_t *dy) BANKED {
 uint8_t i;
 if(!p||!plane||!dy||attribute>7u||memcmp(p,face_key,56))return 0;
 for(i=0;i<4u;i++)if(local[i].x==x&&local[i].y==y&&local[i].attribute==attribute&&
                       !memcmp(plane,local[i].plane,32)){
  *dy=local[i].dy;return 1;
 }
 return 0;
}
