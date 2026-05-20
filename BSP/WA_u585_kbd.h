#ifndef WA_u585_KBD_H
#define WA_u585_KBD_H

#include "stm32u5xx.h"
#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>


#if KBD_V8_STD
   #define KB_ROW 8

#endif

#if KBD_H3_6
   #define KB_ROW 9

// WARNING missing values not allowed by db48x struct construction, KEY_SIGMA = 1


// mapping with free42 method
#define     K_A1     44
#define     K_A2     45
#define     K_A3     46
#define     K_A4     47
#define     K_A5     48
#define     K_A6     49

#define     K_B1     1
#define     K_B2     2
#define     K_B3     3
#define     K_B4     4
#define     K_B5     5
#define     K_B6     6

#define     K_C1     7
#define     K_C2     8
#define     K_C3     9
#define     K_C4     10
#define     K_C5     11
#define     K_C6     12

#define     K_D1     13
#define     K_D2     14
#define     K_D3     15
#define     K_D4     16
#define     K_D5     17
#define     K_D6     18

#define     K_E1     19
#define     K_E2     20
#define     K_E3     21
#define     K_E4     22
#define     K_E5     23
#define     K_E6     24

#define     K_F1     25
#define     K_F2     26
#define     K_F3     27
#define     K_F4     28
#define     K_F5     29

#define     K_G1     30
#define     K_G2     31
#define     K_G3     32
#define     K_G4     33
#define     K_G5     34

#define     K_H1     35
#define     K_H2     36
#define     K_H3     37
#define     K_H4     38
#define     K_H5     39

#define     K_I2     40
#define     K_I3     41
#define     K_I4     42
#define     K_I5     43

// functions to key
#define KB_F1                K_A1         //! Function key 1
#define KB_F2                K_A2         //! Function key 2
#define KB_F3                K_A3         //! Function key 3
#define KB_F4                K_A4         //! Function key 4
#define KB_F5                K_A5         //! Function key 5
#define KB_F6                K_A6         //! Function key 6

#define KB_0                 K_I2         //! 0
#define KB_3                 K_H2         //! 1
#define KB_2                 K_H3         //! 2
#define KB_1                 K_H4         //! 3
#define KB_4                 K_G2         //! 4
#define KB_5                 K_G3         //! 5
#define KB_6                 K_G4         //! 6
#define KB_7                 K_F2         //! 7
#define KB_8                 K_F3         //! 8
#define KB_9                 K_F4         //! 9

#define KB_A                  K_D1         //! A          
#define KB_B                  K_D2         //! B
#define KB_C                  K_D3         //! C
#define KB_D                  K_D4         //! D
#define KB_E                  K_D5         //! E
#define KB_F                  K_D6         //! F

#define KB_G                  K_E1         //! G
#define KB_H                  K_E2         //! H
#define KB_I                  K_E3         //! I
#define KB_J                  K_E4         //! J
#define KB_K                  K_E5         //! K

#define KB_L                 K_F1         //! L
#define KB_M                 K_F2         //! M
#define KB_N                 K_F3         //! N
#define KB_O                 K_F4         //! O
#define KB_P                 K_F5         //! P

#define KB_Q                 K_G1         //! Q
#define KB_R                 K_G2         //! R
#define KB_S                 K_G3         //! S
#define KB_T                 K_G4         //! T
#define KB_U                 K_G5         //! U

#define KB_V                 K_H2         //! V
#define KB_W                 K_H3         //! W
#define KB_X                 K_H4         //! X
#define KB_Y                 K_H5         //! Y

#define KB_Z                 K_I2         //! Z


#define KEY_SIGMA  K_B1
#define KEY_INV    K_D6
#define KEY_SQRT   K_D4
#define KEY_LOG    K_D2
#define KEY_LN     K_B5
#define KEY_XEQ    K_G1         // v0.9.17 ???


#define KEY_N1   K_C1
#define KEY_N2   K_C2
#define KEY_N3   K_C3
#define KEY_N4   K_C4
#define KEY_N5   K_C5
#define KEY_N6   K_C6

