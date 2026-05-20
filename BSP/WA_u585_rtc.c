/*#include "h7a3_rtc.h"
#include "h7a3_kbd.h"
#include "h7a3_bkpram.h"
#include "h7a3_low_power.h"
#include "h7a3_Sharp_LS.h"

#include "SEGGER_RTT.h"
#include "FS.h"



// We want to directly read the RTC, so bypass the shadow registers.
  HAL_RTCEx_EnableBypassShadow(&hrtc);
????????????????????

*/


#include "DBxxxx.h"

#define _USE_64BIT_TIME_T


extern uint8_t       wakeup_from;
extern OS_EVENT      RTC_Event, PB_Event, KBD_Event, USB_Event, DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER;
extern OS_MAILBOX    Mb_Keyboard;


extern RTC_HandleTypeDef hrtc;


time_t  time_t_week = 0;
uint32_t time_t_w_calc_t = 0;

#ifdef DM42
#  pragma GCC push_options
#  pragma GCC optimize("-O0")
#endif // DM42

void RTC_IRQHandler(void){
   HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
//   RTT_vprintf_cr_time( "Rtc irq");
   
   OS_INT_Enter();
   OS_EVENT_Set(&RTC_Event);  // never used ???


   OS_EVENT_Set(&KBD_Event);


      OS_EVENT_Set(&WAKE_UP_EVENT);

//OS_TASKEVENT_Set( &TDB48X, EV_DBx_RTC);
   OS_TASKEVENT_Set( &TKBD, EV_KPo_RTC);

//   st_key_data dts;
//   dts.sys = 1;
//   dts.cmd.sys_cmd = SYS_int_rtc;
//   dts.cmd.sys_data = 0;
//   OS_MAILBOX_Put(&Mb_Keyboard, &dts);  

   wakeup_from =2;
   OS_INT_Leave();

}



/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
  void    Error_Handler(int err_no);



/**
 * @brief Configure LSE as RTC clock source
 */
static void RTC_ConfigureLSE(void)
{

    // Check if LSE is already running and properly configured
    if ((RCC->BDCR & RCC_BDCR_LSERDY) != 0) {
        // LSE already ready, just ensure it's selected as RTC source
        if ((RCC->BDCR & RCC_BDCR_RTCSEL) != RCC_BDCR_RTCSEL_0) {
            RCC->BDCR &= ~RCC_BDCR_RTCSEL;
            RCC->BDCR |= RCC_BDCR_RTCSEL_0;
        }
        return;
    }
    
    // Check if LSE is enabled but not ready yet
    if ((RCC->BDCR & RCC_BDCR_LSEON) != 0) {
        // LSE is enabled, just wait for it to be ready
        uint32_t timeout = 10000000;
        while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0 && timeout > 0) {
            timeout--;
        }
        
        if ((RCC->BDCR & RCC_BDCR_LSERDY) != 0) {
            // LSE is now ready
            RCC->BDCR &= ~RCC_BDCR_RTCSEL;
            RCC->BDCR |= RCC_BDCR_RTCSEL_0;
            return;
        }
        
        // If still not ready, disable and reconfigure
        RCC->BDCR &= ~RCC_BDCR_LSEON;
        for (volatile int i = 0; i < 10000; i++);
    }
    
    // Configure LSE drive capability to highest (required for some boards)
    // LSEDRV[1:0] = 11 (highest drive)
    RCC->BDCR &= ~RCC_BDCR_LSEDRV;
    RCC->BDCR |= RCC_BDCR_LSEDRV_1 | RCC_BDCR_LSEDRV_0;
    
    // Enable LSE bypass if using external clock instead of crystal
    // Uncomment the next line if you're using an external clock source
    // RCC->BDCR |= RCC_BDCR_LSEBYP;
    
    // Enable LSE oscillator
    RCC->BDCR |= RCC_BDCR_LSEON;
    
    // Wait for LSE to be ready with timeout
    uint32_t timeout = 10000000; // ~10 second timeout (LSE can take 1-2 seconds to start)
    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0 && timeout > 0) {
        timeout--;
    }
    
    // If timeout, LSE crystal may not be present or faulty
    if (timeout == 0) {
        // LSE failed to start - handle error
        // Option 1: Try with bypass mode (external clock)
        RCC->BDCR &= ~RCC_BDCR_LSEON;
        for (volatile int i = 0; i < 10000; i++);
        
        RCC->BDCR |= RCC_BDCR_LSEBYP; // Enable bypass
        RCC->BDCR |= RCC_BDCR_LSEON;
        
        timeout = 1000000; // Shorter timeout for bypass mode
        while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0 && timeout > 0) {
            timeout--;
        }
        
        if (timeout == 0) {
            // LSE completely failed - could use LSI as fallback
            // For now, just return (RTC won't work without clock)
            return;
        }
    }
    
    // Select LSE as RTC clock source
    RCC->BDCR &= ~RCC_BDCR_RTCSEL;
    RCC->BDCR |= RCC_BDCR_RTCSEL_0; // LSE selected as RTC clock
    }


