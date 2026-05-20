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
Purpose : BSP for ST STM32U585_B_U585I_IOT02A_Discovery board
--------  END-OF-HEADER  ---------------------------------------------
*/

#include "BSP_USB.h"

#include "DBxxxx.h"
#include "WA_u585_USB.h"


bool usb_connected;
OS_EVENT    OS_RAM   EV_USB_Vbus, USB_Event;


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
static OS_RAM U32  _aSectorBuffer[BUFFER_SIZE / 4];     // Used as sector buffer in order to do read/write sector bursts (~8 sectors at once)





EXTI_HandleTypeDef OS_RAM h_usb_exti_12;

extern uint8_t  wakeup_from;

void USB_EXTI_Callback(void)
{
   OS_INT_Enter();
   RTT_vprintf_cr_time( "EXTi PA1 : USB detected");

   OS_EVENT_Set(&KBD_Event);      // simu stop mode 2

   OS_EVENT_Set(&EV_USB_Vbus);   // use OS_TASKEVENT_Set ???????

   OS_TASKEVENT_Set( &TKBD, EV_KPo_USB);

   usb_connected = true;
   OS_INT_Leave();
//   wakeup_from = 3;
}

bool Usb_Detect(void)
// return true if is usb connected
{
   usb_connected = HAL_GPIO_ReadPin(USB_DETECT_GPIO_Port, USB_DETECT_Pin) == GPIO_PIN_SET ? true : false;
   return  usb_connected;
}


void Init_Usb_Detect(void)
// init usb detect pin
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

   __HAL_RCC_GPIOA_CLK_ENABLE();

// init PA1, EXTI, rising edge, pull-down
   GPIO_InitStruct.Pin = USB_DETECT_Pin;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
   HAL_GPIO_Init(USB_DETECT_GPIO_Port, &GPIO_InitStruct);

   (void)HAL_EXTI_GetHandle(&h_usb_exti_12, USB_DETECT_EXTI_LINE);
   (void)HAL_EXTI_RegisterCallback(&h_usb_exti_12,  HAL_EXTI_FALLING_CB_ID, USB_EXTI_Callback);
   HAL_NVIC_ClearPendingIRQ(USB_DETECT_EXTI_IRQn);
   HAL_NVIC_SetPriority(USB_DETECT_EXTI_IRQn, 12, 0);
   HAL_NVIC_DisableIRQ(USB_DETECT_EXTI_IRQn);

   if (HAL_GPIO_ReadPin(USB_DETECT_GPIO_Port, USB_DETECT_Pin) == GPIO_PIN_SET)
   {
      // USB connected
      usb_connected = true;
   }
   else
   {
      // USB disconnected
      usb_connected = false;
   }


}



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

  RTT_vprintf_cr_time( "Init FS, checking disk : start");

  Len        = strlen(sInfo);
  NumVolumes = FS_GetNumVolumes();
  for (i = 0; i < NumVolumes; i++) {
    FS_GetVolumeName(i, &acVolumeName[0], sizeof(acVolumeName));



   int res = FS_IsLLFormatted(acVolumeName);
   RTT_vprintf_cr_time( "Init FS, low level : %s", (0 == res) ?"error":"ok");
    if ((res == 0)|| initFS) {
      RTT_vprintf_cr_time("Low level formatting");
      FS_FormatLow(acVolumeName);          /* Erase & Low-level  format the volume */
    }
    res = FS_IsHLFormatted(acVolumeName);
    RTT_vprintf_cr_time( "Init FS, Fat File system : %s", (0 == res) ?"error":"ok");
    if ((res == 0) ||   initFS)
   {
      RTT_vprintf_cr_time("Fat format");
      FS_FormatLow(acVolumeName);          /* Erase & Low-level  format the volume */
      FS_Format(acVolumeName, NULL);       /* High-level format the volume */
    }
   strcat(acVolumeName, "\\Readme.txt");
   pFile = FS_FOpen(acVolumeName, "w");
   RTT_vprintf_cr_time( "Init FS, open readme.txt : %s", (NULL == pFile) ?"error":"ok");
   uint32_t w_len = FS_Write(pFile, sInfo, Len);
   RTT_vprintf_cr_time( "Init FS, writing readme.txt : %s", (w_len != Len) ?"error":"ok");
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




/****** End Of File *************************************************/
