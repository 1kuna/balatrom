#pragma bank 255
#include "num_display.h"
#include "gb_num_format.h"
#include <string.h>
void num_display(char *out,const GbNum *value) CORE_BANKED {char text[16];GbNum copy=*value;uint8_t status=num_format(text,&copy);if(status)strcpy(text,"FORMAT ERR");strncpy(out,text,11);out[11]=0;}
