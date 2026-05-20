/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2025 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS-Ultra * Real time operating system                     *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.20.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : BSP.c
Purpose : BSP for ST STM32U5G9J-DK2
*/

#include "BSP.h"
#include "stm32u5xx.h"

#include "SEGGER_RTT.h" // using SEGGER_RTT_printf

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

#define LED0_BIT           (4)  // LD3, Green, connected to PD4
#define LED1_BIT           (2)  // LD2, Red,   connected to PD2

#define RCC_BASE_ADDR      (0x46020C00u)
#define RCC_AHB2ENR1       (*(volatile unsigned int*)(RCC_BASE_ADDR + 0x8Cu))
#define GPIODEN_BIT        (3)

#define GPIOD_BASE_ADDR    (0x42020C00u)
#define GPIOD_MODER        (*(volatile unsigned int*)(GPIOD_BASE_ADDR + 0x00u))  // GPIOD port mode register
#define GPIOD_IDR          (*(volatile unsigned int*)(GPIOD_BASE_ADDR + 0x10u))  // GPIOD input data register
#define GPIOD_BSRR         (*(volatile unsigned int*)(GPIOD_BASE_ADDR + 0x18u))  // GPIOD bit set/reset register

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       BSP_Init()
*/
void BSP_Init(void) {
  RCC_AHB2ENR1 |=  (1u << GPIODEN_BIT);     // Enable GPIOD clock
  GPIOD_MODER  &= ~(3u << (LED0_BIT * 2))   // Clear mode bits
               &  ~(3u << (LED1_BIT * 2));
  GPIOD_MODER  |=  (1u << (LED0_BIT * 2))   // Set mode to output
               |   (1u << (LED1_BIT * 2));
}

/*********************************************************************
*
*       BSP_SetLED()
*/
void BSP_SetLED(int Index) {
  switch (Index) {
  case 0:
    GPIOD_BSRR |= (1u << (LED0_BIT));
    break;
  case 1:
    GPIOD_BSRR |= (1u << (LED1_BIT));
    break;
  default:
    break;
  }
}

/*********************************************************************
*
*       BSP_ClrLED()
*/
void BSP_ClrLED(int Index) {
  switch (Index) {
  case 0:
    GPIOD_BSRR |= (1u << (LED0_BIT + 16u));
    break;
  case 1:
    GPIOD_BSRR |= (1u << (LED1_BIT + 16u));
    break;
  default:
    break;
  }
}

/*********************************************************************
*
*       BSP_ToggleLED()
*/
void BSP_ToggleLED(int Index) {
  switch (Index) {
  case 0:
    if (GPIOD_IDR & (1u << LED0_BIT)) {
      GPIOD_BSRR |= (1u << (LED0_BIT + 16u));
    } else {
      GPIOD_BSRR |= (1u << LED0_BIT);
    }
    break;
  case 1:
    if (GPIOD_IDR & (1u << LED1_BIT)) {
      GPIOD_BSRR |= (1u << (LED1_BIT + 16u));
    } else {
      GPIOD_BSRR |= (1u << LED1_BIT);
    }
    break;
  default:
    break;
  }
}

RTC_HandleTypeDef hrtc;  // still needed for HAL_RTC_SetTime/SetDate


