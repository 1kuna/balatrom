#ifdef __SDCC
#pragma bank 255
#define SHOP_LABEL_BANKED __banked
#else
#define SHOP_LABEL_BANKED
#endif
#include <stdint.h>
#include <string.h>
#ifdef __SDCC
#include "a1_ui_runtime.h"
#include "paged_inventory.h"
#include "joker_ids.h"
#include "joker_history.h"
#include "frontend_numeric.h"
#include "num_extra.h"
#else
typedef struct {const char *s;uint8_t x,y,font,color;} A1Text;
#endif

/* Pure shop-only text. Copy into the caller's 15-byte RAM buffer; no pointer
 * into this switched ROM bank escapes. Full names elsewhere are unchanged.
 * The existing 14-character row keeps its final character blank as a gutter.
 * IDs are the immutable catalog order, not a second gameplay eligibility list.
 */
uint8_t frontend_shop_joker_short(uint8_t id,char out[15]) SHOP_LABEL_BANKED {
    const char *s;
    switch(id){
        case 3:s="WRATHFUL JKR";break;
        case 4:s="GLUTTON. JKR";break;
        case 33:s="ABSTRACT JKR";break;
        case 34:s="DELAYED GRAT.";break;
        case 56:s="FACELESS JKR";break;
        case 81:s="RES. PARKING";break;
        case 85:s="FORTUNE TELL.";break;
        case 97:s="SPARE TROUS.";break;
        case 108:s="SOCK/BUSKIN";break;
        case 139:s="SHOOT MOON";break;
        case 140:s="DRIVER'S LIC.";break;
        default:return 0;
    }
    strcpy(out,s);
    return 1;
}

/* Native shop family order: Joker, Planet, Tarot, Spectral, Playing card.
 * Full size/family labels are retained whenever they fit the existing row with
 * its one-character gutter (13 visible chars). REG=REGULAR, JMB=JUMBO and
 * MEG=MEGA only where the full label would exceed that readable extent.
 * Invalid inputs retain shop_view's existing Celestial/Regular fallback.
 */
void frontend_shop_booster_label(uint8_t family,uint8_t size,char out[15]) SHOP_LABEL_BANKED {
    static const char *const labels[5][3]={
        {"REG BUFFOON","JUMBO BUFFOON","MEGA BUFFOON"},
        {"REG CELESTIAL","JMB CELESTIAL","MEG CELESTIAL"},
        {"REG ARCANA","JUMBO ARCANA","MEGA ARCANA"},
        {"REG SPECTRAL","JMB SPECTRAL","MEGA SPECTRAL"},
        {"REG STANDARD","JMB STANDARD","MEGA STANDARD"}
    };
    strcpy(out,labels[family<5?family:1][size<3?size:0]);
}

#include "retail_text_joker_names.inc"

#include "retail_text_planet_labels.inc"

#include "retail_text_tarot_labels.inc"

#include "retail_text_spectral_labels.inc"

/* Private UI copy API. Caller must establish public identity before passing id.
 * Names include the catalog; descriptions cover the 102-id release pool
 * (all 102 ids), all Tarots, Planets and Spectrals. */
void frontend_shop_name(uint8_t kind,uint8_t id,char out[33]) SHOP_LABEL_BANKED {
 const char *s="ITEM";
 if(kind==0&&id<150)s=joker_names[id];
 else if(kind==1&&id<12)s=planet_labels[id];
 else if(kind==2&&id<22)s=tarot_labels[id];
 else if(kind==3&&id<18)s=spectral_labels[id];
 strcpy(out,s);
}

