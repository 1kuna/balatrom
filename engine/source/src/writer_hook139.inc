/* Recording lifetime is owned by pub143_begin/finish/restore. The BANKED
 * fused leaf checks the physical range and records only changed chunks.
 * Caller splits all recorded chunks at16-byte boundaries. */
extern uint8_t coop_live_held;
volatile uint8_t pub139_record_enabled,pub139_record_busy,pub139_changed;
void pub139_put_changed_raw(void) BANKED;
uint8_t pub139_tracks(uint8_t,uint16_t) BANKED;
void pub139_mark(uint8_t,uint16_t) BANKED;
