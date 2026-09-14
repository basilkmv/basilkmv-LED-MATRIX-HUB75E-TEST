
/* Includes ------------------------------------------------------------------*/
#include "dp3364s.h"
#include "dp3364sReg.h"
#include "board_matrixConfig.h"
#include "board_led.h"

#define PWM_TYPE          2U
#define DELAY_ON_PIN      0
#define NFRAME            (uint32_t)2
#define NLINE             (uint32_t)DISPLAY_SCAN_LINES 
#define DCLK              (uint32_t)(3000000)
#define NCLK_ON_LINE      (uint32_t)160 //time of line = (2*(reg0x05[7:4]+1) + 2*(reg0x05[3:0]+1) + 4*(reg0x04[6:0]+1)/(reg0x06[2:0]+1) in DCLK
#define NCLK_ROW          (uint32_t)4
#define NCLK_FIRST_FRAME  (uint32_t)12

extern uint8_t screenBuffer[];
static uint16_t cdtRow;
static uint16_t cdtFirstFrame;
static uint16_t curFrame = 0;
static uint8_t curLine = 0;
static volatile uint8_t pwmEn = 0;
static volatile uint8_t fPwmStop = 0;

//for refrash frame
static uint16_t clkCnt;
static uint16_t frameCnt;
static uint16_t lineCnt;
static uint16_t rowHi;
static uint16_t rowLow;
static volatile uint8_t complite = 0;

static regConf registers[REG_NUM] = {
  {0,0x0000,0x0000,0x0000,0},//not used
  {1,0x0100,0x0100,0x0100,0},//not used
  {2,0x021f,0x021f,0x021f,5},//line number
  {3,0x033f,0x033f,0x033f,5},//GROUP_SET [6:0] (number of PWM display packets)
  {4,0x047f,0x047f,0x047f,5},//PWM_WIDTH[6:0] (line pwm width)
  {5,0x0504,0x0504,0x0504,5},
  {6,0x0607,0x0607,0x0607,5},//PLL_DIV[2:0] DECOUP_RAT[7:3]  FGCLK = FDCLK *(PLL_DIV[2:0]+1)
  {7,0x0709,0x0709,0x0709,5},//gamma
  {8,0x08ff,0x08ff,0x08ff,5},//brightness
  {9,0x09e0,0x09e0,0x09e0,5},
  {10,0x0abe,0x0abe,0x0abe,5},//[0]
  {11,0x0b28,0x0b2b,0x0b31,5},
  {12,0x0c40,0x0c40,0x0c40,5},//SYNC_MODE[7:6] LP_MODE[5:4](energy-saveing mode)
  {13,0x0d18,0x0d12,0x0d18,5},
  {14,0x0e00,0x0e00,0x0e00,5},//not used
  {15,0x0f20,0x0f20,0x0f20,0},//not used
  {16,0x1000,0x1000,0x1000,0},//not used
  {17,0x1100,0x1100,0x1100,0},//not used
  {18,0x1200,0x1200,0x1200,0},//not used
  {19,0x1300,0x1300,0x1300,0},//not used
  {20,0x1400,0x1400,0x1400,0},//not used
  {21,0x1500,0x1500,0x1500,0}
};  

static void pwmInit(void);
static void pwmStart(void);
static void pwmStop(void);
static void resetClkRow(void);
static void sendClkRow(void);
static void sendLastClkRow(void);
static void pullNClock(uint32_t n);
static void pullVsync(void);
static void pullConfReg(regConf reg);
static void pullPreCmd(void);


static void sendPreCmd(void);
static void sendVsync(void);
static void sendConfReg(regConf reg);
static void sendOn(void);
static void sendOff(void);
static void sendNClock(uint32_t n);
static void sendClock(void);
static void delay(uint32_t nops);
static void delaySignal(void);

