
/* Includes ------------------------------------------------------------------*/
#include "icnd1065.h"
#include "icnd1065Reg.h"
#include "board_matrixConfig.h"

#define DELAY_ON_PIN      0
#define NFRAME            (uint32_t)2
#define NLINE             (uint32_t)DISPLAY_SCAN_LINES 
#define DCLK              (uint32_t)(6000000)
#define NCLK_ON_LINE      (uint32_t)90 
#define NCLK_ROW          (uint32_t)4

extern uint8_t screenBuffer[];
static uint16_t cdtRow;
static uint8_t curLine = 0;
static volatile uint8_t pwmEn = 0;
static volatile uint8_t fPwmStop = 0;

//for refrash frame
static uint16_t clkCnt;
static uint16_t lineCnt;
static uint16_t rowHi;
static uint16_t rowLow;
static volatile uint8_t complite = 0;
static uint16_t pr_dclk;
static uint16_t pr_line;
static uint16_t pr_pwm;

static regConf registers[REG_NUM] = {
  {1,0x001f,0x001f,0x001f,5},//scan line[4:0]
  {2,0x184f,0x184f,0x184f,5},
  {3,0x222f,0x222f,0x222f,5},
  {4,0x3046,0x3046,0x3046,5},
  {5,0x4100,0x4100,0x4100,5},
// Яркость
  {6,0x50ff,0x50ff,0x50ff,5},
  {7,0x601c,0x601c,0x601c,5},
  {8,0x7027,0x7027,0x702c,5},
  {9,0x8000,0x8000,0x8000,5},
  {10,0x9184,0x9184,0x9184,5},
  {11,0xa000,0xa000,0xa000,5},
  {12,0xb046,0xb047,0xb048,5},//Gradual[7:5],Coupling[4:0]
  {13,0xcb02,0xcb02,0xcb02,5},//GCLK_PLL_P[4:0] 
  {14,0xd502,0xd502,0xd502,5},//GCLK_PLL_N[8:9],GCLK_PLL_M[4:0]
  {15,0xe000,0xe000,0xe000,5},
  {16,0xf000,0xf000,0xf000,5}
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
static void sendDataReset(void);
static void sendFsync(void);
static void sendConfReg(regConf reg);
static void sendOn(void);
static void sendOff(void);
static void sendNClock(uint32_t n);
static void sendClock(void);
static void delay(uint32_t nops);
static void delaySignal(void);

void matrixReFresh(void)
{
  uint16_t y;
  uint8_t bit;
  uint8_t chip;
  uint8_t out;
  uint8_t rgb;
  uint16_t bufferPos;
  uint16_t tmp1,tmp2;

  pwmStop();
  resetClkRow(); 
  for (bit = 0; bit < 16; bit++){
    for(y = 0; y < DISPLAY_SCAN_LINES; y++){
      tmp1 =  y * MATRIX_BUFF_LINE_SIZE + bit;
      for(chip = 0; chip < MATRIX_NUMBER_CHIPS; chip++){
          tmp2 = tmp1 + chip*16* BYTE_FOR_PIXEL;
        for(out = 0; out < 16; out++){
          //index = y * DISPLAY_WIDTH * BYTE_FOR_PIXEL + chip*16* BYTE_FOR_PIXEL + out* BYTE_FOR_PIXEL + bit;  
          bufferPos =   tmp2 + out* BYTE_FOR_PIXEL;
          if(bit < BYTE_FOR_PIXEL){
            rgb = screenBuffer[bufferPos] ;
            RGB_SET(rgb);
          }
          else{
            RGB_SET_00();
          }
          if(chip == MATRIX_NUMBER_CHIPS - 1 && out == 15) 
            LAT_HIGH; 
            sendClkRow();
        }
          LAT_LOW;
      }
    }
  }
  sendLastClkRow();
  sendVsync();
  sendFsync();
  pwmStart();

}

void matrixRegInit(void){
uint32_t i;

  sendDataReset();
  sendNClock(8);
  sendOn();
  for(i=0; i<32; i++){
    sendFsync();
    sendNClock(16);
  }

  for(i = 0; i < REG_NUM; i++){
    if(registers[i].latNum !=0){
      sendPreCmd();
      sendConfReg(registers[i]);
    }
  }
}

void matrixRegReInit(void){
uint8_t i;
  pwmStop();
  sendDataReset();
  sendNClock(8);
  sendOn();
  for(i=0; i<32; i++){
    sendFsync();
    sendNClock(16);
  }

  for(i = 0; i < REG_NUM; i++){
    if(registers[i].latNum !=0){
      sendPreCmd();
      sendConfReg(registers[i]);
    }
  }
}

void matrixSetBrightness(uint8_t bright){
uint8_t val;
 
  if(bright > 100) bright = 100;
  val = ((uint16_t)bright*255/100)&0x00FF;
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
    //curFrame = 0;
    SCANLINE_SET(curLine);
    curLine = 1;
    DCLK_TMR->cval = 0;
    LINE_TMR->cval = 0;
    ROW_TMR->c4dt = cdtRow;
    ROW_TMR->cval = pr_pwm;
  
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


  crm_periph_clock_enable(DCLK_TMR_CLK, TRUE);
  crm_periph_clock_enable(LINE_TMR_CLK, TRUE);
  crm_periph_clock_enable(ROW_TMR_CLK, TRUE);
  crm_clocks_freq_get(&crm_clocks_freq_struct);

  pr_dclk = (uint16_t)((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK*2)) - 1;
  pr_line = (uint16_t)(NCLK_ON_LINE-1);
  pr_pwm = (uint16_t) ((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK*2/(NCLK_ON_LINE))) - 1;
  cdtRow = pr_pwm - (uint16_t) ((crm_clocks_freq_struct.apb1_freq * 2 )/(DCLK/NCLK_ROW));

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
  
  tmr_channel_value_set(ROW_TMR, TMR_SELECT_CHANNEL_4, cdtRow);
  ROW_TMR->cval = pr_pwm;
  nvic_irq_enable(LINE_IRQn,1,0);
}

void LINE_IRQHandler(void){
if(tmr_flag_get(LINE_TMR, TMR_OVF_FLAG) != RESET){
		tmr_flag_clear(LINE_TMR, TMR_OVF_FLAG);
    if (fPwmStop == 1 && curLine == 0){
      DCLK_TMR->ctrl1 &= 0xFFFE;
      LINE_TMR->ctrl1 &= 0xFFFE;
      ROW_TMR->ctrl1 &= 0xFFFE;
      pwmEn = 0;
      return;
    }
    SCANLINE_SET(curLine);
    ROW_TMR->ctrl1 |= 0x0001;
    curLine++;
    if (curLine == NLINE){
      curLine = 0;
    }
  }
}

static void resetClkRow(void){
  clkCnt = 0;
  lineCnt = 0;
  complite = 0;
  rowHi = 0;
  rowLow = NCLK_ROW;
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
    SCANLINE_SET(lineCnt);
    lineCnt++;
    if(lineCnt == NLINE)
      lineCnt = 0;
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
      SCANLINE_SET(lineCnt);
      if(complite) 
        return;
      lineCnt++;
      if(lineCnt == NLINE){
        lineCnt = 0;
        complite = 1;
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

static void sendDataReset(void){
  uint8_t clk = 9;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
}

static void sendFsync(void){
  uint8_t clk = 8;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
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
  uint8_t clk = 11;
  sendNClock(16-clk);
  LAT_HIGH;
  while(clk--)
      sendClock();
  LAT_LOW;
}

static void sendOff(void) {
  uint8_t clk = 12;
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
