#ifdef __SDCC
#pragma bank 255
#endif
#include "creation_service.h"
#include <string.h>
uint8_t creation_effect_supported(uint8_t kind,uint8_t id) CORE_BANKED {
 return (kind==ITEM_TAROT&&(id==0||id==2||id==4||id==20))||
  (kind==ITEM_SPECTRAL&&(id==5||id==16));
}
uint8_t creation_effect_prepare(const GameState *g,const CreationPolicy *p,const CreationSource *source,CreationPlan *plan) CORE_BANKED {
 CreationRequest request;
 if(!plan)return 0;memset(plan,0,sizeof(*plan));plan->error=CREATION_ERROR_INPUT;
 if(g!=&game||!source||source->mode==CREATION_NONE||!creation_effect_supported(source->kind,source->id))return 0;
 memset(&request,0,sizeof request);request.forced_kind=255;request.count=1;
 if(source->kind==ITEM_TAROT){
  if(source->id==0){
   if(!((g->last_consumable_kind==ITEM_TAROT&&g->last_consumable_id>0&&g->last_consumable_id<22)||
       (g->last_consumable_kind==ITEM_PLANET&&g->last_consumable_id<12)))return 0;
   request.kind=CREATION_TAROT_PLANET;request.stream=CS_FOOL;
   request.forced_kind=g->last_consumable_kind;request.forced_id=g->last_consumable_id;
  }else if(source->id==2){request.kind=ITEM_PLANET;request.count=2;request.stream=CS_PRIESTESS;}
  else if(source->id==4){request.kind=ITEM_TAROT;request.count=2;request.stream=CS_EMPEROR;}
  else {request.kind=ITEM_JOKER;request.stream=CS_JUDGEMENT;}
 }else {
  request.kind=ITEM_JOKER;
  if(source->id==5){request.rarity=CREATION_RARE;request.stream=CS_WRAITH;}
  else {request.rarity=CREATION_LEGENDARY;request.stream=CS_SOUL;}
 }
 return creation_prepare(g,p,source,&request,1,plan);
}
