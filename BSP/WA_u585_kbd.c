//#include "h7a3_kbd.h"
//#include "h7a3_rtc.h"
//#include "h7a3_usb.h"
//#include "h7a3_low_power.h"
//#include "h7a3_Sharp_LS.h"

#include "DBxxxx.h"




/* stop mode 2, wake-up by button pc13, colons and usb  */

uint8_t OS_RAM wakeup_from = 0;

EXTI_HandleTypeDef OS_RAM h_pb_exti_0;

EXTI_HandleTypeDef OS_RAM h_kbd_exti_15;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_14;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_13;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_12;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_11;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_10;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_9;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_8;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_7;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_6;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_5;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_4;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_3;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_2;
EXTI_HandleTypeDef OS_RAM h_kbd_exti_1;


//EXTI_HandleTypeDef OS_RAM h_kbd_col_exti[6];
EXTI_HandleTypeDef OS_RAM h_kbd_exti[16];


extern OS_MAILBOX  Mb_Keyboard;
extern OS_EVENT     EV_USB_Vbus, RTC_Event, PB_Event, KBD_Event, USB_Event, DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER;

extern   OS_TASK     TKBD, TDB48X, TUSB;


const char SyCmdDesc[SYS_LAST][32]={
"",
"SYS_1sec",
"SYS_1mn",
"SYS_RESET",
"P_OFF",
"SYS_WAKE_UP",
"SLEEPING",
"USB_event",
"SYS_int_rtc",
};







/* button PA0, maintenant usb detect */

/**
  * @brief  Key EXTI line detection callbacks.
  * @retval BSP status
  */
static void BUTTON_USER_EXTI_Callback(void)
{
   OS_INT_Enter();
   RTT_vprintf_cr_time( "Button exti0");
   OS_EVENT_Set(&EV_USB_Vbus);
   OS_INT_Leave();
}


static void KBD_EXTI_Callback(void)
{
   OS_INT_Enter();
   RTT_vprintf_cr_time( "Kbd exti, %s", power_names[db_power_state]);

   OS_EVENT_Set(&KBD_Event); // utilisé pour simu stop 2

//   OS_EVENT_Set(&WAKE_UP_EVENT);

   OS_TASKEVENT_Set( &TKBD, EV_KPo_KBD);
   wakeup_from = 1;

   OS_INT_Leave();
}


void USB_EXTI_Callback(void);

void init_button_pa0(void){
  GPIO_InitTypeDef gpio_init_structure;
  BUTTON_USER_GPIO_CLK_ENABLE();

   /* Configure Button pin as input with External interrupt */
   gpio_init_structure.Pin = BUTTON_USER_PIN;
   gpio_init_structure.Pull = GPIO_PULLUP;
   gpio_init_structure.Speed = GPIO_SPEED_FREQ_LOW;
   gpio_init_structure.Mode = GPIO_MODE_IT_FALLING;
   HAL_GPIO_Init(BUTTON_USER_GPIO_PORT, &gpio_init_structure);

//#define BUTTON_USER_EXTI_IRQn                 EXTI15_10_IRQn
//#define BUTTON_USER_EXTI_LINE                 EXTI_LINE_13
   (void)HAL_EXTI_GetHandle(&h_pb_exti_0, BUTTON_USER_EXTI_LINE);
   (void)HAL_EXTI_RegisterCallback(&h_pb_exti_0,  HAL_EXTI_COMMON_CB_ID, BUTTON_USER_EXTI_Callback);

//BUTTON_USER_EXTI_LINE
            __HAL_GPIO_EXTI_CLEAR_IT(BUTTON_USER_PIN);

 /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((BUTTON_USER_EXTI_IRQn), BSP_BUTTON_USER_IT_PRIORITY, 0x00);
    HAL_NVIC_EnableIRQ((BUTTON_USER_EXTI_IRQn));
}

void EXTI0_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_pb_exti_0);     
}



void EXTI1_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[1]);     
}  

void EXTI2_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[2]);     
}

void EXTI3_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[3]);     
}

void EXTI4_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[4]);     
}

void EXTI5_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[5]);     
}