void matrixReFresh(void)
{
//32mS
  uint16_t x,y,sect;
  int8_t bit;
  uint32_t pos;
  uint32_t bufferPos;
  uint8_t rgb;
  
  pwmStop();
  resetClkRow();
  for(y = 0; y < DISPLAY_SCAN_LINES; y++) { 
    for(x = 0; x < 16; x++) { 
      bufferPos = y * MATRIX_BUFF_LINE_SIZE + x * BYTE_FOR_PIXEL;
      for(sect = 0; sect < MATRIX_NUMBER_CHIPS; sect++) { 
        pos = bufferPos + sect * 16 * BYTE_FOR_PIXEL;
        for (bit = 0; bit < 3; bit++){
          RGB_SET_00();
          sendClkRow();
        }  
        for (; bit < BYTE_FOR_PIXEL + 3; bit++){
          rgb = screenBuffer[pos + bit - 3] ;
          RGB_SET(rgb);
          sendClkRow();
        }  
        for(; bit < 16; bit++) {
          RGB_SET_00();
          if(sect == MATRIX_NUMBER_CHIPS - 1 && bit == 15) 
            LAT_HIGH; 
          sendClkRow();
        }
        LAT_LOW;
      }
    }
  }
  sendLastClkRow();
  sendVsync();
  pwmStart();
}

void matrixRegInit(void){
//12.4 mS
uint32_t i;
  for(i = 0; i < REG_NUM; i++){
    if(registers[i].latNum !=0){
      sendPreCmd();
      sendConfReg(registers[i]);
    }
  }
  /*
    For on matrix must send 32 VSync after init
  */
  for (i=0; i<32; i++)
    sendVsync();  
}

void matrixRegReInit(void){
uint8_t i;
  pwmStop();
  for(i = 0; i < REG_NUM; i++){
    if(registers[i].latNum !=0){
      sendPreCmd();
      sendConfReg(registers[i]);
    }
  }
  /*
    For on matrix must send 32 VSync after init
  */
  for (i = 0; i < 32; i++){
    sendVsync();
  }  
}

void matrixSetBrightness(uint8_t bright){
uint8_t val;
uint16_t ms,ls;
/*
GAIN reg 0x08XX
  ms = XX[7:6]
  ls = XX[5:0]
  bright = 0:99%
  div = 100%/(3+1) = 25;//3 - ms = 0b11
  ms = bright/div;
  ls = bright*63/(div*(ms+1));// 63 - ls = 0b111111 
*/ 
  
  if(bright > 99) bright = 99;
  ms = (bright)/25;
  ls = bright*64/(25*(ms+1));
  val = ((ms<<6)|ls)&0xFF;
  registers[REG_BRIGHT].valueR =  (registers[REG_BRIGHT].valueR & 0xff00) | val;
  registers[REG_BRIGHT].valueG =  (registers[REG_BRIGHT].valueG & 0xff00) | val;
  registers[REG_BRIGHT].valueB =  (registers[REG_BRIGHT].valueB & 0xff00) | val;
  pwmStop();
  
  sendPreCmd();
  sendConfReg(registers[REG_BRIGHT]);
  pwmStart();
}

void matrixInit(void)
{
  gpio_init_type gpio_init_struct;
  gpio_default_para_init(&gpio_init_struct);

	crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
	crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);
	crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE);
  
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

	gpio_init_struct.gpio_pins = R1_GPIO_PIN | G1_GPIO_PIN | B1_GPIO_PIN | R2_GPIO_PIN | G2_GPIO_PIN | B2_GPIO_PIN ;
  gpio_init(R1_GPIO_PORT, &gpio_init_struct);
    
  gpio_init_struct.gpio_pins = A_GPIO_PIN | B_GPIO_PIN | C_GPIO_PIN | D_GPIO_PIN | E_GPIO_PIN;
  gpio_init(A_GPIO_PORT, &gpio_init_struct);
  
  gpio_init_struct.gpio_pins = CLK_GPIO_PIN;
  gpio_init(CLK_GPIO_PORT, &gpio_init_struct);
  
  gpio_init_struct.gpio_pins = LAT_GPIO_PIN;
  gpio_init(LAT_GPIO_PORT, &gpio_init_struct);
  
  gpio_init_struct.gpio_pins = OE_GPIO_PIN;
  gpio_init(OE_GPIO_PORT, &gpio_init_struct);
  
  gpio_pin_mux_config(OE_GPIO_PORT, OE_GPIO_PINS_SOURCE, OE_GPIO_MUX);
  gpio_pin_mux_config(CLK_GPIO_PORT, CLK_GPIO_PINS_SOURCE, CLK_GPIO_MUX);
  
  
  RGB_SET_00();
  CLK_LOW;
  LAT_LOW;
  OE_LOW;
  
  pwmInit();
}

