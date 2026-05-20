#ifndef WA_u585_USB_H
#define WA_u585_USB_H


#include "stdbool.h"
#include "stdint.h"
#include "stm32u5xx.h"  // Device specific header file, contains CMSIS

#include "RTOS.h"
#include "USB.h"
#include "USB_MSD.h"

// not available, use usb pins 
// In Full-Speed mode, D+ (PA12) is pulled high through a 1.5kΩ resistor when device is connected.
#define USB_DETECT_Pin           GPIO_PIN_1
#define USB_DETECT_GPIO_Port     GPIOA
#define USB_DETECT_EXTI_IRQn     EXTI1_IRQn   
#define USB_DETECT_EXTI_LINE       EXTI_LINE_1


extern   OS_EVENT     EV_USB_Vbus, USB_Event;
extern bool usb_connected;


 void _FSTest(bool initFS);
 void _AddMSD(void);

void Init_Usb_Detect(void);

bool Usb_Detect(void);


#endif   // WA_u585_USB_H


