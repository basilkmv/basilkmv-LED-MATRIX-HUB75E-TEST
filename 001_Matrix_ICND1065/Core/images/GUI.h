/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GUI_H
#define __GUI_H

/* Includes ------------------------------------------------------------------*/
#include "board_conf.h"
//#define GUI_DRAW_BMP444_12 0  //BGR444 Выравнивание вправо
#define GUI_DRAW_BMPM444_12 1 //RGB444 Выравнивание вправо use this format

typedef struct{
  uint32_t xSize;
  uint32_t ySize;
  uint32_t BytesPerLine;
  uint32_t BitsPerPixel;
  uint8_t* ImageData;  // Pointer to picture data
  uint16_t* Pallete; // Pointer to palette
  uint32_t ImageType;//GUI_DRAW_BMP444_16

} GUI_BITMAP;

#endif /* __GUI_H */
