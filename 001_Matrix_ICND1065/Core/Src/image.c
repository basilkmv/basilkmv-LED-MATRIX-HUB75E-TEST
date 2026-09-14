
#include "image.h"
#include "main.h"
#include "board_led.h"
#include "board_matrix.h"
#include "SystemTimer.h"
#include "matrix_string.h"


//#define TEST
#define PERIOD  20
static uint32_t timer;
static void process(void);
static uint16_t* pImage;

#if defined TEST
#define N_FRAMES 2
#include "..//images/frame_00.c"
#include "..//images/frame_01.c"

static uint16_t* img[N_FRAMES] = {
  (uint16_t*)_acframe_00,
  (uint16_t*)_acframe_01,
  };
#else
#define N_FRAMES 54
#include "..//images/frame_00.c"
#include "..//images/frame_01.c"
#include "..//images/frame_02.c"
#include "..//images/frame_03.c"
#include "..//images/frame_04.c"
#include "..//images/frame_05.c"
#include "..//images/frame_06.c"
#include "..//images/frame_07.c"
#include "..//images/frame_08.c"
#include "..//images/frame_09.c"
#include "..//images/frame_10.c"
#include "..//images/frame_11.c"
#include "..//images/frame_12.c"
#include "..//images/frame_13.c"
#include "..//images/frame_14.c"
#include "..//images/frame_15.c"
#include "..//images/frame_16.c"
#include "..//images/frame_17.c"
#include "..//images/frame_18.c"
#include "..//images/frame_19.c"
#include "..//images/frame_20.c"
#include "..//images/frame_21.c"
#include "..//images/frame_22.c"
#include "..//images/frame_23.c"
#include "..//images/frame_24.c"
#include "..//images/frame_25.c"
#include "..//images/frame_26.c"
#include "..//images/frame_27.c"
#include "..//images/frame_28.c"
#include "..//images/frame_29.c"
#include "..//images/frame_30.c"
#include "..//images/frame_31.c"
#include "..//images/frame_32.c"
#include "..//images/frame_33.c"
#include "..//images/frame_34.c"
#include "..//images/frame_35.c"
#include "..//images/frame_36.c"
#include "..//images/frame_37.c"
#include "..//images/frame_38.c"
#include "..//images/frame_39.c"
#include "..//images/frame_40.c"
#include "..//images/frame_41.c"
#include "..//images/frame_42.c"
#include "..//images/frame_43.c"
#include "..//images/frame_44.c"
#include "..//images/frame_45.c"
#include "..//images/frame_46.c"
#include "..//images/frame_47.c"
#include "..//images/frame_48.c"
#include "..//images/frame_49.c"
#include "..//images/frame_50.c"
#include "..//images/frame_51.c"
#include "..//images/frame_52.c"
#include "..//images/frame_53.c"
static uint16_t* img[N_FRAMES] = {
  (uint16_t*)_acframe_00,
  (uint16_t*)_acframe_01,
  (uint16_t*)_acframe_02,
  (uint16_t*)_acframe_03,
  (uint16_t*)_acframe_04,
  (uint16_t*)_acframe_05,
  (uint16_t*)_acframe_06,
  (uint16_t*)_acframe_07,
  (uint16_t*)_acframe_08,  
  (uint16_t*)_acframe_09,  
  (uint16_t*)_acframe_10,
  (uint16_t*)_acframe_11,
  (uint16_t*)_acframe_12,
  (uint16_t*)_acframe_13,
  (uint16_t*)_acframe_14,
  (uint16_t*)_acframe_15,
  (uint16_t*)_acframe_16,
  (uint16_t*)_acframe_17,
  (uint16_t*)_acframe_18,
  (uint16_t*)_acframe_19,
  (uint16_t*)_acframe_20,
  (uint16_t*)_acframe_21,
  (uint16_t*)_acframe_22,
  (uint16_t*)_acframe_23,
  (uint16_t*)_acframe_24,
  (uint16_t*)_acframe_25,
  (uint16_t*)_acframe_26,
  (uint16_t*)_acframe_27,
  (uint16_t*)_acframe_28,
  (uint16_t*)_acframe_29,
  (uint16_t*)_acframe_30,
  (uint16_t*)_acframe_31,
  (uint16_t*)_acframe_32,
  (uint16_t*)_acframe_33,
  (uint16_t*)_acframe_34,
  (uint16_t*)_acframe_35,
  (uint16_t*)_acframe_36,
  (uint16_t*)_acframe_37,
  (uint16_t*)_acframe_38,
  (uint16_t*)_acframe_39,
  (uint16_t*)_acframe_40,
  (uint16_t*)_acframe_41,
  (uint16_t*)_acframe_42,
  (uint16_t*)_acframe_43,
  (uint16_t*)_acframe_44,
  (uint16_t*)_acframe_45,
  (uint16_t*)_acframe_46,
  (uint16_t*)_acframe_47,
  (uint16_t*)_acframe_48,
  (uint16_t*)_acframe_49,
  (uint16_t*)_acframe_50,
  (uint16_t*)_acframe_51,
  (uint16_t*)_acframe_52,
  (uint16_t*)_acframe_53,

  };
#endif  
  
static uint8_t frameCnt = 0;
static uint16_t w,h; 
static uint8_t Str[10];
static uint32_t dtime;
static uint16_t cadrCnt;
static float fps;

void ImageInit(void){
  pImage = (uint16_t*)_acframe_00;
  BSP_MatrixScreenBufferClr();
  w = bmframe_00.xSize-1;
  h = bmframe_00.ySize-1;
  MatrixStringInit();
  
  
  
}
void Image_PeriodicHandler(void){
  uint32_t localtime = SystemTimer_GetTick();
  if(localtime - timer > PERIOD){
    process();
    timer = localtime;
  }
}

static void process(void){
  pImage = img[frameCnt++];
  if (frameCnt == N_FRAMES)
    frameCnt = 0;
    
  BSP_MatrixRenderImageRGB444(0, 0, w , h,pImage);
  
  MatrixSetTextColor(COLOR_BLUE);
  sprintf((void *)Str,"P2.5-32S-320X160 ICND1065");
  MatrixDrawStringAtOver(0,0,Str);
  
  sprintf((void *)Str,"Image%ix%i RGB444",w+1,h+1);
  MatrixDrawStringAtOver(0,50,Str);

  BSP_MatrixFillRect(100,35,127,63,COLOR_GREEN);
  MatrixSetTextColor(COLOR_WHITE);
  sprintf((void *)Str,"FPS");
  MatrixDrawStringAtOver(106,40,Str);
  
  MatrixSetBackColor(COLOR_GREEN);
  MatrixSetTextColor(COLOR_BLUE);
  sprintf((void *)Str,"%3.1f",fps);
  MatrixDrawStringAt(106,50,Str);

  BSP_MatrixReFresh();

  cadrCnt++;
  if (cadrCnt == 64){
    fps = 1000*64.0f/(SystemTimer_GetTick()- dtime);
    cadrCnt = 0;
    dtime = SystemTimer_GetTick();
  }
}

