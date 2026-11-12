#ifndef A1_UI_RUNTIME_H
#define A1_UI_RUNTIME_H
#include <gb/gb.h>
#include <stdint.h>
typedef struct {
 const char *s;
 uint8_t x,y,font,color;
} A1Text;
/* Text positions are native pixels. Fonts are exact offline F3/F5 masks.
 * Every changed cell is rebuilt from its ROM base with all live overlays,
 * preserving fields that share a cell and avoiding a full-frame RAM buffer. */
void a1_ui_publish(uint8_t x,uint8_t y,uint8_t w,uint8_t h,
                   const A1Text *fields,uint8_t count,uint8_t plain) BANKED;
void a1_ui_publish_focus(uint8_t x,uint8_t y,uint8_t w,uint8_t h,
                   const A1Text *fields,uint8_t count,uint8_t plain) BANKED;
uint8_t a1_text_width(const char *s,uint8_t font) BANKED;
uint8_t a1_number_text(const void *v,uint8_t size,char out[16],uint8_t width,uint8_t big) BANKED;
#endif
