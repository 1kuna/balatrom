/* Main-thread publication. Boot installs the callback only when
 * PRIVATE_PUB143_NATIVE_TEST is enabled; the default build leaves it disabled.
 */
#include "publication143.h"
void raster_ferry_window(void) NONBANKED;
void _standard_VBL_handler(void) NONBANKED;
static uint8_t pub143_prepared,pub143_metadata_bits[12];
void pub143_boot(void) BANKED {
 CRITICAL {
  native_palette_ready=0;pub143_enabled=0;
  pub143_metadata_bank=2;pub143_pending=pub143_status=pub143_job_count=0;
  pub139_record_enabled=pub139_record_busy=pub143_prepared=0;
#ifdef PRIVATE_PUB143_NATIVE_TEST
  /* Boot precedes renderer/audio installation: standard is the sole node.
   * Supported callback API keeps it present, immediately after the finisher.
   */
  remove_VBL(_standard_VBL_handler);add_VBL(pub143_vblank);add_VBL(_standard_VBL_handler);
  pub143_enabled=1;
#endif
 }
}
void pub143_begin(void) BANKED {
 uint8_t zero[16];uint16_t at;
 pub143_prepared=pub139_record_enabled=0;
 if(!pub143_enabled||!coop_live_held||coop_ready_held!=1u||native_palette_ready!=1u||
    raster_live||raster_fault||pub143_metadata_bank!=2u)return;
 memset(zero,0,sizeof(zero));
 for(at=0xdbb0u;at<0xdc00u;at+=16u)raster_put_block(7u,at,zero,16u);
 raster_put_block(7u,0xdc70u,zero,12u);
 if(coop_checkpoint())pub139_record_enabled=1;
}
uint8_t pub143_prepare(void) BANKED {
 uint8_t *bits=raster_pixels,jobs[98],count,blocks,i,value,metadata_blocks=0;
 pub143_prepared=0;
 if(!pub139_record_enabled||!coop_checkpoint()||!coop_resident||!coop_live_held||
    coop_ready_held!=1u||native_palette_ready!=1u||raster_live||raster_fault||
    native_table_bank1_owner!=NATIVE_BANK1_VALID||pub143_metadata_bank!=2u||
    SCX_REG||SCY_REG||_shadow_OAM_base!=0xc0u||!(KEY1_REG&0x80u)||
    (LCDC_REG&0x98u)!=0x90u)return 0;
 pub147_read_journal();
 raster_get_block(7u,0xdc70u,pub143_metadata_bits,12u);
 /* Count only set metadata bits; the original bitmap is preserved. */
 for(i=0;i<12u;i++){value=pub143_metadata_bits[i];while(value){value&=value-1u;metadata_blocks++;}}
 if(metadata_blocks>16u)return 0;
 /* A cancelled selection can return to the already displayed image. The
  * scheduler rejects zero jobs; preserve its existing IRQ contract with one
  * unchanged map block, rather than falling back to an LCD-off full transfer. */
 value=0;for(i=0;i<80u;i++)value|=bits[i];
 if(!value){jobs[0]=0; jobs[1]=0xd2u; jobs[2]=0; jobs[3]=0x98u; jobs[4]=0; jobs[5]=7; jobs[6]=0;count=blocks=1u;}
 else if(!pub138_make_jobs(bits,jobs,&count,&blocks))return 0;
 if(!coop_checkpoint())return 0;
 /* Staging is complete: descriptor writes are outside the journal. */
 pub139_record_enabled=0;
 raster_put_block(7u,0xdc00u,jobs,count*7u);
 if(!coop_checkpoint())return 0;
 pub143_job_count=count;pub143_prepared=1;return 1;
}
uint8_t pub143_finish(void) BANKED {
 uint8_t buf[16],i;uint16_t at;
 if(!pub143_prepared||!pub143_enabled||pub143_metadata_bank!=2u||raster_fault||
    native_palette_ready!=1u||!coop_live_held||_shadow_OAM_base!=0xc0u)return 0;
 pub143_prepared=0;
 /* Prepared finish may enter outside VBlank. Guard the short final arm and
  * revalidate IRQ-owned state after the wait before publishing anything. */
 raster_ferry_window();
 if(!pub143_enabled||pub143_metadata_bank!=2u||raster_fault||
    native_palette_ready!=1u||!coop_live_held||_shadow_OAM_base!=0xc0u)return 0;
 CRITICAL {pub143_status=0;pub143_pending=1;}
 wait_vbl_done();
 if(pub143_status!=2u){pub143_pending=pub143_status=0;return 0;}
 /* Atomic displayed NEW already exists. Main stays the sole owner; input
  * callbacks continue queuing. No game/RNG/input dispatch is called here.
  */
 coop_active=0;
 for(at=0;at<160u;at+=16u){raster_get_block(7u,0xda00u+at,buf,16u);memcpy((uint8_t*)0xc000u+at,buf,16u);}
 raster_ferry_window();CRITICAL {_shadow_OAM_base=0xc0u;}
 for(i=0;i<96u;i++)if(pub143_metadata_bits[i>>3]&(1u<<(i&7u))){
  at=0xd300u+(uint16_t)i*16u;
  raster_get_block(3u,at,buf,16u);raster_put_block(2u,at,buf,16u);
 }
 raster_ferry_window();pub139_metadata_handoff();
 pub143_status=0;return 1;
}
