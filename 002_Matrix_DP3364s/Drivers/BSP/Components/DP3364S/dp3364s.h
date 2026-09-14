/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOARD_MATRIX_LL_H
#define __BOARD_MATRIX_LL_H
#include "board_conf.h"

void matrixInit(void);
void matrixReInit(void);
void matrixRegInit(void);
void matrixRegReInit(void);
void matrixReFresh(void); 
void matrixSetBrightness(uint8_t brigth);


#endif /* __BOARD_MATRIX_LL_H */

/*****************************END OF FILE****/
