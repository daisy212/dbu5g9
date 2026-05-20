#ifndef U5G_LTDC_H
#define U5G_LTDC_H

#include "RTOS.h"


#include "stdbool.h"
#include "stdint.h"


#include "global.h"

//#include "Display.h"
#include "ltdc_TFT035_7.h"
#include "stm32u5xx.h"
#include "DBx_typedef.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


extern const uint16_t BL_levels[];

void LTDC_IRQHandler(void);
void GPU2D_ER_IRQHandler(void);
void GPU2D_IRQHandler(void);
void DMA2D_IRQHandler(void);


void TIM3_PWM_SetDuty(uint16_t duty_tenth);
void CopyRegionToVRAM(int Xmin, int Xmax, int Ymin, int Ymax);

void LTDC_SetPixel( uint16_t x, uint16_t y, int32_t pixel);
void LTDC_WaitForTransfer(LCD_Handle_t *hlcd);
uint8_t LTDC_Init(LCD_Handle_t *p_hlcd);

uint8_t* LTDC_GetFramebuffer(void);


#ifdef __cplusplus
}
#endif // __cplusplus



#endif  //U5G_LTDC_H