#define KEY_STO    K_B2
#define KEY_RDN    K_D3
#define KEY_SIN    K_D4
#define KEY_COS    K_D5
#define KEY_TAN    K_D6

#define KEY_ENTER  K_H1

#define KEY_EQ_AL    K_G1          // equation and algebric : '

#define KEY_SWAP   K_E3
#define KEY_CHS    K_E4
#define KEY_E      K_E5
#define KEY_BSP    K_E6


#define KEY_7      K_F2
#define KEY_8      K_F3
#define KEY_9      K_F4
#define KEY_DIV    K_F5


#define KEY_UP     K_B3
#define KEY_EXIT   K_C1          // used 15x
#define KEY_LEFT   K_C2
#define KEY_DOWN   K_C3
#define KEY_RIGHT  K_C4

#define KEY_SHIFT    K_C6
#define KEY_SHIFT2   K_C5
#define KEY_ALPHA    K_B6
#define KEY_CAT      K_B5



#define KEY_4      K_G2
#define KEY_5      K_G3
#define KEY_6      K_G4
#define KEY_MUL    K_G5
#define KEY_1      K_H2
#define KEY_2      K_H3
#define KEY_3      K_H4
#define KEY_SUB    K_H5
#define KEY_EXIT   K_C1          // used 15x
#define KEY_0      K_I2
#define KEY_DOT    K_I3
#define KEY_RUN    K_I4
#define KEY_ADD    K_I5


#define KEY_F1    44
#define KEY_F2    45
#define KEY_F3    46
#define KEY_F4    47
#define KEY_F5    48
#define KEY_F6    49



#define KEY_SCREENSHOT 50
#define KEY_SH_UP      51
#define KEY_SH_DOWN    52

#define KEY_DOUBLE_RELEASE 99

#define KEY_PAGEUP     KEY_DIV
#define KEY_PAGEDOWN   KEY_MUL


#endif



/********************************************************************************
 Keyboard setting
*********************************************************************************/
#define KB_COL 6

// maximum keys pressed at same time
#define KB_MAX_KEY 4

// shifting raw 
#define KB_SHIFT_SCRUT 3

// scrutation period in msec
#define KB_SCRUT_PERIOD    (10)

// key ok after x scrutation, anti rebound 
#define KB_VALID_COUNT     (3)

// auto repeat time in msec for db48x
#define KB_DB_REPEAT_FIRST (1000)
#define KB_DB_REPEAT_PERIOD (100)


#define COL_1_Pin          GPIO_PIN_7
#define COL_1_GPIO_Port    GPIOB
#define COL_2_Pin          GPIO_PIN_8   
#define COL_2_GPIO_Port    GPIOB
#define COL_3_Pin          GPIO_PIN_9
#define COL_3_GPIO_Port    GPIOG
#define COL_4_Pin          GPIO_PIN_10
#define COL_4_GPIO_Port    GPIOG
#define COL_5_Pin          GPIO_PIN_12
#define COL_5_GPIO_Port    GPIOG
#define COL_6_Pin          GPIO_PIN_15
#define COL_6_GPIO_Port    GPIOG


#define ROW_A_Pin          GPIO_PIN_1
#define ROW_A_GPIO_Port    GPIOB

// PC0 is used by spifi
//#define ROW_B_Pin          GPIO_PIN_0
//#define ROW_B_GPIO_Port    GPIOC
#define ROW_B_Pin          GPIO_PIN_2
#define ROW_B_GPIO_Port    GPIOF


#define ROW_C_Pin          GPIO_PIN_12
#define ROW_C_GPIO_Port    GPIOC
#define ROW_D_Pin          GPIO_PIN_3
#define ROW_D_GPIO_Port    GPIOE
#define ROW_E_Pin          GPIO_PIN_2
#define ROW_E_GPIO_Port    GPIOE
#define ROW_F_Pin          GPIO_PIN_7     
#define ROW_F_GPIO_Port    GPIOD
#define ROW_G_Pin          GPIO_PIN_5
#define ROW_G_GPIO_Port    GPIOD
#define ROW_H_Pin          GPIO_PIN_11
#define ROW_H_GPIO_Port    GPIOC
#define ROW_I_Pin          GPIO_PIN_10
#define ROW_I_GPIO_Port    GPIOC