static void pwmStart(void){
  if (!pwmEn){
    
    //set as MUX
    //CLK_GPIO_PORT->cfgr  &= (uint32_t)~(0x03 << (CLK_GPIO_PINS_SOURCE * 2));
    //CLK_GPIO_PORT->cfgr  |= (uint32_t)( GPIO_MODE_MUX<< (CLK_GPIO_PINS_SOURCE * 2));
    CLK_GPIO_PORT->cfgr = (CLK_GPIO_PORT->cfgr & 0xFFF3FFFF)| 0x00080000;
    //OE_GPIO_PORT->cfgr  &= (uint32_t)~(0x03 << (OE_GPIO_PINS_SOURCE * 2));
    //OE_GPIO_PORT->cfgr  |= (uint32_t)( GPIO_MODE_MUX<< (OE_GPIO_PINS_SOURCE * 2));
    OE_GPIO_PORT->cfgr = (OE_GPIO_PORT->cfgr & 0xFFFFCFFF)| 0x00002000;
    
    curLine = 0;
    curFrame = 0;
    SCANLINE_SET(curLine);
    curLine = 1;
    DCLK_TMR->cval = 0;
    LINE_TMR->cval = 0;
    ROW_TMR->c4dt = cdtFirstFrame;
  
    ROW_TMR->ctrl1 |= 0x0001;
    LINE_TMR->ctrl1 |= 0x0001;
    DCLK_TMR->ctrl1 |= 0x0001;

    pwmEn = 1;
  }
}

static void pwmStop(void){
  if(pwmEn){
    fPwmStop = 1;
    while(pwmEn);
    fPwmStop = 0;
    //set as output
    //CLK_GPIO_PORT->cfgr  &= (uint32_t)~(0x03 << (CLK_GPIO_PINS_SOURCE * 2));
    //CLK_GPIO_PORT->cfgr  |= (uint32_t)( GPIO_MODE_OUTPUT<< (CLK_GPIO_PINS_SOURCE * 2));
    CLK_GPIO_PORT->cfgr = (CLK_GPIO_PORT->cfgr & 0xFFF3FFFF)| 0x00040000;
    //OE_GPIO_PORT->cfgr  &= (uint32_t)~(0x03 << (OE_GPIO_PINS_SOURCE * 2));
    //OE_GPIO_PORT->cfgr  |= (uint32_t)( GPIO_MODE_OUTPUT<< (OE_GPIO_PINS_SOURCE * 2));
    OE_GPIO_PORT->cfgr = (OE_GPIO_PORT->cfgr & 0xFFFFCFFF)| 0x00001000;
    
    CLK_LOW;
    OE_LOW;
  }
}

