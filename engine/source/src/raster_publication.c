#pragma bank 255
#include "raster_runtime.h"
/* Bank5 owns BG tiles0..255, bank6 owns256..359. Staging is prospective and
 * writable only during the synchronous preparation; no IRQ reads these banks.
 * Rejected attempts may overwrite it. Only a fully admitted attempt publishes. */
void raster_stage_bg(uint8_t pair,const uint8_t *planes) BANKED {
 uint16_t tile=(uint16_t)(pair/20u)*40u+pair%20u;uint8_t half;
 for(half=0;half<2u;half++){
  raster_put_block(tile<256u?5u:6u,0xd000u+(tile&255u)*16u,planes+half*16u,16);
  tile+=20u;
 }
}
static uint8_t pair_dirty(const uint8_t *dirty,uint8_t pair){
 return !!(dirty[pair>>3]&(1u<<(pair&7u)));
}
static uint8_t job(uint8_t count,uint8_t bank,uint16_t source,uint16_t destination,uint8_t vbk,uint8_t blocks){
 if(count>=182u||bank<2u||bank>7u||!blocks||blocks>128u||vbk>1u||
    (source&15u)||(destination&15u)||source<0xd000u||source>0xdff0u||destination<0x8000u||destination>0x9ff0u||
    source+(uint16_t)blocks*16u>0xe000u||destination+(uint16_t)blocks*16u>0xa000u)return 255;
 raster_planes[0]=source;raster_planes[1]=source>>8;raster_planes[2]=destination;raster_planes[3]=destination>>8;
 raster_planes[4]=bank|(vbk<<3);raster_planes[5]=blocks-1u;
 raster_put_block(7,0xdb00u+(uint16_t)count*6u,raster_planes,6);return count+1u;
}
uint8_t raster_publish_prepared(const uint8_t *dirty) BANKED {
 uint16_t start;uint8_t row,half,col,count,bank,pair=0,jobs=0,skip=raster_live;
 /* Coalesce adjacent prepared tiles, bounded by GDMA's128-block and WRAM
  * bank boundaries. The transfer helper refuses LCD-on or invalid ranges. */
 /* Legacy VRAM residency is invalid even if WRAM preparation is reusable. */
 if(!skip||raster_dirty_pairs==180u){
  jobs=job(jobs,5,0xd000,0x8800,1,128);jobs=job(jobs,5,0xd800,0x9000,1,128);jobs=job(jobs,6,0xd000,0x8800,0,104);
 }else for(row=0;row<9u;row++,pair+=20u)for(half=0;half<2u;half++){
  col=0;
  while(col<20u){
   if(!pair_dirty(dirty,pair+col)){col++;continue;}
   start=(uint16_t)row*40u+half*20u+col;count=0;bank=start<256u?5u:6u;
   do{col++;count++;}while(col<20u&&(start+count<256u?5u:6u)==bank&&pair_dirty(dirty,pair+col));
   jobs=job(jobs,bank,0xd000u+(start&255u)*16u,0x8800u+(start&255u)*16u,bank==5u,count);if(jobs==255u)return 0;
  }
 }
 if(raster_upper_count)jobs=job(jobs,raster_build_bank,0xd600u,0x8000u,0,raster_upper_count*2u);
 if(raster_lower_count)jobs=job(jobs,raster_build_bank,0xd880u,0x8280u,0,raster_lower_count*2u);
 if(jobs==255u)return 0;
 if(skip&&raster_dirty_pairs<180u)skip|=2u;
 raster_begin_publish();
 if(jobs&&!raster_publish_jobs(jobs))return 0;
 /* Raw transfer routines have restored bank1 before these polls. They keep
  * short press/release edges observable while the LCD stops VBlank capture. */
 raster_capture_inputs();
 if(!raster_publish_maps(skip))return 0;
 raster_capture_inputs();
 raster_end_publish();raster_capture_inputs();return 1;
}
