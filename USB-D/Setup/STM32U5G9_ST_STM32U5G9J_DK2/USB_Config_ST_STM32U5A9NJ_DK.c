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
Purpose : Config file for STM32U5A9 DK board
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "USB_HW_ST.h"
#include "BSP_USB.h"
#include "stm32u5xx.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USB_ISR_ID    OTG_HS_IRQn
#define USB_ISR_PRIO  254

//
// There are two versions of the USB driver:
// With and without cache handling in connection with DMA.
// The STM32U5A9 only uses a data cache for specific memory areas in
// address range 0x60000000 - 0x9FFFFFFF, for external memory.
// There are two options for selecting a USB driver:
//
// USB_STM32U5A9_CACHE_HANDLING_OPTION == 0:
// * The USB driver without cache handling is used.
// * The variable _EPBufferPool[] must be placed in a non-cached memory area.
// * Best performance for all USB transfers from/into non-cached memory area.
// * Reduced performance for USB transfers from/into cached memory areas.
// * Recommended when not using cached memory for USB or at all.
//
// USB_STM32U5A9_CACHE_HANDLING_OPTION == 1:
// * The USB driver with cache handling is used.
// * Best performance for all USB transfers from/into cache aligned buffers.
// * Recommended when large USB data buffers are placed in external memory.
//
#define USB_STM32U5A9_CACHE_HANDLING_OPTION    0

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
#if USB_SUPPORT_TRANSFER_ISO
  static U32 _EPBufferPool[2336 + 512];
#else
  static U32 _EPBufferPool[2336];
#endif

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

#if USB_STM32U5A9_CACHE_HANDLING_OPTION == 0
//
// For this option cached memory is invalid for DMA
//
static int _CheckValidDMAAddress(const void * pMem) {
  //
  // Check address range for OCTOSPI1 bank (0x9XXXXXXX)
  //
  if (((U32)pMem >> 28) == 0x9) {
    return 1;
  }
  return 0;
}

#else /* USB_STM32U5A9_CACHE_HANDLING_OPTION == 1 */
//
// Cache management
//
static void _CleanDCache(void *p, unsigned long NumBytes) {
  U32 Addr;
  U32 CR;

  Addr = (U32)p;
  //
  // Check address range for OCTOSPI1 bank (0x9XXXXXXX)
  //
  if ((Addr >> 28) == 0x9) {
    DCACHE1_NS->CMDRSADDRR = Addr;
    DCACHE1_NS->CMDREADDRR = (Addr + NumBytes - 1) & ~0x1F;
    CR = (DCACHE1_NS->CR & ~(DCACHE_CR_CACHECMD_Msk | DCACHE_CR_STARTCMD)) | DCACHE_CR_CACHECMD_0;
    DCACHE1_NS->CR = CR;
    DCACHE1_NS->FCR = DCACHE_FCR_CCMDENDF | DCACHE_FCR_CBSYENDF;
    DCACHE1_NS->CR = CR | DCACHE_CR_STARTCMD;
    while ((DCACHE1_NS->SR & DCACHE_SR_CMDENDF) == 0) {}
  }
}

static void _InvalidateDCache(void *p, unsigned long NumBytes) {
  U32 Addr;
  U32 CR;

  Addr = (U32)p;
  //
  // Check address range for OCTOSPI1 bank (0x9XXXXXXX)
  //
  if ((Addr >> 28) == 0x9) {
    DCACHE1_NS->CMDRSADDRR = Addr;
    DCACHE1_NS->CMDREADDRR = (Addr + NumBytes - 1) & ~0x1F;
    CR = (DCACHE1_NS->CR & ~(DCACHE_CR_CACHECMD_Msk | DCACHE_CR_STARTCMD)) | DCACHE_CR_CACHECMD_1;
    DCACHE1_NS->CR = CR;
    DCACHE1_NS->FCR = DCACHE_FCR_CCMDENDF | DCACHE_FCR_CBSYENDF;
    DCACHE1_NS->CR = CR | DCACHE_CR_STARTCMD;
    while ((DCACHE1_NS->SR & DCACHE_SR_CMDENDF) == 0) {}
  }
}

