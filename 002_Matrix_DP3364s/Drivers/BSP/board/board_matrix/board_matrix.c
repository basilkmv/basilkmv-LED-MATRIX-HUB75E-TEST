
/* Includes ------------------------------------------------------------------*/
#include "board_matrix.h"
#include "../../Components/dp3364s/dp3364s.h"
#include "main.h"


#define TEST_COLORS 8
static uint16_t testColor[TEST_COLORS] = {
  0xFFFF,//White
  0xffe0,//Yellow
  0xa7ff,//Light Blue
  0x07E0,//Green
  0xE814,//Puple
  0xF800,//Red
  0x001F,//blue
  0x0000,//black
}; 

#define TIMER_SCREEN        1 //ms
#define TIMER_BRIGHT        50 //ms
#define TIMER_REINIT        50000 //ms

uint8_t screenBuffer[MATRIX_BUFF_SIZE];

static uint32_t timerScreen;
static uint32_t timerBright;
static uint32_t timerReInit;
static volatile uint8_t updateScreen = 0;
static volatile uint8_t updateBright = 0;
static uint16_t currentBright = 50;

//LUT convertation of 5bit to 16bit
static const uint16_t LUT_5[32] = {
     0,  68, 273, 614,1091,1705,2455,3342,4364,5524,6819,8252,9820,11525,13366,15344,
  17458,19708,22095,24618,27278,30074,33006,36075,39280,42622,46100,49714,53465,57352,61375,65535,
};
//LUT convertation of 6bit to 16bit
static const uint16_t LUT_6[64] = {
     0,  17,  66, 149, 264, 413, 594, 809,1057,1337,1651,1998,2378,2790,3236,3715,
  4227,4772,5350,5961,6605,7282,7992,8735,9511,10320,11162,12037,12945,13886,14861,15868,
  16908,17981,19088,20227,21399,22605,23843,25114,26419,27756,29127,30530,31967,33436,34939,36474,
  38043,39645,41279,42947,44648,46381,48148,49948,51781,53647,55545,57477,59442,61440,63471,65535,
  };

void BSP_MatrixPeriodicHandler(){
uint32_t localtime = SystemTimer_GetTick();
  if(localtime - timerScreen > TIMER_SCREEN){
    if (updateScreen){
      matrixReFresh();
      updateScreen = 0;
    }  
    timerScreen = localtime;
  }

  if(localtime - timerBright > TIMER_BRIGHT){
    if(updateBright){
      matrixSetBrightness(currentBright);
      updateBright = 0;
    }  
    timerBright = localtime;
  }
  
  if(localtime - timerReInit > TIMER_REINIT){
    BSP_MatrixReInit();
    timerReInit = localtime;
  }
}
void BSP_MatrixReFresh(void){
  updateScreen = 1;
}
void BSP_MatrixSetBrightness(uint8_t newBright){
  if (newBright != currentBright){
      currentBright = newBright;
      updateBright = 1;
  }
}
void BSP_MatrixInit(void)
{
  //Clear screen buffer
  BSP_MatrixScreenBufferClr();
  //Draw test screen
  BSP_MatrixDrawTest();
  //Init ll driver
  matrixInit();
  //Init config reg of matrix
  matrixRegInit();
  matrixReFresh();
  matrixSetBrightness(currentBright);
  matrixRegReInit();
}
void BSP_MatrixReInit(void)
{
  matrixRegReInit();
  matrixReFresh();
  matrixSetBrightness(currentBright);
}

void BSP_MatrixScreenBufferClr(void){
  for(uint16_t i = 0; i < MATRIX_BUFF_SIZE; i++) 
    screenBuffer[i] = 0x00;
}

