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
Purpose : USB driver header file for ST devices.
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USB_HW_ST_H
#define USB_HW_ST_H

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/
#include "USB.h"

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

/*********************************************************************
*
*       Aliases
*
**********************************************************************
*/

#define USB_Driver_ST_STM32                USB_Driver_ST_STM32x32
#define USB_Driver_ST_STM32F3xx6           USB_Driver_ST_STM32x16
#define USB_Driver_ST_STM32F3xx8           USB_Driver_ST_STM32x16
#define USB_Driver_ST_STM32F3xxB           USB_Driver_ST_STM32x32
#define USB_Driver_ST_STM32F3xxC           USB_Driver_ST_STM32x32
#define USB_Driver_ST_STM32F3xxD           USB_Driver_ST_STM32x16
#define USB_Driver_ST_STM32F3xxE           USB_Driver_ST_STM32x16
#define USB_Driver_ST_STM32F0              USB_Driver_ST_STM32F0xx

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/
//lint -save -e9004 -e762  D:999 This definitions should be later removed from USB.h
void USB_DRIVER_STM32F7xxHS_ConfigPHY(U8 UsePHY);
void USB_DRIVER_STM32F4xxHS_ConfigPHY(U8 UsePHY);
void USB_DRIVER_STM32H5xx_ConfigAddr(U32 RegBaseAddr, U32 RamBaseAddr);
void USB_DRIVER_STM32U5xx_ConfigAddr(U32 RegBaseAddr, U32 RamBaseAddr);

int USB_DRIVER_STM32L4xx_DetectChargingPort(void);

/*********************************************************************
*
*       Available target USB drivers
*
**********************************************************************
*/
extern const USB_HW_DRIVER USB_Driver_ST_STM32x32;
extern const USB_HW_DRIVER USB_Driver_ST_STM32x16;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F107;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F0xx;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F4xxFS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F4xxHS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F4xxHS_DMA;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F4xxHS_inFS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F7xxFS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F7xxFS_DynMem;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F7xxHS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F7xxHS_DynMem;
extern const USB_HW_DRIVER USB_Driver_ST_STM32F7xxHS_DMA;
extern const USB_HW_DRIVER USB_Driver_ST_STM32H7xxHS_DMA;
extern const USB_HW_DRIVER USB_Driver_ST_STM32H7xxFS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32H7xxFS_DynMem;
extern const USB_HW_DRIVER USB_Driver_ST_STM32H7xxHS_inFS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32H7xxHS_inFS_DynMem;
extern const USB_HW_DRIVER USB_Driver_ST_STM32L4xx;
extern const USB_HW_DRIVER USB_Driver_ST_STM32L4x2;
extern const USB_HW_DRIVER USB_Driver_ST_STM32L5x2;
extern const USB_HW_DRIVER USB_Driver_ST_STM32H5xx;
extern const USB_HW_DRIVER USB_Driver_ST_STM32U5xx_FS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32U5xx_HS;
extern const USB_HW_DRIVER USB_Driver_ST_STM32U5xx_HS_NoCache;
extern const USB_HW_DRIVER USB_Driver_ST_STM32U5xx_NG;
//lint -restore

#if defined(__cplusplus)
  }              /* Make sure we have C-declarations in C++ programs */
#endif

#endif /* USB_HW_EHCI_H */

/*************************** End of file ****************************/
