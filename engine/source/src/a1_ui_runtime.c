#pragma bank 255
#include "a1_ui_runtime.h"
#include "a1_assets.h"
#include "frontend_numeric.h"
#include "raster_runtime.h"
#include <gb/cgb.h>
#include <string.h>
void raster_capture_inputs(void) NONBANKED;
void a1_blend_mask(uint8_t,uint8_t,uint8_t) __sdcccall(0) NONBANKED;
uint8_t native_ui_rows(uint8_t,uint8_t,uint8_t,uint8_t,const A1Text*,uint8_t,uint8_t) BANKED;
#include "cooperative_native_redirect.h"
uint8_t a1_text_width(const char *s,uint8_t font) BANKED {
 uint8_t n=strlen(s);return n?n*(font==5u?6u:4u)-1u:0u;
}
/* The existing numeric adapter remains the authority. Integer adapter
 * overflow uses a visibly scientific leading digit, with the fuller adapter
 * output separately shown in the public detail pane. No truncation-as-exact. */
uint8_t a1_number_text(const void *v,uint8_t size,char out[16],uint8_t width,uint8_t big) BANKED {
 uint8_t n,max;
 frontend_format_number(v,size,out);n=strlen(out);
 if(big&&n*6u-1u<=width)return 5u;
 if(n*4u-1u<=width)return 3u;
 max=(width+1u)/4u;
 if(size==8u){frontend_format_number_width(v,size,out,max);if(strlen(out)>max)strcpy(out,"ERR");}
 else if(max>=3u&&n<=10u){out[1]='E';out[2]='0'+n-1u;out[3]=0;}
 else strcpy(out,"ERR");
 return 3u;
}
static void a1_ui_publish_rows(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const A1Text *fields,uint8_t count,uint8_t plain,uint8_t row_mask) {
 uint8_t lengths[17],active[17],active_count,af,cx,cy,slot,attr,f,j,r,len,step,gh,font,color,lo,hi,first_char,last_char,first_row,last_row;int8_t row_dy[17];int16_t gx,dx,dy;const A1Text *cur;
 if(count>17u)return;for(f=0;f<count;f++)lengths[f]=strlen(fields[f].s);
 for(cy=y;cy<y+h;cy++){
  if(row_mask!=255u&&!(row_mask&(1u<<(cy-5u))))continue;
  /* Vertical eligibility depends only on this row. Preserve original field
   * order, including palette-only fields with no character in this cell. */
  active_count=0;
  for(f=0,cur=fields;f<count;f++,cur++){
   gh=cur->font==5u?7u:5u;dy=(int16_t)cur->y-(int16_t)cy*8;
   if(dy>=8||dy+gh<=0)continue;
   active[active_count]=f;row_dy[active_count]=(int8_t)dy;active_count++;
  }
  a1_ui_map_row_copy(cy,raster_pixels+48u);
  for(cx=x;cx<x+w;cx++){
   if(coop_active&&!coop_checkpoint())return;
   slot=raster_pixels[48u+cx];if(slot<52u)continue;
   attr=raster_pixels[68u+cx];
   if((plain&1u)&&!(plain&128u)){memset(raster_pixels,0,16);attr=4u;}
   else if((plain&128u)&&cy==16u)a1_status_base_copy(plain&7u,cx,raster_pixels);
   else a1_ui_base_copy(cx,cy,raster_pixels);
   /* Boss names need the full35px panel width. Remove only the decorative
    * chip's fixed9x9 footprint; preserve panel corners and target row. */
   if((plain&2u)&&!(plain&128u)&&cx<2u&&cy<2u){for(r=0;r<8u;r++)if(cy?r<4u:r>=3u){uint8_t mask=cx?240u:31u;raster_pixels[r*2u]|=mask;raster_pixels[r*2u+1u]&=~mask;}}
   for(af=0;af<active_count;af++){
    f=active[af];cur=fields+f;dy=row_dy[af];
    step=cur->font==5u?6u:4u;
    if(cur->color>>2)attr=(attr&~7u)+((cur->color>>2)-1u);
    len=lengths[f];font=cur->font;color=cur->color;lo=color&1u?255u:0u;hi=color&2u?255u:0u;
    /* Only glyphs whose ink width intersects this cell. Integer division
     * truncates toward zero; positive left edge gives the exact first glyph. */
    dx=(int16_t)cx*8-(int16_t)cur->x;
    first_char=dx>0?(font==5u?dx/6u:(uint16_t)dx>>2):0u;
    dx+=7;
    if(dx<0)continue;
    last_char=(font==5u?dx/6u:(uint16_t)dx>>2)+1u;
    if(last_char>len)last_char=len;
    first_row=dy>0?dy:0u;last_row=dy+(font==5u?7u:5u);if(last_row>8u)last_row=8u;
    for(j=first_char,gx=(int16_t)cur->x+(uint16_t)first_char*step;j<last_char;j++,gx+=step){
     dx=gx-(int16_t)cx*8;
     if(dx+step-1<=0)continue;
     if(font==5u)a1_font5_mask_copy(cur->s[j],(int8_t)dx,(int8_t)dy,raster_pixels+16u);
     else a1_font3_mask_copy(cur->s[j],(int8_t)dx,(int8_t)dy,raster_pixels+16u);
     a1_blend_mask(first_row,last_row,color);
    }
   }
   VBK_REG=0;set_bkg_data(slot,1,raster_pixels);raster_capture_inputs();
   raster_pixels[48u+cx]=slot;raster_pixels[68u+cx]=attr;
  }
  set_tiles(x,cy,w,1,(uint8_t *)0x9800u,raster_pixels+48u+x);
  VBK_REG=1;set_tiles(x,cy,w,1,(uint8_t *)0x9800u,raster_pixels+68u+x);VBK_REG=0;raster_capture_inputs();
 }
}

