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
-------------------------- END-OF-HEADER -----------------------------

File    : FS_NOR_HW_SPIFI_STM32U585_ST_B_U585I_IOT02A_Discovery.c
Purpose : Low-level flash driver for STM32U5 OCTO SPI interface.
Literature:
  [1] RM0456 Reference manual STM32U575/585 Arm-based 32-bit MCUs
    (\\fileserver\Techinfo\Company\ST\MCU\STM32\STM32U5\RM0456-STM32U575585.pdf)
  [2] Datasheet STM32U585xx
    (\\fileserver\Techinfo\Company\ST\MCU\STM32\STM32U5\DS13086_STM32U585AI_Rev4.pdf)
  [3] Errata sheet STM32U575xx STM32U585xx
    (\\fileserver\Techinfo_rw\Company\ST\MCU\STM32\STM32U5\ES0499_Rev_0.3_Pub.pdf)
  [4] UM2839 User manual Discovery kit for IoT node with STM32U5 Series
    (\\fileserver\Techinfo\Company\ST\MCU\STM32\STM32U5\EvalBoard\B-U585I-IOT02A Discovery kit\UM2839_B-U585i-iot02a_discovery-kit-(stm32u5-series)_rev3.pdf)
  [5] Datasheet MX25LM51245G
    ("\\FILESERVER\Techinfo\Company\Macronix\SPI_NOR_Flash\MX25LM51245G, 3V, 512Mb, v1.1.pdf")
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "FS.h"
#include "SEGGER_RTT.h" // utilisation SEGGER_RTT_printf
#include "BSP.h"
#include "RTOS.h"

#include "stm32u5xx_hal.h"


#if USE_EmFile
   #include "DBxxxx.h"
   #include "RTOS.h"
   #include "stm32u5xx.h"
   #include "FS_OS.h"
#endif

/*********************************************************************
*
*       Defines, non-configurable
*
**********************************************************************
*/

// SPI Configuration
#define SPI_INSTANCE            SPI1
#define SPI_CLOCK_ENABLE()      __HAL_RCC_SPI1_CLK_ENABLE()
#define SPI_CLOCK_DISABLE()     __HAL_RCC_SPI1_CLK_DISABLE()

// GPIO Configuration (PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI)
#define SPI_GPIO_PORT           GPIOA
#define SPI_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()

#define SPI_SCK_PIN             GPIO_PIN_5      // PA5
#define SPI_MISO_PIN            GPIO_PIN_6      // PA6
#define SPI_MOSI_PIN            GPIO_PIN_5      // PB5
#define SPI_CS_PIN              GPIO_PIN_0      // PB0

#define SPI_GPIO_AF             GPIO_AF5_SPI1

// Flash Memory Specifications
/*
#define FLASH_PAGE_SIZE         256
#define FLASH_SECTOR_SIZE       4096
#define FLASH_BLOCK_SIZE        65536
#define FLASH_TOTAL_SIZE        (8 * 1024 * 1024)  // 8MB
*/



#ifdef DM42
#  pragma GCC push_options
#  pragma GCC optimize("-O0")
#endif // DM42


// SPI Timeout
#define SPI_TIMEOUT_MS          5

HAL_StatusTypeDef fs_spi_status;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

SPI_HandleTypeDef hSPI;
U8 _IsInitialized = 0;


/*********************************************************************
*
*       _HW_EnableCS
*
*  Function description
*    Enables chip select (pulls CS low).
*/
 void _HW_EnableCS(U8 Unit) {
    FS_USE_PARA(Unit);
    OS_TASK_EnterRegion();
   OS_INT_Disable();
    HAL_GPIO_WritePin(GPIOB, SPI_CS_PIN, GPIO_PIN_RESET);
}

/*********************************************************************
*
*       _HW_DisableCS
*
*  Function description
*    Disables chip select (pulls CS high).
*/
 void _HW_DisableCS(U8 Unit) {
   FS_USE_PARA(Unit);
   HAL_GPIO_WritePin(GPIOB, SPI_CS_PIN, GPIO_PIN_SET);
   OS_TASK_LeaveRegion();
   OS_INT_Enable();
}


void _HW_DeInit(U8 Unit);

/*********************************************************************
*
*       _HW_Read
*
*  Function description
*    Reads data from SPI.
*/
void _HW_Read(U8 Unit, U8 *pData, int NumBytes) {
   FS_USE_PARA(Unit);
   fs_spi_status = HAL_SPI_Receive(&hSPI, pData, NumBytes, SPI_TIMEOUT_MS);
   if ( HAL_OK != fs_spi_status)
   {
      SEGGER_RTT_printf(0, "\nFS spi err Read : %d", fs_spi_status);
//      _HW_DeInit(0);
//      RTT_vprintf_cr_T_F( _HW_Read, "\nFS spi err Read : %d", fs_spi_status);

   }
}

/*********************************************************************
*
*       _HW_Write
*
*  Function description
*    Writes data to SPI.
*/
static void _HW_Write(U8 Unit, const U8 *pData, int NumBytes) {
   FS_USE_PARA(Unit);
   fs_spi_status =   HAL_SPI_Transmit(&hSPI, (U8*)pData, NumBytes, SPI_TIMEOUT_MS);
   if ( HAL_OK != fs_spi_status)
   {
      SEGGER_RTT_printf(0, "\nFS spi err Write : %d", fs_spi_status);
//      _HW_DeInit(0);
//      RTT_vprintf_cr_T_F( _HW_Write, "\nFS spi err Write : %d", fs_spi_status);
   }
}