/********************************************************************************
 button PC13 setting
*********************************************************************************/

/* IRQ priorities */
#define BSP_BUTTON_USER_IT_PRIORITY         15U

/**
 * @brief Key push-button
 */
#define BUTTON_USER_PIN                       GPIO_PIN_0
#define BUTTON_USER_GPIO_PORT                 GPIOA
#define BUTTON_USER_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOA_CLK_ENABLE()
#define BUTTON_USER_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOA_CLK_DISABLE()

#define BUTTON_USER_EXTI_IRQn                 EXTI0_IRQn
#define BUTTON_USER_EXTI_LINE                 EXTI_LINE_0



// à supprimer et à mettre dans le tableau de déclaration
//#define KBD_COL1_EXTI_LINE                 EXTI_LINE_13
//#define KBD_COL2_EXTI_LINE                 EXTI_LINE_14
//#define KBD_COL3_EXTI_LINE                 EXTI_LINE_10
//#define KBD_COL4_EXTI_LINE                 EXTI_LINE_15
//#define KBD_COL5_EXTI_LINE                 EXTI_LINE_2
//#define KBD_COL6_EXTI_LINE                 EXTI_LINE_1

#define COL1_EXTI_n     7
#define COL2_EXTI_n     8
#define COL3_EXTI_n     9
#define COL4_EXTI_n     10
#define COL5_EXTI_n     12
#define COL6_EXTI_n     15


//#define   COL1_IRQn        EXTI13_IRQn
//#define   COL2_IRQn        EXTI14_IRQn
//#define   COL3_IRQn        EXTI10_IRQn
//#define   COL4_IRQn        EXTI15_IRQn
//#define   COL5_IRQn        EXTI2_IRQn
//#define   COL6_IRQn        EXTI1_IRQn


typedef  struct {
   GPIO_TypeDef * gpio;
   uint32_t pin;
   uint32_t missing_key;
   uint32_t exti_line_n;
}
st_Pin;


typedef enum {
   SYS_nothing =0,
   SYS_1sec,
   SYS_1mn,
   SYS_RESET,
   SYS_P_OFF,
   SYS_WAKE_UP,
   SYS_SLEEPING,
   SYS_USB_event,
   SYS_int_rtc,
   SYS_LAST,
} t_SYS_CMD;

typedef struct {
         uint8_t key3;
         uint8_t key2;
         uint8_t key1;
         bool released;
         uint8_t key;
} st_key_data;

typedef struct {
   t_SYS_CMD  sys_cmd;
   uint32_t sys_data;
} st_sys_data;


typedef  enum {
   kb_scrut_std,
   kb_scrut_int,
   kb_scrut_int_exit
}keyb_mode;


typedef struct {
   uint32_t       key_time[KB_MAX_KEY];
   uint32_t       key_value[KB_MAX_KEY];
   uint32_t       sleeping_soon;
   uint32_t       c_sec;
   uint32_t       sleeping_time_sec;


   uint64_t       raw;
   keyb_mode      mode;
   uint32_t       ValidCount;
   st_key_data    dts;
} keyboard;

extern const st_Pin kbd_row[KB_ROW];
extern const char SyCmdDesc[SYS_LAST][32];


void init_button_pa0(void);
void KeyboardInit(keyboard *p_kbd);
uint32_t keyb_corr(uint32_t key);
void Kbd_Scrut_Set(keyboard *p_kbd, keyb_mode mode);

uint64_t Scrutation(keyboard *p_kbd, bool sending);

void Send_key(uint8_t key);
uint32_t key_DB_to_DM(uint32_t key);
uint32_t RTT_Key_Decode( int key);

extern  const uint8_t dmcp_position[] ;


#endif // WA_u585_KBD_H
