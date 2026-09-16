
/* Includes ------------------------------------------------------------------*/
#include "fm6353.h"
#include "fm6353Reg.h"
#include "board_matrixConfig.h"
#include "board_led.h"

#define DELAY_ON_PIN      0

#define NLINE             (uint32_t)DISPLAY_SCAN_LINES 
#define FPWM              (uint32_t)(3000000)
#define PULS_OE           138//(138)
#define PULS_OE_DELAY     2
#define PULS_ON_LINE      (PULS_OE + PULS_OE_DELAY + 64)//62


extern uint8_t screenBuffer[];
static uint16_t cdtOE;
static uint8_t curLine = 0;
static volatile uint8_t pwmEn = 0;
static volatile uint8_t fPwmStop = 0;

//for refrash frame
static uint16_t clkCnt;
static uint16_t frameCnt;
static uint16_t lineCnt;
static volatile uint8_t complite = 0;
static uint16_t pr_nOE;
static uint16_t pr_line;
static uint16_t pr_pwm;

static regConf registers[REG_NUM] = {
  {0,0x1f00,0x1f00,0x1f00,4},//line number
  {1,0x03ff,0x03ff,0x03ff,6},////Яркость
  {2,0x0000,0x0000,0x0000,8},//
  {3,0x0000,0x0000,0x0000,10},//

};  

static void pwmInit(void);
static void pwmStart(void);
static void pwmStop(void);
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
  
 
  for(y = 0; y < DISPLAY_SCAN_LINES; y++) { 
    for(x = 0; x < 16; x++) { 
      bufferPos = y * MATRIX_BUFF_LINE_SIZE + x * BYTE_FOR_PIXEL;
      for(sect = 0; sect < MATRIX_NUMBER_CHIPS; sect++) { 
        pos = bufferPos + sect * 16 * BYTE_FOR_PIXEL;
        for (bit = 0; bit < 16; bit++){
          if (bit < BYTE_FOR_PIXEL){
              rgb = screenBuffer[pos + bit] ;
              RGB_SET(rgb);
          }
          else
              RGB_SET_00();
          if(sect == MATRIX_NUMBER_CHIPS - 1 && bit == 15) 
            LAT_HIGH; 
          sendClock();
        }
        LAT_LOW;
      }
    }
  }
  pwmStop();
  sendVsync();
  pwmStart();
}

void matrixRegInit(void){

uint32_t i;
  
  sendPreCmd();
  sendOn();
  for(i = 0; i < REG_NUM; i++){
    if(registers[i].latNum !=0){
      sendPreCmd();
      sendConfReg(registers[i]);
    }
  }
  sendVsync();
}

void matrixRegReInit(void){
uint8_t i;
  pwmStop();
  sendPreCmd();
  sendOn();
  for(i = 0; i < REG_NUM; i++){
    if(registers[i].latNum !=0){
      sendPreCmd();
      sendConfReg(registers[i]);
    }
  }
}

#define MIN_BRIGHT  0x210
#define MAX_BRIGHT  0x3FF
void matrixSetBrightness(uint8_t bright){
uint16_t val;

  val = (MIN_BRIGHT + (MAX_BRIGHT - MIN_BRIGHT)*bright/100);
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
  
  gpio_pin_mux_config(OE_GPIO_PORT, OE_GPIO_PINS_SOURCE, OE_GPIO_MUX);
  gpio_init_struct.gpio_pins = OE_GPIO_PIN;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init(OE_GPIO_PORT, &gpio_init_struct);
  
  
  RGB_SET_00();
  CLK_LOW;
  LAT_LOW;
  
  pwmInit();
}

static void pwmStart(void){
  if (!pwmEn){
    curLine = 0;
    SCANLINE_SET(curLine);
    LINE_TMR->cval = 0;
    OE_CNT_TMR->cval = pr_nOE;
    OE_TMR->cval = 0;

    LINE_TMR->ctrl1 |= 0x0001;
    OE_CNT_TMR->ctrl1 |= 0x01;
    OE_TMR->ctrl1 |= 0x01;
    
    curLine++;
    pwmEn = 1;
  }
}

static void pwmStop(void){
  if(pwmEn){
    fPwmStop = 1;
    while(pwmEn);
    fPwmStop = 0;
  }
}