void EXTI6_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[6]);     
}

void EXTI7_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[7]);     
}

void EXTI8_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[8]);     
}

void EXTI9_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[9]);     
}

void EXTI10_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[10]);     
}

void EXTI11_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[11]);     
}

void EXTI12_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[12]);     
}

void EXTI13_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[13]);     
}

void EXTI14_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[14]);     
}

void EXTI15_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_kbd_exti[15]);     
}



extern EXTI_HandleTypeDef  h_usb_exti_9;
/*
void EXTI9_5_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_usb_exti_9);     // 9

//   EXTI->PR1 = 0xffff;
}


*/


/* Keyboard */

const st_Pin kbd_row[KB_ROW]=
// defition of pins raw, outputs, low = scrutation
{
   { ROW_A_GPIO_Port, ROW_A_Pin, 0, 0},
   { ROW_B_GPIO_Port, ROW_B_Pin, 0, 0},
#if KB_ROW==9
   { ROW_C_GPIO_Port, ROW_C_Pin, 0, 0},
#endif
   { ROW_D_GPIO_Port, ROW_D_Pin, 0, 0},
   { ROW_E_GPIO_Port, ROW_E_Pin, 0, 0},
   { ROW_F_GPIO_Port, ROW_F_Pin, 3, 0},
   { ROW_G_GPIO_Port, ROW_G_Pin, 3, 0},
   { ROW_H_GPIO_Port, ROW_H_Pin, 3, 0},
   { ROW_I_GPIO_Port, ROW_I_Pin, 3, 0},
};

const uint32_t  ExtiLineDef[16] = {
   EXTI_LINE_0,
   EXTI_LINE_1,
   EXTI_LINE_2,
   EXTI_LINE_3,
   EXTI_LINE_4,
   EXTI_LINE_5,
   EXTI_LINE_6,
   EXTI_LINE_7,
   EXTI_LINE_8,
   EXTI_LINE_9,
   EXTI_LINE_10,
   EXTI_LINE_11,
   EXTI_LINE_12,
   EXTI_LINE_13,
   EXTI_LINE_14,
   EXTI_LINE_15,
};

const uint32_t  Irq_n_ExtiDef[16] = {
   EXTI0_IRQn,
   EXTI1_IRQn,
   EXTI2_IRQn,
   EXTI3_IRQn,
   EXTI4_IRQn,
   EXTI5_IRQn,
   EXTI6_IRQn,
   EXTI7_IRQn,
   EXTI8_IRQn,
   EXTI9_IRQn,
   EXTI10_IRQn,
   EXTI11_IRQn,
   EXTI12_IRQn,
   EXTI13_IRQn,
   EXTI14_IRQn,
   EXTI15_IRQn,
};


const st_Pin kbd_col[]=
// definition of pins col, inputs, (with interrupts)
{
   { COL_1_GPIO_Port, COL_1_Pin, 0, COL1_EXTI_n },
   { COL_2_GPIO_Port, COL_2_Pin, 0, COL2_EXTI_n},
   { COL_3_GPIO_Port, COL_3_Pin, 0, COL3_EXTI_n},
   { COL_4_GPIO_Port, COL_4_Pin, 0, COL4_EXTI_n},
   { COL_5_GPIO_Port, COL_5_Pin, 0, COL5_EXTI_n},
   { COL_6_GPIO_Port, COL_6_Pin, 0, COL6_EXTI_n},
};

void KeyboardInit(keyboard *p_kbd)
/* pin initialisation, no interrupts for column */
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* GPIO Ports Clock Enable */
  HAL_PWREx_EnableVddIO2();
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

// init row
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
   for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++)
   {
      GPIO_InitStruct.Pin = kbd_row[i_row].pin;
      HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
      HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
   }

// init col
   GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