/*
288 MHz | -> TMT2  ->| -> CDCLK (6MHz)
                     | -> TMR4 -> IRQ OVF   | -> LINE switch (DCLK/64)
        | ->                                | -> TMR3 (One puls manual start) -> PWM -> ROW (OE)
*/
static void pwmInit(void){
  tmr_output_config_type  tmr_oc_init_structure;
  crm_clocks_freq_type    crm_clocks_freq_struct;
  uint16_t                pr_dclk;
  uint16_t                pr_line;
  uint16_t                pr_pwm;

  crm_periph_clock_enable(DCLK_TMR_CLK, TRUE);
  crm_periph_clock_enable(LINE_TMR_CLK, TRUE);
  crm_periph_clock_enable(ROW_TMR_CLK, TRUE);
  crm_clocks_freq_get(&crm_clocks_freq_struct);

  pr_dclk = (uint16_t)((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK*2)) - 1;
  pr_line = (uint16_t)(NCLK_ON_LINE-1);
  pr_pwm = (uint16_t) ((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK/(NCLK_ON_LINE - 1))) - 1;
  cdtRow = pr_pwm - (uint16_t) ((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK/NCLK_ROW));
  cdtFirstFrame = pr_pwm - (uint16_t) ((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK/NCLK_FIRST_FRAME));

  //Генерация DCLK
  tmr_base_init(DCLK_TMR, pr_dclk, 0);
  tmr_cnt_dir_set(DCLK_TMR, TMR_COUNT_UP);
  tmr_clock_source_div_set(DCLK_TMR, TMR_CLOCK_DIV1);
  tmr_output_default_para_init(&tmr_oc_init_structure);
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_SWITCH;
  tmr_oc_init_structure.oc_idle_state = FALSE;
  tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_oc_init_structure.oc_output_state = TRUE;
  tmr_output_channel_config(DCLK_TMR, TMR_SELECT_CHANNEL_2, &tmr_oc_init_structure);
  //tmr_channel_value_set(DCLK_TMR, TMR_SELECT_CHANNEL_2, pr_dclk/2);
  //Включаем тактирование для ведомого таймера TMR4
  tmr_sub_sync_mode_set(DCLK_TMR, TRUE);
  tmr_primary_mode_select(DCLK_TMR, TMR_PRIMARY_SEL_C2ORAW);

  //генератор линий
  //Включаем тактирование от TMR2
  //Источник тактирования для слейва
  tmr_trigger_input_select(LINE_TMR, TMR_SUB_INPUT_SEL_IS0);
  tmr_sub_mode_select(LINE_TMR, TMR_SUB_EXTERNAL_CLOCK_MODE_A);
  
  tmr_base_init(LINE_TMR, pr_line, 0);
  tmr_cnt_dir_set(LINE_TMR, TMR_COUNT_UP);
  tmr_clock_source_div_set(LINE_TMR, TMR_CLOCK_DIV1);
  tmr_output_default_para_init(&tmr_oc_init_structure);
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_oc_init_structure.oc_idle_state = FALSE;
  tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
  tmr_oc_init_structure.oc_output_state = FALSE;
  tmr_output_channel_config(LINE_TMR, TMR_SELECT_CHANNEL_4, &tmr_oc_init_structure);
  tmr_channel_value_set(LINE_TMR, TMR_SELECT_CHANNEL_4, pr_line/2);
 
  tmr_flag_clear(LINE_TMR, TMR_OVF_FLAG);
  tmr_interrupt_enable(LINE_TMR, TMR_OVF_INT, TRUE);


//Генерация ROW or OE
  tmr_base_init(ROW_TMR, pr_pwm, 0);
  tmr_cnt_dir_set(ROW_TMR, TMR_COUNT_DOWN);
  tmr_clock_source_div_set(ROW_TMR, TMR_CLOCK_DIV1);
  //PWM out
  tmr_output_default_para_init(&tmr_oc_init_structure);
  tmr_one_cycle_mode_enable(ROW_TMR, TRUE);
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_B;
  tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_LOW;
  tmr_oc_init_structure.oc_output_state = TRUE;
  tmr_output_channel_config(ROW_TMR, TMR_SELECT_CHANNEL_4, &tmr_oc_init_structure);
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_output_channel_config(ROW_TMR, TMR_SELECT_CHANNEL_4, &tmr_oc_init_structure);
  
  tmr_channel_value_set(ROW_TMR, TMR_SELECT_CHANNEL_4, cdtFirstFrame);
  ROW_TMR->cval = pr_pwm;
  nvic_irq_enable(LINE_IRQn,1,0);
}

void LINE_IRQHandler(void){
if(tmr_flag_get(LINE_TMR, TMR_OVF_FLAG) != RESET){
		tmr_flag_clear(LINE_TMR, TMR_OVF_FLAG);
    if (fPwmStop == 1 && curFrame == 0 && curLine == 0){
      DCLK_TMR->ctrl1 &= 0xFFFE;
      LINE_TMR->ctrl1 &= 0xFFFE;
      ROW_TMR->ctrl1 &= 0xFFFE;
      pwmEn = 0;
      return;
    }
    SCANLINE_SET(curLine);
    if (curFrame == 0 && curLine == 0){
      ROW_TMR->c4dt = cdtFirstFrame;
    }
    else{
      ROW_TMR->c4dt = cdtRow;
    }
    curLine++;
    if (curLine == NLINE){
      curLine = 0;
      curFrame++;
      if(curFrame == NFRAME)
        curFrame = 0;
    }
    //ROW_TMR->ctrl1_bit.tmren = 1;
    ROW_TMR->ctrl1 |= 0x0001;
  }
}

