#pragma bank 255
#include <gb/gb.h>
#include <string.h>
#include "boss_labels.h"

/* Public boss fields only. Copy into caller-owned Frame tail storage; no
 * switched-bank string pointer escapes. Nine small-font characters fit the
 * existing 36px sidebar. Complete active rule text is already in fixed ROM.
 */
void frontend_boss_sidebar(uint8_t boss,uint8_t disabled,uint8_t ox,
                           char out[15]) BANKED {
    static const char *const hands[]={"HIGH","PAIR","2-PAIR","3-KIND",
        "STR","FLUSH","FULL","4-KIND","STR-FL","5-KIND","FL-HS","FL-5"};
    const char *s;uint8_t i;
    if(disabled){strcpy(out,"DISABLED");return;}
    if(boss==2u&&ox<12u){strcpy(out,"OX ");s=hands[ox];for(i=0;s[i];i++)out[i+3u]=s[i];out[i+3u]=0;return;}
    switch(boss){
        case 0:s="SMALL";break;
        case 1:s="BIG";break;
        case 12:s="C. BELL";break;
        case 22:s="V. LEAF";break;
        case 23:s="V. VESSEL";break;
        case 28:s="A. ACORN";break;
        case 29:s="C. HEART";break;
        default:
            s=boss_name(boss);
            if(s[0]=='T'&&s[1]=='H'&&s[2]=='E'&&s[3]==' ')s+=4;
            if(strlen(s)>9u)s="BOSS";
            break;
    }
    strcpy(out,s);
}
