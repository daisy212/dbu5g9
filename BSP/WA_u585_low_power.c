/* Helper for entering stop mode 2 
 * stop mode 2 : stm32h7b3 only : <50µA


*/

#include "DBxxxx.h"



#include "SEGGER_RTT.h" // using SEGGER_RTT_printf


t_POWER_STATE db_power_state;
t_POWER_STATE db_previous_power_state;

const char* power_names[] = {
    "PW_wake_up",
    "PW_running",
    "PW_sleeping",
    "PW_near_deep_sleep",
    "PW_request_Poff",
    "PW_deep_sleep",
    "PW_power_on",
    "PW_power_on_waiting",
    "PW_near_sleeping",
    "PW_waking_up_from_POFF",
    "PW_LAST",
    "t_POWER_STATE"
};

bool wake_from_lptim3 =  false;



#ifdef DM42
#  pragma GCC push_options
#  pragma GCC optimize("-O0")
#endif // DM42

void init_unused_pins(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

// PA13, PA14 : debug
// PA11, PA12 : usb
// PA4 PA5 PA6 PA7 : flash spi
// PA0 : Button, usb detect
// PA10, PA2, PA8, PA15, PA1, PA9 : KBD
// PA3 : NC
// PH3 : boot, pull down to zero on the board
   __HAL_RCC_GPIOA_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
   GPIO_InitStruct.Pin = GPIO_PIN_3 |GPIO_PIN_11 | GPIO_PIN_12;     
//   GPIO_InitStruct.Pin = GPIO_PIN_3 |GPIO_PIN_11 ;     
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


/*   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Pin = GPIO_PIN_3;     
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
*/

// PB0, PB1, PB3, PB4, PB5, PB6, PB7, PB13, PB14, : KBD
// PB10, PB12, PB15 : SP12 LCD
// PB8, PB9 : CAN, i2c, NC
// PB2 : BOOT0, pull down on board
// PB11 : not on qfp48

   __HAL_RCC_GPIOB_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin =  GPIO_PIN_8 | GPIO_PIN_9 ;
//   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Pin = GPIO_PIN_2;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

//   __HAL_RCC_GPIOB_CLK_DISABLE();

// PC13 : led
// PC14, PC15 : LSE
   __HAL_RCC_GPIOC_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
//   GPIO_InitStruct.Pin = GPIO_PIN_ALL & (!(GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15));
   GPIO_InitStruct.Pin = GPIO_PIN_13;
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
//   __HAL_RCC_GPIOC_CLK_DISABLE();

__HAL_RCC_GPIOC_CLK_ENABLE();
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);

   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
//   GPIO_InitStruct.Pin = GPIO_PIN_ALL & (!( GPIO_PIN_14 | GPIO_PIN_15));
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);




   // Disable VREFBUF if enabled
   HAL_SYSCFG_DisableVREFBUF();

}


void disable_debug(void)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   // debug pins
   GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

   // Clear DBGMCU_CR register to disable debug in all low power modes, 
   // debug : 0.9mA
   DBGMCU->CR = 0x00000000;

}

void ready_to_stop2_periph(void)
{

// lcd spi
//   SHARP_SPI_DeInit(&hlcd);


// sdmmc1
//    __HAL_RCC_SDMMC1_CLK_DISABLE();

// test
//HAL_SYSCFG_VREFBUF_HighImpedanceConfig(SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE);
// test

   // Disable VREFBUF if enabled
   HAL_SYSCFG_DisableVREFBUF();




// Disable all clocks not needed
__HAL_RCC_LSI_DISABLE();
__HAL_RCC_HSI48_DISABLE();
//__HAL_RCC_USART3_CLK_DISABLE();
//__HAL_RCC_USART1_CLK_DISABLE();
//__HAL_RCC_USART3_CLK_DISABLE();


  //      HAL_SuspendTick();
SysTick->CTRL = 0;  // Stops counter and clock
SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;  // Clear pending

// check timer 7, HAL timer
//__HAL_RCC_TIM7_CLK_SLEEP_ENABLE();


//   HAL_SuspendTick();
//__HAL_RCC_TIM7_CLK_DISABLE();


// --- 1. Clear all NVIC pending IRQs ---
    for (uint32_t i = 0; i < (sizeof(NVIC->ICPR) / sizeof(NVIC->ICPR[0])); i++) {
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    // --- 2. Clear all EXTI pending bits ---
    // (STM32U5 uses RPRx/FPRx, not PRx)
    EXTI->RPR1 = 0xFFFFFFFFu;
    EXTI->FPR1 = 0xFFFFFFFFu;

    // (U585 has only one EXTI block)
    // If there were RPR2/FPR2, they’d be handled here.
//HAL_PWREx_EnableFlashPowerDown();

// enable low power regulator : done
// utile ???
//HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_SVOS_SCALE5);



}

void ready_to_stop2_gpio(void)
// for stm32h7a3 nucleo !!!!!!!!!!!!
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

// debug : PA13 PA14
// usb : PA11 PA12
// kbd row : PA4, PA6, PA15
// usb spi : 
   __HAL_RCC_GPIOA_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;

   GPIO_InitStruct.Pin = GPIO_PIN_ALL & !( GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_4 | GPIO_PIN_6|GPIO_PIN_15 | GPIO_PIN_9);    // except debug + gpio
//   GPIO_InitStruct.Pin = GPIO_PIN_All ;    // no conso diff
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//   __HAL_RCC_GPIOA_CLK_DISABLE();

// drivers led  : PB0, PB14 ==> conso 150µA !!!!!!!!!
// kbd col : PB4, PB5, PB8, PB9, PB12, PB15
// kbd row : PB3, PB13
   __HAL_RCC_GPIOB_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_14 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_10 |GPIO_PIN_11 ;    // conso ok  : 30µA
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   __HAL_RCC_GPIOB_CLK_DISABLE();

// sdmmc1 : PC8, PC9, PC10, PC11, PC12
// button : PC13
// kbd row : PC6, PC7
   __HAL_RCC_GPIOC_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_ALL & !(GPIO_PIN_13|GPIO_PIN_6|GPIO_PIN_7);
//   GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;      
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
   __HAL_RCC_GPIOC_CLK_DISABLE();

// sdmmc1 : PD2
// Card detect sdmmc : PD7
// kbd row : PD15
   __HAL_RCC_GPIOD_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_ALL & !(GPIO_PIN_15 | GPIO_PIN_7);      
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Pin =  GPIO_PIN_7;      
   HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
   __HAL_RCC_GPIOD_CLK_DISABLE();

// led PE1
   __HAL_RCC_GPIOE_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_ALL;      
   HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
   __HAL_RCC_GPIOE_CLK_DISABLE();

   __HAL_RCC_GPIOF_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_ALL;      
   HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
   __HAL_RCC_GPIOF_CLK_DISABLE();

   // port G
// kbd row : PG9
   __HAL_RCC_GPIOG_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Pin = GPIO_PIN_ALL & !(GPIO_PIN_9);
   HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
   __HAL_RCC_GPIOG_CLK_DISABLE();

// spi lcd

// kbd


}