/*
288 MHz | -> TMT2  -> IRQ |-> LINE SWITCH TIM3 start  
                          | -> TMR5 (one cycle) count 138 -> TMR_PRIMARY_SEL_C4ORAW|
                                                                                   | -> TMR3 PWM -> OE 
*/
static void pwmInit(void){
  tmr_output_config_type  tmr_oc_init_structure;
  crm_clocks_freq_type    crm_clocks_freq_struct;
  uint32_t fpuls;

  crm_periph_clock_enable(LINE_TMR_CLK, TRUE);
  crm_periph_clock_enable(OE_TMR_CLK, TRUE);
  crm_periph_clock_enable(OE_CNT_TMR_CLK, TRUE);
  
  crm_clocks_freq_get(&crm_clocks_freq_struct);

  fpuls = (crm_clocks_freq_struct.apb1_freq * 2) / FPWM;
  pr_line = (uint16_t)(fpuls * PULS_ON_LINE - 1);
  pr_nOE = (uint16_t) (fpuls * (PULS_OE + PULS_OE_DELAY) - 1);
  cdtOE = (uint16_t) (fpuls * PULS_OE) - 1;
  pr_pwm = (uint16_t) (fpuls - 1);

  //Генерация line
  tmr_base_init(LINE_TMR, pr_line,0);
  tmr_cnt_dir_set(LINE_TMR, TMR_COUNT_UP);
  tmr_clock_source_div_set(LINE_TMR, TMR_CLOCK_DIV1);
  tmr_flag_clear(LINE_TMR, TMR_OVF_FLAG);
  tmr_interrupt_enable(LINE_TMR, TMR_OVF_INT, TRUE);
  
  //Generator OE
  tmr_base_init(OE_TMR, pr_pwm, 0);
  tmr_cnt_dir_set(OE_TMR, TMR_COUNT_UP);
  tmr_clock_source_div_set(OE_TMR, TMR_CLOCK_DIV1);
  tmr_output_default_para_init(&tmr_oc_init_structure);
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
  tmr_oc_init_structure.oc_idle_state = FALSE;
  tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_LOW;
  tmr_oc_init_structure.oc_output_state = TRUE;
  tmr_output_channel_config(OE_TMR, TMR_SELECT_CHANNEL_4, &tmr_oc_init_structure);
  tmr_channel_value_set(OE_TMR, TMR_SELECT_CHANNEL_4, (pr_pwm+1)/2);
  //Включаем тактирование для ведомого таймера TMR5
  tmr_sub_sync_mode_set(OE_TMR, TRUE);
  tmr_primary_mode_select(OE_TMR, TMR_PRIMARY_SEL_C4ORAW);
  
  //Источник тактирования для слейва TIM5
  tmr_trigger_input_select(OE_TMR, TMR_SUB_INPUT_SEL_IS2);
  tmr_sub_mode_select(OE_TMR, TMR_SUB_HANG_MODE);
  
  
  //Счетчик импульсов в внешнего пина 138-3 - зависит от FPWM и оптимизации
  tmr_base_init(OE_CNT_TMR, pr_nOE, 0);
  tmr_cnt_dir_set(OE_CNT_TMR, TMR_COUNT_DOWN);
  tmr_clock_source_div_set(OE_CNT_TMR, TMR_CLOCK_DIV1);
  tmr_one_cycle_mode_enable(OE_CNT_TMR, TRUE);
  
  tmr_output_default_para_init(&tmr_oc_init_structure);
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_B;
  tmr_oc_init_structure.oc_idle_state = FALSE;
  tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_LOW;
  tmr_oc_init_structure.oc_output_state = FALSE;
  tmr_output_channel_config(OE_CNT_TMR, TMR_SELECT_CHANNEL_4, &tmr_oc_init_structure);  
  tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;//
  tmr_output_channel_config(OE_CNT_TMR, TMR_SELECT_CHANNEL_4, &tmr_oc_init_structure);  
  
  tmr_channel_value_set(OE_CNT_TMR, TMR_SELECT_CHANNEL_4, cdtOE);
  OE_CNT_TMR->cval = pr_nOE;
  
  //Включаем тактирование для ведомого таймера TMR5
  tmr_sub_sync_mode_set(OE_CNT_TMR, TRUE);
  tmr_primary_mode_select(OE_CNT_TMR, TMR_PRIMARY_SEL_C4ORAW);

  nvic_irq_enable(LINE_IRQn,0,0);

}

void LINE_IRQHandler(void){
if(tmr_flag_get(LINE_TMR, TMR_OVF_FLAG) != RESET){
		tmr_flag_clear(LINE_TMR, TMR_OVF_FLAG);
    if (fPwmStop == 1 && curLine == 0){
      pwmEn = 0;
      LINE_TMR->ctrl1 &= ~0x01;
      return;
    }
    SCANLINE_SET(curLine);
    
    OE_TMR->cval = 0;
    OE_CNT_TMR->ctrl1 |= 0x01;
    
    curLine++;
    if (curLine == NLINE)
      curLine = 0;
  }
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