void BSP_MatrixDrawTest(void){
  BSP_MatrixFillRect(0,0,127,63,COLOR_WHITE);
  BSP_MatrixFillRect(8,8,127-8,63-8,COLOR_GREEN);
  BSP_MatrixFillRect(16,16,127-16,63-16,COLOR_RED);
  BSP_MatrixFillRect(16+4,16+4,127-16-4,63-16-4,COLOR_CYAN);
  BSP_MatrixFillRect(16+8,16+8,127-16-8,63-16-8,COLOR_BLUE);
  BSP_MatrixFillRect(0, 0,  0, 63, 0xf800);
  BSP_MatrixFillRect(1, 0,  1, 63, 0xe000);
  BSP_MatrixFillRect(2, 0,  2, 63, 0x8000);
  BSP_MatrixFillRect(3, 0,  3, 63, 0x7000);
  BSP_MatrixFillRect(4, 0,  4, 63, 0x4000);
  BSP_MatrixFillRect(5, 0,  5, 63, 0x3800);
  
  BSP_MatrixFillRect(6, 0,  6, 63, 0x07e0);
  BSP_MatrixFillRect(7, 0,  7, 63, 0x0700);
  BSP_MatrixFillRect(8, 0,  8, 63, 0x03e0);
  BSP_MatrixFillRect(9, 0,  9, 63, 0x01e0);
  BSP_MatrixFillRect(10, 0, 10, 63, 0x01c0);
  BSP_MatrixFillRect(11, 0, 11, 63, 0x00e0);
  
  BSP_MatrixFillRect(12, 0,  12, 63, 0x001f);
  BSP_MatrixFillRect(13, 0,  13, 63, 0x001c);
  BSP_MatrixFillRect(14, 0,  14, 63, 0x0010);
  BSP_MatrixFillRect(15, 0,  15, 63, 0x000e);
  BSP_MatrixFillRect(16, 0,  16, 63, 0x0008);
  BSP_MatrixFillRect(17, 0,  17, 63, 0x3807);
}

void BSP_MatrixFillRect(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t color565){
  uint16_t red16Bit = LUT_5[((color565&0xf800) >> 11)];//((color565&0xf800));
  uint16_t green16Bit = LUT_6[((color565&0x07e0) >> 5)];//((color565&0x07e0) << 5);
  uint16_t blue16Bit = LUT_5[(color565&0x001f )];//((color565&0x001f) << 11);
  uint16_t posX,posY;
  uint32_t bufferPos;
  uint16_t inputMask;
  uint8_t outputmask;
  uint8_t rgb = 0x00;
  uint8_t bit;
 
  for (posY = y; posY <= end_y; posY++){
    for (posX = x; posX <= end_x; posX++){
      bufferPos = posX * BYTE_FOR_PIXEL + posY * MATRIX_BUFF_LINE_SIZE;
      outputmask = 0x07;
      inputMask = 0x8000;
      if(posY > DISPLAY_SCAN_LINES - 1){
        outputmask = 0x38;//b00111000
        bufferPos -= DISPLAY_SCAN_LINES * MATRIX_BUFF_LINE_SIZE;
      }
      for(bit = 0; bit < BYTE_FOR_PIXEL; bit++) {
        if( bufferPos + bit >= MATRIX_BUFF_SIZE)
          continue; 
        rgb = 0x00;
        inputMask = 0x8000 >> bit;
        if(red16Bit & inputMask) rgb += 1;
        if(green16Bit & inputMask) rgb += 2;
        if(blue16Bit & inputMask) rgb += 4;
        if(posY > DISPLAY_SCAN_LINES - 1) rgb <<= 3;
        
        screenBuffer[bufferPos + bit] = (screenBuffer[bufferPos + bit] & ~outputmask) | rgb;
      }
    }
  }
}

void BSP_MatrixRenderRect(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t* buff){
  uint16_t red16Bit;
  uint16_t green16Bit;
  uint16_t blue16Bit;
  uint16_t posX,posY;
  uint32_t bufferPos;
  uint16_t inputMask;
  uint8_t outputmask;
  uint8_t rgb = 0x00;
  uint16_t color565;
  uint16_t *p = buff;
  uint8_t bit;

  for (posY = y; posY <= end_y; posY++){
    for (posX = x; posX <= end_x; posX++){
      color565 = *p++;
      if (posY >= DISPLAY_HEIGHT || posX >= DISPLAY_WIDTH)
        continue;
      red16Bit = LUT_5[((color565&0xf800) >> 11)];
      green16Bit = LUT_6[((color565&0x07e0) >> 5)];
      blue16Bit = LUT_5[(color565&0x001f )];
      bufferPos = posX * BYTE_FOR_PIXEL + posY * MATRIX_BUFF_LINE_SIZE;
      outputmask = 0x07;
      inputMask = 0x8000;
      if(posY > DISPLAY_SCAN_LINES - 1){
        outputmask = 0x38;//b00111000
        bufferPos -= DISPLAY_SCAN_LINES * MATRIX_BUFF_LINE_SIZE;
      }
      for(bit = 0; bit < BYTE_FOR_PIXEL; bit++) {
        rgb = 0x00;
        inputMask = 0x8000 >> bit;
        if(red16Bit & inputMask) rgb += 1;
        if(green16Bit & inputMask) rgb += 2;
        if(blue16Bit & inputMask) rgb += 4;
        if(posY > DISPLAY_SCAN_LINES - 1) rgb <<= 3;
        screenBuffer[bufferPos + bit] = (screenBuffer[bufferPos + bit] & ~outputmask) | rgb;
      }
    }
  }
}

