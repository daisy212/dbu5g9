

//#include "ltdc_TFT035_7.h"
#include "u5g_ltdc.h"




static volatile int _WaitForDMA2D;
static OS_SEMAPHORE _Sema_WaitForDMA2D;
static OS_SEMAPHORE _Sema_WaitForVSYNC;

static LTDC_HandleTypeDef  _hltdc;
static DMA2D_HandleTypeDef _hdma2d;
static int                 _lcd_int_active_line;
static int                 _lcd_int_porch_line;

const uint16_t BL_levels[] = {
    10, 20, 50, 100, 200, 300 ,400, 500, 600 , 700, 800 ,980, 1000
    //BL1, BL2, BL5, BL10, BL20, BL30, BL40, BL50, BL60, BL70, BL80, BL100
};

//#define LCD_BYTES_PER_PIXEL   (LCD_BITS_PER_PIXEL / 8)

extern U8 LCD_RAM _db48xVRAM[LCD_TOTAL_BYTES] __attribute__ ((aligned (16)));
extern U8 LCD_RAM _aVRAM[LCD_TOTAL_BYTES] __attribute__ ((aligned (16)));

extern "C" {
   void    Error_Handler(int err_no);
}

/*********************************************************************
*
*       Interrupt handlers
*
**********************************************************************
*/
/*********************************************************************
*
*       DMA2D_IRQHandler
*/
void DMA2D_IRQHandler(void) {
  DMA2D->IFCR = (U32)DMA2D_IFCR_CTCIF;
  DMA2D->IFCR = (U32)DMA2D_IFCR_CCTCIF;
  // Release the DMA2D for the next transfer
#if (USE_OS == 1)
  OS_EnterInterrupt();
  OS_SEMAPHORE_Give(&_Sema_WaitForDMA2D);
  OS_LeaveInterrupt();
#else
  _WaitForDMA2D = 0;
#endif
}

/*********************************************************************
*
*       GPU2D_IRQHandler
*/
void GPU2D_IRQHandler(void) {
//  HAL_GPU2D_IRQHandler(&hgpu2d);
}

/*********************************************************************
*
*       GPU2D_ER_IRQHandler
*/
void GPU2D_ER_IRQHandler(void) {
//  HAL_GPU2D_ER_IRQHandler(&hgpu2d);
}


void LTDC_IRQHandler(void) {

  LTDC->ICR = (U32)LTDC_IER_LIE;
}


/* -------------------------------------------------------
 * MX_TIM3_Init
 * TIM3 CH4 → PE6, APB2=160MHz, PWM@40kHz, 0.0–99.9%
 * ARR=3999 → period = 160MHz/2000 = 80kHz
 * 1 step (0.1%) = 2 ticks
 * -------------------------------------------------------*/
TIM_HandleTypeDef htim3;

/* -------------------------------------------------------
 * HAL_TIM_PWM_MspInit  — clock + GPIO for TIM3/PE6
 * -------------------------------------------------------*/
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
}


void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM3)
    {
        __HAL_RCC_TIM3_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();

        /* PE6 → AF2 = TIM3_CH4 on STM32U5G9 */
        GPIO_InitStruct.Pin       = GPIO_PIN_6;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;  /* verify in DS Fig. AF map */
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
}



void MX_TIM3_Init(void)
{
    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};
    TIM_OC_InitTypeDef      sConfigOC          = {0};

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 0;               /* TIM_CLK = 160 MHz        */
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.Period            = 2667;             /* 160MHz/2667 = 60 kHz  , /3200 = 50kHz   */
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.RepetitionCounter = 0;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
        Error_Handler(1);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
        Error_Handler(1);

    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
        Error_Handler(1);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
        Error_Handler(1);

    /* Initial duty = 50.0% → pulse = 500 steps × 2 = 1000 */
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 20;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
        Error_Handler(1);

    HAL_TIM_Base_MspInit(&htim3);
}


/* -------------------------------------------------------
 * TIM3_PWM_SetDuty
 *   duty_tenth : 0 (0.0%) … 999 (99.9%)
 *   CCR = duty_tenth * 4   (each 0.1% = 4 ticks)
 * -------------------------------------------------------*/
void TIM3_PWM_SetDuty(uint16_t duty_tenth)
{
uint32_t duty = duty_tenth * htim3.Init.Period / 1000;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, duty);
}

/* -------------------------------------------------------
 * Start PWM — call once after MX_TIM3_Init()
 * -------------------------------------------------------*/
void TIM3_PWM_Start(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
}