//   HAL_GPIO_Init(COL_3_GPIO_Port, &GPIO_InitStruct);
   for (uint32_t i_col = 0; i_col < KB_COL; i_col++)
   {
      GPIO_InitStruct.Pin = kbd_col[i_col].pin;
      HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);

      (void)HAL_EXTI_GetHandle(&h_kbd_exti[kbd_col[i_col].exti_line_n], ExtiLineDef[kbd_col[i_col].exti_line_n]);
      (void)HAL_EXTI_RegisterCallback( &h_kbd_exti[kbd_col[i_col].exti_line_n],  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
      HAL_NVIC_SetPriority(Irq_n_ExtiDef[kbd_col[i_col].exti_line_n], 12, 0);
      HAL_NVIC_DisableIRQ(Irq_n_ExtiDef[kbd_col[i_col].exti_line_n]);


   }
// init interrupts
  /* EXTI interrupt init*/

    (void)HAL_EXTI_GetHandle(&h_pb_exti_0, BUTTON_USER_EXTI_LINE);
    (void)HAL_EXTI_RegisterCallback(&h_pb_exti_0,  HAL_EXTI_COMMON_CB_ID, BUTTON_USER_EXTI_Callback);

//    (void)HAL_EXTI_GetHandle(&h_col1_exti_13, KBD_COL1_EXTI_LINE);
//    (void)HAL_EXTI_GetHandle(&h_col2_exti_14, KBD_COL2_EXTI_LINE);
//    (void)HAL_EXTI_GetHandle(&h_col3_exti_10, KBD_COL3_EXTI_LINE);
//    (void)HAL_EXTI_GetHandle(&h_col4_exti_15, KBD_COL4_EXTI_LINE);
//    (void)HAL_EXTI_GetHandle(&h_col5_exti_2, KBD_COL5_EXTI_LINE);
//    (void)HAL_EXTI_GetHandle(&h_col6_exti_1, KBD_COL6_EXTI_LINE);
//    (void)HAL_EXTI_RegisterCallback(&h_col1_exti_13,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
//    (void)HAL_EXTI_RegisterCallback(&h_col2_exti_14,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
//    (void)HAL_EXTI_RegisterCallback(&h_col3_exti_10,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
//    (void)HAL_EXTI_RegisterCallback(&h_col4_exti_15,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
//    (void)HAL_EXTI_RegisterCallback(&h_col5_exti_2,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
//    (void)HAL_EXTI_RegisterCallback(&h_col6_exti_1,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);

//   HAL_NVIC_SetPriority(EXTI0_IRQn, 12, 0);
//   HAL_NVIC_DisableIRQ(EXTI0_IRQn);
//   HAL_NVIC_SetPriority(EXTI1_IRQn, 12, 1);
//   HAL_NVIC_DisableIRQ(EXTI1_IRQn);
//   HAL_NVIC_SetPriority(EXTI2_IRQn, 12, 2);
//   HAL_NVIC_DisableIRQ(EXTI2_IRQn);
//   HAL_NVIC_SetPriority(EXTI10_IRQn, 12, 3);
//   HAL_NVIC_DisableIRQ(EXTI10_IRQn);
//   HAL_NVIC_SetPriority(EXTI13_IRQn, 12, 4);
//   HAL_NVIC_DisableIRQ(EXTI13_IRQn);
//   HAL_NVIC_SetPriority(EXTI15_IRQn, 12, 5);
//   HAL_NVIC_DisableIRQ(EXTI15_IRQn);
//   HAL_NVIC_SetPriority(EXTI14_IRQn, 12, 6);
//   HAL_NVIC_DisableIRQ(EXTI14_IRQn);



   for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
      p_kbd->key_value[ii] = 0; 
      p_kbd->key_time[ii] = 0;
   }
   p_kbd->ValidCount = KB_VALID_COUNT;

}


uint32_t keyb_corr(uint32_t key)
/* keyboard correction for missing keys in the 8*6 matrix */
{
#if KBD_V8_STD

   for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
      if (kbd_row[i_row].missing_key != 0){
         if ((key >= (kbd_row[i_row].missing_key + (i_row+1)*10)) &&(key <(kbd_row[i_row].missing_key+6+ (i_row+1)*10))) return key-1;
      }
   }
#endif
   return key;
}


