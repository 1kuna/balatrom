#ifdef __SDCC
#pragma bank 255
#endif
#include "frontend_phase_chrome.h"
#include <string.h>
#include "frontend_phase_chrome_tiles.inc"

static void command(FPChromeCommand *out,uint8_t key,uint8_t x,uint8_t y,
                    uint8_t palette,uint8_t style,const char *s) {
    uint8_t len=0;
    out->key=key;out->x=x;out->y=y;out->palette=palette;out->style=style;
    while(s[len]&&len<24){out->text[len]=s[len];len++;}
    out->text[len]=0;
    out->cells=style==FP_CHROME_HEADING?(len*6u+7u)/8u:(len+1u)/2u;
    if(style==FP_CHROME_TEXT&&y>=16)out->cells=15;
}
uint8_t fp_chrome_command(uint8_t view,uint8_t family,uint8_t ordinal,
                         FPChromeCommand *out) FP_CHROME_BANKED {
    const char *title;
    memset(out,0,sizeof(*out));
    if(view==FP_CHROME_SHOP){
        if(ordinal==0)command(out,FP_KEY_SHOP_TITLE,5,0,0,FP_CHROME_HEADING,"SHOP");
        else if(ordinal==1)command(out,FP_KEY_SHOP_BUY,5,16,2,FP_CHROME_TEXT,"A BUY  START NEXT");
        else if(ordinal==2)command(out,FP_KEY_SHOP_SECONDARY,5,17,0,FP_CHROME_TEXT,"SEL REROLL  B SELL LAST");
    }else if(view==FP_CHROME_PACK){
        if(ordinal==0){
            title=family==FP_CHROME_BUFFOON?"BUFFOON PACK":family==FP_CHROME_CELESTIAL?"CELESTIAL PACK":family==FP_CHROME_ARCANA?"ARCANA PACK":family==FP_CHROME_STANDARD?"STANDARD PACK":"SPECTRAL PACK";
            command(out,FP_KEY_PACK_TITLE,5,1,0,FP_CHROME_HEADING,title);
        }else if(ordinal==1)command(out,FP_KEY_PACK_ACTIONS,5,16,2,FP_CHROME_TEXT,"A CHOOSE  B SKIP");
    }else if(view==FP_CHROME_BLIND_WIN){
        if(ordinal==0)command(out,FP_KEY_BLIND_TITLE,6,6,0,FP_CHROME_HEADING,"BLIND CLEARED");
        else if(ordinal==1)command(out,FP_KEY_BLIND_SCORE_LABEL,6,8,0,FP_CHROME_TEXT,"ROUND SCORE");
        else if(ordinal==2)command(out,FP_KEY_BLIND_ACTIONS,5,16,2,FP_CHROME_TEXT,"A/START: SHOP");
    }else if(view==FP_CHROME_RUN_LOSS){
        if(ordinal==0)command(out,FP_KEY_LOSS_TITLE,6,6,0,FP_CHROME_HEADING,"RUN LOST");
        else if(ordinal==1)command(out,FP_KEY_LOSS_MESSAGE,6,10,0,FP_CHROME_TEXT,"TRY ANOTHER RUN");
        else if(ordinal==2)command(out,FP_KEY_LOSS_ACTIONS,5,16,2,FP_CHROME_TEXT,"START: TITLE");
    }else if(view==FP_CHROME_ANTE8_WIN){
        if(ordinal==0)command(out,FP_KEY_ANTE8_TITLE,6,7,0,FP_CHROME_HEADING,"ANTE 8 COMPLETE");
        else if(ordinal==1)command(out,FP_KEY_ANTE8_MESSAGE,10,10,0,FP_CHROME_HEADING,"YOU WIN");
        else if(ordinal==2)command(out,FP_KEY_ANTE8_ACTIONS,5,16,2,FP_CHROME_TEXT,"START: TITLE");
    }
    if(view==FP_CHROME_INVENTORY){
        if(ordinal==0)command(out,FP_KEY_ITEMS_TITLE,5,2,0,FP_CHROME_HEADING,"CONSUMABLES");
        else if(ordinal==1)command(out,FP_KEY_ITEMS_ACTIONS,5,16,2,FP_CHROME_TEXT,"A USE  B/DOWN BACK");
        else if(ordinal==2)command(out,FP_KEY_ITEMS_SECONDARY,5,17,0,FP_CHROME_TEXT,"LEFT/RIGHT FOCUS");
    }else if(view==FP_CHROME_JOKERS){
        if(ordinal==0)command(out,FP_KEY_JOKERS_TITLE,5,1,0,FP_CHROME_HEADING,"OWNED JOKERS");
        else if(ordinal==1)command(out,FP_KEY_JOKERS_ACTIONS,5,16,2,FP_CHROME_TEXT,"B SELL  DOWN EXIT");
        else if(ordinal==2)command(out,FP_KEY_JOKERS_SECONDARY,5,17,0,FP_CHROME_TEXT,"START/SELECT EXIT");
    }
    return out->key!=0;
}

void fp_chrome_focus_tiles(uint8_t out[32]) FP_CHROME_BANKED {
    /* Caret x5..11, y2..5. Lower bit plane fills teal; upper adds gold. */
    static const uint8_t tiles[32]={
        255,0,255,0,255,7,255,3,255,1,255,0,255,0,255,0,
        255,0,255,0,255,240,255,224,255,192,255,128,255,0,255,0
    };
    memcpy(out,tiles,32);
}

uint8_t fp_chrome_heading_tile(uint8_t key,uint8_t family,uint8_t column,
                               uint8_t out[16]) FP_CHROME_BANKED {
    const uint8_t *tiles=0;
    uint8_t count=0;
    switch(key){
        case FP_KEY_ITEMS_TITLE:tiles=chrome_items;count=9;break;
        case FP_KEY_JOKERS_TITLE:tiles=chrome_jokers;count=9;break;
        case FP_KEY_SHOP_TITLE:tiles=chrome_shop;count=3;break;
        case FP_KEY_PACK_TITLE:
            if(family==FP_CHROME_BUFFOON){tiles=chrome_buffoon;count=9;}
            else if(family==FP_CHROME_CELESTIAL){tiles=chrome_celestial;count=11;}
            else if(family==FP_CHROME_ARCANA){tiles=chrome_arcana;count=9;}
            else if(family==FP_CHROME_STANDARD){tiles=chrome_standard;count=10;}
            else{tiles=chrome_spectral;count=10;}
            break;
        case FP_KEY_BLIND_TITLE:tiles=chrome_blind;count=10;break;
        case FP_KEY_BLIND_ACTIONS:tiles=chrome_shop_action;count=10;break;
        case FP_KEY_LOSS_TITLE:tiles=chrome_loss;count=6;break;
        case FP_KEY_LOSS_ACTIONS:case FP_KEY_ANTE8_ACTIONS:tiles=chrome_title_action;count=9;break;
        case FP_KEY_ANTE8_TITLE:tiles=chrome_ante8;count=12;break;
        case FP_KEY_ANTE8_MESSAGE:tiles=chrome_you_win;count=6;break;
    }
    if(!tiles||column>=count){memset(out,0,16);return 0;}
    memcpy(out,tiles+(uint16_t)column*16u,16);
    return 1;
}