void    Error_Handler(int err_no);





void SystemClock_Config_MSIS24_MSIK24(void)
/* without PLL, systeme clock = 24Mhz from MSIS, MSIK = 24MHz */
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE3) != HAL_OK)
  {
    Error_Handler(1);
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
//  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI|RCC_OSCILLATORTYPE_MSIK;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_1;
  RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_2;
  RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(2);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler(3);
  }

  /** Enables the Clock Security System
  */
  HAL_RCCEx_EnableLSECSS();
}


void SystemClock_Config_P50(void)
/* with PLL, systeme clock = (N=25)*(MSIS/3/4) = 50Mhz, with MSIS=24Mhz ( 20< N < 60), MSIK = 24MHz */
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE3) != HAL_OK)
  {
    Error_Handler(4);
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
//  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI|RCC_OSCILLATORTYPE_MSIK;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_1;
  RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_1;
  RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV2;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 25;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 4;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(5);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler(6);
  }

  /** Enables the Clock Security System
  */
  HAL_RCCEx_EnableLSECSS();}




void SystemClock_Config_P160(void)
/* with PLL, systeme clock = (N=40)*(MSIS/3/2) = 160Mhz, with MSIS=24Mhz ( 20< N < 40), MSIK = 24MHz */
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler(12);
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();


//  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI
                              |RCC_OSCILLATORTYPE_MSIK;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_1;
  RCC_OscInitStruct.MSIKClockRange = RCC_MSIKRANGE_1;
  RCC_OscInitStruct.MSIKState = RCC_MSIK_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV2;
  RCC_OscInitStruct.PLL.PLLM = 3;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(13);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler(14);
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLModeSelection(RCC_MSISPLL_MODE_SEL);
  HAL_RCCEx_EnableMSIPLLMode();

  /** Enables the Clock Security System
  */
  HAL_RCCEx_EnableLSECSS();

}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config_P16_R64(void)
/* do nothing, periphals using MSIK at 24Mhz */
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

//    Error_Handler(14);
}



int _HW_Init(U8 Unit);

void Restore_After_STOP2(void)
{
      SystemClock_Config_P160();
//   PeriphCommonClock_Config_P16_R64();

   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();

//   HAL_SuspendTick();

}

extern U8 _IsInitialized;
void _HW_DeInit(U8 Unit);

uint32_t EnterSTOP2(void)
{
   int32_t start_sleeping_time = Get_Elapsed_Dual_Res(0);
//   SHARP_SPI_DeInit(&hlcd);
   init_unused_pins();
   HAL_LPTIM_MspDeInit(&hlptim1);


    GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
   GPIO_InitStruct.Pin = GPIO_PIN_13;
   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


   ready_to_stop2_periph();



// deinit in nor driver
//   _HW_DeInit(0);

/*
// bug 3.6mA systématique si tempo présente !!!!
for (uint32_t ii = 0; ii<9999; ii++){}
// clear nvic pending interrupts
  for (uint32_t i = 0; i < (sizeof(NVIC->ICPR)/sizeof(NVIC->ICPR[0])); ++i) {
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

*/
#if SIMULATE_STOP2
   // simulation stop mode 2
   OS_EVENT_GetTimed(&KBD_Event, 9999999);
//   SystemClock_Config_P64();        // cpu using HSI 64Mhz

#else

   #if STOP2_ENABLE_DEBUG
      HAL_DBGMCU_EnableDBGStopMode();
   #else 
//      disable_debug();
HAL_DBGMCU_DisableDBGStopMode(); // test, seems ok


   #endif // STOP2_ENABLE_DEBUG

   HAL_ICACHE_Disable();
   do
   {
      wake_from_lptim3 =  false;
      HAL_PWREx_EnterSTOP2Mode(  PWR_STOPENTRY_WFI);  //
   }
   while ( wake_from_lptim3);

#endif // SIMULATE_STOP2

//   HAL_PWREx_DisableFlashPowerDown();
   Restore_After_STOP2();
   HAL_ICACHE_Enable();
//   return lptim3_elapsed_ticks_i32(sleep_time);      

   return     Get_Elapsed_Dual_Res(start_sleeping_time);

}




/* Includes ------------------------------------------------------------------*/
//#include "stm32u5xx_hal.h"
//#include "stm32u5xx_hal_tim.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef        htim7;


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  This function configures the TIM7 as a time base source.
  *         The time source is configured  to have 1ms time base with a dedicated
  *         Tick interrupt priority.
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
  * @param  TickPriority: Tick interrupt priority.
  * @retval HAL status
  */

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  return HAL_OK;
}


