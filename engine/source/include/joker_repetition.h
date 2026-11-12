#ifndef JOKER_REPETITION_H
#define JOKER_REPETITION_H
#include "game.h"
#include "joker_ids.h"

#define JR_SCORED 0
#define JR_HELD 1
#define JR_HELD_ROUND_END 2
#define JR_NONE 255
#define JR_RECIPE_CAP (JOKER_CAP + 1)
#define JR_FROM_SEAL 0
#define JR_FROM_JOKER 1
#define JC_OK 0
#define JC_NO_TARGET 1
#define JC_DEBUFFED 2
#define JC_DEPTH_LIMIT 3
#define JC_INVALID 4

/* All operands and outputs MUST live in RAM across CORE_BANKED calls.
   Slot indexes refer to the live ordered Joker list at query time. */
typedef struct {
 uint8_t issuer_index, target_index, copy_depth, status;
 uint8_t immediate_target, immediate_compatible;
} JokerCopy;
typedef struct {
 uint8_t phase, card_debuffed, seal, is_face, first_scoring_card;
 uint8_t hands_left; /* original post-decrement value; 0 enables Dusk */
 uint8_t held_has_effect; /* next(effects[1]) || #effects > 1 */
 /* Source ability.extra for Chad/Sock/Dusk/Mime/Hack, by TARGET slot.
    Defaults can be filled by jr_defaults; Seltzer always emits one. */
 uint8_t extra[81];
 uint8_t rank; /* source other_card:get_id(), 0 for Stone */
} JokerRepetitionContext;
typedef struct {
 uint8_t kind, issuer_index, target_index, copy_depth, repetitions;
} JokerRepetitionRecipe;
typedef struct {
 uint8_t base_runs, recipe_count, valid;
 uint16_t total_runs;
 JokerRepetitionRecipe recipes[JR_RECIPE_CAP];
} JokerRepetitionPacket;

uint8_t jc_compatible(uint8_t id) CORE_BANKED;
void jc_resolve(const GameState *g, uint8_t issuer, JokerCopy *out) CORE_BANKED;
void jr_defaults(const GameState *g, JokerRepetitionContext *c) CORE_BANKED;
void jr_capture(const GameState *g, const JokerRepetitionContext *c,
                JokerRepetitionPacket *out) CORE_BANKED;
#endif
