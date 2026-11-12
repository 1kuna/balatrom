/* Called only after the transaction's final validation and commit barrier.
 * Fast path owns no content/cache and changes only LCDC.ON. The late-line
 * fallback retains GBDK's original safe display_off behavior. */
static void coop_blank_vblank(void){
 uint8_t line,blanked=0;
 if(!(LCDC_REG&LCDCF_ON))return;
 if(!(KEY1_REG&128u)){display_off();return;}
 CRITICAL {
  line=LY_REG;
  /* Two complete following VBlank scanlines bound the small register leaf.
   * No ROM call, loop, input dispatch or publication occurs in this block. */
  if(line>=144u&&line<=151u){LCDC_REG&=~LCDCF_ON;blanked=1;}
 }
 if(!blanked)display_off();
}
