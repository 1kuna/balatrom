#pragma bank 255
#include <gb/gb.h>
#include "pb_selection_tables.h"
uint16_t pb_selection_lines_label_000(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_label_011(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_001(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_002(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_003(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_004(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_005(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_006(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_007(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_008(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_009(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_010(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_012(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_013(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_014(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_015(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_016(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_017(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_018(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_019(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_020(uint16_t,uint8_t) BANKED;
uint16_t pb_selection_lines_number_021(uint16_t,uint8_t) BANKED;
static uint8_t label_equal(const char *a,const char *b){
 uint8_t c;do{c=(uint8_t)*a++;if(c!=(uint8_t)*b++)return 0;}while(c);return 1;
}
uint8_t pb_selection_label_kind(const char *s) BANKED {
 if(label_equal(s,"HIGH CARD"))return 0u;
 if(label_equal(s,"PAIR"))return 1u;
 if(label_equal(s,"TWO PAIR"))return 2u;
 if(label_equal(s,"THREE OF A KIND"))return 3u;
 if(label_equal(s,"STRAIGHT"))return 4u;
 if(label_equal(s,"FLUSH"))return 5u;
 if(label_equal(s,"FULL HOUSE"))return 6u;
 if(label_equal(s,"FOUR OF A KIND"))return 7u;
 if(label_equal(s,"STRAIGHT FLUSH"))return 8u;
 if(label_equal(s,"FIVE OF A KIND"))return 9u;
 if(label_equal(s,"FLUSH HOUSE"))return 10u;
 if(label_equal(s,"FLUSH FIVE"))return 11u;
 if(label_equal(s,"SELECT CARDS"))return 12u;
 if(label_equal(s,"????"))return 13u;
 return 255u;
}
uint16_t pb_selection_label_record(uint8_t profile,uint16_t value,uint8_t pair) BANKED {
 if(pair==92u)return 65535u;
 if(profile==0u&&value>=0u&&value<1301u)return pb_selection_lines_label_000(value,pair);
 if(profile==10u&&value>=0u&&value<1301u)return pb_selection_lines_label_011(value,pair);
 return 65535u;
}
uint16_t pb_selection_number_record(uint8_t profile,uint8_t role,uint16_t value,uint8_t pair) BANKED {
 if(pair==92u)return 65535u;
 if(profile==0u&&role==0u&&value>=0u&&value<2048u)return pb_selection_lines_number_001(value,pair);
 if(profile==0u&&role==0u&&value>=2048u&&value<4096u)return pb_selection_lines_number_002(value,pair);
 if(profile==0u&&role==0u&&value>=4096u&&value<6144u)return pb_selection_lines_number_003(value,pair);
 if(profile==0u&&role==0u&&value>=6144u&&value<8192u)return pb_selection_lines_number_004(value,pair);
 if(profile==0u&&role==0u&&value>=8192u&&value<10001u)return pb_selection_lines_number_005(value,pair);
 if(profile==0u&&role==1u&&value>=0u&&value<2048u)return pb_selection_lines_number_006(value,pair);
 if(profile==0u&&role==1u&&value>=2048u&&value<4096u)return pb_selection_lines_number_007(value,pair);
 if(profile==0u&&role==1u&&value>=4096u&&value<6144u)return pb_selection_lines_number_008(value,pair);
 if(profile==0u&&role==1u&&value>=6144u&&value<8192u)return pb_selection_lines_number_009(value,pair);
 if(profile==0u&&role==1u&&value>=8192u&&value<10001u)return pb_selection_lines_number_010(value,pair);
 if(profile==10u&&role==0u&&value>=0u&&value<2048u)return pb_selection_lines_number_012(value,pair);
 if(profile==10u&&role==0u&&value>=2048u&&value<4096u)return pb_selection_lines_number_013(value,pair);
 if(profile==10u&&role==0u&&value>=4096u&&value<6144u)return pb_selection_lines_number_014(value,pair);
 if(profile==10u&&role==0u&&value>=6144u&&value<8192u)return pb_selection_lines_number_015(value,pair);
 if(profile==10u&&role==0u&&value>=8192u&&value<10001u)return pb_selection_lines_number_016(value,pair);
 if(profile==10u&&role==1u&&value>=0u&&value<2048u)return pb_selection_lines_number_017(value,pair);
 if(profile==10u&&role==1u&&value>=2048u&&value<4096u)return pb_selection_lines_number_018(value,pair);
 if(profile==10u&&role==1u&&value>=4096u&&value<6144u)return pb_selection_lines_number_019(value,pair);
 if(profile==10u&&role==1u&&value>=6144u&&value<8192u)return pb_selection_lines_number_020(value,pair);
 if(profile==10u&&role==1u&&value>=8192u&&value<10001u)return pb_selection_lines_number_021(value,pair);
 return 65535u;
}
