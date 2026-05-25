#ifndef WA_u585_SPI2DMA_H
#define WA_u585_SPI2DMA_H

#include "stm32u5xx.h"
#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>

#include "DBx_typedef.h"


LCD_Status_t SHARP_SPI_Init(LCD_Handle_t *p_hlcd);
LCD_Status_t SHARP_SPI_DeInit(LCD_Handle_t *p_hlcd);

#define HSPILCD            hspi2


#endif // WA_u585_SPI2DMA_H
