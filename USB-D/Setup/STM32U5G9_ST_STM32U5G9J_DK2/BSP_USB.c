/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
Purpose : BSP for ST STM32U5G9_STM32U5G9J_DK2 board
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "BSP_USB.h"
#include "RTOS.h"
#include "stm32u5xx.h"  // Device specific header file, contains CMSIS

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
static USB_ISR_HANDLER * _pfPD_IsrHandler;

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
void OTG_HS_IRQHandler(void);
void UCPD1_IRQHandler(void);
#ifdef __cplusplus
}
#endif

/*********************************************************************
*
*       OTG_HS_IRQHandler
*/
void OTG_HS_IRQHandler(void) {
  OS_EnterInterrupt(); // Inform embOS that interrupt code is running
  if (_pfUSBISRHandler) {
    (_pfUSBISRHandler)();
  }
  OS_LeaveInterrupt(); // Inform embOS that interrupt code is left
}

/*********************************************************************
*
*       UCPD1_IRQHandler
*/
void UCPD1_IRQHandler(void) {
  OS_EnterInterrupt(); // Inform embOS that interrupt code is running
  if (_pfPD_IsrHandler != NULL) {
    (_pfPD_IsrHandler)();
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
*       BSP_USBC_InstallISR()
*/
void BSP_USBC_InstallISR(int ISRIndex, void (*pfISR)(void)){
  _pfPD_IsrHandler = pfISR;
  NVIC_SetPriority((IRQn_Type)ISRIndex, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ((IRQn_Type)ISRIndex);
}

/****** End Of File *************************************************/
