/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOARD_MATRIX_H
#define __BOARD_MATRIX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Include LED component Driver */
#include "board_conf.h"
//--------------------------------------------------------------------------
// includes
//--------------------------------------------------------------------------	
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "board_matrixConfig.h"

typedef enum{
  COLOR_WHITE   = 0xFFFF,
  COLOR_BLACK   = 0x0000,
  COLOR_LBLUE   = 0xa7ff, 
  COLOR_BLUE    = 0x001F,
  COLOR_RED     = 0xF800,
  COLOR_GREEN   = 0x07E0,
  COLOR_CYAN    = 0x07ff,
  COLOR_LLGRAY  = 0xf79E,
  COLOR_LGRAY   = 0xd69a,
  COLOR_GRAY    = 0x8410,
  COLOR_DGRAY   = 0x52AA,//0x4208,
  COLOR_YELLOW  = 0xffe0,
} color_e;

void BSP_MatrixPeriodicHandler(void);
void BSP_MatrixInit(void);
void BSP_MatrixReInit(void);
void BSP_MatrixScreenBufferClr(void);
void BSP_MatrixFillRect(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t color565);
void BSP_MatrixRenderRect(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t* buff);
void BSP_MatrixRenderRectOver(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t* buff);
void BSP_MatrixRenderImageRGB444(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t* buff);
void BSP_MatrixSetBrightness(uint8_t bright);
void BSP_MatrixReFresh(void);
void BSP_MatrixDrawTest(void);


#ifdef __cplusplus
}
#endif

#endif /* __BOARD_MATRIX_H */

/*****************************END OF FILE****/
