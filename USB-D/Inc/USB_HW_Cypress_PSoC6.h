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

#ifndef USB_HW_CYPRESS_PSOC6_H
#define USB_HW_CYPRESS_PSOC6_H

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
*       Types
*
**********************************************************************
*/
typedef struct {
  void *     pDmaChannel;
  U8         Prio;
} USB_CYPRESS_PSoC6_DMA_CH_CFG;

typedef struct {
  void     (*pfTrigger)(unsigned Endpoint);
  USB_CYPRESS_PSoC6_DMA_CH_CFG ChannelCfg[8];
} USB_CYPRESS_PSoC6_DMA_CONFIG;

typedef struct {
  unsigned (*pfInitDMADescr)   (U32 * pDesc, int IsIn, U32 Size, PTR_ADDR DataRegAddr, const void *pBuffer);
  unsigned (*pfUpdateDMADescr) (U32 * pDesc, U32 Size, const void *pData);
  void     (*pfEnableDMA)      (const USB_CYPRESS_PSoC6_DMA_CH_CFG *pDMA, U32 * pDesc, U8 FirstDescr);
  void     (*pfDisableDMA)     (const USB_CYPRESS_PSoC6_DMA_CH_CFG *pDMA);
} USB_CYPRESS_PSoC6_DMA_API;

extern const USB_CYPRESS_PSoC6_DMA_API USB_DRIVER_Cypress_PSoC6_DWx;

/*********************************************************************
*
*       API functions
*
**********************************************************************
*/
void USB_DRIVER_Cypress_PSoC6_SysTick(void);
void USB_DRIVER_Cypress_PSoC6_Resume(void);
void USB_DRIVER_Cypress_PSoC6_ConfigDMA(const USB_CYPRESS_PSoC6_DMA_API *pAPI, const USB_CYPRESS_PSoC6_DMA_CONFIG *pCFG);

/*********************************************************************
*
*       Available target USB drivers
*
**********************************************************************
*/
extern const USB_HW_DRIVER USB_Driver_Cypress_PSoC6;
extern const USB_HW_DRIVER USB_Driver_Cypress_PSoC6_DMA;

#if defined(__cplusplus)
  }              /* Make sure we have C-declarations in C++ programs */
#endif

#endif /* USB_HW_CYPRESS_PSOC6_H */

/*************************** End of file ****************************/