void frontend_shop_effect(uint8_t kind,uint8_t id,char out[33]) SHOP_LABEL_BANKED {
 const char *s="EFFECT NOT AVAILABLE";
 if(kind==0)switch(id){
 case 0:s="+4 MULT";break;
 case 1:s="+3 MULT / SCORED DIAMOND";break;
 case 2:s="+3 MULT / SCORED HEART";break;
 case 3:s="+3 MULT / SCORED SPADE";break;
 case 4:s="+3 MULT / SCORED CLUB";break;
 case 5:s="+8 MULT IF HAND HAS PAIR";break;
 case 6:s="+12 MULT IF HAND HAS 3-KIND";break;
 case 7:s="+10 MULT IF HAND HAS TWO PAIR";break;
 case 8:s="+12 MULT IF HAND HAS STRAIGHT";break;
 case 9:s="+10 MULT IF HAND HAS FLUSH";break;
 case 10:s="+50 CHIPS IF HAND HAS PAIR";break;
 case 11:s="+100 CHIPS IF HAND HAS 3-KIND";break;
 case 12:s="+80 CHIPS IF HAND HAS TWO PAIR";break;
 case 13:s="+100 CHIPS IF HAND HAS STRAIGHT";break;
 case 14:s="+80 CHIPS IF HAND HAS FLUSH";break;
 case 15:s="+20 MULT IF PLAYING 1-3 CARDS";break;
 case 16:s="X1 / EMPTY SLOT; STENCILS ADD X1";break;
 case 17:s="4-CARD STRAIGHTS AND FLUSHES";break;
 case 18:s="RETRIGGER HELD CARD EFFECTS";break;
 case 21:s="+30 CHIPS / DISCARD LEFT";break;
 case 22:s="+15 MULT WITH NO DISCARDS LEFT";break;
 case 27:s="RETRIGGER SCORED CARDS LAST HAND";break;
 case 28:s="+2X LOWEST HELD RANK TO MULT";break;
 case 30:s="+8 MULT / SCORED A,2,3,5,8";break;
 case 31:s="X1 +0.2X PER STEEL CARD IN DECK";break;
 case 32:s="+30 CHIPS / SCORED FACE";break;
 case 33:s="+3 MULT / OWNED JOKER";break;
 case 34:s="NO DISCARDS USED: $2 / ONE LEFT";break;
 case 35:s="RETRIGGER SCORED 2,3,4,5";break;
 case 36:s="ALL CARDS COUNT AS FACE CARDS";break;
 case 38:s="+4 MULT / SCORED 2,4,6,8,10";break;
 case 39:s="+31 CHIPS / SCORED A,3,5,7,9";break;
 case 40:s="SCORED ACE: +20 CHIPS,+4 MULT";break;
 case 42:s="+MULT = TIMES HAND TYPE PLAYED";break;
 case 43:s="+1 MULT/HAND; SCORED FACE RESETS";break;
 case 47:s="X3 IF ALL HELD CARDS ARE BLACK";break;
 case 48:s="GAINS +15 CHIPS PER STRAIGHT";break;
 case 49:s="STARTS +100 CHIPS, -5/HAND, GONE";break;
 case 51:s="ALL PLAYED CARDS SCORE";break;
 case 52:s="+2 CHIPS / CARD LEFT TO DRAW";break;
 case 54:s="GAINS +0.1X PER PLANET USED";break;
 case 55:s="SCORED CARDS KEEP +5 CHIPS";break;
 case 56:s="DISCARD 3+ FACE CARDS: EARN $5";break;
 case 57:s="GAINS +1 MULT/HAND, -1/DISCARD";break;
 case 61:s="X3 REPEAT HAND TYPE THIS ROUND";break;
 case 62:s="GAINS +3 MULT PER SKIPPED PACK";break;
 case 64:s="GAINS +4 CHIPS PER 4-CARD HAND";break;
 case 68:s="STRAIGHTS ALLOW ONE-RANK GAPS";break;
 case 69:s="GAINS +0.25X/CARD ADDED TO DECK";break;
 case 71:s="X1.5 MULT / HELD KING";break;
 case 72:s="ROUND END: $1 / NINE IN DECK";break;
 case 73:s="STARTS $1/ROUND; +$2/BOSS BEATEN";break;
 case 77:s="FIRST SCORED FACE: X2 MULT";break;
 case 80:s="+4 MULT / CARD BELOW START SIZE";break;
 case 81:s="HELD FACE: 1 IN 2 TO EARN $1";break;
 case 85:s="+1 MULT / TAROT USED THIS RUN";break;
 case 88:s="+25 CHIPS / STONE CARD IN DECK";break;
 case 89:s="EARN $4 AT ROUND END";break;
 case 92:s="+2 CHIPS / DOLLAR OWNED";break;
 case 95:s="GAINS +2 MULT PER SHOP REROLL";break;
 case 96:s="STARTS +20 MULT, -4/ROUND, GONE";break;
 case 97:s="GAINS +2 MULT PER TWO PAIR";break;
 case 99:s="STARTS X2, -0.01X/CARD DISCARDED";break;
 case 100:s="SCORED 4/10: +10 CHIPS,+4 MULT";break;
 case 101:s="RETRIGGER SCORED; 10 HANDS TOTAL";break;
 case 103:s="+5 MULT / SCORED FACE";break;
 case 107:s="X3 MULT ON LAST HAND OF ROUND";break;
 case 108:s="RETRIGGER SCORED FACE CARDS";break;
 case 109:s="+MULT = OTHER JOKERS SELL VALUE";break;
 case 112:s="RED SUITS MATCH; BLACK MATCH";break;
 case 114:s="FIRST SCORED CARD TRIGGERS 3X";break;
 case 117:s="+50 CHIPS / SCORED SPADE";break;
 case 118:s="+7 MULT / SCORED CLUB";break;
 case 122:s="COPY COMPATIBLE JOKER ON RIGHT";break;
 case 123:s="GAINS +8 CHIPS PER SCORED 2";break;
 case 130:s="X2 MULT IF HAND HAS PAIR";break;
 case 131:s="X3 MULT IF HAND HAS 3-KIND";break;
 case 132:s="X4 MULT IF HAND HAS 4-KIND";break;
 case 133:s="X3 MULT IF HAND HAS STRAIGHT";break;
 case 134:s="X2 MULT IF HAND HAS FLUSH";break;
 case 137:s="COPY COMPATIBLE LEFTMOST JOKER";break;
 case 138:s="ROUND END: $1 / PLANET TYPE USED";break;
 case 139:s="+13 MULT / HELD QUEEN";break;
 case 140:s="X3 WITH 16+ ENHANCED DECK CARDS";break;
 case 144:s="+2 MULT / EACH $5 OWNED";break;
 /* Release-pool descriptions; wording follows
  * the implemented handlers in joker_effects/card_mutation/num_counters/economy. */
 case 19:s="SPEND DOWN TO -$20";break;
 case 24:s="X4 EVERY 6TH HAND PLAYED";break;
 case 26:s="RANDOM +0 TO +23 MULT";break;
 case 41:s="SCORED FACE: 1 IN 2 TO EARN $2";break;
 case 45:s="ROUND END: +$3 SELL VALUE";break;
 case 74:s="+0.2X / HAND; MOST PLAYED RESETS";break;
 case 75:s="SCORED FACE CARDS TURN GOLD";break;
 case 83:s="+$1 INTEREST / $5 OWNED";break;
 case 90:s="GAINS +0.25X PER LUCKY TRIGGER";break;
 case 91:s="X1.5 MULT / UNCOMMON JOKER";break;
 case 105:s="SCORED GOLD CARD: EARN $4";break;
 case 115:s="SCORED DIAMOND: EARN $1";break;
 case 116:s="SCORED HEART: 1 IN 2 FOR X1.5";break;
 case 121:s="X3 IF HAND HAS ALL 4 SUITS";break;
 case 127:s="X2 IF SCORING CLUB + OTHER SUIT";break;
 case 129:s="ROUND: GAINS +0.5X/JACK DISCARD";break;
 case 142:s="PLANETS AND CELESTIAL PACKS FREE";break;
 case 145:s="GAINS +1X / FACE CARD DESTROYED";break;
 case 146:s="X2 MULT / SCORED KING OR QUEEN";break;
 case 147:s="GAINS +1X / 23 CARDS DISCARDED";break;
 case 148:s="DISABLES EVERY BOSS BLIND";break;
 case 149:s="SHOP END: COPY ITEM AS NEGATIVE";break;
 default:s="NOT IN NATIVE JOKER POOL";break;
 }
 else if(kind==1){
 static const char *const planets[12]={"LEVEL UP PAIR","LEVEL UP 3-KIND","LEVEL UP FULL HOUSE","LEVEL UP 4-KIND","LEVEL UP FLUSH","LEVEL UP STRAIGHT","LEVEL UP TWO PAIR","LEVEL UP STRAIGHT FLUSH","LEVEL UP HIGH CARD","LEVEL UP FIVE OF A KIND","LEVEL UP FLUSH HOUSE","LEVEL UP FLUSH FIVE"};
 if(id<12)s=planets[id];
 }else if(kind==2){
 static const char *const tarots[22]={"COPY LAST TAROT/PLANET USED","MAKE UP TO 2 CARDS LUCKY","CREATE UP TO 2 PLANETS","MAKE UP TO 2 CARDS MULT","CREATE UP TO 2 TAROTS","MAKE UP TO 2 CARDS BONUS","MAKE 1 CARD WILD","MAKE 1 CARD STEEL","MAKE 1 CARD GLASS","DOUBLE MONEY; GAIN UP TO $20","1 IN 4: EDITION ON BASE JOKER","RAISE UP TO 2 CARD RANKS","DESTROY UP TO 2 SELECTED CARDS","COPY RIGHT CARD TO LEFT; PICK 2","GAIN JOKER SELL TOTAL; UP TO $50","MAKE 1 CARD GOLD","MAKE 1 CARD STONE","MAKE UP TO 3 CARDS DIAMONDS","MAKE UP TO 3 CARDS CLUBS","MAKE UP TO 3 CARDS HEARTS","CREATE 1 JOKER IF ROOM","MAKE UP TO 3 CARDS SPADES"};
 if(id<22)s=tarots[id];
 }else if(kind==3){
 /* Spectral rows follow consumables.c/creation_effects.c. */
 static const char *const spectrals[18]={"DESTROY 1; ADD 3 ENHANCED FACES","DESTROY 1; ADD 2 ENHANCED ACES","DESTROY 1; ADD 4 ENHANCED 2-10","ADD GOLD SEAL TO 1 CARD","FOIL, HOLO OR POLY ON 1 CARD","CREATE RARE JOKER; MONEY TO $0","HAND TO 1 RANDOM SUIT","HAND TO 1 RANDOM RANK; -1 SIZE","JOKER TO NEGATIVE; -HAND SIZE","DESTROY 5 RANDOM CARDS; EARN $20","COPY RANDOM JOKER; LOSE OTHERS","ADD RED SEAL TO 1 CARD","POLY ON JOKER; DESTROY OTHERS","ADD BLUE SEAL TO 1 CARD","ADD PURPLE SEAL TO 1 CARD","ADD 2 COPIES OF 1 CARD","CREATE A LEGENDARY JOKER","LEVEL UP EVERY POKER HAND"};
 if(id<18)s=spectrals[id];
 }
 strcpy(out,s);
}

