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

File    : FS_DirOperations.c
Purpose : Demonstrates the usage of API functions that operate
          on directories.

Additional information:
  Preparations:
    Works out-of-the-box with any storage device.

  Expected behavior:
    This sample creates three directories. In each directory three
    files are created. After creating the directories and files,
    the contents of each directory is shown.

  Sample output:
    Start
    High-level format...OK
    Create directory \Dir00
    Create files ...OK
    Create directory \Dir01
    Create files ...OK
    Create directory \Dir02
    Create files ...OK
    Contents of
    DIR00 (Dir) Attributes: ---- Size: 0
    Contents of \DIR00
      . (Dir) Attributes: ---- Size: 0
      .. (Dir) Attributes: ---- Size: 0
      FILE0000.TXT       Attributes: A--- Size: 19
      FILE0001.TXT       Attributes: A--- Size: 19
      FILE0002.TXT       Attributes: A--- Size: 19

    DIR01 (Dir) Attributes: ---- Size: 0
    Contents of \DIR01
      . (Dir) Attributes: ---- Size: 0
      .. (Dir) Attributes: ---- Size: 0
      FILE0000.TXT       Attributes: A--- Size: 19
      FILE0001.TXT       Attributes: A--- Size: 19
      FILE0002.TXT       Attributes: A--- Size: 19

    DIR02 (Dir) Attributes: ---- Size: 0
    Contents of \DIR02
      . (Dir) Attributes: ---- Size: 0
      .. (Dir) Attributes: ---- Size: 0
      FILE0000.TXT       Attributes: A--- Size: 19
      FILE0001.TXT       Attributes: A--- Size: 19
      FILE0002.TXT       Attributes: A--- Size: 19


    Finished
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/
#include "SEGGER.h"
#include "SEGGER_RTT.h" // using SEGGER_RTT_printf
#include "RTOS.h"
#include "FS.h"

#include <stdio.h>
#include <string.h>


#include "stm32u5xx_hal.h"


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define VOLUME_NAME       ""
#define MAX_RECURSION     5
#define NUM_DIRS          3
#define NUM_FILES         3

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static char _ac[512];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _CreateFiles
*/
static void _CreateFiles(const char * sPath) {
  int       i;
  FS_FILE * pFile;
  char      acFileName[40];

  FS_X_Log("Create files...");
  for (i = 0; i < NUM_FILES; i++) {
    snprintf(acFileName, (int)sizeof(acFileName), "%s\\file%.4d.txt", sPath, i);
    pFile = FS_FOpen(acFileName, "w");
    if (pFile != NULL) {
      (void)FS_Write(pFile, acFileName, strlen(acFileName));
      (void)FS_FClose(pFile);
      FS_X_Log(".");
    } else {
      FS_X_Log("ERROR\n");
      return;
    }
  }
  FS_X_Log("OK\n");
}