static void resetClkRow(void){
  clkCnt = 0;
  lineCnt = 0;
  frameCnt = 0;
  complite = 0;
  rowHi = 0;
  rowLow = NCLK_FIRST_FRAME;// + 1;
  SCANLINE_SET(lineCnt);
  lineCnt++;
}
static void sendClkRow(void){
  if(clkCnt == rowHi)
    OE_HIGH;
  if(clkCnt == rowLow)
    OE_LOW;
  clkCnt++;
  if(clkCnt == NCLK_ON_LINE){
    clkCnt = 0;
    rowLow = NCLK_ROW;
    SCANLINE_SET(lineCnt);
    lineCnt++;
    if(lineCnt == NLINE){
      lineCnt = 0;
      frameCnt++;
      if(frameCnt == NFRAME){
        frameCnt = 0;
        rowLow = NCLK_FIRST_FRAME;
      }
    } 
  }
  sendClock();
}  
static void sendLastClkRow(void){
  while(1){ 
    if(clkCnt == rowHi)
      OE_HIGH;
    if(clkCnt == rowLow)
      OE_LOW;
    clkCnt++;
    if(clkCnt == NCLK_ON_LINE){
      clkCnt = 0;
      rowLow = NCLK_ROW;
      SCANLINE_SET(lineCnt);
      if(complite) 
        return;
      lineCnt++;
      if(lineCnt == NLINE){
        lineCnt = 0;
        frameCnt++;
        if(frameCnt == NFRAME){
          frameCnt = 0;
          complite = 1;
          rowLow = NCLK_FIRST_FRAME;
        }
      }
    }
    sendClock();
  }
}  
static void pullVsync(void){
  uint8_t clk = 3;
  pullNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClkRow();
  LAT_LOW;
}
static void pullPreCmd(void){
  uint8_t clk = 14;
  pullNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClkRow();
  LAT_LOW;
}

static void pullConfReg(regConf reg){
  uint8_t chipCont = MATRIX_NUMBER_CHIPS;
  uint8_t bitCount;
  uint8_t latches = 16 - reg.latNum;
  uint16_t dataMask;
  uint16_t data;
  
  while(chipCont--) {
    for(bitCount = 0; bitCount < 16; bitCount++) {
      dataMask = 0x8000 >> bitCount;
      
      //B2G2R2B1G1R1      
      data = 0;
      if (reg.valueR & dataMask)
        data +=  0x09;
      if (reg.valueG & dataMask)
        data +=  0x12;
      if (reg.valueB & dataMask)
        data +=  0x24;
      RGB_SET(data);  

      if(chipCont == 0 && bitCount == latches) LAT_HIGH;
      sendClkRow();
    }
    LAT_LOW;
  }
}

static void pullNClock(uint32_t n) {
  while(--n)
    sendClkRow();
}


static void sendPreCmd(void){
  uint8_t clk = 14;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
}

static void sendVsync(void){
  uint8_t clk = 3;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
}

static void sendConfReg(regConf reg){
  uint8_t chipCont = MATRIX_NUMBER_CHIPS;
  uint8_t bitCount;
  uint8_t latches = 16 - reg.latNum;
  uint16_t dataMask;
  uint16_t data;
  
  while(chipCont--) {
    for(bitCount = 0; bitCount < 16; bitCount++) {
      dataMask = 0x8000 >> bitCount;
      
      //B2G2R2B1G1R1      
      data = 0;
      if (reg.valueR & dataMask)
        data +=  0x09;
      if (reg.valueG & dataMask)
        data +=  0x12;
      if (reg.valueB & dataMask)
        data +=  0x24;
      RGB_SET(data);  

      if(chipCont == 0 && bitCount == latches) LAT_HIGH;
      sendClock();
    }
    LAT_LOW;
  }
}

static void sendOn(void) {
  uint8_t clk = 12;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
}

static void sendOff(void) {
  uint8_t clk = 13;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
}
static void sendNClock(uint32_t n) {
  while(--n)
    sendClock();
}

#pragma push
#pragma O0
static void sendClock(void) {
  CLK_HIGH;
  delaySignal();
  CLK_LOW;
}


static void delay(uint32_t nops)
{
  while(nops--) NOP();
}

static void delaySignal(void)
{
#if DELAY_ON_PIN != 0
  uint32_t nops = DELAY_ON_PIN;
  delay(nops);
#endif  
}

#pragma pop  


/*****************************END OF FILE*********************/
