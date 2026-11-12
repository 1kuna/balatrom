#include "paged_inventory.h"
#pragma bank 255
#include "round_end.h"
#include "scoring.h"
#include "events.h"
#include "boss.h"
#include "joker_repetition.h"
#include "joker_economy.h"
#include "num_extra.h"
#include <string.h>
/* To the Moon is an ownership passive, not a copied score/payout effect.
 * Derive current interest amount from active instances; no lossy counters,
 * acquisition callbacks, new save fields or unsupported voucher cap changes. */
static uint16_t interest(const GameState*g){uint8_t i,units;uint16_t amount=1;
 if(g->money<=0)return 0;units=g->money/5>5?5:g->money/5;
 for(i=0;i<g->joker_count;i++)if(PG_JBYTE(g,i,jokers,0u)==J_TO_THE_MOON&&!pg_debuff(g,i))amount++;
 return units*amount;
}
static const uint8_t hand_planet[12]={/* OWNER_INSTALL_SLOT:hand_planet */};
static uint8_t held_effect(Card *card){uint8_t table=0;if(card->flags&CARD_DEBUFF)return 0;if(card->enhancement==ENH_GOLD)table=1;
 if(card->seal==SEAL_BLUE&&game.consumable_count<PG_CMAX){game_generate_consumable(ITEM_PLANET,hand_planet[game.last_hand]);table=1;}return table;}
uint8_t game_round_end(void) CORE_BANKED {uint8_t i,table;uint16_t rep;int32_t reward;uint32_t dollars;JokerPayoutContext payout;JokerPayoutEffect effect;JokerRepetitionContext c;JokerRepetitionPacket packet;
 joker_event(JE_ROUND_END);if(game.numeric_error)return 0;
 for(i=0;i<game.hand_count;i++){
 Card *card=&game.hand[i];table=held_effect(card);memset(&c,0,sizeof(c));c.phase=JR_HELD_ROUND_END;c.card_debuffed=card->flags&CARD_DEBUFF;c.seal=card->seal;c.held_has_effect=table;c.rank=card->enhancement==ENH_STONE?0:card->rank;jr_defaults(&game,&c);jr_capture(&game,&c,&packet);if(!packet.valid)return 0;
 for(rep=0;rep<packet.total_runs;rep++){if(rep)held_effect(card);if(!(card->flags&CARD_DEBUFF)&&card->enhancement==ENH_GOLD&&!game_adjust_money(3))return 0;}
 }
 reward=(game.blind==2&&game.ante==8?8:3+game.blind)+game.hands+interest(&game);
 memset(&payout,0,sizeof(payout));payout.discards_used_available=payout.planet_history_available=1;payout.discards_used=game.discards_used;payout.discards_left=game.discards;payout.used_planet_mask=game.used_planet_mask;
 for(i=0;i<game.joker_count;i++){payout.joker_index=i;payout.debuffed=PG_DEBUFF(&game,i);joker_dollar_bonus(&game,PG_JBYTE(&game,i,jokers,0u),&payout,&effect);if(effect.numeric_status&(GBN_DOMAIN|GBN_NAN|GBN_DIVZERO))return 0;if(effect.implemented){if(num_to_u32_exact(&dollars,&effect.dollars)||dollars>(uint32_t)(2147483647L-reward))return 0;reward+=(int32_t)dollars;}}
 return game_adjust_money(reward);
}
