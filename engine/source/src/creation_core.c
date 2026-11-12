#include "paged_inventory.h"
#pragma bank 255
#include "creation_core.h"
#include "joker_config.h"
#include "joker_ids.h"
#include "boss_optional.h"
#include "raster_runtime.h"
#include <string.h>
static void policy_store(const CreationPolicy*p){uint8_t n;uint16_t i;const uint8_t*b=(const uint8_t*)p;for(i=0;i<sizeof(*p);i+=n){n=sizeof(*p)-i>16?16:sizeof(*p)-i;raster_put_block(4,0xd800u+i,b+i,n);}}
static void policy_load(CreationPolicy*p){uint8_t n;uint16_t i;uint8_t*b=(uint8_t*)p;for(i=0;i<sizeof(*p);i+=n){n=sizeof(*p)-i>16?16:sizeof(*p)-i;raster_get_block(4,0xd800u+i,b+i,n);}}


uint8_t creation_core_error,creation_commit_active;
static uint8_t certified(uint8_t id){uint8_t i;if(id>=J_CAINO&&id<=J_PERKEO)return 1;for(i=0;i<joker_pool_count;i++)if(joker_pool[i]==id)return 1;return 0;}
uint8_t creation_core_generate(uint8_t kind,uint8_t stream) CORE_BANKED {
 CreationPlan plan;CreationPolicy policy;
 CreationSource source={0,0,0,0};CreationRequest request={0,1,0,0,0,255,0};uint8_t flags,i;
 if(creation_commit_active){creation_core_error=30;return 0;}
 creation_core_error=0;if(!creation_policy_defaults(&game,&policy)){creation_core_error=1;return 0;}
 creation_policy_all_unlocked(&policy);for(i=0;i<joker_pool_count;i++)creation_policy_key(policy.acquire_safe,ITEM_JOKER,joker_pool[i],1);for(i=J_CAINO;i<=J_PERKEO;i++)creation_policy_key(policy.acquire_safe,ITEM_JOKER,i,1);request.kind=kind;request.stream=stream;
 if(!creation_prepare(&game,&policy,&source,&request,1,&plan)){creation_core_error=10+plan.error;return 0;}
 creation_commit_active=1;flags=creation_commit(&game,&policy,&plan);creation_commit_active=0;if(!(flags&CREATION_OK)){creation_core_error=20;return 0;}if(kind==ITEM_JOKER)boss_refresh_owned(&game);return 1;
}
uint8_t creation_core_use(uint8_t mode,uint8_t slot,uint8_t kind,uint8_t id) CORE_BANKED {
 CreationPlan plan;CreationPolicy policy;
 CreationSource source;uint8_t i;
 if(creation_commit_active){creation_core_error=30;return 0;}
 if(!creation_effect_supported(kind,id)||!creation_policy_defaults(&game,&policy))return 0;
 creation_policy_all_unlocked(&policy);
 /* Current admitted constructors initialize only instance state. Current use
 callbacks write only instance state, usedPlanets and lastConsumable metadata.
 Chicot boss mutation is deferred until all rollback checks pass in creation_commit. */
 for(i=0;i<joker_pool_count;i++)creation_policy_key(policy.acquire_safe,ITEM_JOKER,joker_pool[i],1);for(i=J_CAINO;i<=J_PERKEO;i++)creation_policy_key(policy.acquire_safe,ITEM_JOKER,i,1);
 policy.use_safe=1;for(i=0;i<game.joker_count;i++)if(!certified(PG_JBYTE(&game,i,jokers,0u)))policy.use_safe=0;
 source.mode=mode;source.slot=slot;source.kind=kind;source.id=id;
 if(!creation_effect_prepare(&game,&policy,&source,&plan))return 0;
 creation_commit_active=1;i=creation_commit(&game,&policy,&plan);creation_commit_active=0;if(i&CREATION_OK)boss_refresh_owned(&game);return i;
}

uint8_t creation_core_offer_begin(void) CORE_BANKED {
 CreationPolicy policy;
 if(creation_commit_active||!creation_policy_defaults(&game,&policy))return 0;
 creation_policy_all_unlocked(&policy);policy_store(&policy);return 1;
}
uint8_t creation_core_offer(uint8_t kind,uint8_t *actual_kind,uint8_t *id) CORE_BANKED {
 CreationPolicy policy;CreationRequest request={0,1,0,1,0,255,0};CreationItem item;uint16_t seed=game.seed;
 if(creation_commit_active||kind<ITEM_PLANET||kind>ITEM_SPECTRAL||!actual_kind||!id)return 0;
 policy_load(&policy);request.kind=kind;request.stream=kind==ITEM_TAROT?CS_PACK_ARCANA:kind==ITEM_PLANET?CS_PACK_CELESTIAL:CS_PACK_SPECTRAL;
 if(!creation_preview_offer(&game,&policy,&request,&item,&seed))return 0;
 policy_store(&policy);*actual_kind=item.kind;*id=item.id;game.seed=seed;return 1;
}
