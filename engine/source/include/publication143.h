#ifndef PUBLICATION143_H
#define PUBLICATION143_H
#include <gb/gb.h>
extern volatile uint8_t pub143_enabled;
extern __at(0xffec) volatile uint8_t pub143_metadata_bank;
extern __at(0xffed) volatile uint8_t pub143_pending;
extern __at(0xffee) volatile uint8_t pub143_status;
extern __at(0xffef) volatile uint8_t pub143_job_count;
extern volatile uint8_t pub139_record_enabled,pub139_record_busy;
void pub143_boot(void) BANKED;
void pub143_begin(void) BANKED;
uint8_t pub143_prepare(void) BANKED;
uint8_t pub143_finish(void) BANKED;
void pub143_vblank(void) NONBANKED;
void pub139_metadata_handoff(void) BANKED;
#endif