/* Playing-card modifier lines. family 0 enhancement, 1 edition,
 * 2 seal; values as game.h. Wording follows scoring.c (scored/held effects),
 * round_end.c (gold held, blue seal) and boss_core.c (purple seal). Each line
 * is at most 29 characters: the native TABLE detail pane (x 42..159, 4 px
 * advance) and the Standard pack row (15 cells) both fit it unwrapped.
 * Negative never occurs on playing cards and has no line. 0 = no line. */
uint8_t frontend_modifier_effect(uint8_t family,uint8_t value,char out[33]) SHOP_LABEL_BANKED {
 static const char *const enh[9]={0,"BONUS: +30 CHIPS","MULT: +4 MULT","WILD: COUNTS AS EVERY SUIT","GLASS: X2 MULT, 1 IN 4 BREAKS","STEEL: X1.5 MULT WHILE HELD","STONE: +50 CHIPS,NO RANK/SUIT","GOLD: $3 IF HELD AT ROUND END","LUCKY: 1/5 +20 MULT, 1/15 $20"};
 static const char *const ed[4]={0,"FOIL: +50 CHIPS","HOLO: +10 MULT","POLY: X1.5 MULT"};
 static const char *const seal[5]={0,"GOLD SEAL: $3 WHEN SCORED","RED SEAL: TRIGGERS TWICE","BLUE SEAL: HELD AT END=PLANET","PURPLE SEAL: TAROT ON DISCARD"};
 const char *s=family==0u&&value<9u?enh[value]:family==1u&&value<4u?ed[value]:family==2u&&value<5u?seal[value]:0;
 out[0]=0;if(!s)return 0;strcpy(out,s);return 1;
}

