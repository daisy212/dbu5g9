//#include "RTOS.h"
#include "BSP.h"
#include "IP.h"
#include "IP_FTP_SERVER.h"

//#include "FS.h"
#include <time.h>
#include "DBxxxx.h"

#include "USB_IP.h"

#include "SEGGER_RTT.h" // using SEGGER_RTT_printf
#include "stm32u5xx.h"


#define NTP_SERVER  "us.pool.ntp.org"

extern RTC_HandleTypeDef hrtc;  // still needed for HAL_RTC_SetTime/SetDate

enum {
   TASK_PRIO_SNTP = 140
   ,TASK_PRIO_IP_WEBSOCKET = 150
   ,TASK_PRIO_FTPS_CHILD
   ,TASK_PRIO_FTPS_PARENT
   ,TASK_PRIO_IP_TASK           // Priority should be higher than all IP application tasks.
#if USE_RX_TASK
  ,TASK_PRIO_IP_RX_TASK        // Must be the highest priority of all IP related tasks, comment out to read packets in ISR
#endif
};


//
// Task stacks and Task-Control-Blocks.
//
static OS_STACKPTR int Stack_IP[TASK_STACK_SIZE_IP_TASK/sizeof(int)];             // Stack of the IP_Task.
static OS_STACKPTR int Stack_pFTP[2048];                  /* Task stack */
static OS_STACKPTR int Stack_WSK[2048];                  /* Task stack */
static OS_STACKPTR int Stack_SNTP[1024];                  /* Task stack */

static OS_TASK        TCB_SNTP, TCB_IP, TCB_pFTP, TCB_WSK;                                                    // Task-Control-Block of the IP_Task.

static IP_HOOK_ON_STATE_CHANGE _StateChangeHook;


static int  _IFaceIdCte = 0;                                           // Get the last registered interface ID as this is most likely the interface we want to use in this sample.
static int32_t ip_connected = 0;

/*********************************************************************
*
*       _OnStateChange()
*
* Function description
*   Callback that will be notified once the state of an interface
*   changes.
*
* Parameters
*   IFaceId   : Zero-based interface index.
*   AdminState: Is this interface enabled ?
*   HWState   : Is this interface physically ready ?
*/
static void _OnStateChange(unsigned IFaceId, U8 AdminState, U8 HWState) {
  //
  // Check if this is a disconnect from the peer or a link down.
  // In this case call IP_Disconnect() to get into a known state.
  //
    IP_Logf_Application( "\n_OnStateChange AdminState:%d HWstate:%d\n", AdminState, HWState);


   if (((AdminState == IP_ADMIN_STATE_DOWN) && (HWState == 1)) ||  // Typical for dial-up connection e.g. PPP when closed from peer. Link up but app. closed.
      ((AdminState == IP_ADMIN_STATE_UP)   && (HWState == 0))) {  // Typical for any Ethernet connection e.g. PPPoE. App. opened but link down.
      IP_Disconnect(IFaceId);     
      ip_connected = -1;                                  // Disconnect the interface to a clean state.

   }
   if  ((AdminState == IP_ADMIN_STATE_UP)   && (HWState == 1)) { 
      ip_connected = 1;  
   }
}

void FormatNTPTime(U32 NTPTimestamp, char *pBuf, int BufSize) {
    time_t  UnixTime;
    struct  tm *pTM;
    
    UnixTime = (time_t)(NTPTimestamp - 2208988800UL);
    pTM      = gmtime(&UnixTime);
    strftime(pBuf, BufSize, "%Y-%m-%d %H:%M:%S", pTM);
}

void UpdateRTCFromNTP(U32 NTPTimestamp) {
    time_t        UnixTime;
    struct tm    *pTM;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    UnixTime = (time_t)(NTPTimestamp - 2208988800UL);
    pTM      = gmtime(&UnixTime);

    // Fill RTC time
    sTime.Hours          = (uint8_t)pTM->tm_hour;
    sTime.Minutes        = (uint8_t)pTM->tm_min;
    sTime.Seconds        = (uint8_t)pTM->tm_sec;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // Fill RTC date
    sDate.Year    = (uint8_t)(pTM->tm_year - 100);  // tm_year is years since 1900, RTC wants years since 2000
    sDate.Month   = (uint8_t)(pTM->tm_mon  + 1);    // tm_mon is 0-based
    sDate.Date    = (uint8_t) pTM->tm_mday;
    sDate.WeekDay = (uint8_t)(pTM->tm_wday == 0 ? RTC_WEEKDAY_SUNDAY : pTM->tm_wday);  // tm_wday: 0=Sunday, HAL: 1=Monday..7=Sunday

    // Write to RTC — date MUST be set after time on STM32
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    SEGGER_RTT_printf(0, "RTC updated: %04d-%02d-%02d %02d:%02d:%02d\n",
                      pTM->tm_year + 1900,
                      pTM->tm_mon  + 1,
                      pTM->tm_mday,
                      pTM->tm_hour,
                      pTM->tm_min,
                      pTM->tm_sec);
}


