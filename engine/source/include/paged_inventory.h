#ifndef PAGED_INVENTORY_H
#define PAGED_INVENTORY_H
#include "game.h"
#define PG_SPAN 1714u
#define PG_PAYLOAD 1682u
#define PG_LIVE 0xb94du
#define PG_DURABLE 0xb29bu
#define PG_JMAX 81u
#define PG_CMAX 255u
uint8_t *pg_jptr(uint8_t,uint8_t) CORE_BANKED;
uint8_t *pg_cptr(uint8_t,uint8_t) CORE_BANKED;
uint8_t pg_fits(uint16_t,uint16_t) CORE_BANKED;
uint8_t pg_resize(uint8_t,uint8_t) CORE_BANKED;
void pg_reset(void) CORE_BANKED;
uint8_t pg_debuff(const GameState*,uint8_t) CORE_BANKED;
uint8_t pg_hidden(const GameState*,uint8_t) CORE_BANKED;
void pg_flags_clear(GameState*,uint8_t) CORE_BANKED;
void pg_flag_set(GameState*,uint8_t,uint8_t,uint8_t) CORE_BANKED;
uint8_t pg_any_debuff(const GameState*) CORE_BANKED;
void pg_flags_remove(GameState*,uint8_t) CORE_BANKED;
void pg_flags_swap(GameState*,uint8_t,uint8_t) CORE_BANKED;
uint32_t pg_hash(uint32_t) CORE_BANKED;
void pg_undo(uint8_t) CORE_BANKED;
uint8_t pg_save(uint16_t) CORE_BANKED;
uint8_t pg_load(const GameState*,uint16_t,uint8_t) CORE_BANKED;
#define PG_JBYTE(g,i,f,o) (*((i)<8u?&(g)->f[i]:pg_jptr((i),(o))))
#define PG_JNUM(g,i,f,o) (*((i)<8u?&(g)->f[i]:(GbNum*)pg_jptr((i),(o))))
#define PG_JUID(g,i) (*((i)<8u?&(g)->joker_uid[i]:(uint16_t*)pg_jptr((i),12u)))
#define PG_CBYTE(g,i,f,o) (*((i)<4u?&(g)->f[i]:pg_cptr((i),(o))))
#define PG_DEBUFF(g,i) ((i)<8u?(((g)->joker_debuff>>(i))&1u):pg_debuff((g),(i)))
void pg_effect_put(uint8_t,const void*) CORE_BANKED;
void pg_effect_get(uint8_t,void*) CORE_BANKED;
void pg_recipe_put(uint8_t,const void*) CORE_BANKED;
void pg_recipe_get(uint8_t,void*) CORE_BANKED;
uint8_t *pg_jfield(const GameState*,uint8_t,uint8_t) CORE_BANKED;
uint8_t *pg_cfield(const GameState*,uint8_t,uint8_t) CORE_BANKED;
uint8_t pg_any_hidden(const GameState*) CORE_BANKED;
uint8_t pg_destroy_enter(void) CORE_BANKED;
void pg_destroy_leave(void) CORE_BANKED;
void pg_destroy_put(uint8_t,uint8_t,uint16_t) CORE_BANKED;
uint16_t pg_destroy_get(uint8_t,uint8_t) CORE_BANKED;
#endif