uint32_t HAL_GetTick(void)
{
  return Get_Elapsed_Dual_Res(0);
}

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM7 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_SuspendTick(void)
{
  /* Disable TIM7 update Interrupt */
//  __HAL_TIM_DISABLE_IT(&htim7, TIM_IT_UPDATE);
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM7 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_ResumeTick(void)
{
  /* Enable TIM7 Update interrupt */
  __HAL_TIM_ENABLE_IT(&htim7, TIM_IT_UPDATE);
}






/**
  * @brief This function handles TIM7 global interrupt.
  */
void TIM7_IRQHandler(void)
{
  /* USER CODE BEGIN TIM7_IRQn 0 */

  /* USER CODE END TIM7_IRQn 0 */
  HAL_TIM_IRQHandler(&htim7);
  /* USER CODE BEGIN TIM7_IRQn 1 */

  /* USER CODE END TIM7_IRQn 1 */
}



// using lptim1, lptim3
LPTIM_HandleTypeDef hlptim1, hlptim3;

//int64_t ui64_lptim3=0;

int64_t time_at_PON;
int64_t tm_time_delta;
/*
int64_t rd_lptim3_i64(void) {
    HAL_NVIC_DisableIRQ(LPTIM3_IRQn);
   int64_t  tmp = ui64_lptim3 + (((int64_t)LPTIM3->CNT)*1000)/1024;
    HAL_NVIC_EnableIRQ(LPTIM3_IRQn);
   return tmp;
}
*/
/*
void sync_lptim3(void)
{
   time_at_PON = rd_lptim3_i64();
}
*/
/*
int32_t lptim3_elapsed_ticks_i32(int32_t start){
   int32_t  tmp = rd_lptim3_i32() - start;
   if (tmp > 0) return  tmp;
   else return 0x7fffffff;
}

int32_t rd_lptim3_i32(void) 
{
   int64_t tmp = rd_lptim3_i64() - time_at_PON;
   return (int32_t) tmp; 
}
*/
extern uint8_t       wakeup_from;

void LPTIM1_IRQHandler(void)
{

   OS_INT_Enter();
   RTT_vprintf_cr_time( "irq LPTIM1, %s", power_names[db_power_state]);

  HAL_LPTIM_IRQHandler(&hlptim1);

//   OS_EVENT_Set(&KBD_Event);
//   OS_EVENT_Set(&WAKE_UP_EVENT);
   OS_TASKEVENT_Set( &TKBD, EV_KPo_LPTIM);
   OS_TASKEVENT_Set(&TDB48X, EV_DBx_LPTIM1);

HAL_LPTIM_TimeOut_Stop_IT(&hlptim1);

//   ui64_lptim1 += HAL_LPTIM_ReadAutoReload(&hlptim1);
 //  ui64_lptim1 += 4096;
   wakeup_from = 12;
   OS_INT_Leave();

}



/**
  * @brief This function handles LPTIM3 global interrupt.
  */
void LPTIM3_IRQHandler_old(void)
{
  /* USER CODE BEGIN LPTIM3_IRQn 0 */

  /* USER CODE END LPTIM3_IRQn 0 */
  HAL_LPTIM_IRQHandler(&hlptim3);
//   ui64_lptim3 += 60000;
  wake_from_lptim3 = true;
  /* USER CODE BEGIN LPTIM3_IRQn 1 */

  /* USER CODE END LPTIM3_IRQn 1 */
}


/**
  * @brief LPTIM MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hlptim: LPTIM handle pointer
  * @retval None
  */
void HAL_LPTIM_MspInit_ok(LPTIM_HandleTypeDef* hlptim)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hlptim->Instance==LPTIM1)
  {
    /* USER CODE BEGIN LPTIM1_MspInit 0 */

    /* USER CODE END LPTIM1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
    PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(1);
    }

    /* Peripheral clock enable */
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    /* LPTIM1 interrupt Init */
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 14, 0);
    HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
    /* USER CODE BEGIN LPTIM1_MspInit 1 */

    /* USER CODE END LPTIM1_MspInit 1 */

  }

}




/**
  * @brief LPTIM MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hlptim: LPTIM handle pointer
  * @retval None
  */
void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef* hlptim)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hlptim->Instance==LPTIM1)
  {
    /* USER CODE BEGIN LPTIM1_MspInit 0 */

    /* USER CODE END LPTIM1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
    PeriphClkInit.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(1);
    }

    /* Peripheral clock enable */
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    /* LPTIM1 interrupt Init */
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 14, 1);
    HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
    /* USER CODE BEGIN LPTIM1_MspInit 1 */

    /* USER CODE END LPTIM1_MspInit 1 */
  }
  else if(hlptim->Instance==LPTIM3)
  {
    /* USER CODE BEGIN LPTIM3_MspInit 0 */

    /* USER CODE END LPTIM3_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM34;
    PeriphClkInit.Lptim34ClockSelection = RCC_LPTIM34CLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(1);
    }

    /* Peripheral clock enable */
    __HAL_RCC_LPTIM3_CLK_ENABLE();
    /* LPTIM3 interrupt Init */
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 14, 2);
    HAL_NVIC_EnableIRQ(LPTIM3_IRQn);
    /* USER CODE BEGIN LPTIM3_MspInit 1 */

    /* USER CODE END LPTIM3_MspInit 1 */
  }
  else if(hlptim->Instance==LPTIM4)
  {
    /* USER CODE BEGIN LPTIM4_MspInit 0 */

    /* USER CODE END LPTIM4_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM34;
    PeriphClkInit.Lptim34ClockSelection = RCC_LPTIM34CLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(1);
    }

    /* Peripheral clock enable */
    __HAL_RCC_LPTIM4_CLK_ENABLE();
    /* USER CODE BEGIN LPTIM4_MspInit 1 */

    /* USER CODE END LPTIM4_MspInit 1 */
  }

}



/**
  * @brief LPTIM MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hlptim: LPTIM handle pointer
  * @retval None
  */
