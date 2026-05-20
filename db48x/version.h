#ifndef VERSION_H
#define VERSION_H

#define DB48X_VERSION      "0.9.18"
#define PROGRAM_NAME       "DB48X"
#define PROGRAM_VERSION    DB48X_VERSION

#define DBu5G9     (1)
#define DBu585     (0)
#define DBh743     (0)
#define DBh7a3     (0)

#if  DBh743
   #define HARD_NAME "DBh743"
   #define HARD_VERSION "2.a"
   // display used
   #define SHARP_27_400x240  (1)          // hor
   #define SHARP_32_536x336  (0)          // ver

#elif DBu5G9

   #define HARD_NAME    "DBu5g9"
   #define HARD_VERSION "1.f"
   #define DB_MEM_SIZE  (1280)

   // display used
   #define TFT_LTDC           (1)          // hor
   #define SHARP_27_400x240   (0)          // hor
   #define SHARP_32_536x336   (0)          // ver
   #define LCD_ST7796_480x320 (0)
   // keyboard used
   #define KBD_V8_STD      (0)
   #define KBD_H3_6        (1)

   #define OS_RAM       __attribute__((section(".RAM1")))    
   #define FONT_QSPI    __attribute__((section(".FLASH1")))    
   #define LCD_RAM      __attribute__((section(".vram"), aligned(32)))   



#elif DBu585
   #define HARD_NAME "DBu585WA"
   #define HARD_VERSION "0h"
   // display used
   #define SHARP_27_400x240  (1)          // hor
   #define SHARP_32_536x336  (0)          // ver

   // keyboard used
   #define KBD_V8_STD      (0)
   #define KBD_H3_6        (1)

   #define OS_RAM       __attribute__((section(".RAM1")))    // 
   #define LCD_RAM      __attribute__((section(".lcd_mem"), aligned(32)))
   #define DB_ROM2      __attribute__((section(".FLASH2")))       // 1Mo
   #define FONT_QSPI    DB_ROM2
   #define DB_MEM_SIZE    (448)


#elif DBh7a3
   #define HARD_NAME "DBh7a3"
   #define HARD_VERSION "0d"

   // display used
   #define SHARP_27_400x240  (0)          // hor
   #define SHARP_32_536x336  (1)          // ver
   // keyboard disposition
   #define KBD_V8_STD      (0)
   #define KBD_H3_6        (1)

// memory map
   #define OS_RAM       __attribute__((section(".DTCM_RAM1")))    // 128ko
   #define BOARD_RAM1   __attribute__((section(".RAM1")))         // 128ko
   #define DB_RAM       __attribute__((section(".AXI_RAM1")))     // 1Mo
   #define DB_ROM1      __attribute__((section(".FLASH1")))       // 1Mo
   #define DB_ROM2      __attribute__((section(".FLASH2")))       // 1Mo
   #define FONT_QSPI    DB_ROM2
   #define DB48x_ROM    DB_ROM2
   #define DB_MEM_SIZE    960

// top of RAM1 : RAM1lcd, easy access from python jlink
   #define LCD_RAM      __attribute__((section(".RAM1lcd"), aligned(32)))
   #define SysCORECLOCK_BAT      (80000000)
#else
   #error wrong processor setting
#endif

#define T_POWER_OFF_sec  (60*5)

#define DBxxx        (DBh743 | DBu585 | DBh7a3 | DBu5G9)
#define USE_EmFile   (DBxxx)


#if SHARP_27_400x240
   #define LCD_WIDTH           400
   #define LCD_HEIGHT          240  
   #define LCD_VER             0
   #define LCD_BOX              0
   #define LCD_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT/8)
   #define LCDb_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT/8 + LCD_HEIGHT*2 + 2)

#elif SHARP_32_536x336
   #define LCD_WIDTH           536
   #define LCD_HEIGHT          336
   #define LCD_VER              1
   #define LCD_BOX              0
   #define LCD_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT/8)
   #define LCDb_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT/8 + LCD_WIDTH*2 + 2)

#elif LCD_ST7796_480x320
   #define LCD_WIDTH           480
   #define LCD_HEIGHT          320
   #define LCD_VER              0
   #define LCD_BOX              1
   #define CONFIG_COLOR         1
   #define LCD_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT*2)
   #define LCDb_TOTAL_BYTES          (256)


#elif TFT_LTDC
   #define LCD_XSIZE       640
   #define LCD_YSIZE       480
   #define LCD_WIDTH       LCD_XSIZE
   #define LCD_SCAN        LCD_XSIZE
   #define LCD_HEIGHT      LCD_YSIZE
     
   #define LCD_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT*2)
   #define LCDb_TOTAL_BYTES          (LCD_WIDTH*LCD_HEIGHT*2)
   
   #define LCD_VER             0
   #define CONFIG_COLOR    (1)
#else
   #error wrong lcd setting
#endif      // lcd

// debug without entering in stop mode(h743), or enabling debug in stop mode (+1.mA)

#define SIMULATE_STOP2        (0)
#define STOP_LOW_P_TASK       (0)

#if DEBUG
   #define STOP2_ENABLE_DEBUG    (1)        // with stop 2, 1mA
#else
   #define STOP2_ENABLE_DEBUG    (0)        // with stop 2, 1mA
#endif

#define RecorderToDisk        (0)
#define RecorderToRTT         (0)


// enable o3 optimisation, d3d code
#define DM42   (1)

#define HELPINDEX_NAME "/help/db48x.idx"
// version 0.9.15 : help file is bigger now for db50
#define HELPFILE_NAME "/help/db48x.md"
//#define HELPFILE_NAME "/help/db50x.md"








#endif // VERSION_H