/*********************************************************************
*
*       _LTDC_ClockConfig
*/
static HAL_StatusTypeDef _LTDC_ClockConfig(LTDC_HandleTypeDef *hltdc) {
  HAL_StatusTypeDef         status;
  RCC_PeriphCLKInitTypeDef  PeriphClkInit = {0};
  RCC_OscInitTypeDef        rcc_oscinitstruct = {0};

  // Prevent unused argument(s) compilation warning
  UNUSED(hltdc);
  // LCD clock configuration
  // Typical PCLK is 25 MHz so the PLL3R is configured to provide this clock
  // LCD clock configuration
  // PLL3_VCO Input = HSI_VALUE/PLL3M = 16 Mhz / 4(PLL3M) = 4
  // PLL3_VCO Output = PLL3_VCO Input * PLL3N = 4 Mhz * 125 = 500
  // PLLLCDCLK = PLL3_VCO Output/PLL3R = 500/20 = 25Mhz, ou 500/25 = 20Mhz
  // LTDC clock frequency = PLLLCDCLK = 25 Mhz
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInit.LtdcClockSelection   = RCC_LTDCCLKSOURCE_PLL3;
  PeriphClkInit.PLL3.PLL3Source      = RCC_PLLSOURCE_HSI;
  PeriphClkInit.PLL3.PLL3M           = 4;                // 16Mhz/4 = 4Mhz
  PeriphClkInit.PLL3.PLL3N           = 125;              // 16Mhz/4*125=500Mhz
  PeriphClkInit.PLL3.PLL3P           = 8;
  PeriphClkInit.PLL3.PLL3Q           = 2;
  PeriphClkInit.PLL3.PLL3R           = 20;         // 16/4*125/20=25Mhz
//  PeriphClkInit.PLL3.PLL3R           = 25;       // 16/4*125/25=20Mhz

  PeriphClkInit.PLL3.PLL3RGE         = RCC_PLLVCIRANGE_0;
  PeriphClkInit.PLL3.PLL3FRACN       = 0;
  PeriphClkInit.PLL3.PLL3ClockOut    = RCC_PLL3_DIVR;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    status = HAL_ERROR;
  }
  if (HAL_RCC_OscConfig(&rcc_oscinitstruct) != HAL_OK) {
    status = HAL_ERROR;
  } else {
    status = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
  }
  return status;
}

/*********************************************************************
*
*       _ClockInit
*/
static void _ClockInit(void) {
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_DSI;
  PeriphClkInit.DsiClockSelection    = RCC_DSICLKSOURCE_PLL3;
  PeriphClkInit.LtdcClockSelection   = RCC_LTDCCLKSOURCE_PLL3;
  PeriphClkInit.PLL3.PLL3Source      = RCC_PLLSOURCE_HSI;
  PeriphClkInit.PLL3.PLL3M           = 4;
  PeriphClkInit.PLL3.PLL3N           = 125;
  PeriphClkInit.PLL3.PLL3P           = 8;
  PeriphClkInit.PLL3.PLL3Q           = 2;
  PeriphClkInit.PLL3.PLL3R           = 24;
  PeriphClkInit.PLL3.PLL3RGE         = RCC_PLLVCIRANGE_0;
  PeriphClkInit.PLL3.PLL3FRACN       = 0;
  PeriphClkInit.PLL3.PLL3ClockOut    = RCC_PLL3_DIVP|RCC_PLL3_DIVR;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/*********************************************************************
*
*       _LTDC_Init
*/
static HAL_StatusTypeDef _LTDC_Init(LTDC_HandleTypeDef *hltdc, uint32_t Width, uint32_t Height) {
  hltdc->Instance = LTDC;
  hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  hltdc->Init.HorizontalSync     = PORCH_HSYNC;
  hltdc->Init.AccumulatedHBP     = PORCH_HSYNC + PORCH_HBP;
  hltdc->Init.AccumulatedActiveW = PORCH_HSYNC + Width + PORCH_HBP;
  hltdc->Init.TotalWidth         = PORCH_HSYNC + Width + PORCH_HBP + PORCH_HFP;
  hltdc->Init.VerticalSync       = PORCH_VSYNC;
  hltdc->Init.AccumulatedVBP     = PORCH_VSYNC + PORCH_VBP;
  hltdc->Init.AccumulatedActiveH = PORCH_VSYNC + Height + PORCH_VBP;
  hltdc->Init.TotalHeigh         = PORCH_VSYNC + Height + PORCH_VBP + PORCH_VFP;

  hltdc->Init.Backcolor.Blue  = 0xFF;
  hltdc->Init.Backcolor.Green = 0xFF;
  hltdc->Init.Backcolor.Red   = 0xFF;

  return HAL_LTDC_Init(hltdc);
}

/*********************************************************************
*
*       _LTDC_MspInit
*/
static void _LTDC_MspInit(LTDC_HandleTypeDef *hltdc) {
  GPIO_InitTypeDef  GPIO_InitStruct;

  if (hltdc->Instance == LTDC) {
    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                        |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                        |GPIO_PIN_15|GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_LTDC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                        |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                        |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_LTDC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_LTDC;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_LTDC;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // LCD_ON
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // LCD_DE
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // LCD_BL_CTRL
//    GPIO_InitStruct.Pin = GPIO_PIN_6;
//    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    MX_TIM3_Init();
    TIM3_PWM_Start();


    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);         // DE
//    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);           // BL

    // GPIO Pins for logic analyzer
    //Configure GPIO pins : VSYNC_FREQ_Pin RENDER_TIME_Pin FRAME_RATE_Pin MCU_ACTIVE_Pin
/*    HAL_GPIO_WritePin(GPIOC, VSYNC_FREQ_Pin|RENDER_TIME_Pin|FRAME_RATE_Pin|MCU_ACTIVE_Pin, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = VSYNC_FREQ_Pin|RENDER_TIME_Pin|FRAME_RATE_Pin|MCU_ACTIVE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
 */
  }
}

