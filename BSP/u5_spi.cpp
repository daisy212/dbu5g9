
#include "u5_spi.h"

#include "DBxxxx.h"


#include "SEGGER_RTT.h"


/* LCD Display SHARP LH032 // LH027 */



//#define DEBUG_LCD 1
#define MIRROR 1


/* Static memory allocation */
//static uint8_t LCD_RAM lcd_framebuffer[LCD_TOTAL_BYTES] = {0};
//static uint8_t LCD_RAM lcd_dma_buffer[LCD_MAX_DMA_SIZE] = {0};

extern U8 LCD_RAM _db48xVRAM[LCD_TOTAL_BYTES]   __attribute__ ((aligned (16)));
extern U8 LCD_RAM _aVRAM[LCDb_TOTAL_BYTES]       __attribute__ ((aligned (16)));
extern LCD_Handle_t          hlcd;


SPI_HandleTypeDef   OS_RAM hspi1;
DMA_HandleTypeDef OS_RAM hdma_spi1_tx;


/* Private function prototypes */
static LCD_Status_t LCD_SendCommand(LCD_Handle_t *hlcd, uint8_t cmd);
static uint16_t LCD_BuildDMABuffer_27(LCD_Handle_t *hlcd, uint8_t *buffer);
static uint16_t LCD_BuildDMABuffer_32_direct(LCD_Handle_t *hlcd, uint8_t *buffer);
static uint16_t LCD_BuildDMABuffer_32_rot(LCD_Handle_t *hlcd, uint8_t *buffer);


static void LCD_UpdateModifiedRange(LCD_Handle_t *hlcd, uint16_t line);
LCD_Status_t SHARP_SPI_Init(LCD_Handle_t *p_hlcd);



/**
 * @brief Initialize the LCD driver
 * @param hlcd: LCD handle
 * @param hspi: SPI handle (must be pre-configured)
 * @param use_dma: Enable DMA transfers
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_Sharp_Init(LCD_Handle_t *p_hlcd, bool use_dma)
{
   if (!p_hlcd) {
      return LCD_ERROR;
   }
   p_hlcd->spi_off = true;

    /* Initialize handle */
    memset(p_hlcd, 0, sizeof(LCD_Handle_t));
    p_hlcd->p_lcd_buf = &_db48xVRAM[0];
    p_hlcd->p_dma_buf = &_aVRAM[0];
    p_hlcd->config.use_dma = use_dma;
    p_hlcd->config.timeout_ms = 200;
    p_hlcd->vcom_state = false;

   LCD_Status_t result =   SHARP_SPI_Init(p_hlcd);
   if ( result != LCD_OK)
      return LCD_ERROR;


    /* Verify SPI configuration */
    if (p_hlcd->config.hspi->Init.DataSize != SPI_DATASIZE_8BIT) {
        return LCD_ERROR;
    }
/* HAL_StatusTypeDef status2;
modif suppression spi avec callback, utilisation weak
status2 = HAL_SPI_RegisterCallback( hlcd->config.hspi, HAL_SPI_TX_COMPLETE_CB_ID, &LCD_SPI_TxCpltCallback);
*/
   /* Clear static buffers */
   memset(_db48xVRAM, 0, LCD_TOTAL_BYTES);
   memset(_aVRAM, 0, LCDb_TOTAL_BYTES);

   p_hlcd->initialized = true; // LCD_Clear() check initialized !

   /* Clear display */
   LCD_Clear(p_hlcd);
   return   p_hlcd->status ;

}


uint32_t LCD_GetFramebuffer_size(void)
{
    return LCD_TOTAL_BYTES;
}






/**
 * @brief Get pointer to DMA buffer
 * @retval uint8_t*: Pointer to DMA buffer
 */
uint8_t* LCD_GetDMABuffer(void)
{
    return _aVRAM;
}



/**
 * @brief Deinitialize the LCD driver
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_DeInit(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
    p_hlcd->status = LCD_NOT_INIT;

        return p_hlcd->status;
    }

    /* Wait for any ongoing transfer */
    SPI_WaitForTransfer(p_hlcd);

    /* Clear display */