/* Small focus panes retain the original per-cell compositor. */
void a1_ui_publish(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const A1Text *fields,uint8_t count,uint8_t plain) BANKED {
 uint8_t row_mask=native_ui_rows(x,y,w,h,fields,count,plain);
 if(!row_mask)return;
 uint8_t lengths[17],cx,cy,slot,attr,f,j,r,len,step,gh,font,color,lo,hi,first_char,last_char,first_row,last_row;int16_t gx,gy,dx,dy;const A1Text *cur;
 if(count>2u||h>2u){a1_ui_publish_rows(x,y,w,h,fields,count,plain,row_mask);return;}
 if(count>17u)return;for(f=0;f<count;f++)lengths[f]=strlen(fields[f].s);
 for(cy=y;cy<y+h;cy++){
  a1_ui_map_row_copy(cy,raster_pixels+48u);
  for(cx=x;cx<x+w;cx++){
   if(coop_active&&!coop_checkpoint())return;
   slot=raster_pixels[48u+cx];if(slot<52u)continue;
   attr=raster_pixels[68u+cx];
   if((plain&1u)&&!(plain&128u)){memset(raster_pixels,0,16);attr=4u;}
   else if((plain&128u)&&cy==16u)a1_status_base_copy(plain&7u,cx,raster_pixels);
   else a1_ui_base_copy(cx,cy,raster_pixels);
   /* Boss names need the full35px panel width. Remove only the decorative
    * chip's fixed9x9 footprint; preserve panel corners and target row. */
   if((plain&2u)&&!(plain&128u)&&cx<2u&&cy<2u){for(r=0;r<8u;r++)if(cy?r<4u:r>=3u){uint8_t mask=cx?240u:31u;raster_pixels[r*2u]|=mask;raster_pixels[r*2u+1u]&=~mask;}}
   for(f=0,cur=fields;f<count;f++,cur++){
    step=cur->font==5u?6u:4u;gh=cur->font==5u?7u:5u;
    gy=cur->y;dy=gy-(int16_t)cy*8;
    if(dy>=8||dy+gh<=0)continue;
    if(cur->color>>2)attr=(attr&~7u)+((cur->color>>2)-1u);
    len=lengths[f];font=cur->font;color=cur->color;lo=color&1u?255u:0u;hi=color&2u?255u:0u;
    /* Only glyphs whose ink width intersects this cell. Integer division
     * truncates toward zero; positive left edge gives the exact first glyph. */
    dx=(int16_t)cx*8-(int16_t)cur->x;
    first_char=dx>0?(font==5u?dx/6u:(uint16_t)dx>>2):0u;
    dx+=7;
    if(dx<0)continue;
    last_char=(font==5u?dx/6u:(uint16_t)dx>>2)+1u;
    if(last_char>len)last_char=len;
    first_row=dy>0?dy:0u;last_row=dy+(font==5u?7u:5u);if(last_row>8u)last_row=8u;
    for(j=first_char,gx=(int16_t)cur->x+(uint16_t)first_char*step;j<last_char;j++,gx+=step){
     dx=gx-(int16_t)cx*8;
     if(dx+step-1<=0)continue;
     if(font==5u)a1_font5_mask_copy(cur->s[j],(int8_t)dx,(int8_t)dy,raster_pixels+16u);
     else a1_font3_mask_copy(cur->s[j],(int8_t)dx,(int8_t)dy,raster_pixels+16u);
     a1_blend_mask(first_row,last_row,color);
    }
   }
   VBK_REG=0;set_bkg_data(slot,1,raster_pixels);raster_capture_inputs();
   raster_pixels[48u+cx]=slot;raster_pixels[68u+cx]=attr;
  }
  set_tiles(x,cy,w,1,(uint8_t *)0x9800u,raster_pixels+48u+x);
  VBK_REG=1;set_tiles(x,cy,w,1,(uint8_t *)0x9800u,raster_pixels+68u+x);VBK_REG=0;raster_capture_inputs();
 }
}