/* Native TABLE modifier slots. Kept in this bank so native_table.c
 * (bank 82) holds only the guarded call. Contract: the caller passes a public
 * face-up focused card's 4-byte native key (key[1]>>2 enhancement, key[2]&15
 * edition, key[2]>>4 seal) and its details() lines[6][32]. lines[0] is free
 * scratch (its SEL/MONEY row is already published); lines[2..4] are the
 * y50/58/66 slots holding kept chips ("KEPT +n"), full target ("TGT ..") and
 * full score ("SC .."); lines[5] is the caption. Precedence, so every effect
 * of the focused card stays readable:
 *  1. Kept chips join the caption ("<CARD> +n CHIPS") when that fits 29
 *     characters and the caption is not the "CH .. MU .." preview overflow;
 *     otherwise the kept line reads "KEPT +n CHIPS".
 *  2. Effect lines (enhancement, edition, seal) take the first slots; kept,
 *     target and score follow in that order. Occupants that no longer fit are
 *     dropped, score first; the sidebar keeps compact target and score.
 * With no modifier the slots are untouched. New fields are appended at f[n]
 * (x 42, font 3, colour 2); fields of emptied slots draw nothing. Returns n. */
uint8_t frontend_modifier_fields(A1Text *f,uint8_t n,char lines[][32],const uint8_t *key) SHOP_LABEL_BANKED {
 uint8_t m,v,e=0,k,s,o0=0,o1=0,cnt=0,has;
 if(lines[2][0]){
  if((lines[5][0]!='C'||lines[5][1]!='H'||lines[5][2]!=' ')&&strlen(lines[5])+strlen(lines[2]+5)+7u<=29u){strcat(lines[5]," ");strcat(lines[5],lines[2]+5);strcat(lines[5]," CHIPS");lines[2][0]=0;}
  else strcat(lines[2]," CHIPS");
 }
 for(m=0;m<3u;m++){v=m==0u?key[1]>>2:m==1u?key[2]&15u:key[2]>>4;if(v&&frontend_modifier_effect(m,v,lines[0]))e++;}
 if(e){
  for(s=2;s<5u;s++)if(lines[s][0]){if(!cnt)o0=s;else if(cnt==1u)o1=s;cnt++;}
  if(cnt>3u-e)cnt=3u-e;
  if(cnt>1u&&o1!=3u+e)strcpy(lines[3u+e],lines[o1]);
  if(cnt)strcpy(lines[0],lines[o0]);
  for(s=2;s<5u;s++)if(s>=2u+e+cnt)lines[s][0]=0;
  if(cnt)strcpy(lines[2u+e],lines[0]);
  for(m=0,s=2;m<3u;m++){v=m==0u?key[1]>>2:m==1u?key[2]&15u:key[2]>>4;if(v&&frontend_modifier_effect(m,v,lines[s]))s++;}
 }
 for(s=2;s<5u;s++){if(!lines[s][0])continue;has=0;for(k=0;k<n;k++)if(f[k].s==lines[s])has=1;
  if(!has){f[n].s=lines[s];f[n].x=42u;f[n].y=34u+8u*s;f[n].font=3u;f[n].color=2u;n++;}}
 return n;
}

