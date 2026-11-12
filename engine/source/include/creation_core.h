#ifndef CREATION_CORE_H
#define CREATION_CORE_H
#include "creation_service.h"
uint8_t creation_core_use(uint8_t mode,uint8_t slot,uint8_t kind,uint8_t id) CORE_BANKED;
uint8_t creation_core_generate(uint8_t kind,uint8_t stream) CORE_BANKED;
extern uint8_t creation_commit_active;
uint8_t creation_core_offer_begin(void) CORE_BANKED;
uint8_t creation_core_offer(uint8_t kind,uint8_t *actual_kind,uint8_t *id) CORE_BANKED;
#endif