//    LCD_Clear(hlcd);

    /* Clear static buffers */
//    memset(_db48xVRAM, 0, LCD_TOTAL_BYTES);
//    memset(_aVRAM, 0, LCD_MAX_DMA_SIZE);

    p_hlcd->initialized = false;
p_hlcd->status = LCD_NOT_INIT;
    return p_hlcd->status;
}


/**
 * @brief Clear the entire display
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_Clear(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
    p_hlcd->status = LCD_NOT_INIT;
        return p_hlcd->status;
    }

     if (true == p_hlcd->spi_off) {
         SHARP_SPI_Init(p_hlcd);
      p_hlcd->spi_off = false;
    }


    /* Wait for any ongoing transfer */
    p_hlcd->status = SPI_WaitForTransfer(p_hlcd);
    if (p_hlcd->status != LCD_OK) {
        return p_hlcd->status;
    }

    /* Toggle VCOM */
    LCD_ToggleVCOM(p_hlcd);

    /* Send clear command */
    uint8_t cmd = LCD_CMD_CLEAR_ALL | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);
    p_hlcd->status = LCD_SendCommand(p_hlcd, cmd);
    if (p_hlcd->status != LCD_OK) {
        return p_hlcd->status;
    }

    /* Clear framebuffer and modified lines */
    memset(_db48xVRAM, 0xff, LCD_TOTAL_BYTES);
   p_hlcd->modified.x_min = LCD_WIDTH-1;
   p_hlcd->modified.y_min = LCD_HEIGHT-1;
   p_hlcd->modified.x_max = 0;
   p_hlcd->modified.y_max = 0;
p_hlcd->status = LCD_OK;
    return p_hlcd->status;
}





/**
 * @brief Clear all modified line flags
 * @param hlcd: LCD handle
 */
void LCD_ClearModifiedLines(LCD_Handle_t *p_hlcd)
{
   if (!p_hlcd || !p_hlcd->initialized) return;
   p_hlcd->modified.x_min = LCD_WIDTH-1;
   p_hlcd->modified.y_min = LCD_HEIGHT-1;
   p_hlcd->modified.x_max = 0;
   p_hlcd->modified.y_max = 0;
}

/**
 * @brief Toggle VCOM state (must be called periodically)
 * @param hlcd: LCD handle
 * it's only a boolean flag, no direct action on display, soft VCOM
 */
void LCD_ToggleVCOM(LCD_Handle_t *p_hlcd)
{
    if (p_hlcd) {
        p_hlcd->vcom_state = !p_hlcd->vcom_state;
    }
}


/**
 * @brief Wait for transfer completion
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t SPI_WaitForTransfer(LCD_Handle_t *p_hlcd)
{
   if (!p_hlcd) return LCD_ERROR;
//if (p_hlcd->config.hspi->State == HAL_SPI_STATE_READY) return LCD_OK;
   if  (p_hlcd->transfer_complete) return LCD_OK;
   return OS_EVENT_GetTimed(&EV_LCD_DMA_END, p_hlcd->config.timeout_ms ) == 0 ? LCD_OK : LCD_TIMEOUT;
}





/**
 * @brief Update only the modified lines in a single DMA transfer
         do nothing, or wait for end of previous refresh
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */

// (int Xmin, int Xmax, int Ymin, int Ymax)