#ifdef __SDCC
/* Owned-joker current values. One line from the same GameState
 * fields and formulas as each handler (joker_num_counters.c, joker_effects.c,
 * joker_missing.c, joker_final12_core.c, joker_economy.c, joker_history.c).
 * Returns 0 where the value depends on the hand being scored (Supernova,
 * Card Sharp, Raised Fist, Mystic-free hand rules), is rolled at scoring
 * (Misprint), is a probability (Business, Parking, Bloodstone) or is already
 * shown (Egg: the SELL line). Rule text is unchanged. Bounded: <=32 chars. */
static void jn_tail(char *out,const char *pre,const char *num,const char *post){strcpy(out,pre);strcat(out,num);strcat(out,post);}
static void jn_u32(char *out,const char *pre,uint32_t v,const char *post){char d[16];frontend_format_number(&v,4,d);jn_tail(out,pre,d,post);}
static void jn_num(char *out,const char *pre,const GbNum *v,const char *post){char d[16];frontend_format_number(v,8,d);jn_tail(out,pre,d,post);}
static uint8_t jn_deck(const GameState *g,uint8_t what){uint8_t i,c=0;const Card *d;
 for(i=0;i<g->deck_count&&i<DECK_CAP;i++){d=&g->deck[i];
  if(what==0u?d->enhancement==ENH_STONE:what==1u?d->enhancement==ENH_STEEL:what==2u?d->enhancement!=ENH_NONE:(d->enhancement!=ENH_STONE&&d->rank==9u))c++;}
 return c;}
