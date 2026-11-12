#ifndef CREATION_SERVICE_H
#define CREATION_SERVICE_H
#include "game.h"
#define CREATION_KEYS 27
#define CREATION_MAX 2
#define CREATION_NONE 0
#define CREATION_INVENTORY 1
#define CREATION_PACK 2
#define CREATION_RANDOM 0
#define CREATION_RARE 3
#define CREATION_LEGENDARY 4
#define CREATION_TAROT_PLANET 4
#define CREATION_OK 1
#define CREATION_SOURCE_CONSUMED 2
#define CREATION_USE_DISPATCHED 4
#define CREATION_ERROR_INPUT 1
#define CREATION_ERROR_ROOM 2
#define CREATION_ERROR_UNSUPPORTED 3
#define CREATION_ERROR_STALE 4
/* Enumerated original append identity; named desktop streams remain separate. */
enum {CS_FOOL,CS_PRIESTESS,CS_EMPEROR,CS_JUDGEMENT,CS_WRAITH,CS_SOUL,
 CS_SUPERPOSITION,CS_SEANCE,CS_VAGABOND,CS_CARTOMANCER,CS_PURPLE,CS_PACK_ARCANA,CS_PACK_CELESTIAL,CS_PACK_SPECTRAL,CS_PACK_BUFFOON,CS_STANDARD};
typedef struct {
 uint8_t unlocked[CREATION_KEYS],banned[CREATION_KEYS],used[CREATION_KEYS];
 /* Capability, not eligibility: never cull unsupported entries from pools.
    Selected entries lacking nonfailing/no-RNG acquire implementation reject. */
 uint8_t acquire_safe[19];
 uint16_t inflation;
 uint8_t ready,edition_rate,discount_percent,all_eternal;
 uint8_t consumable_buffer,joker_buffer,use_safe;
} CreationPolicy;
typedef struct {uint8_t mode,slot,kind,id;} CreationSource;
typedef struct {
 uint8_t kind,count,rarity,soulable,stream,forced_kind,forced_id;
} CreationRequest; /* forced_kind255 means ordinary pool selection */
typedef struct {uint8_t kind,id,edition,eternal,sell;} CreationItem;
typedef struct {
 uint32_t game_preimage,policy_preimage,draws;
 uint16_t seed_before,seed_after,next_joker_uid;
 int32_t money_before;
 CreationSource source;
 CreationItem items[CREATION_MAX];
 uint8_t inventory_kind[CONSUMABLE_CAP],inventory_id[CONSUMABLE_CAP];
 uint8_t pack_ids[5],pack_kinds[5],inventory_count,pack_count,pack_choices,joker_count;
 uint8_t count,valid,error,set_money_zero;
 uint8_t native_before[20],native_after[20];
 uint32_t seal;
} CreationPlan;
/* Explicit fresh-original unlock defaults + native live-key snapshot. Profiles,
   bans, modified prices and capability extensions must be supplied by caller. */
uint8_t creation_policy_defaults(const GameState *g,CreationPolicy *p) CORE_BANKED;
/* Explicit user/profile choice: does not bypass bans, used keys or mechanic gates. */
void creation_policy_all_unlocked(CreationPolicy *p) CORE_BANKED;
void creation_policy_key(uint8_t *bits,uint8_t kind,uint8_t id,uint8_t set) CORE_BANKED;
uint8_t creation_prepare(const GameState *g,const CreationPolicy *p,
 const CreationSource *source,const CreationRequest *requests,uint8_t count,
 CreationPlan *plan) CORE_BANKED;
/* Returns explicit flags; owns source removal + one use event BEFORE creations.
   No failure after mutation under declared nonfailing/no-RNG capabilities. */
uint8_t creation_commit(GameState *g,const CreationPolicy *p,CreationPlan *plan) CORE_BANKED;
uint8_t creation_effect_supported(uint8_t kind,uint8_t id) CORE_BANKED;
uint8_t creation_effect_prepare(const GameState *g,const CreationPolicy *p,
 const CreationSource *source,CreationPlan *plan) CORE_BANKED;
uint8_t creation_preview_offer(const GameState *g,CreationPolicy *p,const CreationRequest *request,CreationItem *item,uint16_t *seed) CORE_BANKED;
#endif