/*********************************************************************
*
*       _ShowDir
*
*/
static void _ShowDir(const char * sDirName, int MaxRecursion) {
  FS_FIND_DATA fd;
  char         acFileName[20];
  char         acDummy[20];
  int          NumBytes;
  int          r;

  NumBytes = MAX_RECURSION - MaxRecursion;
  memset(acDummy, (int)' ', (unsigned)NumBytes);
  acDummy[NumBytes] = '\0';
  snprintf(_ac, (int)sizeof(_ac), "%sContents of %s \n", acDummy, sDirName);
  FS_X_Log(_ac);
  if (MaxRecursion != 0) {
    r = FS_FindFirstFile(&fd, sDirName, acFileName, (int)sizeof(acFileName));
    if (r == 0) {
      do {
        U8 Attr;

        Attr = fd.Attributes;
        snprintf(_ac, (int)sizeof(_ac), "%s %s %s Attributes: %s%s%s%s Size: %lu\n",
                                               acDummy, fd.sFileName,
                                               ((Attr & FS_ATTR_DIRECTORY) != 0u) ? "(Dir)" : "     ",
                                               ((Attr & FS_ATTR_ARCHIVE)   != 0u) ? "A" : "-",
                                               ((Attr & FS_ATTR_READ_ONLY) != 0u) ? "R" : "-",
                                               ((Attr & FS_ATTR_HIDDEN)    != 0u) ? "H" : "-",
                                               ((Attr & FS_ATTR_SYSTEM)    != 0u) ? "S" : "-",
                                               fd.FileSize);
        FS_X_Log(_ac);
        if ((Attr & FS_ATTR_DIRECTORY) != 0u) {
          char acDirName[256];
          //
          // Show contents of each directory in the root
          //
          if (*fd.sFileName != '.') {
            snprintf(acDirName, (int)sizeof(acDirName), "%s%c%s", sDirName, FS_DIRECTORY_DELIMITER, fd.sFileName);
            _ShowDir(acDirName, MaxRecursion - 1);
          }
        }

      } while (FS_FindNextFile(&fd) != 0);
      FS_FindClose(&fd);
    } else if (r == 1) {
      FS_X_Log("Empty directory");
    } else {
      snprintf(_ac, (int)sizeof(_ac), "ERROR: Unable to open directory %s\n", sDirName);
      FS_X_Log(_ac);
    }
    FS_X_Log("\n");
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
#define W25Qxx_CMD_JedecID                 0x9F // JEDEC ID  
#define W25Qxx_FLASH_ID                   0xEF4017        // W25Q64JV JEDEC ID
#define OSPI_W25Qxx_OK                       0     // W25QxxÍ¨ÐÅÕý³£
#define W25Qxx_ERROR_INIT                   -1     // ³õÊ¼»¯´íÎó
#define W25Qxx_ERROR_WriteEnable            -2     // Ð´Ê¹ÄÜ´íÎó
#define W25Qxx_ERROR_AUTOPOLLING            -3     // ÂÖÑ¯µÈ´ý´íÎó£¬ÎÞÏìÓ¦
#define W25Qxx_ERROR_Erase                  -4     // ²Á³ý´íÎó
#define W25Qxx_ERROR_TRANSMIT               -5     // ´«Êä´íÎó
#define W25Qxx_ERROR_MemoryMapped          -6    // ÄÚ´æÓ³ÉäÄ£Ê½´íÎó


#define W25Qxx_CMD_FastReadQuad_IO        0xEB     // 1-4-4Ä£Ê½ÏÂ(1ÏßÖ¸Áî4ÏßµØÖ·4ÏßÊý¾Ý)£¬¿ìËÙ¶ÁÈ¡Ö¸Áî

OSPI_HandleTypeDef hospi1;

void HAL_OSPI_MspInit(OSPI_HandleTypeDef* ospiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(ospiHandle->Instance==OCTOSPI1)
  {
  /* USER CODE BEGIN OCTOSPI1_MspInit 0 */

  /* USER CODE END OCTOSPI1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
//      Error_Handler();
    }

    /* OCTOSPI1 clock enable */
    __HAL_RCC_OSPIM_CLK_ENABLE();
    __HAL_RCC_OSPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**OCTOSPI1 GPIO Configuration
    PA2     ------> OCTOSPIM_P1_NCS
    PA3     ------> OCTOSPIM_P1_CLK
    PC0     ------> OCTOSPIM_P1_IO3
    PC3     ------> OCTOSPIM_P1_IO2
    PC2     ------> OCTOSPIM_P1_IO1
    PC1     ------> OCTOSPIM_P1_IO0
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF3_OCTOSPI1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN OCTOSPI1_MspInit 1 */

  /* USER CODE END OCTOSPI1_MspInit 1 */
  }
}


/* OCTOSPI1 init function */
void MX_OCTOSPI1_Init(void)
{

  /* USER CODE BEGIN OCTOSPI1_Init 0 */

  /* USER CODE END OCTOSPI1_Init 0 */

  OSPIM_CfgTypeDef sOspiManagerCfg = {0};

  /* USER CODE BEGIN OCTOSPI1_Init 1 */

  /* USER CODE END OCTOSPI1_Init 1 */
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 8;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
//  HAL_OSPI_MEMTYPE_MACRONIX
// HAL_OSPI_MEMTYPE_MICRON
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize = 23;
  hospi1.Init.ChipSelectHighTime = 1;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_3;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi1.Init.ClockPrescaler = 2;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
//    Error_Handler();
  }
  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_HIGH;
  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
//    Error_Handler();
  }
  /* USER CODE BEGIN OCTOSPI1_Init 2 */

  /* USER CODE END OCTOSPI1_Init 2 */

}


#include "stm32u5xx.h"

OSPI_HandleTypeDef hospi1;  // still needed for command/receive HAL calls

