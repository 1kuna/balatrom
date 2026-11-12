#ifdef __SDCC
#pragma bank 255
#endif
#include "frontend_phase_layout.h"
#include <string.h>
#include "frontend_font_rows.inc"

void fp_layout_tile(uint8_t pattern,uint8_t out[16]) FP_CHROME_BANKED {
    uint8_t row;
    memset(out,0,16);
    if(pattern==FP_LAYOUT_SPINE){
        /* Match the full 40px sidebar footprint used by TARGET and footer. */
        for(row=0;row<8;row++)out[row*2]=0;
    }else if(pattern==FP_LAYOUT_SIDE_RULE){
        out[14]=255;
    }else if(pattern==FP_LAYOUT_CONTENT_RULE){
        for(row=0;row<8;row++)out[row*2]=255;
        out[14]=255;out[15]=255;
    }
}

void fp_layout_action_tile(const char chars[2],uint8_t column,uint8_t cells,
                           uint8_t out[16]) FP_CHROME_BANKED {
    uint8_t row,j,c,mask;
    memset(out,0,16);
    for(row=0;row<8;row++)out[row*2]=255;
    for(j=0;j<2;j++){
        c=(uint8_t)chars[j];if(!c)c=' ';if(c>=128)c='?';
        for(row=0;row<5;row++){
            mask=j?frontend_font_rows[(uint16_t)c*5+row]<<1:
                   frontend_font_rows[(uint16_t)c*5+row]<<5;
            out[(row+1)*2+1]|=mask;out[(row+1)*2]&=(uint8_t)~mask;
        }
    }
    out[1]=out[15]=255;
    for(row=1;row<7;row++){
        if(!column){out[row*2]|=128;out[row*2+1]|=128;}
        if(column+1==cells){out[row*2]|=1;out[row*2+1]|=1;}
    }
}

uint8_t fp_layout_wrap_line(const char *text,uint8_t offset,uint8_t width,
                            char out[33]) FP_CHROME_BANKED {
    uint8_t n=0,space=0,begin;
    out[0]=0;if(!width||width>32||offset>32)return 0;
    while(offset<32&&text[offset]==' ')offset++;
    begin=offset;
    while(offset<32&&text[offset]&&n<width){
        out[n++]=text[offset++];if(out[n-1]==' ')space=n;
    }
    if(offset<32&&text[offset]&&text[offset]!=' '&&space){
        n=space-1;offset=begin+space;
    }
    while(n&&out[n-1]==' ')n--;
    out[n]=0;while(offset<32&&text[offset]==' ')offset++;
    return offset<32&&text[offset]?offset:0;
}