void RTC_Init(void) {
    U32 Timeout;

    // Step 1: Enable PWR clock
    __HAL_RCC_PWR_CLK_ENABLE();

    // Step 2: Enable backup domain write access
    SET_BIT(PWR->DBPR, PWR_DBPR_DBP);
    Timeout = 1000;
    while (HAL_IS_BIT_CLR(PWR->DBPR, PWR_DBPR_DBP)) {
        OS_Delay(1);
        if (--Timeout == 0) {
            SEGGER_RTT_printf(0, "RTC: backup domain access timeout\n");
            return;
        }
    }

    // Step 3: Only do full init if LSE not already running
    // (preserves time across resets)
    if (READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == 0) {

        // Reset backup domain
        SET_BIT(RCC->BDCR,   RCC_BDCR_BDRST);
        CLEAR_BIT(RCC->BDCR, RCC_BDCR_BDRST);
         // high drive
        MODIFY_REG(RCC->BDCR, RCC_BDCR_LSEDRV, RCC_BDCR_LSEDRV_1 | RCC_BDCR_LSEDRV_0);  
        // Enable LSE
        SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);
        // LSE available for lptim
        SET_BIT(RCC->BDCR,    RCC_BDCR_LSESYSEN); 

        // Wait for LSE ready
        Timeout = 5000;
        while (READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == 0) {
            OS_Delay(1);
            if (--Timeout == 0) {
                SEGGER_RTT_printf(0, "RTC: LSE timeout\n");
                return;
            }
        }
        SEGGER_RTT_printf(0, "RTC: LSE ready\n");

        // Select LSE as RTC clock source
        CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCSEL);
        SET_BIT(RCC->BDCR,   RCC_BDCR_RTCSEL_0);  // 01 = LSE

        // Enable RTC, lptim1 and lptim3 clock 
        SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
        SET_BIT(RCC->APB3ENR, RCC_APB3ENR_LPTIM1EN);
        SET_BIT(RCC->APB3ENR, RCC_APB3ENR_LPTIM3EN);

        // Enable RTC APB clock
        SET_BIT(RCC->APB3ENR, RCC_APB3ENR_RTCAPBEN);

        // Disable RTC write protection
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;

        // Enter init mode
        SET_BIT(RTC->ICSR, RTC_ICSR_INIT);
        Timeout = 1000;
        while (READ_BIT(RTC->ICSR, RTC_ICSR_INITF) == 0) {
            OS_Delay(1);
            if (--Timeout == 0) {
                SEGGER_RTT_printf(0, "RTC: init mode timeout\n");
                RTC->WPR = 0xFF;
                return;
            }
        }

        // Set predividers for 32768Hz LSE → 1Hz
        RTC->PRER = (127UL << RTC_PRER_PREDIV_A_Pos) |
                    (255UL << RTC_PRER_PREDIV_S_Pos);

        // 24h format
        CLEAR_BIT(RTC->CR, RTC_CR_FMT);

        // Exit init mode
        CLEAR_BIT(RTC->ICSR, RTC_ICSR_INIT);

        // Re-enable write protection
        RTC->WPR = 0xFF;

        SEGGER_RTT_printf(0, "RTC: init OK\n");

    } else {
        // LSE already running — just make sure RTC clock and APB are on
        SET_BIT(RCC->BDCR,    RCC_BDCR_RTCEN);
        // LSE available for lptim
        SET_BIT(RCC->BDCR,    RCC_BDCR_LSESYSEN);     

        

        SET_BIT(RCC->APB3ENR, RCC_APB3ENR_RTCAPBEN);
        SET_BIT(RCC->APB3ENR, RCC_APB3ENR_LPTIM1EN);
        SET_BIT(RCC->APB3ENR, RCC_APB3ENR_LPTIM3EN);



        SEGGER_RTT_printf(0, "RTC: LSE already running, skipping init\n");
    }

    // Init handle for HAL_RTC_SetTime/SetDate used in UpdateRTCFromNTP()
    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv   = 127;
    hrtc.Init.SynchPrediv    = 255;
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp   = RTC_OUTPUT_PULLUP_NONE;
    hrtc.Init.BinMode        = RTC_BINARY_NONE;
}


/*********************************************************************
*
*       FS_X_GetTimeDate
*
*  Function description
*    Current time and date in a format suitable for the file system.
*
*  Additional information
*    Bit 0-4:   2-second count (0-29)
*    Bit 5-10:  Minutes (0-59)
*    Bit 11-15: Hours (0-23)
*    Bit 16-20: Day of month (1-31)
*    Bit 21-24: Month of year (1-12)
*    Bit 25-31: Count of years from 1980 (0-127)
*/
U32 FS_X_GetTimeDate(void) 
// Rtc read for file system time
{
   U32 r;
   U16 Sec, Min, Hour;
   U16 Day, Month, Year;

   RTC_TimeTypeDef      time_rtc;
   RTC_DateTypeDef      date_rtc;
// allway together !!!!!!
   HAL_RTC_GetTime(&hrtc, &time_rtc, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &date_rtc, RTC_FORMAT_BIN);

   Sec   = time_rtc.Seconds;     // 0 based.  Valid range: 0..59
   Min   = time_rtc.Minutes;     // 0 based.  Valid range: 0..59
   Hour  = time_rtc.Hours;       // 0 based.  Valid range: 0..23
   Day   = date_rtc.Date;        // 1 based.    Means that 1 is 1. Valid range is 1..31 (depending on month)
   Month = date_rtc.Month;       // 1 based.    Means that January is 1. Valid range is 1..12.
   Year  = date_rtc.Year + 20;      // 1980 based. Means that 2007 would be 27.
   r   = Sec / 2 + (Min << 5) + (Hour  << 11);
   r  |= (U32)(Day + (Month << 5) + (Year  << 9)) << 16;
   return r;
}



/*************************** End of file ****************************/