/* Dedicated original compositor for ordinary one/two-row focus updates. */
void a1_ui_publish_focus(uint8_t x,uint8_t y,uint8_t w,uint8_t h,const A1Text *fields,uint8_t count,uint8_t plain) BANKED {
 native_ui_rows(x,y,w,h,fields,255u,plain); /* invalidate overlapping rows */
 uint8_t lengths[17],cx,cy,slot,attr,f,j,r,len,step,gh,font,color,lo,hi,first_char,last_char,first_row,last_row;int16_t gx,gy,dx,dy;const A1Text *cur;
 if(count>17u)return;for(f=0;f<count;f++)lengths[f]=strlen(fields[f].s);
 for(cy=y;cy<y+h;cy++){
  a1_ui_map_row_copy(cy,raster_pixels+48u);
  for(cx=x;cx<x+w;cx++){
   if(coop_active&&!coop_checkpoint())return;
   slot=raster_pixels[48u+cx];if(slot<52u)continue;
   attr=raster_pixels[68u+cx];
   if((plain&1u)&&!(plain&128u)){memset(raster_pixels,0,16);attr=4u;}
   else if((plain&128u)&&cy==16u)a1_status_base_copy(plain&7u,cx,raster_pixels);
   else a1_ui_base_copy(cx,cy,raster_pixels);
   /* Boss names need the full35px panel width. Remove only the decorative
    * chip's fixed9x9 footprint; preserve panel corners and target row. */
   if((plain&2u)&&!(plain&128u)&&cx<2u&&cy<2u){for(r=0;r<8u;r++)if(cy?r<4u:r>=3u){uint8_t mask=cx?240u:31u;raster_pixels[r*2u]|=mask;raster_pixels[r*2u+1u]&=~mask;}}
   for(f=0,cur=fields;f<count;f++,cur++){
    step=cur->font==5u?6u:4u;gh=cur->font==5u?7u:5u;
    gy=cur->y;dy=gy-(int16_t)cy*8;
    if(dy>=8||dy+gh<=0)continue;
    if(cur->color>>2)attr=(attr&~7u)+((cur->color>>2)-1u);
    len=lengths[f];font=cur->font;color=cur->color;lo=color&1u?255u:0u;hi=color&2u?255u:0u;
    /* Only glyphs whose ink width intersects this cell. Integer division
     * truncates toward zero; positive left edge gives the exact first glyph. */
    dx=(int16_t)cx*8-(int16_t)cur->x;
    first_char=dx>0?(font==5u?dx/6u:(uint16_t)dx>>2):0u;
    dx+=7;
    if(dx<0)continue;
    last_char=(font==5u?dx/6u:(uint16_t)dx>>2)+1u;
    if(last_char>len)last_char=len;
    first_row=dy>0?dy:0u;last_row=dy+(font==5u?7u:5u);if(last_row>8u)last_row=8u;
    for(j=first_char,gx=(int16_t)cur->x+(uint16_t)first_char*step;j<last_char;j++,gx+=step){
     dx=gx-(int16_t)cx*8;
     if(dx+step-1<=0)continue;
     if(font==5u)a1_font5_mask_copy(cur->s[j],(int8_t)dx,(int8_t)dy,raster_pixels+16u);
     else a1_font3_mask_copy(cur->s[j],(int8_t)dx,(int8_t)dy,raster_pixels+16u);
     a1_blend_mask(first_row,last_row,color);
    }
   }
   VBK_REG=0;set_bkg_data(slot,1,raster_pixels);raster_capture_inputs();
   raster_pixels[48u+cx]=slot;raster_pixels[68u+cx]=attr;
  }
  set_tiles(x,cy,w,1,(uint8_t *)0x9800u,raster_pixels+48u+x);
  VBK_REG=1;set_tiles(x,cy,w,1,(uint8_t *)0x9800u,raster_pixels+68u+x);VBK_REG=0;raster_capture_inputs();
 }
}