LCD_Status_t SPI_UpdateModifiedLines(LCD_Handle_t *p_hlcd)
{
   char buff[40];
   /* initialisation error */
   if (!p_hlcd || !p_hlcd->initialized)  return LCD_ERROR;

    /* Wait for any ongoing transfer */
    LCD_Status_t status = SPI_WaitForTransfer(p_hlcd);
    if (status != LCD_OK) {
        return status;
    }

    /* Toggle VCOM */
    LCD_ToggleVCOM(p_hlcd);

    /* Build DMA buffer with only modified lines 
      And clear modified lines
   */
    uint16_t dma_size;

#if SHARP_27_400x240
   sprintf(buff, "%d<Y<%d", p_hlcd->modified.y_min, p_hlcd->modified.y_max);
     dma_size = LCD_BuildDMABuffer_27(p_hlcd, _aVRAM);
#endif
#if SHARP_32_536x336
   sprintf(buff, "%d<X<%d", p_hlcd->modified.x_min, p_hlcd->modified.x_max);
   dma_size = LCD_BuildDMABuffer_32_rot(p_hlcd, _aVRAM);
#endif


    if (dma_size == 0) return LCD_OK;

// for h7, clean Dcache
//SCB_CleanDCache_by_Addr((uint32_t*)_aVRAM, sizeof(_aVRAM));


    /* Send data */
    p_hlcd->transfer_complete = false;
    OS_EVENT_Reset(&EV_LCD_DMA_END);

    if (p_hlcd->config.use_dma) 
    {
         /* Single DMA transfer with all modified lines */
         HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
         HAL_StatusTypeDef hal_status = HAL_SPI_Transmit_DMA(p_hlcd->config.hspi, 
                                                           _aVRAM, 
                                                           dma_size);
           if (hal_status != HAL_OK) 
           {
               p_hlcd->transfer_complete = true;
               return LCD_DMA1;
           }
    } else {
        /* Polling transfer */
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
        HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(p_hlcd->config.hspi, 
                                                       _aVRAM, 
                                                       dma_size, 
                                                       p_hlcd->config.timeout_ms);
        p_hlcd->transfer_complete = true;
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

        if (hal_status != HAL_OK) {
            return LCD_DMA2;
        }
    }
   p_hlcd->need_refresh = 0;
   return LCD_OK;
}


bool LCD_Need_Refresh(LCD_Handle_t *p_hlcd)
{
   return (p_hlcd->need_refresh > 0 ? true: false);
}


/**
 * @brief Set a pixel in the framebuffer
 * @param hlcd: LCD handle
 * @param x: X coordinate (0 to LCD_WIDTH-1)
 * @param y: Y coordinate (0 to LCD_HEIGHT-1)
 * @param pixel: Pixel state (true = 1, 0 = white, -1 : invert)
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_SetPixel(LCD_Handle_t *p_hlcd, uint16_t x, uint16_t y, int8_t pixel)
{
    if (!p_hlcd || !p_hlcd->initialized || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return LCD_ERROR;
    }

#if  MIRROR 
    x = LCD_WIDTH -x -1;
#endif

    uint32_t byte_index = (y * LCD_BYTES_PER_LINE) + (x / 8);
    uint8_t bit_index = x % 8;
    uint8_t old_value = _db48xVRAM[byte_index];

    if (pixel==0) {
        _db48xVRAM[byte_index] |= (1 << bit_index);
    } 
    else    if (pixel==-1) {
        _db48xVRAM[byte_index] ^= (1 << bit_index);
    } 
    else if (pixel== 1){
        _db48xVRAM[byte_index] &= ~(1 << bit_index);
    }
    LCD_MarkLineModified( p_hlcd, x, y, x, y);

    return LCD_OK;
}


/**
 * @brief Get a pixel from the framebuffer
 * @param hlcd: LCD handle
 * @param x: X coordinate (0 to LCD_WIDTH-1)
 * @param y: Y coordinate (0 to LCD_HEIGHT-1)
 * @retval bool: Pixel state (true = black, false = white)
 */
bool LCD_GetPixel(LCD_Handle_t *p_hlcd, uint16_t x, uint16_t y)
{
    if (!p_hlcd || !p_hlcd->initialized || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return false;
    }

    uint32_t byte_index = (y * LCD_BYTES_PER_LINE) + (x / 8);
    uint8_t bit_index = x % 8;

    return (_db48xVRAM[byte_index] & (1 << bit_index)) == 0;
}



/**
 * @brief Send a command to the LCD
 * @param hlcd: LCD handle
 * @param cmd: Command byte
 * @retval LCD_Status_t
 */
