#ifndef AUDIO_SFX_H
#define AUDIO_SFX_H
#include <gb/gb.h>
#include "game.h"
enum {SFX_SELECT=1,SFX_PLAY,SFX_DISCARD,SFX_BUY,SFX_WIN,SFX_LOSS,SFX_DENIED};
void audio_sfx_init(void) NONBANKED;
void audio_sfx_emit(uint8_t cue) NONBANKED;
void audio_sfx_before(const GameState *state,uint8_t pressed) NONBANKED;
void audio_sfx_after(const GameState *state) NONBANKED;
void audio_sfx_tick(void) NONBANKED;
#endif