void MX_OCTOSPI1_Init_pasok(void) {
    U32 Timeout;

    //
    // ── CLOCKS ──────────────────────────────────────────────────────────────
    //

    // Select SYSCLK as OSPI clock source (CSSEL bits in CCIPR2)
    MODIFY_REG(RCC->CCIPR2, RCC_CCIPR2_OCTOSPISEL_Msk, 0U);  // 00 = SYSCLK


    // Enable OSPIM, OSPI1, GPIO clocks

    SET_BIT(RCC->AHB2ENR1, RCC_AHB2ENR1_GPIOAEN);
    SET_BIT(RCC->AHB2ENR1, RCC_AHB2ENR1_GPIOBEN);
    SET_BIT(RCC->AHB2ENR1, RCC_AHB2ENR1_GPIOCEN);
   SET_BIT(RCC->AHB2ENR1, RCC_AHB2ENR1_OCTOSPIMEN);

   SET_BIT(RCC->AHB2ENR2, RCC_AHB2ENR2_OCTOSPI1EN);
   SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DCACHE1EN);


    //
    // ── GPIO ────────────────────────────────────────────────────────────────
    //
    // PA2 = OCTOSPIM_P1_NCS  AF10
    // PA3 = OCTOSPIM_P1_CLK  AF10
    // PC0 = OCTOSPIM_P1_IO3  AF3
    // PC1 = OCTOSPIM_P1_IO0  AF10
    // PC2 = OCTOSPIM_P1_IO1  AF10
    // PC3 = OCTOSPIM_P1_IO2  AF10
    //
    // All: AF push-pull, pull-up, very high speed
    //

    // PA2, PA3 — mode AF (10)
    MODIFY_REG(GPIOA->MODER,
               GPIO_MODER_MODE2 | GPIO_MODER_MODE3,
               GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    // Output type push-pull (reset value = 0, nothing to do)
    CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    // Speed very high (11)
    SET_BIT(GPIOA->OSPEEDR,
            GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
    // Pull-up (01)
    MODIFY_REG(GPIOA->PUPDR,
               GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3,
               GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD3_0);
    // AF10 for PA2, PA3 → AFRL bits [11:8] and [15:12]
    MODIFY_REG(GPIOA->AFR[0],
               GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3,
               (10U << GPIO_AFRL_AFSEL2_Pos) | (10U << GPIO_AFRL_AFSEL3_Pos));

    // PC0 — mode AF (3)
    MODIFY_REG(GPIOC->MODER,
               GPIO_MODER_MODE0,
               GPIO_MODER_MODE0_1);
    CLEAR_BIT(GPIOC->OTYPER, GPIO_OTYPER_OT0);
    SET_BIT(GPIOC->OSPEEDR, GPIO_OSPEEDR_OSPEED0);
    MODIFY_REG(GPIOC->PUPDR,
               GPIO_PUPDR_PUPD0,
               GPIO_PUPDR_PUPD0_0);
    // AF3 for PC0 → AFRL bits [3:0]
    MODIFY_REG(GPIOC->AFR[0],
               GPIO_AFRL_AFSEL0,
               (3U << GPIO_AFRL_AFSEL0_Pos));

    // PC1, PC2, PC3 — mode AF (10)
    MODIFY_REG(GPIOC->MODER,
               GPIO_MODER_MODE1 | GPIO_MODER_MODE2 | GPIO_MODER_MODE3,
               GPIO_MODER_MODE1_1 | GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    CLEAR_BIT(GPIOC->OTYPER,
              GPIO_OTYPER_OT1 | GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    SET_BIT(GPIOC->OSPEEDR,
            GPIO_OSPEEDR_OSPEED1 | GPIO_OSPEEDR_OSPEED2 | GPIO_OSPEEDR_OSPEED3);
    MODIFY_REG(GPIOC->PUPDR,
               GPIO_PUPDR_PUPD1 | GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3,
               GPIO_PUPDR_PUPD1_0 | GPIO_PUPDR_PUPD2_0 | GPIO_PUPDR_PUPD3_0);
    // AF10 for PC1, PC2, PC3 → AFRL bits [7:4],[11:8],[15:12]
    MODIFY_REG(GPIOC->AFR[0],
               GPIO_AFRL_AFSEL1 | GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3,
               (10U << GPIO_AFRL_AFSEL1_Pos) |
               (10U << GPIO_AFRL_AFSEL2_Pos) |
               (10U << GPIO_AFRL_AFSEL3_Pos));

    //
    // ── OCTOSPI1 REGISTERS ──────────────────────────────────────────────────
    //

    // DCR1: MemoryType=Macronix(1), DeviceSize=23→22, CSHT=1→0,
    //       DelayBlockBypass=bypassed, ClockMode=3(CKMODE=1), FRCK=0
    MODIFY_REG(OCTOSPI1->DCR1,
               OCTOSPI_DCR1_MTYP   |
               OCTOSPI_DCR1_DEVSIZE |
               OCTOSPI_DCR1_CSHT   |
               OCTOSPI_DCR1_DLYBYP |
               OCTOSPI_DCR1_FRCK   |
               OCTOSPI_DCR1_CKMODE,
               (1U  << OCTOSPI_DCR1_MTYP_Pos)    |  // Macronix
               (22U << OCTOSPI_DCR1_DEVSIZE_Pos)  |  // DeviceSize-1
               (0U  << OCTOSPI_DCR1_CSHT_Pos)     |  // ChipSelectHighTime-1
               OCTOSPI_DCR1_DLYBYP                |  // delay block bypassed
               OCTOSPI_DCR1_CKMODE);                  // clock mode 3

    // DCR2: WrapSize=not supported(0), Prescaler=2→1
    MODIFY_REG(OCTOSPI1->DCR2,
               OCTOSPI_DCR2_WRAPSIZE | OCTOSPI_DCR2_PRESCALER,
               (0U << OCTOSPI_DCR2_WRAPSIZE_Pos)  |  // wrap not supported
               (1U << OCTOSPI_DCR2_PRESCALER_Pos));   // prescaler-1

    // DCR3: ChipSelectBoundary=0, MaxTran=0
    OCTOSPI1->DCR3 = 0U;

    // DCR4: Refresh=0
    OCTOSPI1->DCR4 = 0U;

    // CR: FifoThreshold=8→7, DualQuad=disabled
    MODIFY_REG(OCTOSPI1->CR,
               OCTOSPI_CR_FTHRES | OCTOSPI_CR_DMM,
               (7U << OCTOSPI_CR_FTHRES_Pos));  // FifoThreshold-1, DMM=0

    // Wait until not busy
    Timeout = 1000;
    while (READ_BIT(OCTOSPI1->SR, OCTOSPI_SR_BUSY) != 0U) {
        OS_Delay(1);
        if (--Timeout == 0) {
            SEGGER_RTT_printf(0, "OSPI: busy timeout\n");
            return;
        }
    }

    // TCR: SampleShifting=half cycle, DHQC=disabled
    MODIFY_REG(OCTOSPI1->TCR,
               OCTOSPI_TCR_SSHIFT | OCTOSPI_TCR_DHQC,
               OCTOSPI_TCR_SSHIFT);  // half cycle shift, DHQC=0

    // Enable OCTOSPI1
    SET_BIT(OCTOSPI1->CR, OCTOSPI_CR_EN);

    //
    // ── OCTOSPIM (IO Manager) ───────────────────────────────────────────────
    //
    // ClkPort=1, NCSPort=1, IOLowPort=HAL_OSPIM_IOPORT_1_HIGH(=0x04)
    // instance=0 (OCTOSPI1), no mux
    //

    // Disable OCTOSPI1 during OSPIM config
    CLEAR_BIT(OCTOSPI1->CR, OCTOSPI_CR_EN);

    // PCR[0] = port 1: set CLK enable, source=instance0
    MODIFY_REG(OCTOSPIM->PCR[0],
               OCTOSPIM_PCR_CLKEN | OCTOSPIM_PCR_CLKSRC,
               OCTOSPIM_PCR_CLKEN);  // CLKSRC=0 → OSPI1

    // PCR[0] = port 1: set NCS enable, source=instance0
    MODIFY_REG(OCTOSPIM->PCR[0],
               OCTOSPIM_PCR_NCSEN | OCTOSPIM_PCR_NCSSRC,
               OCTOSPIM_PCR_NCSEN);  // NCSSRC=0 → OSPI1

    // IOLowPort = HAL_OSPIM_IOPORT_1_HIGH = port 1, high nibble (IO4-7)
    // IOHEN enable, IOHSRC=0 → OSPI1
    MODIFY_REG(OCTOSPIM->PCR[0],
               OCTOSPIM_PCR_IOHEN | OCTOSPIM_PCR_IOHSRC,
               OCTOSPIM_PCR_IOHEN);  // IOHSRC=0 → OSPI1

    // Re-enable OCTOSPI1
    SET_BIT(OCTOSPI1->CR, OCTOSPI_CR_EN);

    // Fill handle for HAL command/receive functions
    hospi1.Instance = OCTOSPI1;
    hospi1.State    = HAL_OSPI_STATE_READY;

    SEGGER_RTT_printf(0, "OSPI: init OK\n");
}

uint32_t OSPI_W25Qxx_ReadID(void)   
{
   OSPI_RegularCmdTypeDef  sCommand;   // OSPI´«ÊäÅäÖÃ

   uint8_t  OSPI_ReceiveBuff[3]; // ´æ´¢OSPI¶Áµ½µÄÊý¾Ý
   uint32_t W25Qxx_ID;  // Æ÷¼þµÄID

   sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;         // Í¨ÓÃÅäÖÃ
   sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;                // flash ID
   sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;        // 1ÏßÖ¸ÁîÄ£Ê½
   sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;        // Ö¸Áî³¤¶È8Î»
   sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;   // ½ûÖ¹Ö¸ÁîDTRÄ£Ê½
   sCommand.AddressMode        = HAL_OSPI_ADDRESS_NONE;              // ÎÞµØÖ·Ä£Ê½
   sCommand.AddressSize        = HAL_OSPI_ADDRESS_24_BITS;           // µØÖ·³¤¶È24Î»   
   sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;      // ÎÞ½»Ìæ×Ö½Ú
   sCommand.DataMode           = HAL_OSPI_DATA_1_LINE;               // 1ÏßÊý¾ÝÄ£Ê½
   sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;          // ½ûÖ¹Êý¾ÝDTRÄ£Ê½
   sCommand.NbData             = 3;                                  // ´«ÊäÊý¾ÝµÄ³¤¶È
   sCommand.DummyCycles        = 0;                                  // ¿ÕÖÜÆÚ¸öÊý
   sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;               // ²»Ê¹ÓÃDQS
   sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;       // Ã¿´Î´«ÊäÊý¾Ý¶¼·¢ËÍÖ¸Áî   

   sCommand.Instruction        = W25Qxx_CMD_JedecID;                 // Ö´ÐÐ¶ÁÆ÷¼þIDÃüÁî


   HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE);   // ·¢ËÍÖ¸Áî

   HAL_OSPI_Receive (&hospi1, OSPI_ReceiveBuff, HAL_OSPI_TIMEOUT_DEFAULT_VALUE); // ½ÓÊÕÊý¾Ý

   W25Qxx_ID = (OSPI_ReceiveBuff[0] << 16) | (OSPI_ReceiveBuff[1] << 8 ) | OSPI_ReceiveBuff[2]; // ½«µÃµ½µÄÊý¾Ý×éºÏ³ÉID

   return W25Qxx_ID; // ·µ»ØID
}

int8_t OSPI_W25Qxx_MemoryMappedMode(void)
{
   OSPI_RegularCmdTypeDef     sCommand;      // QSPI´«ÊäÅäÖÃ
   OSPI_MemoryMappedTypeDef   sMemMappedCfg; // ÄÚ´æÓ³Éä·ÃÎÊ²ÎÊý

   sCommand.OperationType           = HAL_OSPI_OPTYPE_COMMON_CFG;             // Í¨ÓÃÅäÖÃ
   sCommand.FlashId                 = HAL_OSPI_FLASH_ID_1;                    // flash ID

   sCommand.Instruction             = W25Qxx_CMD_FastReadQuad_IO;             // 1-4-4Ä£Ê½ÏÂ(1ÏßÖ¸Áî4ÏßµØÖ·4ÏßÊý¾Ý)£¬¿ìËÙ¶ÁÈ¡Ö¸Áî
   sCommand.InstructionMode         = HAL_OSPI_INSTRUCTION_1_LINE;            // 1ÏßÖ¸ÁîÄ£Ê½
   sCommand.InstructionSize         = HAL_OSPI_INSTRUCTION_8_BITS;            // Ö¸Áî³¤¶È8Î»
   sCommand.InstructionDtrMode      = HAL_OSPI_INSTRUCTION_DTR_DISABLE;       // ½ûÖ¹Ö¸ÁîDTRÄ£Ê½

   sCommand.AddressMode             = HAL_OSPI_ADDRESS_4_LINES;               // 4ÏßµØÖ·Ä£Ê½
   sCommand.AddressSize             = HAL_OSPI_ADDRESS_24_BITS;               // µØÖ·³¤¶È24Î»
   sCommand.AddressDtrMode          = HAL_OSPI_ADDRESS_DTR_DISABLE;           // ½ûÖ¹µØÖ·DTRÄ£Ê½

   sCommand.AlternateBytesMode      = HAL_OSPI_ALTERNATE_BYTES_NONE;          // ÎÞ½»Ìæ×Ö½Ú    
   sCommand.AlternateBytesDtrMode   = HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE;   // ½ûÖ¹Ìæ×Ö½ÚDTRÄ£Ê½ 

   sCommand.DataMode                = HAL_OSPI_DATA_4_LINES;                  // 4ÏßÊý¾ÝÄ£Ê½
   sCommand.DataDtrMode             = HAL_OSPI_DATA_DTR_DISABLE;              // ½ûÖ¹Êý¾ÝDTRÄ£Ê½ 

   sCommand.DummyCycles             = 6;                                      // ¿ÕÖÜÆÚ¸öÊý
   sCommand.DQSMode                 = HAL_OSPI_DQS_DISABLE;                   // ²»Ê¹ÓÃDQS 
   sCommand.SIOOMode                = HAL_OSPI_SIOO_INST_EVERY_CMD;           // Ã¿´Î´«ÊäÊý¾Ý¶¼·¢ËÍÖ¸Áî   

   // Ð´ÅäÖÃ
   if (HAL_OSPI_Command(&hospi1, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
   {
      return W25Qxx_ERROR_TRANSMIT; // ´«ÊäÊý¾Ý´íÎó
   }   

   sMemMappedCfg.TimeOutActivation  = HAL_OSPI_TIMEOUT_COUNTER_DISABLE;    // ½ûÓÃ³¬Ê±¼ÆÊýÆ÷, nCS ±£³Ö¼¤»î×´Ì¬
   sMemMappedCfg.TimeOutPeriod      = 0;  // ³¬Ê±ÅÐ¶ÏÖÜÆÚ
   // ¿ªÆôÄÚ´æÓ³ÉäÄ£Ê½
   if (HAL_OSPI_MemoryMapped(&hospi1,  &sMemMappedCfg) != HAL_OK) // ½øÐÐÅäÖÃ
   {
      return W25Qxx_ERROR_MemoryMapped;   // ÉèÖÃÄÚ´æÓ³ÉäÄ£Ê½´íÎó
   }
   return OSPI_W25Qxx_OK;  // ÅäÖÃ³É¹¦
}


/*********************************************************************
*
*       MainTask
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask2(void);
#ifdef __cplusplus
}
#endif
void MainTask2(void) {
  int i;
  int r;

  MX_OCTOSPI1_Init();

uint32_t id_test = OSPI_W25Qxx_ReadID();

  SEGGER_RTT_printf(0, "ID %x", id_test);

  FS_X_Log("Start\n");
  FS_Init();
r=  FS_FormatLow(VOLUME_NAME);
  r = FS_FormatLLIfRequired(VOLUME_NAME);
  //
  // High level volume format
  //
  FS_X_Log("High-level format...");
#if FS_SUPPORT_FAT
  r = FS_FormatSD(VOLUME_NAME);
#else
  r = FS_Format(VOLUME_NAME, NULL);
#endif
  if (r != 0) {
    SEGGER_RTT_printf(0, "ERROR: Could not format storage device %d\n", r);

  } else {
    FS_X_Log("OK\n");
    //
    //  Create 3 folders
    //
    for (i = 0; i < NUM_DIRS; i++) {
      char acDirName[20];

      (void)snprintf(acDirName, (int)sizeof(acDirName), "%s%cDir%.2d", VOLUME_NAME, FS_DIRECTORY_DELIMITER, i);
      (void)snprintf(_ac, (int)sizeof(_ac), "Create directory %s\n", acDirName);
      FS_X_Log(_ac);
      r = FS_MkDir(acDirName);
      //
      // If directory has been successfully created
      // Create the files in that directory.
      //
      if (r == 0) {
        _CreateFiles(acDirName);
      } else {
        snprintf(_ac, (int)sizeof(_ac), "ERROR: Could not create the directory %s\n", acDirName);
        FS_X_Log(_ac);
      }
    }
    //
    // Show contents of root directory
    //
    _ShowDir(VOLUME_NAME, MAX_RECURSION);
  }
  FS_Unmount(VOLUME_NAME);
  FS_X_Log("Finished\n");
  for (;;) {
    ;
  }
}

/*************************** End of file ****************************/