static const SEGGER_CACHE_CONFIG _CacheConfig = {
  32,                            // CacheLineSize of CPU
  NULL,                          // pfDMB
  _CleanDCache,                  // pfClean
  _InvalidateDCache              // pfInvalidate
};
#endif

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
  // PWR_VOSR_VOS + PWR_VOSR_BOOSTEN should be already set correctly in SystemInit()
  //
#if 0
  PWR->VOSR     = (PWR->VOSR & ~(PWR_VOSR_VOS_Msk))
                |  (3 << PWR_VOSR_VOS_Pos)
                ;
  while ((PWR->VOSR & (1uL << PWR_VOSR_VOSRDY_Pos)) == 0) {
  }
#endif
  //
  // Power up USB
  //
  RCC->APB3ENR |= (1uL << RCC_APB3ENR_SYSCFGEN_Pos);
  PWR->VOSR    |= PWR_VOSR_USBBOOSTEN;
  while ((PWR->VOSR & (1uL << PWR_VOSR_USBBOOSTRDY_Pos)) == 0) {
  }
  PWR->VOSR    |= PWR_VOSR_USBPWREN;
  PWR->SVMCR   |= (PWR_SVMCR_USV)
               |  (PWR_SVMCR_UVMEN)
               ;
  //
  // Set USB Ref clock to 16 MHz (HSE)
  //
  SYSCFG->OTGHSPHYCR = (3u << 2);
  SYSCFG->OTGHSPHYTUNER2 = (SYSCFG->OTGHSPHYTUNER2 & ~(SYSCFG_OTGHSPHYTUNER2_COMPDISTUNE_Msk | SYSCFG_OTGHSPHYTUNER2_SQRXTUNE_Msk))
                         | (2 << SYSCFG_OTGHSPHYTUNER2_COMPDISTUNE_Pos)
                         | (0 <<  SYSCFG_OTGHSPHYTUNER2_SQRXTUNE_Pos)
                         ;
  SYSCFG->OTGHSPHYCR  |= (1 << 0);
  //
  // Set USB clock selector to HSE
  //
  RCC->CCIPR2 &= ~RCC_CCIPR2_USBPHYCSEL_Msk;
  //
  //  Enable clock for OTG_HS
  //
  RCC->AHB2ENR1 |=  (1uL << RCC_AHB2ENR1_OTGEN_Pos)
                |   (1uL << RCC_AHB2ENR1_USBPHYCEN_Pos)
                ;
  USB_OS_Delay(10);
  //
  // Reset USB clock
  //
  RCC->AHB2RSTR1 |=  (1uL << RCC_AHB2RSTR1_OTGRST_Pos);
  USB_OS_Delay(10);
  RCC->AHB2RSTR1 &= ~(1uL << RCC_AHB2RSTR1_OTGRST_Pos);
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
  USB_OTG_HS->GCCFG  |= 0
                     |  USB_OTG_GCCFG_VBVALOVAL
                     |  USB_OTG_GCCFG_VBVALEXTOEN
                     ;
#if USB_STM32U5A9_CACHE_HANDLING_OPTION == 0
  USBD_AddDriver(&USB_Driver_ST_STM32U5xx_HS_NoCache);
  USBD_SetCheckAddress(_CheckValidDMAAddress);
#else
  USBD_AddDriver(&USB_Driver_ST_STM32U5xx_HS);
  USBD_SetCacheConfig(&_CacheConfig, sizeof(_CacheConfig));
#endif
  USBD_AssignMemory(_EPBufferPool, sizeof(_EPBufferPool));
  USBD_SetISREnableFunc(_EnableISR);
}

/*********************************************************************
*
*       USBD_X_EnableInterrupt
*/
void USBD_X_EnableInterrupt(void) {
  NVIC_EnableIRQ(USB_ISR_ID);
}

/*********************************************************************
*
*       USBD_X_DisableInterrupt
*/
void USBD_X_DisableInterrupt(void) {
  NVIC_DisableIRQ(USB_ISR_ID);
}

/*************************** End of file ****************************/
