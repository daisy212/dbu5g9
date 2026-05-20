#ifndef WA_u585_LOW_POWER_H
#define WA_u585_LOW_POWER_H

#include "stm32u5xx.h"
#include "stdbool.h"
#include "stdint.h"
#include <time.h>
#include <stdio.h>

#include "RTOS.h"
#include "IP.h"
#include "DBx_typedef.h"


//#define TIME_BEFORE_STOP2_msec   (500)
#define TIME_BEFORE_STOP2_msec   (KB_SCRUT_PERIOD*5)


typedef enum {
/* etats possible :
 * 0 : emerging from deep sleep
 * 1 : running, scrutation active
 * 2 : sleep, keyboard with interrupt
 * 3 : entering deep sleep, 
 * 4 : deep sleep
 * 5 : power_on

 */
   PW_wake_up=0,
   PW_running,
   PW_sleeping,
   PW_near_deep_sleep,
   PW_request_Poff,
   PW_deep_sleep,
   PW_power_on,
   PW_power_on_waiting,
   PW_waking_up_from_POFF,
   PW_LAST,
}t_POWER_STATE;

typedef struct Ti_crc { 
    uint64_t Ti64;               // Absolute system time (ms)
    uint64_t Ti_stamp_power_on;  // Reference captured at wake-up
    uint32_t crc;                // CRC32 of Ti64 and Ti_stamp_power_on
} Ti_crc;


extern  CityTimeZone cities[] ;

extern const char* power_names[];
extern LPTIM_HandleTypeDef hlptim1;
extern LPTIM_HandleTypeDef hlptim3;

extern t_POWER_STATE db_power_state;
extern t_POWER_STATE db_previous_power_state;

void ready_to_stop2_periph(void);

void init_unused_pins(void);
void ready_to_stop2_gpio(void);
void Restore_After_STOP2(void);
void disable_debug(void);

//#define TIME_CORRECTION(x)    ((usb_connected ? (7*x)/2:x))
#define TIME_CORRECTION(x)    (x)


//void SystemClock_Config_P280(void);
//void SystemClock_Config_P120(void);
//void SystemClock_Config_P64(void); // no pll for cpu


void SystemClock_Config_MSIS24_MSIK24(void);  // no pll for cpu
void SystemClock_Config_P160(void); // F max
void SystemClock_Config_P50(void);  // with PLL

void MX_LPTIM1_Init(void);
void MX_LPTIM3_Init(void);
/*
int64_t rd_lptim3_i64(void);
void sync_lptim3(void);
int32_t rd_lptim3_i32(void);
int32_t lptim3_elapsed_ticks_i32(int32_t start);
*/

void PeriphCommonClock_Config_P16_R64(void); // do nothing

void    Error_Handler(int err_no);


uint32_t EnterSTOP2(void);

uint32_t ReadBatVoltage(void);

// new
void Init_Power_On_Stamp(void);
int32_t Get_Elapsed_Dual_Res(int32_t stamp_to_compare);
int64_t Get_Elapsed_Dual_Res_64(int32_t stamp_to_compare);

time_t Get_Time_t(int32_t stamp);
int64_t Adjust_Time(IP_NTP_TIMESTAMP *pTimeSNTP);
void Init_Time(bool use_rtc);
bool Init_Rtc(bool use_lptim3);
U32 Segger_GetTimeDate(void);

time_t utc_to_local(int i, time_t utc_time);
int find_city(const char *name);




extern   uint32_t vbat;
extern uint32_t vbat_rd_time;


#endif //WA_u585_LOW_POWER_H