/*********************************************************************
*
*       _InitDisplay
*/
static int _InitDisplay(void) {
  LTDC_LayerCfgTypeDef LayerCfg;

  _hltdc.Instance = LTDC;

// erreur debug !
  _LTDC_MspInit(&_hltdc);


  if (_LTDC_ClockConfig(&_hltdc) != HAL_OK) {
    return -1;
  }
  if (_LTDC_Init(&_hltdc, LCD_XSIZE, LCD_YSIZE) != HAL_OK) {
    return -2;
  }
  LayerCfg.WindowX0        = 0;
  LayerCfg.WindowX1        = LCD_XSIZE;
  LayerCfg.WindowY0        = 0;
  LayerCfg.WindowY1        = LCD_YSIZE;
  LayerCfg.Alpha           = 255;
  LayerCfg.Alpha0          = 0;
  LayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  LayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  LayerCfg.FBStartAdress   = (uint32_t)&_aVRAM[0];
  LayerCfg.ImageWidth      = LCD_XSIZE;
  LayerCfg.ImageHeight     = LCD_YSIZE;
  LayerCfg.Backcolor.Blue  = 0;
  LayerCfg.Backcolor.Green = 0;
  LayerCfg.Backcolor.Red   = 0;
  LayerCfg.PixelFormat     = LTDC_PIXEL_FORMAT_RGB565;
  //
  // Configure a line interrupt when LTDC enters the active area. This is to trigger the VSYNC queue/semaphore
  // Sets the Line Interrupt position
  //
  _lcd_int_active_line = (LTDC->BPCR & 0x7FF) - 1;
  _lcd_int_porch_line  = (LTDC->AWCR & 0x7FF) - 1;
  LTDC->LIPCR = _lcd_int_active_line;
  //
  // Line Interrupt Enable
  //
  LTDC->IER |= LTDC_IER_LIE | LTDC_IER_FUIE;
  HAL_NVIC_SetPriority(LTDC_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(LTDC_IRQn);
  HAL_NVIC_SetPriority(LTDC_ER_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(LTDC_ER_IRQn);
  return HAL_LTDC_ConfigLayer(&_hltdc, &LayerCfg, 0);
}

/*********************************************************************
*
*       _DMA2D_Init
*/
static void _DMA2D_Init(void) {
  __HAL_RCC_DMA2D_CLK_ENABLE();
  _hdma2d.Instance = DMA2D;
  HAL_DMA2D_Init(&_hdma2d);
  HAL_NVIC_SetPriority(DMA2D_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(DMA2D_IRQn);
}


/*********************************************************************
*
*      Spi emulation
*/
#define RCC_BASE_ADDR      (0x46020C00u)
#define RCC_AHB2ENR1       (*(volatile unsigned int*)(RCC_BASE_ADDR + 0x8Cu))
#define GPIOEEN_BIT        (4)
#define GPIOFEN_BIT        (5)

#define GPIOE_BASE_ADDR    (0x42021000u)
#define GPIOE_MODER        (*(volatile unsigned int*)(GPIOE_BASE_ADDR + 0x00u))  // GPIOD port mode register
#define GPIOE_IDR          (*(volatile unsigned int*)(GPIOE_BASE_ADDR + 0x10u))  // GPIOD input data register
#define GPIOE_BSRR         (*(volatile unsigned int*)(GPIOE_BASE_ADDR + 0x18u))  // GPIOD bit set/reset register

#define GPIOF_BASE_ADDR    (0x42021400u)
#define GPIOF_MODER        (*(volatile unsigned int*)(GPIOF_BASE_ADDR + 0x00u))  // GPIOD port mode register
#define GPIOF_IDR          (*(volatile unsigned int*)(GPIOF_BASE_ADDR + 0x10u))  // GPIOD input data register
#define GPIOF_BSRR         (*(volatile unsigned int*)(GPIOF_BASE_ADDR + 0x18u))  // GPIOD bit set/reset register

 // /CS PF0, CLK PF1, MOSI PE5

#define LCD_MOSI_BIT    (5)
#define LCD_MOSI_0   GPIOE_BSRR |= (1u << (LCD_MOSI_BIT + 16u))
#define LCD_MOSI_1   GPIOE_BSRR |= (1u << (LCD_MOSI_BIT))

#define LCD_CS_BIT      (0)
#define LCD_CS_0     GPIOF_BSRR |= (1u << (LCD_CS_BIT + 16u))
#define LCD_CS_1     GPIOF_BSRR |= (1u << (LCD_CS_BIT))

#define LCD_CLK_BIT     (1)
#define LCD_CLK_0    GPIOF_BSRR |= (1u << (LCD_CLK_BIT + 16u))
#define LCD_CLK_1    GPIOF_BSRR |= (1u << (LCD_CLK_BIT))

void spi_clk(void){
   OS_TASK_Delay_us(2); 
   LCD_CLK_0;
   OS_TASK_Delay_us(2); 
   LCD_CLK_1;
   OS_TASK_Delay_us(2); 
}
void SPI_SendData(unsigned char i)
{  
   unsigned char n;
   for(n=0; n<8; n=n+1)         
   {  
      if(i&0x80) 
      {
         LCD_MOSI_1;
      }
      else 
      {
         LCD_MOSI_0;
      }   
      i<<= 1;
      spi_clk();
   }
}
void SPI_WriteComm(unsigned char i)
{
   OS_TASK_Delay_us(10); 
   LCD_CS_0;
   LCD_MOSI_0;
   spi_clk();
   SPI_SendData(i);
   LCD_CS_1;
}

void SPI_WriteData(unsigned char i)
{
   OS_TASK_Delay_us(2); 
   LCD_CS_0;
   LCD_MOSI_1;
   spi_clk();
   SPI_SendData(i);
   LCD_CS_1;
}

 void _Init_ER_TFT35_7(void)
 {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // GPIO Ports Clock Enable
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  // Configure GPIO pin Output Level
//  RCC_AHB2ENR1 |=  (1u << GPIOEEN_BIT);     // Enable GPIOE clock
//  RCC_AHB2ENR1 |=  (1u << GPIOFEN_BIT);     // Enable GPIOF clock
  GPIOF_MODER  &= ~(3u << (LCD_CS_BIT * 2))   // Clear mode bits
               &  ~(3u << (LCD_CLK_BIT * 2));
  GPIOF_MODER  |=  (1u << (LCD_CS_BIT * 2))   // Set mode to output
               |   (1u << (LCD_CLK_BIT * 2));

  GPIOE_MODER  &= ~(3u << (LCD_MOSI_BIT * 2));   // Clear mode bits
  GPIOE_MODER  |=  (1u << (LCD_MOSI_BIT * 2));   // Set mode to output

   LCD_CS_1;
   LCD_CLK_1;
   LCD_MOSI_0;

/* buydisplay
comparaison ave ZH035IA-01A adafruit
=============== RGB Timing ===============
H Active = 640
H Back Porch (Sync Width Not Included) = 20
H Front Porch = 20
H Pulse Width = 2
V Active = 480
V Back Porch (Sync Width Not Included) = 4
V Front Porch = 12
V Pulse Width = 2
=============== initial code===============
//正扫 DCLK=20MH
*/ 
SPI_WriteComm(0xFF);SPI_WriteData(0x30);
SPI_WriteComm(0xFF);SPI_WriteData(0x52);
SPI_WriteComm(0xFF);SPI_WriteData(0x01);  
SPI_WriteComm(0xE3);SPI_WriteData(0x00);  
SPI_WriteComm(0x40);SPI_WriteData(0x00);
SPI_WriteComm(0x03);SPI_WriteData(0x40);
SPI_WriteComm(0x04);SPI_WriteData(0x00);
SPI_WriteComm(0x05);SPI_WriteData(0x03);
SPI_WriteComm(0x08);SPI_WriteData(0x00);
SPI_WriteComm(0x09);SPI_WriteData(0x07);
SPI_WriteComm(0x0A);SPI_WriteData(0x01);
SPI_WriteComm(0x0B);SPI_WriteData(0x32);
SPI_WriteComm(0x0C);SPI_WriteData(0x32);
SPI_WriteComm(0x0D);SPI_WriteData(0x0B);
SPI_WriteComm(0x0E);SPI_WriteData(0x00);

/* RGB interface control
bit 0 : DE polarity, 0 : high enable
bit 1 : CLK polarity 0 : data at rising time
bit 2 : HS polarity, 0 : low level sync
bit 3 : VS polarity, 0 : low level sync
bit 5,4 : 0,0 : sync + DE, 0,1 : sync mode, 1,0 : DE mode
Must match 
  hltdc->Instance = LTDC;
  hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;    // falling edge
  return HAL_LTDC_Init(hltdc);

*/
//SPI_WriteComm(0x23);SPI_WriteData(0xA2);  ??????????????????
SPI_WriteComm(0x23);SPI_WriteData(0x12);  // sync mode



SPI_WriteComm(0x24);SPI_WriteData(0x0c);
SPI_WriteComm(0x25);SPI_WriteData(0x06);
SPI_WriteComm(0x26);SPI_WriteData(0x14);
SPI_WriteComm(0x27);SPI_WriteData(0x14);

SPI_WriteComm(0x38);SPI_WriteData(0x9C); 
SPI_WriteComm(0x39);SPI_WriteData(0xA7); 

// interface rgb .6.5.4 : 24bit 111, 18bit 110, 16bit 101
//SPI_WriteComm(0x3A);SPI_WriteData(0x3a);           // 0x47, ZH035IA-01A
SPI_WriteComm(0x3A);SPI_WriteData(0x50);           // 0x47, ZH035IA-01A

SPI_WriteComm(0x28);SPI_WriteData(0x40);
SPI_WriteComm(0x29);SPI_WriteData(0x01);
SPI_WriteComm(0x2A);SPI_WriteData(0xdf);
SPI_WriteComm(0x49);SPI_WriteData(0x3C);   
SPI_WriteComm(0x91);SPI_WriteData(0x57); 
SPI_WriteComm(0x92);SPI_WriteData(0x57); 
SPI_WriteComm(0xA0);SPI_WriteData(0x55);
SPI_WriteComm(0xA1);SPI_WriteData(0x50);
SPI_WriteComm(0xA4);SPI_WriteData(0x9C);
SPI_WriteComm(0xA7);SPI_WriteData(0x02);  
SPI_WriteComm(0xA8);SPI_WriteData(0x01);  
SPI_WriteComm(0xA9);SPI_WriteData(0x01);  
SPI_WriteComm(0xAA);SPI_WriteData(0xFC);  
SPI_WriteComm(0xAB);SPI_WriteData(0x28);  
SPI_WriteComm(0xAC);SPI_WriteData(0x06);  
SPI_WriteComm(0xAD);SPI_WriteData(0x06);  
SPI_WriteComm(0xAE);SPI_WriteData(0x06);  
SPI_WriteComm(0xAF);SPI_WriteData(0x03);  
SPI_WriteComm(0xB0);SPI_WriteData(0x08);  
SPI_WriteComm(0xB1);SPI_WriteData(0x26);  
SPI_WriteComm(0xB2);SPI_WriteData(0x28);  
SPI_WriteComm(0xB3);SPI_WriteData(0x28); 
 
SPI_WriteComm(0xB4);SPI_WriteData(0x33);        // 0x03 ZH035IA-01A

SPI_WriteComm(0xB5);SPI_WriteData(0x08);  
SPI_WriteComm(0xB6);SPI_WriteData(0x26);  
SPI_WriteComm(0xB7);SPI_WriteData(0x08);  
SPI_WriteComm(0xB8);SPI_WriteData(0x26); 
SPI_WriteComm(0xF0);SPI_WriteData(0x00); 
SPI_WriteComm(0xF6);SPI_WriteData(0xC0);


SPI_WriteComm(0xFF);SPI_WriteData(0x30);
SPI_WriteComm(0xFF);SPI_WriteData(0x52);
SPI_WriteComm(0xFF);SPI_WriteData(0x02);
SPI_WriteComm(0xB0);SPI_WriteData(0x0B);
SPI_WriteComm(0xB1);SPI_WriteData(0x16);
SPI_WriteComm(0xB2);SPI_WriteData(0x17); 
SPI_WriteComm(0xB3);SPI_WriteData(0x2C); 
SPI_WriteComm(0xB4);SPI_WriteData(0x32);  
SPI_WriteComm(0xB5);SPI_WriteData(0x3B);  
SPI_WriteComm(0xB6);SPI_WriteData(0x29); 
SPI_WriteComm(0xB7);SPI_WriteData(0x40);   
SPI_WriteComm(0xB8);SPI_WriteData(0x0d);
SPI_WriteComm(0xB9);SPI_WriteData(0x05);
SPI_WriteComm(0xBA);SPI_WriteData(0x12);
SPI_WriteComm(0xBB);SPI_WriteData(0x10);
SPI_WriteComm(0xBC);SPI_WriteData(0x12);
SPI_WriteComm(0xBD);SPI_WriteData(0x15);
SPI_WriteComm(0xBE);SPI_WriteData(0x19);              
SPI_WriteComm(0xBF);SPI_WriteData(0x0E);
SPI_WriteComm(0xC0);SPI_WriteData(0x16);  
SPI_WriteComm(0xC1);SPI_WriteData(0x0A);
SPI_WriteComm(0xD0);SPI_WriteData(0x0C);
SPI_WriteComm(0xD1);SPI_WriteData(0x17);
SPI_WriteComm(0xD2);SPI_WriteData(0x14);
SPI_WriteComm(0xD3);SPI_WriteData(0x2E);   
SPI_WriteComm(0xD4);SPI_WriteData(0x32);   
SPI_WriteComm(0xD5);SPI_WriteData(0x3C);  
SPI_WriteComm(0xD6);SPI_WriteData(0x22);
SPI_WriteComm(0xD7);SPI_WriteData(0x3D);
SPI_WriteComm(0xD8);SPI_WriteData(0x0D);
SPI_WriteComm(0xD9);SPI_WriteData(0x07);
SPI_WriteComm(0xDA);SPI_WriteData(0x13);
SPI_WriteComm(0xDB);SPI_WriteData(0x13);
SPI_WriteComm(0xDC);SPI_WriteData(0x11);
SPI_WriteComm(0xDD);SPI_WriteData(0x15);
SPI_WriteComm(0xDE);SPI_WriteData(0x19);                   
SPI_WriteComm(0xDF);SPI_WriteData(0x10);
SPI_WriteComm(0xE0);SPI_WriteData(0x17);    
SPI_WriteComm(0xE1);SPI_WriteData(0x0A);
SPI_WriteComm(0xFF);SPI_WriteData(0x30);
SPI_WriteComm(0xFF);SPI_WriteData(0x52);
SPI_WriteComm(0xFF);SPI_WriteData(0x03);   
SPI_WriteComm(0x00);SPI_WriteData(0x2A);
SPI_WriteComm(0x01);SPI_WriteData(0x2A);
SPI_WriteComm(0x02);SPI_WriteData(0x2A);
SPI_WriteComm(0x03);SPI_WriteData(0x2A);
SPI_WriteComm(0x04);SPI_WriteData(0x61);  
SPI_WriteComm(0x05);SPI_WriteData(0x80);   
SPI_WriteComm(0x06);SPI_WriteData(0xc7);   
SPI_WriteComm(0x07);SPI_WriteData(0x01);  
SPI_WriteComm(0x08);SPI_WriteData(0x03); 
SPI_WriteComm(0x09);SPI_WriteData(0x04);
SPI_WriteComm(0x70);SPI_WriteData(0x22);
SPI_WriteComm(0x71);SPI_WriteData(0x80);
SPI_WriteComm(0x30);SPI_WriteData(0x2A);
SPI_WriteComm(0x31);SPI_WriteData(0x2A);
SPI_WriteComm(0x32);SPI_WriteData(0x2A);
SPI_WriteComm(0x33);SPI_WriteData(0x2A);
SPI_WriteComm(0x34);SPI_WriteData(0x61);
SPI_WriteComm(0x35);SPI_WriteData(0xc5);
SPI_WriteComm(0x36);SPI_WriteData(0x80);
SPI_WriteComm(0x37);SPI_WriteData(0x23);
SPI_WriteComm(0x40);SPI_WriteData(0x03); 
SPI_WriteComm(0x41);SPI_WriteData(0x04); 
SPI_WriteComm(0x42);SPI_WriteData(0x05); 
SPI_WriteComm(0x43);SPI_WriteData(0x06); 
SPI_WriteComm(0x44);SPI_WriteData(0x11); 
SPI_WriteComm(0x45);SPI_WriteData(0xe8); 
SPI_WriteComm(0x46);SPI_WriteData(0xe9); 
SPI_WriteComm(0x47);SPI_WriteData(0x11);
SPI_WriteComm(0x48);SPI_WriteData(0xea); 
SPI_WriteComm(0x49);SPI_WriteData(0xeb);
SPI_WriteComm(0x50);SPI_WriteData(0x07); 
SPI_WriteComm(0x51);SPI_WriteData(0x08); 
SPI_WriteComm(0x52);SPI_WriteData(0x09); 
SPI_WriteComm(0x53);SPI_WriteData(0x0a); 
SPI_WriteComm(0x54);SPI_WriteData(0x11); 
SPI_WriteComm(0x55);SPI_WriteData(0xec); 
SPI_WriteComm(0x56);SPI_WriteData(0xed); 
SPI_WriteComm(0x57);SPI_WriteData(0x11); 
SPI_WriteComm(0x58);SPI_WriteData(0xef); 
SPI_WriteComm(0x59);SPI_WriteData(0xf0); 
SPI_WriteComm(0xB1);SPI_WriteData(0x01); 
SPI_WriteComm(0xB4);SPI_WriteData(0x15); 
SPI_WriteComm(0xB5);SPI_WriteData(0x16); 
SPI_WriteComm(0xB6);SPI_WriteData(0x09); 
SPI_WriteComm(0xB7);SPI_WriteData(0x0f); 
SPI_WriteComm(0xB8);SPI_WriteData(0x0d); 
SPI_WriteComm(0xB9);SPI_WriteData(0x0b); 
SPI_WriteComm(0xBA);SPI_WriteData(0x00); 
SPI_WriteComm(0xC7);SPI_WriteData(0x02); 
SPI_WriteComm(0xCA);SPI_WriteData(0x17); 
SPI_WriteComm(0xCB);SPI_WriteData(0x18); 
SPI_WriteComm(0xCC);SPI_WriteData(0x0a); 
SPI_WriteComm(0xCD);SPI_WriteData(0x10); 
SPI_WriteComm(0xCE);SPI_WriteData(0x0e); 
SPI_WriteComm(0xCF);SPI_WriteData(0x0c); 
SPI_WriteComm(0xD0);SPI_WriteData(0x00); 
SPI_WriteComm(0x81);SPI_WriteData(0x00); 
SPI_WriteComm(0x84);SPI_WriteData(0x15); 
SPI_WriteComm(0x85);SPI_WriteData(0x16); 
SPI_WriteComm(0x86);SPI_WriteData(0x10); 
SPI_WriteComm(0x87);SPI_WriteData(0x0a); 
SPI_WriteComm(0x88);SPI_WriteData(0x0c); 
SPI_WriteComm(0x89);SPI_WriteData(0x0e);
SPI_WriteComm(0x8A);SPI_WriteData(0x02); 
SPI_WriteComm(0x97);SPI_WriteData(0x00); 
SPI_WriteComm(0x9A);SPI_WriteData(0x17); 
SPI_WriteComm(0x9B);SPI_WriteData(0x18);
SPI_WriteComm(0x9C);SPI_WriteData(0x0f);
SPI_WriteComm(0x9D);SPI_WriteData(0x09); 
SPI_WriteComm(0x9E);SPI_WriteData(0x0b); 
SPI_WriteComm(0x9F);SPI_WriteData(0x0d); 
SPI_WriteComm(0xA0);SPI_WriteData(0x01); 
SPI_WriteComm(0xFF);SPI_WriteData(0x30);
SPI_WriteComm(0xFF);SPI_WriteData(0x52);
SPI_WriteComm(0xFF);SPI_WriteData(0x02);  
SPI_WriteComm(0x01);SPI_WriteData(0x01);
SPI_WriteComm(0x02);SPI_WriteData(0xDA);
SPI_WriteComm(0x03);SPI_WriteData(0xBA);
SPI_WriteComm(0x04);SPI_WriteData(0xA8);
SPI_WriteComm(0x05);SPI_WriteData(0x9A);
SPI_WriteComm(0x06);SPI_WriteData(0x70);
SPI_WriteComm(0x07);SPI_WriteData(0xFF);
SPI_WriteComm(0x08);SPI_WriteData(0x91);
SPI_WriteComm(0x09);SPI_WriteData(0x90);
SPI_WriteComm(0x0A);SPI_WriteData(0xFF);
SPI_WriteComm(0x0B);SPI_WriteData(0x8F);
SPI_WriteComm(0x0C);SPI_WriteData(0x60);
SPI_WriteComm(0x0D);SPI_WriteData(0x58);
SPI_WriteComm(0x0E);SPI_WriteData(0x48);
SPI_WriteComm(0x0F);SPI_WriteData(0x38);
SPI_WriteComm(0x10);SPI_WriteData(0x2B);
SPI_WriteComm(0xFF);SPI_WriteData(0x30);
SPI_WriteComm(0xFF);SPI_WriteData(0x52);
SPI_WriteComm(0xFF);SPI_WriteData(0x00);   

/* .0 : flip vertical
   .1 : flip horizontal
   .3 : flip RGB BGR
*/
SPI_WriteComm(0x36);SPI_WriteData(0x03);        // 0x0A ZH035IA-01A panel flip
              
   SPI_WriteComm(0x11);SPI_WriteData(0x00);   //sleep out
   OS_TASK_Delay(200); 
   SPI_WriteComm(0x29);SPI_WriteData(0x00);    //display on
   OS_TASK_Delay(20); 




 }


/*********************************************************************
*
*       _LCD_Init
*/
uint8_t _LCD_Init(void) {
  static int IsInited;

  if (!IsInited) {
#if (USE_OS == 1)
    OS_SEMAPHORE_Create(&_Sema_WaitForDMA2D, 0);
    OS_SEMAPHORE_Create(&_Sema_WaitForVSYNC, 0);
#endif
    _ClockInit();
 //   _GPIO_Init();

 _Init_ER_TFT35_7();

    _DMA2D_Init();
    _InitDisplay();
    IsInited = 1;
  }
  return(0);
}


uint8_t LTDC_Init(LCD_Handle_t *p_hlcd)
{
   p_hlcd->status = LCD_OK;
   return _LCD_Init();

}



/**
 * @brief Wait for transfer completion
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
void LTDC_WaitForTransfer(LCD_Handle_t *p_hlcd)
{
    // Wait for any ongoing DMA2D operation to complete
    while (DMA2D->CR & DMA2D_CR_START);
// time out avec event ?
   p_hlcd->status = LCD_OK;
}

uint8_t* LTDC_GetFramebuffer(void)
{
    return &_db48xVRAM[0];
}



uint32_t LCD_GetFramebuffer_size(void)
{
    return LCD_TOTAL_BYTES;
}


void CopyRegionToVRAM(int Xmin, int Xmax, int Ymin, int Ymax)
{    // using dma2d, copy area from Db48 framebuffer to ltdc memory
   // Coordinate sanity checks
    if (Xmin < 0)           Xmin = 0;
    if (Ymin < 0)           Ymin = 0;
    if (Xmax >= LCD_WIDTH)  Xmax = LCD_WIDTH  - 1;
    if (Ymax >= LCD_HEIGHT) Ymax = LCD_HEIGHT - 1;

    if (Xmin > Xmax) return;
    if (Ymin > Ymax) return;

   uint8_t *pDB48x = (&_db48xVRAM[0]);
   uint8_t *pVRAM = &_aVRAM[0];
    int regionW     = Xmax - Xmin + 1;
    int regionH     = Ymax - Ymin + 1;
    int x0          = LCD_WIDTH - 1 - Xmax;
    int OffLine     = LCD_WIDTH - regionW;
    U32 pixelOffset = (Ymin * LCD_WIDTH + x0) * 2;

    void *pSrc = (void *)((U32)pDB48x + pixelOffset);
    void *pDst = (void *)((U32)pVRAM  + pixelOffset);

    // Wait for any ongoing DMA2D operation to complete
    while (DMA2D->CR & DMA2D_CR_START);

    DMA2D->CR      = 0x00000000UL | (1 << 9);              // Memory-to-memory + TCIE
    DMA2D->FGMAR   = (U32)pSrc;                            // Source address
    DMA2D->OMAR    = (U32)pDst;                            // Destination address
    DMA2D->FGOR    = OffLine;                              // Source line offset (pixels)
    DMA2D->OOR     = OffLine;                              // Destination line offset (pixels)
    DMA2D->FGPFCCR = LTDC_PIXEL_FORMAT_RGB565;             // 16-bit pixel format
    DMA2D->NLR     = (U32)(regionW << 16) | (U16)regionH; // Width | Height
    DMA2D->CR     |= DMA2D_CR_START;                       // Start transfer

    // Wait for transfer complete
  //  while (DMA2D->CR & DMA2D_CR_START);
}

void LTDC_SetPixel( uint16_t x, uint16_t y, int32_t pixel)
{
   uint16_t *p_pixel = ( uint16_t *)(&_db48xVRAM[0]);
   if ( x >=LCD_WIDTH  || y > LCD_HEIGHT)   return ;
   if (pixel == -1) 
   {
      p_pixel[LCD_XSIZE - x -1 + y*LCD_XSIZE] ^=0xffff;
   }
   else    if (pixel > 0) 
   {
      p_pixel[LCD_XSIZE - x -1 + y*LCD_XSIZE] = pixel;
   }
   else
   {
     p_pixel[LCD_XSIZE - x -1 + y*LCD_XSIZE] = 0x00;
   }
}

void LTDC_Power(Disp_Power_t NewPower)
{
   switch (NewPower)
   {
      case POWER_ON:
      break;
      case POWER_OFF:
      break;
      case LUMI_STD:
      break;
      case LUMI_MIN:
      break;
      case INIT:
      break;
   }
}