void HAL_LPTIM_MspDeInit(LPTIM_HandleTypeDef* hlptim)
{
  if(hlptim->Instance==LPTIM1)
  {
    /* USER CODE BEGIN LPTIM1_MspDeInit 0 */

    /* USER CODE END LPTIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPTIM1_CLK_DISABLE();

    /* LPTIM1 interrupt DeInit */
    HAL_NVIC_DisableIRQ(LPTIM1_IRQn);
    /* USER CODE BEGIN LPTIM1_MspDeInit 1 */

    /* USER CODE END LPTIM1_MspDeInit 1 */
  }
    else if(hlptim->Instance==LPTIM3)
  {
    /* USER CODE BEGIN LPTIM3_MspDeInit 0 */

    /* USER CODE END LPTIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPTIM3_CLK_DISABLE();

    /* LPTIM3 interrupt DeInit */
    HAL_NVIC_DisableIRQ(LPTIM3_IRQn);
    /* USER CODE BEGIN LPTIM3_MspDeInit 1 */

    /* USER CODE END LPTIM3_MspDeInit 1 */
  }
  else if(hlptim->Instance==LPTIM4)
  {
    /* USER CODE BEGIN LPTIM4_MspDeInit 0 */

    /* USER CODE END LPTIM4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_LPTIM4_CLK_DISABLE();
    /* USER CODE BEGIN LPTIM4_MspDeInit 1 */

    /* USER CODE END LPTIM4_MspDeInit 1 */
  }


}



void MX_LPTIM1_Init(void)
{

  /* USER CODE BEGIN LPTIM1_Init 0 */

  /* USER CODE END LPTIM1_Init 0 */

  /* USER CODE BEGIN LPTIM1_Init 1 */

  /* USER CODE END LPTIM1_Init 1 */
  hlptim1.Instance = LPTIM1;
  hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV32;
  hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.Period = 0;
  hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  hlptim1.Init.RepetitionCounter = 0;
  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
  {
    Error_Handler(123);
  }
  /* USER CODE BEGIN LPTIM1_Init 2 */

  /* USER CODE END LPTIM1_Init 2 */

}


void MX_LPTIM3_Init(void)
{

  /* USER CODE BEGIN LPTIM3_Init 0 */

  /* USER CODE END LPTIM3_Init 0 */

  /* USER CODE BEGIN LPTIM3_Init 1 */

  /* USER CODE END LPTIM3_Init 1 */
  hlptim3.Instance = LPTIM3;
  hlptim3.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim3.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV32;
  hlptim3.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim3.Init.Period = 60*1024-1;
  hlptim3.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim3.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim3.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
  hlptim3.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
  hlptim3.Init.RepetitionCounter = 0;
  if (HAL_LPTIM_Init(&hlptim3) != HAL_OK)
  {
    Error_Handler(1);
  }
  /* USER CODE BEGIN LPTIM3_Init 2 */

  /* USER CODE END LPTIM3_Init 2 */

}


// uint32_t HAL_LPTIM_ReadCounter(const LPTIM_HandleTypeDef *hlptim)



/* ADC handle declaration */
ADC_HandleTypeDef hadc1;
uint32_t vbat;
uint32_t vbat_rd_time;


/**
  * @brief ADC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hadc->Instance==ADC1)
  {
    /* USER CODE BEGIN ADC1_MspInit 0 */

    /* USER CODE END ADC1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
    PeriphClkInit.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_MSIK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(6);
    }

    /* Peripheral clock enable */
    __HAL_RCC_ADC12_CLK_ENABLE();
    /* USER CODE BEGIN ADC1_MspInit 1 */

    /* USER CODE END ADC1_MspInit 1 */

  }

}

/**
  * @brief ADC MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if(hadc->Instance==ADC1)
  {
    /* USER CODE BEGIN ADC1_MspDeInit 0 */

    /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC12_CLK_DISABLE();


// BUG errata 2.2.8, can't stop MSIK in stop mode 2 when selected for adac !!!!!!!!!!!!
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
    PeriphClkInit.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_HCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(6);
    }
    /* USER CODE BEGIN ADC1_MspDeInit 1 */

    /* USER CODE END ADC1_MspDeInit 1 */
  }

}


/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */

void MX_ADC1_Init(void)
{

 /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_14B;
  hadc1.Init.GainCompensation = 0;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler(401);
  }
   ADC_ChannelConfTypeDef sConfig = {0};
    
    
    /* Configure Regular Channel for VBAT (Channel 18) */
    sConfig.Channel = ADC_CHANNEL_18;  /* VBAT channel */
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_391CYCLES;
    sConfig.SingleDiff = ADC_DIFFERENTIAL_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler(402);
    }
}

uint32_t ReadBatVoltage(void)
{
return 3616;
}
uint32_t ReadBatVoltage_no(void)
// return battery voltage in mV
{
   uint32_t adc_value1 = 0, adc_value2 = 0, adc_value3 = 0, u32_vbat =0;


   // Wake up ADC from deep power-down
//        LL_ADC_DisableDeepPowerDown(hadc1.Instance);
//        LL_ADC_EnableInternalRegulator(hadc1.Instance);
//        HAL_Delay(1);

HAL_PWREx_EnableVddA();

   MX_ADC1_Init();

   HAL_ADC_Stop(&hadc1);

//   HAL_ADC_DeInit(&hadc1);
//   LL_ADC_DisableInternalRegulator(hadc1.Instance);
//   LL_ADC_EnableDeepPowerDown(hadc1.Instance);
        
    /* Start ADC calibration */
   if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_DIFFERENTIAL_ENDED) != HAL_OK)
    {
//        Error_Handler(401);
    }
    
   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value1 = HAL_ADC_GetValue(&hadc1);

   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value1 = HAL_ADC_GetValue(&hadc1);

   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value1 = HAL_ADC_GetValue(&hadc1);

   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value1 = HAL_ADC_GetValue(&hadc1);

   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value1 = HAL_ADC_GetValue(&hadc1);

   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value2 = HAL_ADC_GetValue(&hadc1);
    
   HAL_ADC_Start(&hadc1);
   HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
   adc_value3 = HAL_ADC_GetValue(&hadc1);
    
    /* Convert ADC value to voltage */
    /* VBAT = (ADC_Value * VREF) / 32767 * 4  differential */
    /* The factor of 3 is because VBAT is divided by 4 internally */
    /* Assuming VREF = 3.3V */
    u32_vbat = ((adc_value1 + adc_value2 + adc_value3) * 4 * 33000/3)/32767+5; 
    RTT_vprintf_cr_time(  "ADC Value: %d.%d", u32_vbat/10000, (u32_vbat/10)%1000);
   // Return to deep power-down
   HAL_ADC_Stop(&hadc1);
   HAL_ADC_DeInit(&hadc1);
   HAL_PWREx_DisableVddA();         // utile ????????????????? 
   return u32_vbat/10;
}