#define RTC_INIT_MARKER  0x57d1  // Unique magic number in backup register

void RTC_UpdateTimeDate(uint8_t hours, uint8_t minutes, uint8_t seconds,
                        uint8_t day, uint8_t month, uint8_t year, uint8_t weekday);



void RTC_Config_1024_Granularity(void)
{
 HAL_PWR_EnableBkUpAccess();

    // Enable RTC clocks
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTCAPB_CLKAM_ENABLE();

    // Check magic number to detect first run
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != RTC_INIT_MARKER)
    {
        // --- First-time initialization ---
        __HAL_RCC_BACKUPRESET_FORCE();
        __HAL_RCC_BACKUPRESET_RELEASE();



        // 1️⃣ Enable LSE oscillator
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
        RCC_OscInitStruct.LSEState = RCC_LSE_ON;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler(1);

        // 2️⃣ Select LSE as RTC clock source
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) Error_Handler(1);

        // 3️⃣ Initialize RTC peripheral
        hrtc.Instance = RTC;
        hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  /* Configure prescalers for 1024 granularity */
  /* With LSE = 32.768 kHz:
   * - Asynchronous prescaler = 31 (division by 32)
   * - Synchronous prescaler = 1023 (division by 1024)
   * - Result: 32768 / (32 * 1024) = 1 Hz for calendar
   * - SubSeconds will count from 1023 down to 0, providing 1024 steps
   */
  hrtc.Init.AsynchPrediv = 31;   /* 32-1 */
  hrtc.Init.SynchPrediv = 1023;  /* 1024-1 */

        hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
        hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

        if (HAL_RTC_Init(&hrtc) != HAL_OK) Error_Handler(1);

   RTC_SetBuildTime();

        // 5️⃣ Store magic number
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_INIT_MARKER);
    }
    else
    {


//    RTC_ConfigureLSE();
        // --- After reset, RTC backup domain is fine, but no clock selected
            // 2️⃣ Select LSE as RTC clock source

        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) Error_Handler(1);



                hrtc.Instance = RTC;
        hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  /* Configure prescalers for 1024 granularity */
  /* With LSE = 32.768 kHz:
   * - Asynchronous prescaler = 31 (division by 32)
   * - Synchronous prescaler = 1023 (division by 1024)
   * - Result: 32768 / (32 * 1024) = 1 Hz for calendar
   * - SubSeconds will count from 1023 down to 0, providing 1024 steps
   */
  hrtc.Init.AsynchPrediv = 31;   /* 32-1 */
  hrtc.Init.SynchPrediv = 1023;  /* 1024-1 */

        hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
        hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

        if (HAL_RTC_Init(&hrtc) != HAL_OK) Error_Handler(1);


    }
}




/**
  * @brief RTC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hrtc->Instance==RTC)
  {
    /* USER CODE BEGIN RTC_MspInit 0 */

    /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(127);
    }

    /* Peripheral clock enable */
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTCAPB_CLKAM_ENABLE();
    /* USER CODE BEGIN RTC_MspInit 1 */

    /* USER CODE END RTC_MspInit 1 */

  }

}







