/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __REG_ICND1065_H
#define __REG_ICND1065_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board_conf.h"

#define NOP() __ASM("NOP")


#define SCANLINE_SET(num) {                   \
  A_GPIO_PORT->scr = num&0x001F;              \
  A_GPIO_PORT->clr = (~(num))&0x001F;         \
}

#define RGB_SET(num) {                        \
  R1_GPIO_PORT->scr = num&0x003F;             \
  R1_GPIO_PORT->clr = (~(num))&0x003F;        \
}

#define RGB_SET_FF() {                        \
  R1_GPIO_PORT->scr = 0x003F;                 \
}

#define RGB_SET_00() {                        \
  R1_GPIO_PORT->clr = 0x003F;                 \
}


#define CLK_LOW			CLK_GPIO_PORT->clr = CLK_GPIO_PIN;
#define CLK_HIGH		CLK_GPIO_PORT->scr = CLK_GPIO_PIN;
#define LAT_LOW			LAT_GPIO_PORT->clr = LAT_GPIO_PIN;
#define LAT_HIGH		LAT_GPIO_PORT->scr = LAT_GPIO_PIN;
#define OE_LOW			OE_GPIO_PORT->clr = OE_GPIO_PIN;
#define OE_HIGH			OE_GPIO_PORT->scr = OE_GPIO_PIN;



#define REG_NUM     16
#define REG_BRIGHT  5
typedef struct _reg{
  uint8_t num;
  uint16_t valueR;
  uint16_t valueG;
  uint16_t valueB;
  uint8_t  latNum; 
}regConf;


#ifdef __cplusplus
}
#endif

#endif /* __REG_ICND!)^% */

/*****************************END OF FILE****/
