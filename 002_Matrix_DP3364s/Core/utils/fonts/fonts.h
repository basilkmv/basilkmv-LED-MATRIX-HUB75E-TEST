#ifndef LGUI_FONTS_H
#define LGUI_FONTS_H 

#include "stdint.h"

typedef struct _MakiseFont
{
    const uint8_t * table;
    uint16_t width;
    uint16_t height;
    const uint8_t * char_width;
    const uint16_t * char_index;
    uint16_t offset;
    uint16_t num_char;
    uint16_t space_char;    
} LguiFont;

extern const LguiFont F_Arial10;
extern const LguiFont F_Arial12;
extern const LguiFont F_Arial15;
extern const LguiFont F_Arial16;
extern const LguiFont F_Arial24;

#endif