/*****************************************************************************************************
******************************************************************************************************
** Real time, using lptim3, 32k768, div32, one interruption per minute
******************************************************************************************************
*****************************************************************************************************/
volatile Ti_crc g_SystemTime;

// Internal helper to calculate CRC for the whole struct (16 bytes of data)
static uint32_t _Calculate_Full_Ti_Crc(uint64_t t1, uint64_t t2) {
    uint32_t crc = 0xFFFFFFFF;
    uint64_t data[2] = {t1, t2};
    uint32_t *p = (uint32_t *)data;
    
    for (int i = 0; i < 4; i++) { // 4 words = 16 bytes
        crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else crc >>= 1;
        }
    }
    return ~crc;
}

void LPTIM3_IRQHandler(void) {
//   OS_INT_Enter();
    uint32_t isr_status = LPTIM3->ISR;
/*
In the LPTIM status register (ISR), 0x283 breaks down as follows:
    Bit 0 (0x01): CC1IF — Capture/Compare 1 interrupt flag.
    Bit 1 (0x02): CC2IF — Capture/Compare 2 interrupt flag.
    Bit 7 (0x80): ARRM — Autoreload Match (Your 60s heartbeat).
    Bit 9 (0x200): DIEROK — Interrupt Enable Register Update OK.
*/
   LPTIM3->ICR = isr_status;
    if (isr_status & LPTIM_ISR_ARRM) {
    // Validate Integrity
      if (_Calculate_Full_Ti_Crc(g_SystemTime.Ti64, g_SystemTime.Ti_stamp_power_on) != g_SystemTime.crc) 
      {
        // CORRUPTION DETECTED: Emergency Reset
           g_SystemTime.Ti64 = 0;
         g_SystemTime.Ti_stamp_power_on = 0;
         g_SystemTime.crc = _Calculate_Full_Ti_Crc(0, 0);
      }
//      ui64_lptim3 += 60000;
      wake_from_lptim3 = true;

      // Update both fields
      g_SystemTime.Ti64 += 60000;
      g_SystemTime.crc = _Calculate_Full_Ti_Crc(g_SystemTime.Ti64, g_SystemTime.Ti_stamp_power_on);
    }
//    OS_INT_Leave();
}


static uint64_t Get_Absolute_Ms64(void) {
    uint64_t base;
    uint32_t ticks;
    uint32_t captured_crc;

    OS_TASK_EnterRegion(); 

    // Use Disable/Enable to prevent the LPTIM3 ISR from modifying g_SystemTime
    OS_INT_Disable(); 
    base = g_SystemTime.Ti64;
    captured_crc = g_SystemTime.crc;
    ticks = LPTIM3->CNT;
//    if (LPTIM3->ISR & LPTIM_ISR_ARRM) {
//        ticks = LPTIM3->CNT;
//        base += 60000;
//    }
    OS_INT_Enable(); 

    // CRC check with interrupts back on, but scheduler still locked
    if (_Calculate_Full_Ti_Crc(base, g_SystemTime.Ti_stamp_power_on) != captured_crc) {
        OS_TASK_LeaveRegion();
        return 0; 
    }
    uint64_t result = base + ((uint64_t)(ticks * 1000) >> 10);
    OS_TASK_LeaveRegion();
    return result;
}


void Init_Power_On_Stamp(void) {
    uint64_t now = Get_Absolute_Ms64();
    
    OS_TASK_EnterRegion();
    OS_INT_Disable(); 
    g_SystemTime.Ti_stamp_power_on = now;
    g_SystemTime.crc = _Calculate_Full_Ti_Crc(g_SystemTime.Ti64, now);
    OS_INT_Enable();
    OS_TASK_LeaveRegion();
}

/**
 * @brief Manual BCD to Decimal conversion
 */
static uint8_t _From_Bcd(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
 * @brief Manual BCD conversion for RTC registers
 */
static uint8_t _To_Bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}


/**
 * @brief Initializes g_SystemTime from RTC or zero without HAL.
 */