static LCD_Status_t SPI_SendCommand(LCD_Handle_t *p_hlcd, uint8_t cmd)
{
   _aVRAM[0] = cmd;
   _aVRAM[1] = 0x00; // Dummy byte

   p_hlcd->transfer_complete = false;

   HAL_StatusTypeDef hal_status;
   if (p_hlcd->config.use_dma) {

      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
      hal_status = HAL_SPI_Transmit_DMA(p_hlcd->config.hspi, _aVRAM, 2);
      if (hal_status != HAL_OK) {
         p_hlcd->transfer_complete = true;
         return LCD_ERROR;
      }
   } 
   else {
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
      hal_status = HAL_SPI_Transmit(p_hlcd->config.hspi, _aVRAM, 2, p_hlcd->config.timeout_ms);
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
      p_hlcd->transfer_complete = true;
      if (hal_status != HAL_OK) {
         return LCD_ERROR;
      }
   }
   return LCD_OK;
}


/**
 * @brief Build DMA buffer with only modified lines
 * @param hlcd: LCD handle
 * @param buffer: DMA buffer to fill
 * clear modified flags
 * @retval uint16_t: Size of data in buffer (0 on error)
 */

static uint16_t LCD_BuildDMABuffer_27(LCD_Handle_t *p_hlcd, uint8_t *buffer)
{
   if (!p_hlcd || !buffer || p_hlcd->modified.y_min > p_hlcd->modified.y_max) {
     return 0;
   }

   uint16_t buffer_index = 0;
   uint8_t base_cmd = LCD_CMD_WRITE_LINE | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);


   /* Add only modified lines to buffer */
   for (uint16_t line = 0; line < LCD_HEIGHT; line++) {
      if ((line >= p_hlcd->modified.y_min) &&
         (line <= p_hlcd->modified.y_max))
      {
         /* Command byte */
         buffer[buffer_index++] = base_cmd;
         /* Line number (1-based) */
         buffer[buffer_index++] = (uint8_t)(line + 1);
#if MIRROR 
         uint32_t line_offset = (line+1) * LCD_BYTES_PER_LINE;
         for (int32_t ii = 0; ii<   LCD_BYTES_PER_LINE; ii++){

            uint8_t data = _db48xVRAM[--line_offset];
            data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
            data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
            data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
            buffer[buffer_index++] = data;
         }
#else
         /* Copy line data from framebuffer */
         uint32_t line_offset = line * LCD_BYTES_PER_LINE;
         memcpy(&buffer[buffer_index], &_db48xVRAM[line_offset], LCD_BYTES_PER_LINE);
         buffer_index += LCD_BYTES_PER_LINE;
#endif
      }
   }
   /* Add final dummy byte */
   buffer[buffer_index++] = 0x00;
// here,not at the end of dma transfert, otherwise, miss some modifications
   LCD_ClearModifiedLines(p_hlcd);
   return buffer_index;
}

#define LCD_COL_TOTAL_SIZE    (LCD_CMD_HEADER_SIZE + LCD_BYTES_PER_LINE)

static uint16_t LCD_BuildDMABuffer_32_direct(LCD_Handle_t *p_hlcd, uint8_t *buffer)
{
   char mat_8[8];

   uint16_t buffer_index = 0;
   uint8_t base_cmd = LCD_CMD_WRITE_LINE | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);

   /* Add only modified lines to buffer */
   // all for testing
   uint16_t col;
   for ( col = 0; col <LCD_WIDTH ; col= col +1) {      // Vertical refresh 536
         /* Command byte *8 */
         uint16_t col_rev =0;
         buffer_index = col * LCD_COL_TOTAL_SIZE ;
         col_rev = LCD_WIDTH - col ;
         buffer[buffer_index] = base_cmd | (( (col_rev  )&0x3)<<6 );
         buffer[buffer_index + 1] = (uint8_t)((col_rev )>>2);

         memcpy(&buffer[buffer_index+2], &_db48xVRAM[LCD_HEIGHT/8 * col], LCD_HEIGHT/8);



   } // inc x +8
   
   /* Add final dummy byte */
   buffer_index = col * LCD_COL_TOTAL_SIZE ;
//   buffer_index = LCD_COL_TOTAL_SIZE * LCD_WIDTH;
   buffer[buffer_index++] = 0x00;
   buffer[buffer_index++] = 0x00;

