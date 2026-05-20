/*
#include "h7a3_rtc.h"
#include "h7a3_usb.h"

#include "h7a3_low_power.h"


#include "FS.h"
*/

#include "BSP.h"
#include "DBxxxx.h"

/*

#ifdef __cplusplus
extern "C" {     // Make sure we have C-declarations in C++ programs 
#endif
void DBx_Task_usb(void);
#ifdef __cplusplus
}
#endif
*/
void DBx_Task_Usb(void) 
{
   usb_connected = false;
   OS_TASKEVENT MyEvents;
   
   OS_EVENT_GetBlocked(&USB_Start);

//   Init_Usb_Detect();
//   HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
   OS_TASK_Delay_ms(50);

//   OS_EVENT_GetBlocked(&USB_Start);
   RTT_vprintf_cr_time(  "Usb : waiting");

   while(1){
      if (!usb_connected)      OS_EVENT_GetBlocked(&EV_USB_Vbus);
//      usb_connected = true;

// same interrupt than keyboard...
     HAL_NVIC_DisableIRQ(USB_DETECT_EXTI_IRQn);
      RTT_vprintf_cr_time( "USB : connecting");

//      SystemClock_Config_MSIS24_MSIK24();       // no pll
//      SystemClock_Config_P160();                // fmax

      USBD_Init();

//      _FSTest();

      _AddMSD();
      USBD_Start();

      OS_TASKEVENT_Set( &TDB48X, EV_DBx_USB_CON);
      MyEvents = OS_TASKEVENT_GetSingleTimed(EV_USB_ACQ, 50);

      while (Usb_Detect()) {
       //
       // Wait for configuration
       int usb_conting = 0;
         while (((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED)&&(Usb_Detect())) {
            usb_conting +=1;
            if (usb_conting > 500) usb_connected = false;
            OS_TASK_Delay_ms(50);
         BSP_ClrLED(0);

         }
            BSP_ToggleLED(0);
         USBD_MSD_Task();
         OS_TASK_Delay_ms(50);
         if ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) break;
      }
      // deconnexion

      FS_Sync("nor:0:");
  //    usb_connected = false;

      RTT_vprintf_cr_time( "USB : disconnect");
      USBD_DeInit();

 //     Init_Usb_Detect();

      OS_EVENT_Reset(&EV_USB_Vbus);
      HAL_NVIC_ClearPendingIRQ(USB_DETECT_EXTI_IRQn);
      HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
      OS_TASK_Delay_ms(1);

// 135µA, 105µA higher than before usb connection 
// sd detect !!!
// 60µA instead of 30µA
//      HAL_PWREx_DisableUSBVoltageDetector();       
// back to 30µA

//      __HAL_RCC_USB_OTG_HS_CLK_DISABLE();    // needed ????

// can't modify pll when running on pll, pll => HSI, pll mod => pll
//      SystemClock_Config_MSIS24_MSIK24();       // no pll
//      SystemClock_Config_P160();                // fmax
//      SystemCoreClockUpdate();


//__HAL_RCC_USB_CLK_DISABLE();
__HAL_RCC_USB_OTG_HS_CLK_DISABLE();
__HAL_RCC_HSI48_DISABLE();

// 4. Power down USB PHY
//USB_OTG_HS->GCCFG &= ~USB_OTG_GCCFG_PWRDWN;

HAL_PWREx_DisableVddUSB(); // 86µA instead of 100µA, still 26µA off !!!
 
// 7. Disable USB interrupts
HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
HAL_NVIC_ClearPendingIRQ(OTG_HS_IRQn); 

      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);         //cs
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

   __HAL_RCC_GPIOC_CLK_DISABLE();

//      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);         //cs
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
   GPIO_InitStruct.Pin = GPIO_PIN_13;
//   GPIO_InitStruct.Pin = GPIO_PIN_ALL & (!( GPIO_PIN_14 | GPIO_PIN_15));
//   HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);








      OS_TASKEVENT_Set( &TDB48X, EV_DBx_USB_DIS);
// this task if higher priority than DB48X, giving some time to work after sending USB_DIS
      MyEvents = OS_TASKEVENT_GetSingleTimed(EV_USB_ACQ, 50);

// a déplacer ???
//      usb_connected = false;        // ==> power stop enabled
   }
}
