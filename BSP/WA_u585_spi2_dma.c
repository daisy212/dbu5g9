#include "WA_u585_spi2_dma.h"
#include "DBxxxx.h"

/* hardware for lcd spi 
 * SHARP LH032 // LH027 
 * init, de-init
 * interrupts
 * 
 */

extern OS_EVENT   EV_LCD_DMA_END;

SPI_HandleTypeDef   OS_RAM hspi2;
DMA_HandleTypeDef OS_RAM handle_GPDMA1_Channel0;

static LCD_Handle_t OS_RAM *gp_hlcd = &hlcd; // Global handle for DMA callback


/**
 * @brief SPI transfer complete callback
 * @param spi: SPI handle
 */
void LCD_SPI_TxCpltCallback(SPI_HandleTypeDef *spi)
{
#ifdef  DEBUG_LCD
   HAL_SPI_StateTypeDef status = HAL_SPI_GetState(spi);
   SEGGER_RTT_printf(0, "\nSPI2 Call back Status ( 1 ready) %02x ", (int)status );
#endif
   OS_INT_Enter();
   OS_TASKEVENT_Set( &TKBD, EV_KPo_DMA);
   OS_EVENT_Set(&EV_LCD_DMA_END);            // used for function LCD_WaitForTransfer()

   OS_INT_Leave();
   if (gp_hlcd) {
        gp_hlcd->transfer_complete = true;
    }
}


/**
  * @brief This function handles GPDMA1 Channel 0 global interrupt.
  */
void GPDMA1_Channel0_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&handle_GPDMA1_Channel0);
}


/**
  * @brief This function handles SPI2 global interrupt.
  */
void SPI2_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi2);
}


LCD_Status_t SHARP_SPI_Init(LCD_Handle_t *p_hlcd)
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
   p_hlcd->status = LCD_ERROR;
   p_hlcd->spi_off = true;

  /** Initializes the peripherals clock
  */
   PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
   PeriphClkInit.Spi2ClockSelection = RCC_SPI2CLKSOURCE_MSIK;
   if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
   {
      return p_hlcd->status;
   }
   /* Peripheral clock enable */
   __HAL_RCC_SPI2_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPDMA1_CLK_ENABLE();

   /**SPI2 GPIO Configuration
   PB10     ------> SPI2_SCK
   PB12     ------> SPI2_NSS
   PB15     ------> SPI2_MOSI
   */
   GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_15;
   GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
   GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   GPIO_InitStruct.Pin = GPIO_PIN_12;
   GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
   GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

   /* SPI2 DMA Init */
   /* GPDMA1_REQUEST_SPI2_TX Init */
   handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
   handle_GPDMA1_Channel0.Init.Request = GPDMA1_REQUEST_SPI2_TX;
   handle_GPDMA1_Channel0.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
   handle_GPDMA1_Channel0.Init.Direction = DMA_PERIPH_TO_MEMORY;
   handle_GPDMA1_Channel0.Init.SrcInc = DMA_SINC_INCREMENTED;
   handle_GPDMA1_Channel0.Init.DestInc = DMA_DINC_FIXED;
   handle_GPDMA1_Channel0.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
   handle_GPDMA1_Channel0.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
   handle_GPDMA1_Channel0.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
   handle_GPDMA1_Channel0.Init.SrcBurstLength = 1;
   handle_GPDMA1_Channel0.Init.DestBurstLength = 1;
   handle_GPDMA1_Channel0.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
   handle_GPDMA1_Channel0.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
   handle_GPDMA1_Channel0.Init.Mode = DMA_NORMAL;
   if (HAL_DMA_Init(&handle_GPDMA1_Channel0) != HAL_OK)
   {
      p_hlcd->status = LCD_ERROR;
      return LCD_ERROR;
   }
   __HAL_LINKDMA(&hspi2, hdmatx, handle_GPDMA1_Channel0);

   if (HAL_DMA_ConfigChannelAttributes(&handle_GPDMA1_Channel0, DMA_CHANNEL_NPRIV) != HAL_OK)
   {
      p_hlcd->status = LCD_ERROR;
      return LCD_ERROR;
   }

   /* SPI2 interrupt Init */
   HAL_NVIC_SetPriority(SPI2_IRQn, 12, 8);
   HAL_NVIC_EnableIRQ(SPI2_IRQn);

  /* GPDMA1 interrupt Init  needed */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 12, 10);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

   SPI_AutonomousModeConfTypeDef HAL_SPI_AutonomousMode_Cfg_Struct = {0};
   /* SPI2 parameter configuration*/
   hspi2.Instance = SPI2;
   hspi2.Init.Mode = SPI_MODE_MASTER;
   hspi2.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
   hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
   hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
   hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
   hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
   // msik = 16mhz, /16 ==> 1mhz
   // msik = 4Mhz, /2 = 2Mhz
   // msik = 12Mhz, /4 = 3Mhz, 30fps 2.7'
   // msik = 12Mhz, /2 = 6Mhz, 60fp, 2.7'
   // msik = 24Mhz, /4 = 6Mhz, 33fps 3.2', attention au 5v
   // msik = 24Mhz, /2 = 12Mhz, errors
   // 
   hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
   hspi2.Init.FirstBit = SPI_FIRSTBIT_LSB;
   hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
   hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
   hspi2.Init.CRCPolynomial = 0x7;
   hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
   hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
   hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_05DATA;
   // tsSPS datasheet = 3µs, wait sck after CS
   hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_12CYCLE;
   // dans un transfert multi-octets, cs redevient inactif entre les octets
   hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
   hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
   hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
   hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
   hspi2.Init.ReadyMasterManagement = SPI_RDY_MASTER_MANAGEMENT_INTERNALLY;
   hspi2.Init.ReadyPolarity = SPI_RDY_POLARITY_HIGH;
   if (HAL_SPI_Init(&hspi2) != HAL_OK)
   {
      p_hlcd->status = LCD_ERROR;
      return LCD_ERROR;
   }
   HAL_SPI_AutonomousMode_Cfg_Struct.TriggerState = SPI_AUTO_MODE_DISABLE;
   HAL_SPI_AutonomousMode_Cfg_Struct.TriggerSelection = SPI_GRP1_GPDMA_CH0_TCF_TRG;
   HAL_SPI_AutonomousMode_Cfg_Struct.TriggerPolarity = SPI_TRIG_POLARITY_RISING;
   if (HAL_SPIEx_SetConfigAutonomousMode(&hspi2, &HAL_SPI_AutonomousMode_Cfg_Struct) != HAL_OK)
   {
      p_hlcd->status = LCD_ERROR;
      return LCD_ERROR;
   }
   HAL_StatusTypeDef status2 = HAL_SPI_RegisterCallback(&hspi2, HAL_SPI_TX_COMPLETE_CB_ID, &LCD_SPI_TxCpltCallback);
   p_hlcd->status = LCD_OK;
   p_hlcd->spi_off = false;
   p_hlcd->transfer_complete = true;
   p_hlcd->config.hspi = &hspi2;
   return p_hlcd->status;
}