void Kbd_Scrut_Set(keyboard *p_kbd, keyb_mode mode)
// mode kb_scrut, no interrupt, one row at low level at a time, normally all to high
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   EXTI->PR1 = 0xffff; // not ok for u5
   if ( mode == p_kbd->mode) return;
   switch(mode){
      case    kb_scrut_std:
         // gpio scrutation
   
         // init row
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
            for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
               GPIO_InitStruct.Pin = kbd_row[i_row].pin;
               HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
               HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
            }
         // init col
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            HAL_GPIO_Init(COL_3_GPIO_Port, &GPIO_InitStruct);
            for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
               GPIO_InitStruct.Pin = kbd_col[i_col].pin;
               HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
               HAL_NVIC_DisableIRQ(Irq_n_ExtiDef[kbd_col[i_col].exti_line_n]);
            }

//            HAL_NVIC_DisableIRQ(COL1_IRQn);
//            HAL_NVIC_DisableIRQ(COL2_IRQn);
//            HAL_NVIC_DisableIRQ(COL3_IRQn);
//            HAL_NVIC_DisableIRQ(COL4_IRQn);
//            HAL_NVIC_DisableIRQ(COL5_IRQn);
//            HAL_NVIC_DisableIRQ(COL6_IRQn);
            for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
               HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
            }
            for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
               p_kbd->key_value[ii] = 0; 
               p_kbd->key_time[ii] = 0;
            }
            p_kbd->mode = kb_scrut_std;
            break;

      case    kb_scrut_int:
         // init row
         GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
         GPIO_InitStruct.Pull = GPIO_NOPULL;
         GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
         for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
            GPIO_InitStruct.Pin = kbd_row[i_row].pin;
            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_RESET);
            HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
         }
         // init col
         GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
         GPIO_InitStruct.Pull = GPIO_PULLUP;
         GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
         for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
            GPIO_InitStruct.Pin = kbd_col[i_col].pin;
            HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
            __HAL_GPIO_EXTI_CLEAR_IT(kbd_col[i_col].pin);
         }
         OS_TASK_Delay_us(150);
         for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
               HAL_NVIC_EnableIRQ(Irq_n_ExtiDef[kbd_col[i_col].exti_line_n]);
         }
/*         // allow all colomns interrupts
//         __HAL_GPIO_EXTI_CLEAR_IT(COL_1_Pin);
  //       __HAL_GPIO_EXTI_CLEAR_IT(COL_2_Pin);
//         __HAL_GPIO_EXTI_CLEAR_IT(COL_3_Pin);
  //       __HAL_GPIO_EXTI_CLEAR_IT(COL_4_Pin);
//         __HAL_GPIO_EXTI_CLEAR_IT(COL_5_Pin);
  //       __HAL_GPIO_EXTI_CLEAR_IT(COL_6_Pin);
         OS_TASK_Delay_us(150);

         HAL_NVIC_EnableIRQ(COL1_IRQn);
         HAL_NVIC_EnableIRQ(COL2_IRQn);
         HAL_NVIC_EnableIRQ(COL3_IRQn);
         HAL_NVIC_EnableIRQ(COL4_IRQn);
         HAL_NVIC_EnableIRQ(COL5_IRQn);
         HAL_NVIC_EnableIRQ(COL6_IRQn);
*/
         p_kbd->mode = kb_scrut_int;
         p_kbd->ValidCount = KB_VALID_COUNT-1;

         break;

      case    kb_scrut_int_exit:

         // init col, all but col1
         GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
         GPIO_InitStruct.Pull = GPIO_PULLUP;
         HAL_GPIO_Init(COL_3_GPIO_Port, &GPIO_InitStruct);
         for (uint32_t i_col = 1; i_col < KB_COL; i_col++){
            GPIO_InitStruct.Pin = kbd_col[i_col].pin;
            HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
         }

         // init row, all but  C
         GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//         GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
//         GPIO_InitStruct.Pull = GPIO_PULLUP;
         GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
         for (uint32_t i_row = 0 ; i_row < 2; i_row++){
            GPIO_InitStruct.Pin = kbd_row[i_row].pin;
            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
            HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
         }
         for (uint32_t i_row = 3 ; i_row < KB_ROW; i_row++){
            GPIO_InitStruct.Pin = kbd_row[i_row].pin;
            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
            HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);

         }

         for (uint32_t i_col = 1; i_col < KB_COL; i_col++){
            __HAL_GPIO_EXTI_CLEAR_IT(kbd_col[i_col].pin);
         }
      // wake up only with [exit]
