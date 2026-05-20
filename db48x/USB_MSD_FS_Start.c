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
-------------------------- END-OF-HEADER -----------------------------

File    : USB_MSD_FS_Start.c
Purpose : This sample demonstrates the use of the MSD component together
          with emFile.

Additional information:
  Preparations:
    The correct emFile configuration file has
    to be included in the project. Depending on the hardware
    it can be one of the following:
    * FS_ConfigRAMDisk_23k.c
    * FS_ConfigNAND_*.c
    * FS_ConfigMMC_CardMode_*.c
    * FS_ConfigNAND_*.c

  Expected behavior:
    This sample will format the storage medium if necessary and
    create a "Readme.txt" file in the root of the storage
    medium. After the formatting is done and the USB cable has
    been connected to a PC a new MSD volume will show up with
    a "Readme.txt" file in the root directory.

  Sample output:
    The target side does not produce terminal output.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include "USB.h"
#include "USB_MSD.h"
#include "FS.h"
#include "BSP.h"
#include "DBxxxx.h"

extern OS_EVENT        KBD_Event, USB_Event;


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define BUFFER_SIZE       8192

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
//  Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfo = {
  0x8765,         // VendorId
  0x1000,         // ProductId
  "Vendor",       // VendorName
  "MSD device",   // ProductName
  "000013245678"  // SerialNumber. Should be 12 character or more for compliance with Mass Storage Device For Bootability spec.
};
//
// String information used when inquiring the volume 0.
//
static const USB_MSD_LUN_INFO _Lun0Info = {
  "Vendor",     // MSD VendorName
  "DB48x", // MSD ProductName
  "1.00",       // MSD ProductVer
  "134657890"   // MSD SerialNo
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U32 _aSectorBuffer[BUFFER_SIZE / 4];     // Used as sector buffer in order to do read/write sector bursts (~8 sectors at once)

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _FSTest
*/
 void _FSTest(bool initFS) {
  FS_FILE    * pFile;
  unsigned     Len;
  const char * sInfo = "This sample is based on the SEGGER emUSB-Device software with an MSD component.\r\nFor further information please visit: www.segger.com\r\n";
  unsigned     NumVolumes;
  unsigned     i;
  char         acVolumeName[20];
  bool force_format = false;

  Len        = strlen(sInfo);
  NumVolumes = FS_GetNumVolumes();
  for (i = 0; i < NumVolumes; i++) {
    FS_GetVolumeName(i, &acVolumeName[0], sizeof(acVolumeName));



    if (FS_IsLLFormatted(acVolumeName) == 0) {
      RTT_vprintf_cr_time("Low level formatting to do...");
      FS_FormatLow(acVolumeName);          /* Erase & Low-level  format the volume */
    }
    if ((FS_IsHLFormatted(acVolumeName) == 0) || ( true ==   force_format))
 {
      RTT_vprintf_cr_time("Low level formatting to do...");
  //    FS_FormatLow(acVolumeName);          /* Erase & Low-level  format the volume */
      RTT_vprintf_cr_time("High level formatting to do...");
      FS_Format(acVolumeName, NULL);       /* High-level format the volume */
    }
   strcat(acVolumeName, "\\Readme.txt");
   pFile = FS_FOpen(acVolumeName, "w");
   RTT_vprintf_cr_time( "USB : Init FS, open readme.txt : %s", (NULL == pFile) ?"error":"ok");
   uint32_t w_len = FS_Write(pFile, sInfo, Len);
   RTT_vprintf_cr_time( "USB : Init FS, writing readme.txt : %s", (w_len != Len) ?"error":"ok");
    FS_FClose(pFile);

    FS_SetVolumeLabel(acVolumeName, "DB48x");
    FS_Unmount(acVolumeName);
  }
}

/*********************************************************************
*
*       _AddMSD
*
*  Function description
*    Add mass storage device to USB stack
*
*  Notes:
*   (1)  -   This examples uses the internal driver of the file system.
*            The module initializes the low-level part of the file system if necessary.
*            If FS_Init() was not previously called, none of the high level functions
*            such as FS_FOpen, FS_Write etc will work.
*            Only functions that are driver related will be called.
*            Initialization, sector read/write, retrieve device information.
*            The members of the DriverData are used as follows:
*              DriverData.pStart       = VOLUME_NAME such as "nand:", "mmc:1:".
*              DriverData.NumSectors   = Number of sectors to be used - 0 means auto-detect.
*              DriverData.StartSector  = The first sector that shall be used.
*              DriverData.SectorSize will not be used.
*/
 void _AddMSD(void) {
  static U8         _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
  USB_MSD_INIT_DATA InitData;
  USB_MSD_INST_DATA InstData;
  USB_ADD_EP_INFO       EPIn;
  USB_ADD_EP_INFO       EPOut;

  memset(&InitData, 0, sizeof(InitData));
  EPIn.Flags          = 0;                             // Flags not used.
  EPIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
  EPIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
  EPIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPIn  = USBD_AddEPEx(&EPIn, NULL, 0);

  EPOut.Flags         = 0;                             // Flags not used.
  EPOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
  EPOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
  EPOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
  EPOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
  InitData.EPOut = USBD_AddEPEx(&EPOut, _abOutBuffer, sizeof(_abOutBuffer));

  USBD_SetDeviceInfo(&_DeviceInfo);
  USBD_MSD_Add(&InitData);
  //
  // Add logical unit 0:
  //
  memset(&InstData, 0,  sizeof(InstData));
  InstData.pAPI                    = &USB_MSD_StorageByName;    // s. Note (1)
  InstData.DriverData.pStart       = (void *)"";
  InstData.DriverData.pSectorBuffer   = _aSectorBuffer;
  InstData.DriverData.NumBytes4Buffer = sizeof(_aSectorBuffer);
  InstData.pLunInfo = &_Lun0Info;
  USBD_MSD_AddUnit(&InstData);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/




/*********************************************************************
*
*       MainTask
*
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask_usb(void);
#ifdef __cplusplus
}
#endif
void MainTask_usb(void) {
   st_key_data dts;
/*
   Init_Usb_Detect();
   HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
   USB_OS_Delay(50);
   OS_EVENT_Reset(&USB_Event);
   OS_EVENT_GetBlocked(&USB_Start);
   RTT_vprintf_cr_time(  "Usb : waiting");

   while(1){
      OS_EVENT_GetBlocked(&USB_Event);
      HAL_NVIC_DisableIRQ(USB_DETECT_EXTI_IRQn);
      RTT_vprintf_cr_time( "USB : connexion");

      OS_EVENT_Set(&KBD_Event);     // wake-up keyboard

//      HAL_PWREx_EnableVddUSB();
      __HAL_RCC_USB_CLK_ENABLE();
      usb_connected = true;
      USBD_Init();
//      FS_Init();
//      #if FS_SUPPORT_FAT
//      FS_FAT_SupportLFN();
//      #endif

      _FSTest();
      _AddMSD();
      USBD_Start();
      while (Usb_Detect()) {
       //
       // Wait for configuration
       //
         while (((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED)&&(Usb_Detect())) {
            BSP_ToggleLED(0);
            USB_OS_Delay(50);
         }
         BSP_SetLED(0);
         USBD_MSD_Task();
         USB_OS_Delay(50);
      }
      FS_Sync("nor:0:");
      usb_connected = false;
      RTT_vprintf_cr_time( "USB : disconnect");
      USBD_DeInit();
      dts.sys = 1;
      dts.sys_cmd = SYS_USB_event;
      OS_MAILBOX_Put(&Mb_Keyboard, &dts);  
      HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
      USB_OS_Delay(50);
      OS_EVENT_Reset(&USB_Event);
      HAL_PWREx_DisableVddUSB();

   }

   */
}

void EXTI0_IRQHandler(void){
   HAL_GPIO_EXTI_IRQHandler(USB_DETECT_Pin);
   OS_INT_Enter();
   OS_EVENT_Set(&USB_Event);
   OS_EVENT_Set(&KBD_Event);     // wake-up keyboard
   OS_INT_Leave();
}



/**************************** end of file ***************************/
