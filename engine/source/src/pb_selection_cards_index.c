#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
uint16_t pb_selection_cards_000(uint8_t,uint16_t,uint16_t) BANKED;
uint16_t pb_selection_cards_001(uint8_t,uint16_t,uint16_t) BANKED;
uint16_t pb_selection_cards_002(uint8_t,uint16_t,uint16_t) BANKED;
uint16_t pb_selection_cards_003(uint8_t,uint16_t,uint16_t) BANKED;
uint16_t pb_selection_cards_record(uint8_t profile,uint8_t pair,uint16_t left,uint16_t right) BANKED {
 if(profile==0u&&pair>=105u&&pair<=128u)return pb_selection_cards_000(pair,left,right);
 if(profile==0u&&pair>=129u&&pair<=153u)return pb_selection_cards_001(pair,left,right);
 if(profile==0u&&pair>=154u&&pair<=179u)return pb_selection_cards_002(pair,left,right);
 if(profile==10u&&pair>=105u&&pair<=179u)return pb_selection_cards_003(pair,left,right);
 return 65535u;
}