/*         __HAL_GPIO_EXTI_CLEAR_IT(COL_1_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_2_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_3_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_4_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_5_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_6_Pin);
*/
         OS_TASK_Delay_us(150);
         for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
               HAL_NVIC_DisableIRQ(Irq_n_ExtiDef[kbd_col[i_col].exti_line_n]);
         }
/*         HAL_NVIC_DisableIRQ(COL1_IRQn);
         HAL_NVIC_DisableIRQ(COL2_IRQn);
         HAL_NVIC_DisableIRQ(COL3_IRQn);
         HAL_NVIC_DisableIRQ(COL4_IRQn);
         HAL_NVIC_DisableIRQ(COL5_IRQn);
         HAL_NVIC_DisableIRQ(COL6_IRQn);
 */
         HAL_GPIO_WritePin(kbd_row[2].gpio, kbd_row[2].pin, GPIO_PIN_RESET);

         OS_TASK_Delay_us(150);        // purge all pending interrupts

         OS_EVENT_Reset(&KBD_Event);   // purge event 
         for (uint32_t i_col = 1; i_col < KB_COL; i_col++){
            __HAL_GPIO_EXTI_CLEAR_IT(kbd_col[i_col].pin);
         }

/*         __HAL_GPIO_EXTI_CLEAR_IT(COL_1_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_2_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_3_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_4_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_5_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_6_Pin);
*/
//         HAL_NVIC_EnableIRQ(COL1_IRQn);
HAL_NVIC_EnableIRQ(Irq_n_ExtiDef[kbd_col[0].exti_line_n]);

         p_kbd->mode = kb_scrut_int_exit;
         break;
   }
}

uint64_t Scrutation(keyboard *p_kbd, bool sending)
// scrutation clavier
{
   bool key_pressed_counting = false;
   uint32_t key_value_act = 0;
   uint32_t shift = 0;
   p_kbd->raw = 0;
   for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
      HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_RESET);
      OS_TASK_Delay_us(20);
      for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
         key_pressed_counting = false;
         key_value_act = (1 + i_row)*10 + (i_col+1);
         if (HAL_GPIO_ReadPin(kbd_col[i_col].gpio, kbd_col[i_col].pin) == GPIO_PIN_RESET)
         { // key pressed
            p_kbd->sleeping_soon = 0;
            p_kbd->raw |= (uint64_t)1<< (uint64_t)( 6 * i_row + i_col + KB_SHIFT_SCRUT);   
            shift = 0;
            p_kbd->dts.key = 0;
            p_kbd->dts.key1 =0;
            p_kbd->dts.key2 =0;
            p_kbd->dts.key3 =0;
            for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++)
            {
               if (key_value_act == p_kbd->key_value[ii])
               { 
                  p_kbd->key_time[ii]++;
                  key_pressed_counting = true;
                  if (p_kbd->key_time[ii] == p_kbd->ValidCount)
                  {
                     p_kbd->dts.key = keyb_corr(p_kbd->key_value[ii]);
                  }
               }else 
               {
                  if (p_kbd->key_time[ii] >= p_kbd->ValidCount)
                  {
                     p_kbd->ValidCount = KB_VALID_COUNT;
                     shift += 8;
                     if (8 == shift) p_kbd->dts.key1 = keyb_corr(p_kbd->key_value[ii]);
                     if (16 == shift) p_kbd->dts.key2 = keyb_corr(p_kbd->key_value[ii]);
                     if (24 == shift) p_kbd->dts.key3 = keyb_corr(p_kbd->key_value[ii]);
                  }
               }
            }
            if    (p_kbd->dts.key)     {
               p_kbd->dts.released = 0;      
               if ( true == sending)
               {  
                  OS_MAILBOX_Put(&Mb_Keyboard, &p_kbd->dts);     // nouvelle touche pressée             
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_KBD);
               }
            }
            if (key_pressed_counting == false){
               for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
                  if (p_kbd->key_time[ii] == 0) {
                     p_kbd->key_value[ii] = key_value_act;
                     break;   
                  } 
               }
            }           
         }else 
         { // key not pressed
            p_kbd->dts.key =0;
            p_kbd->dts.key1 =0;
            p_kbd->dts.key2 =0;
            p_kbd->dts.key3 =0;
            shift = 0;
            for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
               if ((key_value_act == p_kbd->key_value[ii])&&
                  (p_kbd->key_time[ii] >= KB_VALID_COUNT)){ 
                  // envoyer un message, touche relachée
                     p_kbd->dts.key = keyb_corr(key_value_act);
                     p_kbd->dts.released = 1;
                     p_kbd->key_time[ii] = 0;
                     p_kbd->key_value[ii] = 0;
                  }else 
                  {
                     if (p_kbd->key_time[ii] >= KB_VALID_COUNT){
                        shift += 8;
                        if (8 == shift) p_kbd->dts.key1 = keyb_corr(p_kbd->key_value[ii]);
                        if (16 == shift) p_kbd->dts.key2 = keyb_corr(p_kbd->key_value[ii]);
                        if (24 == shift) p_kbd->dts.key3 = keyb_corr(p_kbd->key_value[ii]);
                     }
                  }
               }
            if    (p_kbd->dts.key)     {
               if ( true == sending )
               {  
                  OS_MAILBOX_Put(&Mb_Keyboard, &p_kbd->dts);     //  touche relachée              
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_KBD);
               }
            }
         }  
      }  
      HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
   }
   return p_kbd->raw;
}


