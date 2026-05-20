/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2023  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
Purpose : Config file for STM32U585_B_U585I_IOT02A_Discovery
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "USB.h"
#include "BSP_USB.h"
#include "stm32u5xx.h"
#include "stm32u5xx_hal.h"

#include "RTOS.h"




/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define USB_FS_ISR_ID                           (67)

/*********************************************************************
*
*       Typedefs
*
**********************************************************************
*/
typedef void USB_ISR_HANDLER  (void);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static USB_ISR_HANDLER * _pfUSBISRHandler;

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/****** Declare ISR handler here to avoid "no prototype" warning. They are not declared in any CMSIS header */

#ifdef __cplusplus
extern "C" {
#endif
void OTG_FS_IRQHandler(void);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       OTG_FS_IRQHandler
*/
void OTG_FS_IRQHandler(void) {
  OS_EnterInterrupt(); // Inform embOS that interrupt code is running
  if (_pfUSBISRHandler) {
    (_pfUSBISRHandler)();
  }
  OS_LeaveInterrupt(); // Inform embOS that interrupt code is left
}

/*********************************************************************
*
*       BSP_USB_InstallISR_Ex()
*/
void BSP_USB_InstallISR_Ex(int ISRIndex, void (*pfISR)(void), int Prio){
  (void)Prio;
  _pfUSBISRHandler = pfISR;
  NVIC_SetPriority((IRQn_Type)ISRIndex, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ((IRQn_Type)ISRIndex);
}

/*********************************************************************
*
*       BSP_USBH_InstallISR_Ex()
*/
void BSP_USBH_InstallISR_Ex(int ISRIndex, void (*pfISR)(void), int Prio){
  (void)Prio;
  _pfUSBISRHandler = pfISR;
  NVIC_SetPriority((IRQn_Type)ISRIndex, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ((IRQn_Type)ISRIndex);
}


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USB_ISR_ID    OTG_FS_IRQn
#define USB_ISR_PRIO  254

/*********************************************************************
*
*       Defines, sfrs
*
**********************************************************************
*/

#define OTG_FS_GOTTGCTL_BVALOVAL (1u << 7)  // B-peripheral session valid override value
#define OTG_FS_GOTTGCTL_BVALOEN  (1u << 6)  // B-peripheral session valid override enable


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U32 _EPBufferPool[1280 / 4];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _EnableISR
*/
static void _EnableISR(USB_ISR_HANDLER * pfISRHandler) {
  BSP_USB_InstallISR_Ex(USB_ISR_ID, pfISRHandler, USB_ISR_PRIO);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       Setup which target USB driver shall be used
*/

/*********************************************************************
*
*       USBD_X_Config
*/
void USBD_X_Config(void) {
  //
  // Configure IO's
  //
  RCC->AHB2ENR1 |= 0
                |  (1uL << RCC_AHB2ENR1_GPIOAEN_Pos)
                ;
  //
  //  PA10: USB_ID
  //
//  GPIOA->MODER   =   (GPIOA->MODER & ~(3UL  <<  20)) | (2UL  <<  20);
//  GPIOA->OTYPER  |=   (1UL  <<  10);
//  GPIOA->OSPEEDR |=   (3UL  <<  20);
//  GPIOA->PUPDR    =   (GPIOA->PUPDR  & ~(3UL  <<  20)) | (1UL << 20);
//  GPIOA->AFR[1]   =   (GPIOA->AFR[1] & ~(15UL << 8)) | (10UL << 8);
  //
  //  PA11: USB_DM
  //
  GPIOA->MODER    =   (GPIOA->MODER  & ~(3UL  <<  22)) | (2UL  <<  22);
  GPIOA->OTYPER  &=  ~(1UL  <<  11);
  GPIOA->OSPEEDR |=   (3UL  <<  22);
  GPIOA->PUPDR   &=  ~(3UL  <<  22);
  GPIOA->AFR[1]   =   (GPIOA->AFR[1]  & ~(15UL << 12)) | (10UL << 12);
  //
  //  PA12: USB_DP
  //
  GPIOA->MODER    =   (GPIOA->MODER  & ~(3UL  <<  24)) | (2UL  <<  24);
  GPIOA->OTYPER  &=  ~(1UL  <<  12);
  GPIOA->OSPEEDR |=   (3UL  <<  24);
  GPIOA->PUPDR   &=  ~(3UL  <<  24);
  GPIOA->AFR[1]   =   (GPIOA->AFR[1]  & ~(15UL << 16)) | (10UL << 16);


  //
  // Enable HSI48
  //
  RCC->CR |= (1uL << RCC_CR_HSI48ON_Pos);
  while ((RCC->CR & (1uL << RCC_CR_HSI48RDY_Pos)) == 0) {
  }
  //
  // Set USB clock selector to HSI48
  // utiliser : RCC_CCIPR1_ICLKSEL_Pos
//  RCC->CCIPR1 &= ~(3uL << RCC_CCIPR1_CLK48MSEL_Pos);
  

/* from stm32cubemx, exemple avec pll2
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_CLK48;
//    PeriphClkInit.IclkClockSelection = RCC_CLK48CLKSOURCE_PLL2;
PeriphClkInit.IclkClockSelection = RCC_CLK48CLKSOURCE_HSI48;
    PeriphClkInit.PLL2.PLL2Source = RCC_PLLSOURCE_HSI;
    PeriphClkInit.PLL2.PLL2M = 3;
    PeriphClkInit.PLL2.PLL2N = 36;
    PeriphClkInit.PLL2.PLL2P = 2;
    PeriphClkInit.PLL2.PLL2Q = 4;
    PeriphClkInit.PLL2.PLL2R = 2;
    PeriphClkInit.PLL2.PLL2RGE = RCC_PLLVCIRANGE_0;
    PeriphClkInit.PLL2.PLL2FRACN = 0;
    PeriphClkInit.PLL2.PLL2ClockOut = RCC_PLL2_DIVQ;
*/
//    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
  
  __HAL_RCC_CLK48_CONFIG(RCC_CLK48CLKSOURCE_HSI48);
  
  //
  //  Enable clock for OTG_HS2
  //
  RCC->AHB2ENR1   |=  (1uL << RCC_AHB2ENR1_OTGEN_Pos);
  USB_OS_Delay(10);
  //
  // Reset USB clock
  //
  RCC->AHB2RSTR2   |=  (1uL << RCC_AHB2RSTR1_OTGRST_Pos);
  USB_OS_Delay(10);
  RCC->AHB2RSTR2  &= ~(1uL << RCC_AHB2RSTR1_OTGRST_Pos);
  USB_OS_Delay(40);
  //
  //  Enable
  //
  PWR->SVMCR  |= (1uL << PWR_SVMCR_USV_Pos);
  //
  // Override the B-Session valid operation from the USB PHY
  //
  USB_OTG_HS->GOTGCTL |= 0
                      | OTG_FS_GOTTGCTL_BVALOVAL
                      | OTG_FS_GOTTGCTL_BVALOEN
                      ;
  USBD_AddDriver(&USB_Driver_ST_STM32U5xx_DynMem);
  USBD_AssignMemory(_EPBufferPool, sizeof(_EPBufferPool));
  USBD_SetISRMgmFuncs(_EnableISR, USB_OS_IncDI, USB_OS_DecRI);
}

/*************************** End of file ****************************/
