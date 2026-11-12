#ifndef HAND_PREVIEW_H
#define HAND_PREVIEW_H
#include "game.h"
typedef struct {char label[31];uint32_t chips,mult;uint16_t level;uint8_t hand_type,hidden,valid;} HandPreview;
void hand_preview(const GameState *g,HandPreview *out) CORE_BANKED;
#endif