/*********************************************************************
*
*       _InitSPI
*
*  Function description
*    Initializes the SPI interface.
*/
 void _InitSPI(void) 
 {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
   RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
   PeriphClkInit.Spi1ClockSelection = RCC_SPI1CLKSOURCE_SYSCLK;     // 160Mhz
//   PeriphClkInit.Spi1ClockSelection = RCC_SPI1CLKSOURCE_MSIK;

    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    // Enable clocks
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
    SPI_CLOCK_ENABLE();
    
    // Configure GPIO pins
    // Set CS high initially
    HAL_GPIO_WritePin(SPI_GPIO_PORT, SPI_CS_PIN, GPIO_PIN_SET);
    // CS pin (PB0) - Manual control
    GPIO_InitStruct.Pin = SPI_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    /**SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PB0     ------> SPI1_NSS
    PB5     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Configure SPI
    hSPI.Instance = SPI_INSTANCE;
    hSPI.Init.Mode = SPI_MODE_MASTER;
    hSPI.Init.Direction = SPI_DIRECTION_2LINES;
    hSPI.Init.DataSize = SPI_DATASIZE_8BIT;
    hSPI.Init.CLKPolarity = SPI_POLARITY_LOW;
    hSPI.Init.CLKPhase = SPI_PHASE_1EDGE;
    hSPI.Init.NSS = SPI_NSS_SOFT;
// 160Mhz / 8 : ok
// 160Mhz /16 : safest
    hSPI.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // Adjust as needed
    hSPI.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hSPI.Init.TIMode = SPI_TIMODE_DISABLE;
    hSPI.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hSPI.Init.CRCPolynomial = 7;
    hSPI.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hSPI.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    
    if (HAL_SPI_Init(&hSPI) != HAL_OK) {
        // Handle error
        return;
    }

/*
   ENTER DEEP POWER-DOWN (B9h)
   RELEASE FROM DEEP POWER-DOWN (ABh)
   RESET ENABLE (66h)
   RESET MEMORY (99h)
*/
const U8 Cmd_release_dpd = 0xab;
const U8 Cmd_reset_ena = 0x66;
const U8 Cmd_reset_mem = 0x99;

_HW_EnableCS(1);
_HW_Write( 1, &Cmd_release_dpd, 1);
_HW_DisableCS(1);


// trop violent ??????
OS_TASK_Delay(1);
_HW_EnableCS(1);
_HW_Write( 1, &Cmd_reset_ena, 1);
_HW_DisableCS(1);

_HW_EnableCS(1);
_HW_Write( 1, &Cmd_reset_mem, 1);
_HW_DisableCS(1);


}

void _DeInitSPI(void) 
/* Spi1 clock disable, pins deinit 
 * SCK, MISO, MOSI pins (PA5, PA6, PA7)
 * CS pin (PA4) - Manual control
*/
{
const U8 Cmd_enter_dpd = 0xb9;

   _HW_EnableCS(1);
   _HW_Write( 1, &Cmd_enter_dpd, 1);
   _HW_DisableCS(1);

   __HAL_RCC_SPI1_CLK_DISABLE();
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);         //cs
   GPIO_InitStruct.Pin = GPIO_PIN_6;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/*********************************************************************
*
*       _HW_Init
*
*  Function description
*    Initializes the hardware.
*/
int _HW_Init(U8 Unit) {
    FS_USE_PARA(Unit);
    if (_IsInitialized == 0) 
    {
        _InitSPI();
        _IsInitialized = 1;
    }
    return 10000000;
}


void _HW_DeInit(U8 Unit) {
    FS_USE_PARA(Unit);
    if (_IsInitialized == 1) 
    {
        _DeInitSPI();
        _IsInitialized = 0;
    }

}



/*********************************************************************
*
*       _HW_Delay
*
*  Function description
*    Delays for the specified number of milliseconds.
*/
static void _HW_Delay(int ms) {
    HAL_Delay(ms);
}

/*********************************************************************
*
*       _HW_Lock
*
*  Function description
*    Locks the SPI interface (if using RTOS).
*/
extern OS_MUTEX Mut_SPIFS;
static void _HW_Lock(U8 Unit) {
    FS_USE_PARA(Unit);
    // Implement locking mechanism if using RTOS
    // For bare metal, this can be empty
    OS_MUTEX_LockBlocked(&Mut_SPIFS);

// efficace ????

}

/*********************************************************************
*
*       _HW_Unlock
*
*  Function description
*    Unlocks the SPI interface (if using RTOS).
*/
static void _HW_Unlock(U8 Unit) {
    FS_USE_PARA(Unit);
    // Implement unlocking mechanism if using RTOS
    // For bare metal, this can be empty
    OS_MUTEX_Unlock(&Mut_SPIFS);
}

/*********************************************************************
*
*       FS_NOR_HW_SPI_Template
*
*  Description
*    Hardware layer API for SPI NOR flash.
*/
const FS_NOR_HW_TYPE_SPI FS_NOR_HW_SPI_MTQL128 = {
    _HW_Init,           // pfInit
    _HW_EnableCS,       // pfEnableCS
    _HW_DisableCS,      // pfDisableCS
    _HW_Read,           // pfRead
    _HW_Write,          // pfWrite
    NULL,               // pfRead_x2  (Dual SPI not used)
    NULL,               // pfWrite_x2 (Dual SPI not used)
    NULL,               // pfRead_x4  (Quad SPI not used)
    NULL,               // pfWrite_x4 (Quad SPI not used)
    NULL,          // pfDelay
    _HW_Lock,           // pfLock
    _HW_Unlock,         // pfUnlock
    NULL,         // pfReadEx
    NULL         // pfWriteEx
};


#ifdef DM42
#  pragma GCC pop_options
#endif // DM42



// new version to do with dma and interrupts  ????


/*************************** End of file ****************************/