// BUG here,not at the end of dma transfert, otherwise, miss some modifications
   LCD_ClearModifiedLines(p_hlcd);
   return LCD_COL_TOTAL_SIZE * LCD_WIDTH + 2;
}


static uint16_t LCD_BuildDMABuffer_32_rot(LCD_Handle_t *p_hlcd, uint8_t *buffer)
/* Add only modified lines to dma buffer */
{
   if (p_hlcd->modified.x_min >= p_hlcd->modified.x_max) return 0;
   char mat_8[8];
   uint16_t col_rev =0, col_rev_min =9999;
   uint16_t buffer_index = 0, buffer_index_mem=0;
   uint8_t base_cmd = LCD_CMD_WRITE_LINE | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);
   uint16_t col_max = LCD_WIDTH -1 -p_hlcd->modified.x_min;
   uint16_t col_min = LCD_WIDTH -1 -p_hlcd->modified.x_max;
   col_min &= 0xfff8; 
   col_max &= 0xfff8; 
   col_max +=  7; 


//   for ( col = 0; col <LCD_WIDTH ; col= col +8) {      // Vertical refresh 536
   for ( uint16_t col = col_min; col < col_max; col= col +8) {      // Vertical refresh 536
         /* Command byte *8 */
         buffer_index = buffer_index_mem;
         for ( uint16_t b_col = 0; b_col < 8; b_col++)
         {
            col_rev = LCD_WIDTH - col - b_col -1;
            col_rev_min = col_rev_min> col_rev ? col_rev :col_rev_min;

            buffer[buffer_index + b_col * LCD_COL_TOTAL_SIZE] = base_cmd | (( (col_rev  )&0x3)<<6 );
            buffer[buffer_index + 1 + b_col * LCD_COL_TOTAL_SIZE] = (uint8_t)((col_rev )>>2);
         }
         for ( int16_t row = LCD_HEIGHT-8; row >=0 ; row= row - 8)
         {
            for ( uint16_t b_col = 0; b_col < 8; b_col++)
            {
               mat_8[b_col] = _db48xVRAM[( row +b_col ) * LCD_BYTES_PER_LINE  + col/8];
            }

            for ( uint16_t b_col = 0; b_col < 8; b_col++)
            {
               buffer[buffer_index +2 + b_col * LCD_COL_TOTAL_SIZE] = 
                     ((mat_8[7]>> b_col) & 0x1) |
                     ((mat_8[6]>> b_col) & 0x1)<<1 |
                     ((mat_8[5]>> b_col) & 0x1)<<2 |
                     ((mat_8[4]>> b_col) & 0x1)<<3 |
                     ((mat_8[3]>> b_col) & 0x1)<<4 |
                     ((mat_8[2]>> b_col) & 0x1)<<5 |
                     ((mat_8[1]>> b_col) & 0x1)<<6 |
                     ((mat_8[0]>> b_col) & 0x1)<<7;

            }
            buffer_index += 1;
         
         } // inc y
      buffer_index_mem += LCD_COL_TOTAL_SIZE*8 ;
   } // inc x +8
   
   /* Add final dummy byte */
//   buffer_index = LCD_COL_TOTAL_SIZE * LCD_WIDTH;
//   buffer_index = (col-p_hlcd->modified.x_min) * LCD_COL_TOTAL_SIZE ;

   buffer[buffer_index_mem++] = 0x00;
   buffer[buffer_index_mem++] = 0x00;

#if DEBUG
   RTT_vprintf_cr_T_F(LCD_BuildDMABuffer_32_rot, "Dma X:%d-%d, Y:%d-%d, size %d, col min : %d", 
         p_hlcd->modified.x_min, p_hlcd->modified.x_max, 
         p_hlcd->modified.y_min, p_hlcd->modified.y_max,
         buffer_index_mem,
         col_rev_min);
#endif

// BUG here,not at the end of dma transfert, otherwise, miss some modifications
   LCD_ClearModifiedLines(p_hlcd);
//   return LCD_COL_TOTAL_SIZE * LCD_WIDTH + 2;
   if (2 == buffer_index_mem) buffer_index_mem = 0;
   return buffer_index_mem;
}