LCD_Status_t SHARP_SPI_DeInit(LCD_Handle_t *p_hlcd) 
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   if   ( true == p_hlcd->spi_off)
      return LCD_OK;
   SPI_WaitForTransfer(p_hlcd);

   /* Peripheral clock disable */
   __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PB10     ------> SPI2_SCK
    PB12     ------> SPI2_NSS
    PB15     ------> SPI2_MOSI
    */
//    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_12|GPIO_PIN_15);
    // test, gain 10µA
    __HAL_RCC_GPIOB_CLK_ENABLE();
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);         
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_RESET);
   GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_15;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

   /* SPI2 DMA DeInit */
   HAL_DMA_DeInit(&handle_GPDMA1_Channel0);

   /* SPI2 interrupt DeInit */
   HAL_NVIC_DisableIRQ(SPI2_IRQn);

   p_hlcd->spi_off = true;
   return LCD_OK;
}


/**
 * @brief Send a command to the LCD
 * @param hlcd: LCD handle
 * @param cmd: Command byte
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_SendCommand(LCD_Handle_t *p_hlcd, uint8_t cmd)
{
U8 lcd_dma_buffer[16];
   lcd_dma_buffer[0] = cmd;
   lcd_dma_buffer[1] = 0x00; // Dummy byte

   p_hlcd->transfer_complete = false;

   HAL_StatusTypeDef hal_status;
   if (p_hlcd->config.use_dma) {

      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
      hal_status = HAL_SPI_Transmit_DMA(p_hlcd->config.hspi, lcd_dma_buffer, 2);
      if (hal_status != HAL_OK) {
         p_hlcd->transfer_complete = true;
         return LCD_ERROR;
      }
   } 
   else {
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
      hal_status = HAL_SPI_Transmit(p_hlcd->config.hspi, lcd_dma_buffer, 2, p_hlcd->config.timeout_ms);
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
      p_hlcd->transfer_complete = true;
      if (hal_status != HAL_OK) {
         return LCD_ERROR;
      }
   }
   return LCD_OK;
}