void BSP_MatrixRenderRectOver(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t* buff){
  uint16_t red16Bit;
  uint16_t green16Bit;
  uint16_t blue16Bit;
  uint16_t posX,posY;
  uint32_t bufferPos;
  uint16_t inputMask;
  uint8_t outputmask;
  uint8_t rgb = 0x00;
  uint16_t color565;
  uint16_t *p = buff;
  uint8_t bit;

  for (posY = y; posY <= end_y; posY++){
    for (posX = x; posX <= end_x; posX++){
      color565 = *p++;
      if (posY >= DISPLAY_HEIGHT || posX >= DISPLAY_WIDTH || color565 == 0)
        continue;
      red16Bit = LUT_5[((color565&0xf800) >> 11)];
      green16Bit = LUT_6[((color565&0x07e0) >> 5)];
      blue16Bit = LUT_5[(color565&0x001f )];
      bufferPos = posX * BYTE_FOR_PIXEL + posY * MATRIX_BUFF_LINE_SIZE;
      outputmask = 0x07;
      inputMask = 0x8000;
      if(posY > DISPLAY_SCAN_LINES - 1){
        outputmask = 0x38;//b00111000
        bufferPos -= DISPLAY_SCAN_LINES * MATRIX_BUFF_LINE_SIZE;
      }
      for(bit = 0; bit < BYTE_FOR_PIXEL; bit++) {
        rgb = 0x00;
        inputMask = 0x8000 >> bit;
        if(red16Bit & inputMask) rgb += 1;
        if(green16Bit & inputMask) rgb += 2;
        if(blue16Bit & inputMask) rgb += 4;
        if(posY > DISPLAY_SCAN_LINES - 1) rgb <<= 3;
        screenBuffer[bufferPos + bit] = (screenBuffer[bufferPos + bit] & ~outputmask) | rgb;
      }
    }
  }
}

void BSP_MatrixRenderImageRGB444(uint16_t x, uint16_t y, uint16_t end_x, uint16_t end_y, uint16_t* buff){
  uint16_t red16Bit;
  uint16_t green16Bit;
  uint16_t blue16Bit;
  uint16_t posX,posY;
  uint32_t bufferPos;
  uint16_t inputMask;
  uint8_t outputmask;
  uint8_t rgb = 0x00;
  uint16_t color444;
  uint16_t *p = buff;
  uint8_t bit;

  for (posY = y; posY <= end_y; posY++){
    for (posX = x; posX <= end_x; posX++){
      color444 = *p++;
      red16Bit = (color444&0xf00) << 4;
      green16Bit = (color444&0x0f0) << 8;
      blue16Bit = (color444&0x000f) << 12;
      bufferPos = posX * BYTE_FOR_PIXEL + posY * MATRIX_BUFF_LINE_SIZE;
      outputmask = 0x07;
      inputMask = 0x8000;
      if(posY > DISPLAY_SCAN_LINES - 1){
        outputmask = 0x38;//b00111000
        bufferPos -= DISPLAY_SCAN_LINES * MATRIX_BUFF_LINE_SIZE;
      }
      for(bit = 0; bit < BYTE_FOR_PIXEL; bit++) {
        rgb = 0x00;
        inputMask = 0x8000 >> bit;
        if(red16Bit & inputMask) rgb += 1;
        if(green16Bit & inputMask) rgb += 2;
        if(blue16Bit & inputMask) rgb += 4;
        if(posY > DISPLAY_SCAN_LINES - 1) rgb <<= 3;
        screenBuffer[bufferPos + bit] = (screenBuffer[bufferPos + bit] & ~outputmask) | rgb;
      }
    }
  }

}
/*****************************END OF FILE****/