void Init_Time(bool use_rtc) {
    uint64_t initial_ms = 0;
OS_TASK_EnterRegion();
    // 1. Attempt to seed from RTC
    if (use_rtc) {
        // Wait for shadow registers to synchronize to ensure fresh data
        RTC->ICSR &= ~RTC_ICSR_RSF;
        uint32_t timeout = 0xFFFF;
        while (!(RTC->ICSR & RTC_ICSR_RSF)) {
            if (--timeout == 0) break; 
        }

        // Read order is critical: SSR -> TR -> DR
        // Reading SSR and TR locks the values in the shadow registers
        uint32_t ssr = RTC->SSR;
        uint32_t tr  = RTC->TR;
        uint32_t dr  = RTC->DR;

        // Convert BCD to Decimal
        struct tm tm_init = {0};
        tm_init.tm_sec  = _From_Bcd((uint8_t)((tr & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_ST_Pos));
        tm_init.tm_min  = _From_Bcd((uint8_t)((tr & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos));
        tm_init.tm_hour = _From_Bcd((uint8_t)((tr & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos));
        
        tm_init.tm_mday = _From_Bcd((uint8_t)(dr & (RTC_DR_DT | RTC_DR_DU)));
        tm_init.tm_mon  = _From_Bcd((uint8_t)((dr & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos)) - 1;
        tm_init.tm_year = _From_Bcd((uint8_t)((dr & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos)) + 100;

        time_t seconds = mktime(&tm_init);

        if (seconds != (time_t)-1) {
            // Calculate sub-second fractional milliseconds
            // Fraction = (PREDIV_S - SSR) / (PREDIV_S + 1)
            uint32_t prediv_s = RTC->PRER & RTC_PRER_PREDIV_S;
            uint32_t ms_part = ((prediv_s - ssr) * 1000) / (prediv_s + 1);
            
            initial_ms = ((uint64_t)seconds * 1000) + ms_part;
        }
    }

    // 2. Atomic Update of the Protected Structure
    OS_INT_Disable();
    g_SystemTime.Ti64 = initial_ms;
    g_SystemTime.Ti_stamp_power_on = initial_ms; 
    g_SystemTime.crc = _Calculate_Full_Ti_Crc(g_SystemTime.Ti64, g_SystemTime.Ti_stamp_power_on);

    // 3. Reset LPTIM3 Phase
    // Clear Enable, then Re-enable to reset internal counters
    LPTIM3->CR &= ~LPTIM_CR_ENABLE;
    LPTIM3->CR |= LPTIM_CR_ENABLE;
    
    LPTIM3->ARR = 61440 - 1; // 60 seconds @ 1024Hz
    LPTIM3->ICR = LPTIM_ICR_ARRMCF; 
    LPTIM3->CR |= LPTIM_CR_CNTSTRT;
    
   OS_INT_Enable(); 
   OS_TASK_LeaveRegion();
}

/**
 * @brief  Initializes or Synchronizes the RTC hardware.
 * @param  use_lptim3: 
 * true  -> Sync RTC to Get_Absolute_Ms64() (SNTP/Uptime).
 * false -> Force RTC to 2000/01/01 14:00:00.
 * @return bool: true if hardware accepted the sequence, false on timeout/error.
 */
bool Init_Rtc(bool use_lptim3) {
    struct tm tm_target = {0};
    uint32_t ms_part = 0;
OS_TASK_EnterRegion();
    if (use_lptim3) {
        // 1a. Sync to high-precision timeline
        uint64_t now_ms = Get_Absolute_Ms64();
        if (now_ms == 0) return false; 

        time_t seconds = (time_t)(now_ms / 1000);
        ms_part = (uint32_t)(now_ms % 1000);
        
        struct tm *tmp = gmtime(&seconds);
        if (tmp == NULL || tmp->tm_year < 100 || tmp->tm_year > 199) return false;
        tm_target = *tmp; 
    } else {
        // 1b. Force Safety Default: 2000/01/01 14:00:00
        tm_target.tm_year = 100; // 2000 (1900 + 100)
        tm_target.tm_mon  = 0;   // January
        tm_target.tm_mday = 1;   // 1st
        tm_target.tm_hour = 14;
        tm_target.tm_min  = 0;
        tm_target.tm_sec  = 0;
        tm_target.tm_wday = 6;   // 2000-01-01 was a Saturday
        ms_part = 0;
    }

    // 2. Unlock RTC Write Protection
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    // 3. Enter Initialization Mode
    RTC->ICSR |= RTC_ICSR_INIT;
    
    uint32_t timeout = 0xFFFF;
    while (!(RTC->ICSR & RTC_ICSR_INITF)) {
        if (--timeout == 0) {
            RTC->WPR = 0xFF;
            OS_TASK_LeaveRegion();
            return false; 
        }
    }

    // 4. Write Time (HT|HU|MNT|MNU|ST|SU)
    RTC->TR = (uint32_t)((_To_Bcd((uint8_t)tm_target.tm_hour) << RTC_TR_HU_Pos)  |
                         (_To_Bcd((uint8_t)tm_target.tm_min)  << RTC_TR_MNU_Pos) |
                         (_To_Bcd((uint8_t)tm_target.tm_sec)  << RTC_TR_ST_Pos));

    // 5. Write Date (YT|YU|WDU|MT|MU|DT|DU)
    RTC->DR = (uint32_t)((_To_Bcd((uint8_t)(tm_target.tm_year - 100)) << RTC_DR_YU_Pos) |
                         (_To_Bcd((uint8_t)(tm_target.tm_mon + 1))    << RTC_DR_MU_Pos) |
                         (_To_Bcd((uint8_t)tm_target.tm_mday)          << RTC_DR_DU_Pos) |
                         ((uint32_t)(tm_target.tm_wday == 0 ? 7 : tm_target.tm_wday) << RTC_DR_WDU_Pos));

    // 6. Exit Initialization Mode
    RTC->ICSR &= ~RTC_ICSR_INIT;

    // 7. Wait for Shadow Register Sync (RSF)
    timeout = 0xFFFF;
    while (!(RTC->ICSR & RTC_ICSR_RSF)) {
        if (--timeout == 0) break; 
    }

    // 8. Re-lock Write Protection
    RTC->WPR = 0xFF;
OS_TASK_LeaveRegion();
    // 9. Final hardware validation
    return (RTC->ICSR & RTC_ICSR_INITS) ? true : false;
}
#define NTP_UNIX_OFFSET 2208988800UL

int64_t Adjust_Time(IP_NTP_TIMESTAMP *pTimeSNTP) {
    uint64_t new_ms;
    uint32_t unix_secs;

    // 1. Convert NTP Seconds to Unix Seconds (if Ti64 is Unix-based)
    if (pTimeSNTP->Seconds >= NTP_UNIX_OFFSET) {
        unix_secs = pTimeSNTP->Seconds - NTP_UNIX_OFFSET;
    } else {
        unix_secs = 0; // Handle error/pre-1970 timestamps
    }

    // 2. Convert NTP Fractions to Milliseconds
    // Math: (Fractions * 1000) / 2^32. 
    // We use 64-bit math to prevent overflow during multiplication.
    uint32_t ms_part = (uint32_t)(((uint64_t)pTimeSNTP->Fractions * 1000) >> 32);

    // 3. Combine into total milliseconds for Ti64
    new_ms = ((uint64_t)unix_secs * 1000) + ms_part;
    
    OS_TASK_EnterRegion();
    OS_INT_Disable();
    uint64_t old_ms = g_SystemTime.Ti64;
    int64_t delta = (int64_t)(new_ms - old_ms);

    g_SystemTime.Ti64 = new_ms;
    // Shift the stamp so elapsed time remains relative to the original event
    g_SystemTime.Ti_stamp_power_on += delta; 
    
    g_SystemTime.crc = _Calculate_Full_Ti_Crc(g_SystemTime.Ti64, g_SystemTime.Ti_stamp_power_on);

    // Reset LPTIM3 phase
    LPTIM3->CR &= ~LPTIM_CR_ENABLE;
    LPTIM3->CR |= LPTIM_CR_ENABLE;
    LPTIM3->ARR = 61440 - 1; 
    LPTIM3->CR |= LPTIM_CR_CNTSTRT;
    OS_INT_Enable();
    OS_TASK_LeaveRegion();
     return delta;

}

/**
 * @brief Unified Dual-Resolution Timer (Task-Safe)
 * @param stamp_to_compare: 0 for uptime, or a previous Dual-Res result.
 * @return int32_t: ms (>0), sec (<0), or 0 (error).
 */
int32_t Get_Elapsed_Dual_Res(int32_t stamp_to_compare) {
    OS_TASK_EnterRegion();
    
    uint64_t now_abs_ms = Get_Absolute_Ms64();
    if (now_abs_ms == 0) { OS_TASK_LeaveRegion(); return 0; }

    uint64_t session_ref_ms = g_SystemTime.Ti_stamp_power_on;
    uint64_t now_rel_ms = now_abs_ms - session_ref_ms;
    uint64_t then_rel_ms = 0;

    if (stamp_to_compare != 0) {
        then_rel_ms = (stamp_to_compare > 0) ? (uint64_t)stamp_to_compare : (uint64_t)(-(int64_t)stamp_to_compare) * 1000;
    }

    if (now_rel_ms < then_rel_ms) { OS_TASK_LeaveRegion(); return 0; }
    
    uint64_t delta_ms = now_rel_ms - then_rel_ms;
    int32_t result;

    if (delta_ms <= 0x7FFFFFFFUL) {
        result = (int32_t)delta_ms;
    } else {
        uint64_t delta_sec = delta_ms / 1000;
        result = (delta_sec > 0x7FFFFFFFUL) ? (int32_t)0x80000000 : -(int32_t)delta_sec;
    }

    OS_TASK_LeaveRegion();
    return result;
}

int64_t Get_Elapsed_Dual_Res_64(int32_t stamp_to_compare) {
    OS_TASK_EnterRegion();
    
    uint64_t now_abs_ms = Get_Absolute_Ms64();
    if (now_abs_ms == 0) { OS_TASK_LeaveRegion(); return 0; }

    uint64_t session_ref_ms = g_SystemTime.Ti_stamp_power_on;
    uint64_t now_rel_ms = now_abs_ms - session_ref_ms;
    uint64_t then_rel_ms = 0;

    if (stamp_to_compare != 0) {
        then_rel_ms = (stamp_to_compare > 0) ? (uint64_t)stamp_to_compare : (uint64_t)(-(int64_t)stamp_to_compare) * 1000;
    }

    if (now_rel_ms < then_rel_ms) { OS_TASK_LeaveRegion(); return 0; }
    
    int64_t delta_ms = now_rel_ms - then_rel_ms;

    OS_TASK_LeaveRegion();
    return delta_ms;
}

/**
 * @brief Converts a polymorphic session stamp back to absolute Unix time_t.
 * @param stamp: Result from a previous Get_Elapsed_Dual_Res(0) call.
 * (Positive for ms, Negative for sec).
 * @return time_t: The absolute Unix timestamp in seconds.
 */
time_t Get_Time_t(int32_t stamp) {
    OS_TASK_EnterRegion();
    uint64_t session_ref_ms = g_SystemTime.Ti_stamp_power_on;
    uint64_t stamp_ms = (stamp >= 0) ? (uint64_t)stamp : (uint64_t)(-(int64_t)stamp) * 1000;
    time_t abs_time = (time_t)((session_ref_ms + stamp_ms) / 1000);
    OS_TASK_LeaveRegion();
    return abs_time;
}


U32 Segger_GetTimeDate(void) 
// Rtc read for file system time
{
   U32 r;
   U16 Sec, Min, Hour;
   U16 Day, Month, Year;
   struct tm    *pTM;

//   RTC_TimeTypeDef      time_rtc;
//   RTC_DateTypeDef      date_rtc;

time_t Untime = Get_Time_t(Get_Elapsed_Dual_Res(0));
pTM      = gmtime(&Untime);


// allway together !!!!!!
//   HAL_RTC_GetTime(&hrtc, &time_rtc, RTC_FORMAT_BIN);
//   HAL_RTC_GetDate(&hrtc, &date_rtc, RTC_FORMAT_BIN);

   Sec   = pTM->tm_sec;     // 0 based.  Valid range: 0..59
   Min   = pTM->tm_min;     // 0 based.  Valid range: 0..59
   Hour  = pTM->tm_hour;       // 0 based.  Valid range: 0..23
   Day   = pTM->tm_mday;        // 1 based.    Means that 1 is 1. Valid range is 1..31 (depending on month)
   Month = pTM->tm_mon  + 1;       // 1 based.    Means that January is 1. Valid range is 1..12.
   Year  = pTM->tm_year - 80;      // 1980 based. Means that 2007 would be 27.
   r   = Sec / 2 + (Min << 5) + (Hour  << 11);
   r  |= (U32)(Day + (Month << 5) + (Year  << 9)) << 16;
   return r;
}



// Local time

/* Returns the last Sunday of a given month/year, at 1:00 UTC (as time_t) */
static time_t last_sunday_at_1h(int year, int month)
{
    struct tm t = {0};
    t.tm_year = year;
    t.tm_mon  = month;      /* 0-based: next month, day 0 = last day of target month */
    t.tm_mday = 0;          /* day 0 of next month = last day of current month */
    t.tm_hour = 1;
    t.tm_min  = 0;
    t.tm_sec  = 0;
    t.tm_isdst = 0;

    time_t t_last = mktime(&t);
    struct tm *lt = gmtime(&t_last);

    /* Roll back to Sunday (tm_wday: 0=Sun) */
    int days_back = lt->tm_wday;
    t_last -= (time_t)(days_back * 86400);

    return t_last;
}

/*
 * Europe DST:
 *   Start : last Sunday of March  at 01:00 UTC  => clocks go forward
 *   End   : last Sunday of October at 01:00 UTC  => clocks go back
 */
bool is_dst_europe(time_t utc_time)
{
    struct tm *t = gmtime(&utc_time);
    int year = t->tm_year;  /* years since 1900, passed as-is to last_sunday_at_1h */

    time_t dst_start = last_sunday_at_1h(year, 2);  /* month 2 = March  (0-based) */
    time_t dst_end   = last_sunday_at_1h(year, 9);  /* month 9 = October (0-based) */

    return (utc_time >= dst_start && utc_time < dst_end);
}

/*
 * US DST (since 2007 - Energy Policy Act):
 *   Start : 2nd Sunday of March    at 02:00 local == ~07:00/08:00 UTC
 *   End   : 1st Sunday of November at 02:00 local
 *
 *   For simplicity we compare in UTC using approximate offsets.
 *   Pass the *standard* offset of the zone so the local 02:00 is correct.
 */
static time_t nth_sunday_at_2h_local(int year, int month, int nth, int std_offset_min)
{
    struct tm t = {0};
    t.tm_year  = year;
    t.tm_mon   = month;
    t.tm_mday  = 1;
    t.tm_hour  = 2;
    t.tm_min   = 0;
    t.tm_sec   = 0;
    t.tm_isdst = 0;

    /* UTC equivalent of 02:00 local standard time on the 1st of the month */
    time_t t1 = mktime(&t) - (time_t)(std_offset_min * 60);

    struct tm *lt = gmtime(&t1);

    /* Advance to first Sunday */
    int days_to_sun = (7 - lt->tm_wday) % 7;
    t1 += (time_t)(days_to_sun * 86400);

    /* Then skip (nth-1) more weeks */
    t1 += (time_t)((nth - 1) * 7 * 86400);

    return t1;
}

/*
 * is_dst_us: generic for any US zone — pass the zone's std_offset_min.
 * Because the function signature is bool(*)(time_t), we provide
 * one wrapper per zone.
 */
static bool _is_dst_us(time_t utc_time, int std_offset_min)
{
    struct tm *t = gmtime(&utc_time);
    int year = t->tm_year;

    time_t dst_start = nth_sunday_at_2h_local(year, 2,  2, std_offset_min); /* 2nd Sun March    */
    time_t dst_end   = nth_sunday_at_2h_local(year, 10, 1, std_offset_min); /* 1st Sun November */

    return (utc_time >= dst_start && utc_time < dst_end);
}

bool is_dst_us_est(time_t utc_time) { return _is_dst_us(utc_time, -300); }
bool is_dst_us_cst(time_t utc_time) { return _is_dst_us(utc_time, -360); }
bool is_dst_us_pst(time_t utc_time) { return _is_dst_us(utc_time, -480); }

/* ── City table ──────────────────────────────────────────────────────────── */

CityTimeZone cities[] = {
    /* CET */
    {"Paris",          "CET",   60, 60, is_dst_europe},
    {"Berlin",         "CET",   60, 60, is_dst_europe},
    {"Warsaw",         "CET",   60, 60, is_dst_europe},
    {"Rome",           "CET",   60, 60, is_dst_europe},
    {"Madrid",         "CET",   60, 60, is_dst_europe},
    {"Vienna",         "CET",   60, 60, is_dst_europe},
    {"Brussels",       "CET",   60, 60, is_dst_europe},
    {"Amsterdam",      "CET",   60, 60, is_dst_europe},
    {"Prague",         "CET",   60, 60, is_dst_europe},
    {"Budapest",       "CET",   60, 60, is_dst_europe},
    {"Zagreb",         "CET",   60, 60, is_dst_europe},
    /* EET */
    {"Sofia",          "EET",  120, 60, is_dst_europe},
    {"Bucharest",      "EET",  120, 60, is_dst_europe},
    {"Athens",         "EET",  120, 60, is_dst_europe},
    {"Helsinki",       "EET",  120, 60, is_dst_europe},
    {"Vilnius",        "EET",  120, 60, is_dst_europe},
    {"Riga",           "EET",  120, 60, is_dst_europe},
    {"Tallinn",        "EET",  120, 60, is_dst_europe},
    {"Kiev",           "EET",  120, 60, is_dst_europe},
    /* WET */
    {"Lisbon",         "WET",    0, 60, is_dst_europe},
    {"Dublin",         "WET",    0, 60, is_dst_europe},
    /* No DST */
    {"Tokyo",          "JST",  540,  0, NULL},
    {"Dubai",          "GST",  240,  0, NULL},
    {"Rio de Janeiro", "BRT", -180,  0, NULL},
    /* US */
    {"New York",       "EST", -300, 60, is_dst_us_est},
    {"Chicago",        "CST", -360, 60, is_dst_us_cst},
    {"Los Angeles",    "PST", -480, 60, is_dst_us_pst},
    {NULL, NULL, 0, 0, NULL}
};

/* ── Conversion ──────────────────────────────────────────────────────────── */

time_t utc_to_local(int i, time_t utc_time)
{
    int offset_min = cities[i].std_offset_min;
    if (cities[i].is_dst_func && cities[i].is_dst_func(utc_time))
        offset_min += cities[i].dst_offset_min;
    return utc_time + (time_t)(offset_min * 60);
}

int find_city(const char *name)
{
    for (int i = 0; cities[i].city; i++)
        if (strcmp(cities[i].city, name) == 0)
            return i;
    return -1;
}





#ifdef DM42
#  pragma GCC pop_options
#endif // DM42