uint8_t frontend_joker_now(const GameState *g,uint8_t i,char out[33]) SHOP_LABEL_BANKED {
 uint8_t id=PG_JBYTE(g,i,jokers,0u),k,c;uint32_t a=0,b;const GbNum *st=&PG_JNUM(g,i,joker_state,1u);GbNum t;char d[3];
 out[0]=0;
 switch(id){
 case J_RUNNER:case J_SQUARE:case J_WEE:case J_ICE_CREAM:jn_num(out,"NOW +",st," CHIPS");return 1;
 case J_TROUSERS:case J_GREEN_JOKER:case J_RIDE_THE_BUS:case J_RED_CARD:case J_FLASH:case J_POPCORN:jn_num(out,"NOW +",st," MULT");return 1;
 case J_CONSTELLATION:case J_HOLOGRAM:case J_OBELISK:case J_LUCKY_CAT:case J_HIT_THE_ROAD:case J_RAMEN:jn_num(out,"NOW X",st,"");return 1;
 case J_SELZER:jn_num(out,"",st," HANDS LEFT");return 1;
 case J_ROCKET:jn_num(out,"NOW $",st," AT ROUND END");return 1;
 case J_LOYALTY_CARD:
  for(k=0;k<12u;k++)a+=g->hands_played[k];
  if(num_to_u32_exact(&b,st)||b>a)return 0;
  b=6u-(a-b)%6u;if(b==1u)strcpy(out,"X4 ON NEXT HAND");else jn_u32(out,"X4 IN ",b," HANDS");return 1;
 case J_ABSTRACT:jn_u32(out,"NOW +",3u*g->joker_count," MULT");return 1;
 case J_BANNER:jn_u32(out,"NOW +",30u*g->discards," CHIPS");return 1;
 case J_BLUE_JOKER:if(g->deck_count>DECK_CAP||g->deck_pos>g->deck_count)return 0;jn_u32(out,"NOW +",2u*(g->deck_count-g->deck_pos)," CHIPS");return 1;
 case J_STONE:jn_u32(out,"NOW +",25u*jn_deck(g,0)," CHIPS");return 1;
 case J_STEEL_JOKER:c=jn_deck(g,1);jn_u32(out,"NOW X",1u+c/5u,"");if(c%5u){d[0]='.';d[1]='0'+(c%5u)*2u;d[2]=0;strcat(out,d);}return 1;
 case J_DRIVERS_LICENSE:c=jn_deck(g,2);if(c>=16u)jn_u32(out,"ACTIVE X3: ",c," ENHANCED");else jn_u32(out,"",c,"/16 ENHANCED CARDS");return 1;
 case J_EROSION:jn_u32(out,"NOW +",g->starting_deck_size>g->deck_count?4u*(g->starting_deck_size-g->deck_count):0u," MULT");return 1;
 case J_FORTUNE_TELLER:jn_num(out,"NOW +",&g->tarot_uses," MULT");return 1;
 case J_SWASHBUCKLER:if(!joker_history_sell_total(g,PG_JUID(g,i),&t))return 0;jn_num(out,"NOW +",&t," MULT");return 1;
 case J_BULL:a=g->money>0?(uint32_t)g->money:0u;jn_u32(out,"NOW +",2u*a," CHIPS");return 1;
 case J_BOOTSTRAPS:a=g->money>0?(uint32_t)g->money:0u;jn_u32(out,"NOW +",2u*(a/5u)," MULT");return 1;
 case J_STENCIL:c=JOKER_RULE_BASE;for(k=0;k<g->joker_count;k++)if(PG_JBYTE(g,k,joker_edition,9u)==ED_NEGATIVE)c++;
  if(c<=g->joker_count){strcpy(out,"NOW X1: NO EMPTY SLOT");return 1;}
  a=c-g->joker_count;for(k=0;k<g->joker_count;k++)if(PG_JBYTE(g,k,jokers,0u)==J_STENCIL)a++;jn_u32(out,"NOW X",a,"");return 1;
 case J_MYSTIC_SUMMIT:strcpy(out,g->discards?"OFF: DISCARDS LEFT":"ACTIVE: +15 MULT");return 1;
 case J_DELAYED_GRAT:if(g->discards_used){strcpy(out,"NOW $0: DISCARD USED");return 1;}jn_u32(out,"NOW $",2u*g->discards," AT ROUND END");return 1;
 case J_CLOUD_9:jn_u32(out,"NOW $",jn_deck(g,3)," AT ROUND END");return 1;
 case J_SATELLITE:if(g->used_planet_mask&0xf000u)return 0;for(k=0;k<12u;k++)if(g->used_planet_mask&(1u<<k))a++;jn_u32(out,"NOW $",a," AT ROUND END");return 1;
 default:return 0;
 }
}
/* Owned-consumable current outcome, same fields as consumables.c:
 * Hermit +min(money,20) (money<=0 gives 0), Temperance +min(joker sell total,
 * 50), The Fool's copy target (last Tarot id>0 or last Planet, else none),
 * Planets the current level of their hand. Others are rules: returns 0. */
