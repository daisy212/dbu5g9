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
Purpose : USB driver header file for EHCI USB controller.
-------------------------- END-OF-HEADER -----------------------------
*/

#ifndef USB_HW_EHCI_H
#define USB_HW_EHCI_H

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
*       Define configurable
*
**********************************************************************
*/

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/
//lint -save -e9004 -e762  D:999 This definitions should be later removed from USB.h
void USB_DRIVER_LPC18xx_ConfigAddr    (U32 BaseAddr);
void USB_DRIVER_KinetisEHCI_ConfigAddr(U32 BaseAddr);
void USB_DRIVER_iMXRT10xx_ConfigAddr  (U32 BaseAddr);
void USB_DRIVER_iMXRT11xx_ConfigAddr  (U32 BaseAddr);
void USB_DRIVER_iMXRT118x_ConfigAddr  (U32 BaseAddr);
void USB_DRIVER_Zynq7010_ConfigAddr   (U32 BaseAddr);

/*********************************************************************
*
*       Available target USB drivers
*
**********************************************************************
*/
extern const USB_HW_DRIVER USB_Driver_Freescale_KinetisEHCI;
extern const USB_HW_DRIVER USB_Driver_NXP_LPC18xx;
extern const USB_HW_DRIVER USB_Driver_NXP_LPC43xx;
extern const USB_HW_DRIVER USB_Driver_NXP_LPC43xx_DynMem;
extern const USB_HW_DRIVER USB_Driver_NXP_iMXRT10xx;
extern const USB_HW_DRIVER USB_Driver_NXP_iMXRT10xx_DynMem;
extern const USB_HW_DRIVER USB_Driver_NXP_iMXRT118x_DynMem;
extern const USB_HW_DRIVER USB_Driver_Xilinx_Zynq7010;
extern const USB_HW_DRIVER USB_Driver_Xilinx_Zynq7010_DynMem;
//lint -restore

void USBD_DRIVER_iMXRT118x_SetV2PHandler(USBD_V2P_FUNC * pfV2PHandler);

#if defined(__cplusplus)
  }              /* Make sure we have C-declarations in C++ programs */
#endif

#endif /* USB_HW_EHCI_H */

/*************************** End of file ****************************/