uint32_t rtc_read_ticks_old(void)
// getting time for os in tickless mode in msec, maximun value a week in msec
{
   RTC_TimeTypeDef      time_rtc;
   RTC_DateTypeDef      date_rtc;


/* verifier si pas d'erreur sur l'horloge !!!!!!!
 * #define RCC_BDCR_LSESYSRDY                  RCC_BDCR_LSESYSRDY_Msk                  //< LSE System Clock (LSESYS) Ready 
 * #define RCC_BDCR_LSESYSEN                   RCC_BDCR_LSESYSEN_Msk                   //< LSE System Clock (LSESYS) Enable 
 * #define RCC_BDCR_LSECSSON                   RCC_BDCR_LSECSSON_Msk                   //!< CSS on LSE Enable 
 * #define RCC_BDCR_LSECSSD                    RCC_BDCR_LSECSSD_Msk                    //!< CSS on LSE failure Detection 
 * #define RCC_BDCR_LSERDY                     RCC_BDCR_LSERDY_Msk                     //!< LSE Oscillator Ready 
*/


   GPIO_InitTypeDef GPIO_InitStruct = {0};
   __HAL_RCC_GPIOC_CLK_ENABLE();
   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);         // LED
   GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);


// allways together !!!!!!
   HAL_RTC_GetTime(&hrtc, &time_rtc, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &date_rtc, RTC_FORMAT_BIN);
   uint32_t prescaler = hrtc.Init.SynchPrediv + 1;
   uint32_t msec = (1000000 - ((time_rtc.SubSeconds * 1000000) / prescaler))/1000;
// prescaler = 1024

   uint32_t   ticks =   (time_rtc.Seconds +
                        time_rtc.Minutes  * 60 +
                        time_rtc.Hours * 3600 +
                        ((date_rtc.WeekDay+6) % 7) * 86400) * 1000 + msec;

   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);         // LED
   return ticks;

}

/*

uint32_t rtc_elapsed_ticks(uint32_t rtc_ticks)
// returning time elapsed since the argument in msec
{
   uint32_t rtc_c_ticks = rtc_read_ticks();
   if (rtc_c_ticks < rtc_ticks) 
      return (rtc_c_ticks + (24*3600*7 )*1000) - rtc_ticks;
   else
      return rtc_c_ticks - rtc_ticks;
}

*/





/**
 * @brief Check if RTC is already configured
 * @retval 1 if configured, 0 if not configured
 */
uint8_t RTC_IsConfigured(void)
{
    return (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == RTC_MAGIC_NUMBER) ? 1 : 0;
}






// Helper to convert month string to number (e.g., "Jun" → 6)
static uint8_t month_str_to_num(const char *month_str) {
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (int i = 0; i < 12; i++) {
        if (strncmp(month_str, months[i], 3) == 0) {
            return i + 1;
        }
    }
    return 1; // Default to January if something fails
}

void RTC_set_time_t(time_t t_time)
{
   char buf[64];
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

   struct tm result;
   struct tm *pt = gmtime_r(&t_time, &result);



    // Format: YYYY-MM-DD HH:MM:SS
    strftime(buf, sizeof(buf), "%a %d-%m-%Y %H:%M:%S", pt);
   RTT_vprintf_cr_time("Setting rtc with : %s", buf);

       // Fill RTC time structure
    sTime.Hours   = pt->tm_hour;
    sTime.Minutes = pt->tm_min;
    sTime.Seconds = pt->tm_sec;
    sTime.TimeFormat = RTC_HOURFORMAT_24;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // Fill RTC date structure
    sDate.Year = pt->tm_year % 100;  // RTC expects year in 2-digit format
    sDate.Month = pt->tm_mon+1;
    sDate.Date = pt->tm_mday;
    sDate.WeekDay = pt->tm_wday;  // You can calculate this if needed


    // Apply the time and date
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
         RTT_vprintf_cr_time("Error Setting rtc time");

       // Error_HandlerMsg("RtcInit");
    }

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
         RTT_vprintf_cr_time("Error Setting rtc date");
      //  Error_HandlerMsg("RtcInit");
    }

   /* Write backup register to indicate RTC is configured */
   HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR8, RTC_MAGIC_NUMBER);


}