void Send_key(uint8_t key)
// sending a key to main task
{
   st_key_data dts;
   dts.key = key;
   dts.key1 = 0;
   dts.key2 = 0;
   dts.key3 = 0;
   dts.released = 0;
   OS_MAILBOX_Put(&Mb_Keyboard, &dts); // press             
   dts.released = 1;
   OS_MAILBOX_Put(&Mb_Keyboard, &dts); // release        
   OS_TASKEVENT_Set( &TDB48X, EV_DBx_KBD);
}




const uint8_t dmcp_position[100] =
// ----------------------------------------------------------------------------
//   Convert DMCP key codes to row/column positions
// ----------------------------------------------------------------------------
{
// functions key A1  A6 : 1  6
    KB_F1,      11,
    KB_F2,      12,
    KB_F3,      13,
    KB_F4,      14,
    KB_F5,      15,
    KB_F6,      16,

    K_B1,       21,
    K_B2,       22,
    K_B3,       23,
    K_B4,       24,
    K_B5,       25,
    K_B6,       26,

    K_C1,      31,
    K_C2,      32,
    K_C3,      33,
    K_C4,      34,
    K_C5,      35,
    K_C6,      36,


    K_D1,      41,
    K_D2,      42,
    K_D3,      43,
    K_D4,      44,
    K_D5,      45,
    K_D6,      46,

    K_E1,      51,
    K_E2,      52,
    K_E3,      53,
    K_E4,      54,
    K_E5,      55,
    K_E6,      56,

    K_F1,      61,
    K_F2,      62,
    K_F3,      63,
    K_F4,      64,
    K_F5,      65,

    K_G1,      71,
    K_G2,      72,
    K_G3,      73,
    K_G4,      74,
    K_G5,      75,

    K_H1,      81,
    K_H2,      82,
    K_H3,      83,
    K_H4,      84,
    K_H5,      85,
    
    K_I2,      92,
    K_I3,      93,
    K_I4,      94,
    K_I5,      95,
//    K_I5,      95,
    0,0
};


uint32_t key_DB_to_DM(uint32_t key){

    const size_t max = sizeof(dmcp_position) / sizeof(dmcp_position[0]);
    for (size_t k = 0; k < max; k += 2)
        if (dmcp_position[k+1] == key){
          return dmcp_position[k];
      }
    return 0;


}



uint32_t RTT_Key_Decode( int key){
   return key;
}