I32 RTC_GetDeltaFromNTP(U32 NTPTimestamp) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};
    struct tm       TM    = {0};
    time_t          RTCUnixTime;
    time_t          NTPUnixTime;

    // Read RTC — date MUST be read after time
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Fill struct tm
    TM.tm_year  = sDate.Year + 100;  // years since 1900
    TM.tm_mon   = sDate.Month - 1;   // 0-based
    TM.tm_mday  = sDate.Date;
    TM.tm_hour  = sTime.Hours;
    TM.tm_min   = sTime.Minutes;
    TM.tm_sec   = sTime.Seconds;
    TM.tm_isdst = 0;

    // Convert UTC struct tm to Unix time
    // timegm() is the UTC equivalent of mktime() — no TZ adjustment
//    RTCUnixTime = timegm(&TM);
RTCUnixTime = mktime(&TM) ;

    NTPUnixTime = (time_t)(NTPTimestamp - 2208988800UL);

    // Positive = RTC ahead, negative = RTC behind
    return (I32)(RTCUnixTime - NTPUnixTime);
}

void _SNTP_Task(void) {
   IP_NTP_TIMESTAMP Time;
   char buff[64];
   int first = 0;
   int delta = 0;
      BSP_ClrLED(0);
   while (1){

      if (ip_connected>0)
      {
         IP_Logf_Application("Requesting time stamp from %s: ", NTP_SERVER);
         int  Status = -11;
         Status = IP_SNTPC_GetTimeStampFromServer(_IFaceIdCte, NTP_SERVER, &Time);
         if (Status < 0) 
         {
            IP_Logf_Application("Communication error : %d\n", Status);
         } 
         else 
         {
            switch (Status) 
            {
               case IP_SNTPC_STATE_NO_ANSWER:
                  IP_Logf_Application("Timeout.");
                  break;
               case IP_SNTPC_STATE_UPDATED:
                  if ( 0 == first) 
                  {
//                     UpdateRTCFromNTP(Time.Seconds);
                     Adjust_Time( &Time);
                     Init_Rtc(true);
                     first = 1;
                  }
                  else
                  {
//                     delta = RTC_GetDeltaFromNTP(Time.Seconds);
                      delta = Adjust_Time( &Time);
                  }
//                  FormatNTPTime(Time.Seconds, buff, sizeof(buff));
                  time_t  UnixTime = Get_Time_t(Get_Elapsed_Dual_Res(0));
                  struct  tm *pTM;
                  pTM = gmtime(&UnixTime);
                  strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", pTM);
                  uint64_t fsec64 =((uint64_t) Time.Fractions * 1000)>>32; 
                  BSP_SetLED(0);
                  IP_Logf_Application("SNTP: %s, %dmsec, Delta %d,   ", buff, fsec64, delta);
                  break;
               case IP_SNTPC_STATE_KOD:
                  IP_Logf_Application("Kiss-Of-Death received. Use another server.");
                  break;
               default:
                  break;
            }
         }
      }
       OS_TASK_Delay_ms( 60 * 1000 * 5 );
   }
// never
}

void _FTPServerParentTask(void);
void  MX_OCTOSPI1_Init(void);
void RTC_Init(void);



void MainTask_usb(void) {

//   RTC_Init();

   IP_Init();
#if USE_SSL
  SSL_Init();
#endif

   IP_AddStateChangeHook(&_StateChangeHook, _OnStateChange);                            // Register hook to be notified on disconnects.

   OS_CREATETASK(&TCB_IP  , "IP_Task"  , IP_Task  , TASK_PRIO_IP_TASK   , Stack_IP);    // Start the IP_Task.

   IP_Connect(_IFaceIdCte);                                                                // Connect the interface if necessary.
   //
   // Wait until link is up and interface is configured.
   //
   while (IP_IFaceIsReadyEx(_IFaceIdCte) == 0) {
      OS_Delay(50);
      BSP_ToggleLED(1);
   }
ip_connected = 1;

   IP_SetGWAddr(_IFaceIdCte, IP_BYTES2ADDR(100, 127, 137, 2));  // two params: iface + GW
   IP_DNS_SetServer(IP_BYTES2ADDR(8, 8, 8, 8));             // one param only, no index
   SEGGER_RTT_printf(0, "GW and DNS configured\n");

   OS_CREATETASK(&TCB_pFTP  , "pFTP_Task"  , _FTPServerParentTask  , TASK_PRIO_FTPS_PARENT   , Stack_pFTP);  
   OS_CREATETASK(&TCB_SNTP  , "SNTP_Task"  , _SNTP_Task  , TASK_PRIO_SNTP   , Stack_SNTP);  

//  OS_CREATETASK(&TCB_WSK  , "WebSock_Task"  , WSK_Task  , TASK_PRIO_IP_WEBSOCKET   , Stack_WSK);

   while (1) {
      OS_Delay(250);
      if (ip_connected > 0)       BSP_ToggleLED(1);
      else BSP_ToggleLED(0);

      if (ip_connected <0) {
int NewState = USBD_GetState();
SEGGER_RTT_printf(0, "USB status %02x\n", NewState);
         OS_Delay(250);
         USBD_Stop();
         OS_Delay(100);
         USBD_Start();
         ip_connected = 0;
      }    
   }
}