void RTC_SetBuildTime(void) {

    // Extract time from __TIME__ = "HH:MM:SS"
    int hour, minute, second;
    sscanf(__TIME__, "%2d:%2d:%2d", &hour, &minute, &second);

    // Extract date from __DATE__ = "Mmm dd yyyy"
    char month_str[4];
    int day, year;
    sscanf(__DATE__, "%3s %2d %4d", month_str, &day, &year);


    // Create a struct tm in local compilation time
    struct tm t = {
        .tm_year = year - 1900,
        .tm_mon  = month_str_to_num(month_str) - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min  = minute,
        .tm_sec  = second,
        .tm_isdst = -1
    };

    time_t local_time = mktime(&t);

//     int offset_min = get_city_utc_offset("Paris", t.tm_year, t.tm_mon, t.tm_mday);
     int offset_min = 0;
     
      time_t utc_time = local_time - offset_min*60;

 struct tm *pt =  gmtime(&utc_time);

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    // Fill RTC time structure
    sTime.Hours   = pt->tm_hour;
    sTime.Minutes = pt->tm_min;
    sTime.Seconds = pt->tm_sec;
    sTime.TimeFormat = RTC_HOURFORMAT_24;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // Fill RTC date structure
    sDate.Year = pt->tm_year % 100;  // RTC expects year in 2-digit format
    sDate.Month = pt->tm_mon+1;
    sDate.Date = pt->tm_mday;
    sDate.WeekDay = pt->tm_wday;  // You can calculate this if needed


    // Apply the time and date
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
       // Error_HandlerMsg("RtcInit");
    }

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
      //  Error_HandlerMsg("RtcInit");
    }

   /* Write backup register to indicate RTC is configured */
   HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR8, RTC_MAGIC_NUMBER);

}






time_t rtc_read_time_t(char *pCity)
// getting time for os in tickless mode
{
   struct tm timeinfo;
   RTC_TimeTypeDef      sTime;
   RTC_DateTypeDef      sDate;

   // toujours ensemble !!!!!!
   HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

   /* fill tm structure */
   timeinfo.tm_year = (sDate.Year) + 100;  /* Years since 1900 (20xx -> 1xx) */
   timeinfo.tm_mon = (sDate.Month) - 1;    /* Months since January (0-11) */
   timeinfo.tm_mday = (sDate.Date);        /* Day of the month (1-31) */
   timeinfo.tm_hour = (sTime.Hours);       /* Hours (0-23) */
   timeinfo.tm_min = (sTime.Minutes);      /* Minutes (0-59) */
   timeinfo.tm_sec = (sTime.Seconds);      /* Seconds (0-59) */
   timeinfo.tm_wday = 0;   /* Not used by mktime */
   timeinfo.tm_yday = 0;   /* Not used by mktime */
   timeinfo.tm_isdst = -1; /* Let mktime determine DST */


   int offset_min = (pCity != NULL) ? get_city_utc_offset(pCity, timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday) : 0;

   /* Convert to time_t */
   return mktime(&timeinfo)+60*offset_min;

}




/* test ??? */
// Helper: check for leap year
bool is_leap(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Get day of the week (0=Sunday, 6=Saturday) using Zeller's Congruence
int day_of_week(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int k = year % 100;
    int j = year / 100;
    return (day + 13*(month+1)/5 + k + k/4 + j/4 + 5*j) % 7;
}





/**
 * @brief Debug function - check RTC status
 * @return Status code for debugging
 */
uint32_t RTC_GetDebugStatus(void)
{
    uint32_t status = 0;
    
    // Bit 0: PWR clock enabled
    if (RCC->AHB3ENR & RCC_AHB3ENR_PWREN) status |= (1 << 0);
    
    // Bit 1: Backup domain write enabled
    if (PWR->DBPR & PWR_DBPR_DBP) status |= (1 << 1);
    
    // Bit 2: LSE enabled
    if (RCC->BDCR & RCC_BDCR_LSEON) status |= (1 << 2);
    
    // Bit 3: LSE ready
    if (RCC->BDCR & RCC_BDCR_LSERDY) status |= (1 << 3);
    
    // Bit 4: RTC clock enabled
    if (RCC->BDCR & RCC_BDCR_RTCEN) status |= (1 << 4);
    
    // Bit 5-6: RTC clock source (01 = LSE)
    status |= ((RCC->BDCR & RCC_BDCR_RTCSEL) >> 8) << 5;
    
    // Bit 7: RTC calendar initialized
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;
    if (RTC->ICSR & RTC_ICSR_INITS) status |= (1 << 7);
    RTC->WPR = 0xFF;
    

        __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTCAPB_CLKAM_ENABLE();

    // Check magic number to detect first run
    // Bit 8: Magic number valid
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == RTC_INIT_MARKER) status |= (1 << 8);
    
    return status;
}


void LSECSSD_IRQHandler(void){

   GPIO_InitTypeDef GPIO_InitStruct = {0};
   __HAL_RCC_GPIOC_CLK_ENABLE();
   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);         // LED
   GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);



}



#ifdef DM42
#  pragma GCC pop_options
#endif // DM42

