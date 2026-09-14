
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MATRIX_STRING_H
#define __MATRIX_STRING_H

#include "stdint.h"



void MatrixStringInit(void);
void MatrixDrawStringAt(uint16_t x, uint16_t y, uint8_t* s);
void MatrixDrawStringAtOver(uint16_t x, uint16_t y, uint8_t* s);
void MatrixDrawChar(uint16_t x, uint16_t y, uint8_t ch);
void MatrixSetTextColor(uint16_t color);
void MatrixSetBackColor(uint16_t color);




#endif /*__MATRIX_STRING_H */  
