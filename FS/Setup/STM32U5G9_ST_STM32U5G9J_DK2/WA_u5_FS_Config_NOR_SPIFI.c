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

File    : FS_ConfigNOR_BM_SPIFI_STM32U585_ST_B_U585I_IOT02A_Discovery.c
Purpose : Configuration file for serial NOR flash connected via OCTO SPI.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "FS.h"
//#include "FS_NOR_HW_SPIFI_STM32U585_ST_B_U585I_IOT02A_Discovery.h"

extern const FS_NOR_HW_TYPE_SPIFI FS_NOR_HW_SPIFI_STM32U5G9;

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#ifndef   ALLOC_SIZE
  #define ALLOC_SIZE          0x6000          // Size of memory dedicated to the file system. This value should be fine tuned according for your system.
#endif

#ifndef   NOR_BASE_ADDR
  #define NOR_BASE_ADDR       0x70000000      // Base address of the NOR flash device to be used as storage.
#endif

#ifndef   NOR_START_ADDR
  #define NOR_START_ADDR      0x70000000      // Start address of the first sector be used as storage. If the entire chip is used for file system, it is identical to the base address.
#endif

#ifndef   NOR_SIZE
  #define NOR_SIZE            0x00800000      // Number of bytes to be used for storage
#endif

#ifndef   LOG_SECTOR_SIZE
  #define LOG_SECTOR_SIZE     512             // Logical sector size
#endif

#ifndef   ALLOW_OCTAL_MODE
  #define ALLOW_OCTAL_MODE    0               // Enable / disable the data transfer via eight data lines.
#endif

#ifndef   ALLOW_QUAD_MODE
  #define ALLOW_QUAD_MODE    1               // Enable / disable the data transfer via eight data lines.
#endif



#ifndef   ALLOW_DTR_MODE
  #define ALLOW_DTR_MODE      0               // Enable / disable the data transfer on both clock edges.
#endif

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

/*********************************************************************
*
*       _apDeviceAll
*/
static const FS_NOR_SPI_TYPE * _apDeviceMacronix[] = {
 &FS_NOR_SPI_DeviceMacronix,
//  &FS_NOR_SPI_DeviceMacronixOctal,
//  &FS_NOR_SPI_DeviceMacronixOctalSTR,
//  &FS_NOR_SPI_DeviceMacronixOctalDTR,
   &FS_NOR_SPI_DeviceWinbond,
//   &FS_NOR_SPI_DeviceWinbondDTR,
   &FS_NOR_SPI_DeviceMicron,
//   &FS_NOR_SPI_DeviceMicron_x2,
};

static const FS_NOR_SPI_TYPE * _apDeviceWinbond[] = {
   &FS_NOR_SPI_DeviceWinbond,
   &FS_NOR_SPI_DeviceWinbondDTR
};

/*********************************************************************
*
*       _DeviceListMacronix
*/
const FS_NOR_SPI_DEVICE_LIST _DeviceListMacronix = {
  (U8)SEGGER_COUNTOF(_apDeviceMacronix),
  _apDeviceMacronix
};

const FS_NOR_SPI_DEVICE_LIST _DeviceListWinbond = {
  (U8)SEGGER_COUNTOF(_apDeviceWinbond),
  _apDeviceWinbond
};



/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       Memory pool used for semi-dynamic allocation.
*/
#ifdef __ICCARM__
  #pragma location="FS_RAM"
  static __no_init U32 _aMemBlock[ALLOC_SIZE / 4];
#endif
#ifdef __CC_ARM
  static U32 _aMemBlock[ALLOC_SIZE / 4] __attribute__ ((section ("FS_RAM"), zero_init));
#endif
#if (!defined(__ICCARM__) && !defined(__CC_ARM))
  static U32 _aMemBlock[ALLOC_SIZE / 4];
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_X_AddDevices
*
*  Function description
*    This function is called by the FS during FS_Init().
*    It is supposed to add all devices, using primarily FS_AddDevice().
*
*  Note
*    (1) Other API functions may NOT be called, since this function is called
*        during initialization. The devices are not yet ready at this point.
*/
void FS_X_AddDevices(void) {
  //
  // Give file system memory to work with.
  //
  FS_AssignMemory(&_aMemBlock[0], sizeof(_aMemBlock));
  //
  // Configure the size of the logical sector and activate the file buffering.
  //
  FS_SetMaxSectorSize(LOG_SECTOR_SIZE);
#if FS_SUPPORT_FILE_BUFFER
  FS_ConfigFileBufferDefault(LOG_SECTOR_SIZE, FS_FILE_BUFFER_WRITE);
#endif
  //
  // Add and configure the NOR driver.
  //
  FS_AddDevice(&FS_NOR_BM_Driver);
  FS_NOR_BM_SetPhyType(0, &FS_NOR_PHY_SPIFI);
  FS_NOR_BM_Configure(0, NOR_BASE_ADDR, NOR_START_ADDR, NOR_SIZE);
  FS_NOR_BM_SetSectorSize(0, LOG_SECTOR_SIZE);
  //
  // Configure the NOR physical layer.
  //
  FS_NOR_SPIFI_SetHWType(0, &FS_NOR_HW_SPIFI_STM32U5G9);
  FS_NOR_SPIFI_SetDeviceList(0, &_DeviceListMacronix);
 FS_NOR_SPIFI_AllowOctalMode(0, ALLOW_OCTAL_MODE);
  FS_NOR_SPIFI_Allow4bitMode(0, ALLOW_QUAD_MODE);
  FS_NOR_SPIFI_AllowDTRMode(0, ALLOW_DTR_MODE);
}





/*************************** End of file ****************************/