uint8_t frontend_consumable_now(const GameState *g,uint8_t kind,uint8_t id,char out[33]) SHOP_LABEL_BANKED {
 static const uint8_t hand_of[12]={1,3,6,7,5,4,2,8,0,9,10,11};GbNum t;uint32_t u;
 out[0]=0;
 if(kind==1u&&id<12u){jn_u32(out,"NOW LVL ",g->hand_levels[hand_of[id]],"");return 1;}
 if(kind!=2u)return 0;
 if(id==9u){u=g->money<=0?0u:g->money<20?(uint32_t)g->money:20u;jn_u32(out,"NOW +$",u,"");return 1;}
 if(id==14u){if(!joker_history_sell_total(g,0,&t)||num_to_u32_exact(&u,&t))return 0;jn_u32(out,"NOW +$",u>50u?50u:u,"");return 1;}
 if(id==0u){
  if(g->last_consumable_kind==2u&&g->last_consumable_id>0u&&g->last_consumable_id<22u)jn_tail(out,"COPIES ",tarot_labels[g->last_consumable_id],"");
  else if(g->last_consumable_kind==1u&&g->last_consumable_id<12u)jn_tail(out,"COPIES ",planet_labels[g->last_consumable_id],"");
  else strcpy(out,"NOTHING TO COPY YET");
  return 1;}
 return 0;
}
#endif

void frontend_shop_pack_detail(uint8_t family,uint8_t size,char out[33]) SHOP_LABEL_BANKED {
 static const char *const kinds[5]={"JOKERS","PLANETS","TAROTS","SPECTRALS","CARDS"};
 uint8_t count;family=family<5?family:1;size=size<3?size:0;
 count=(family==0||family==3)?(size?4:2):(size?5:3);
 strcpy(out,"CHOOSE 1 OF 0 ");out[7]=size==2?'2':'1';out[12]='0'+count;strcat(out,kinds[family]);
}
