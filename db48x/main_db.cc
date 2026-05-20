// ****************************************************************************
//  main.cc                                                       DB48X project
// ****************************************************************************
//
//   File Description:
//
//      The DB48X main RPL loop
//
//
//
//C:\segger\embOS_Ultra_CortexM_ES_Obj_SFL_V5.20.0.0\STM32U585_db48x_v0.a\db48x\src\dmcp\main.cc
//
//
//
//
// ****************************************************************************
//   (C) 2022 Christophe de Dinechin <christophe@dinechin.org>
//   This software is licensed under the terms outlined in LICENSE.txt
// ****************************************************************************
//   This file is part of DB48X.
//
//   DB48X is free software: you can redistribute it and/or modify
//   it under the terms outlined in the LICENSE.txt file
//
//   DB48X is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// This code is distantly derived from the SwissMicro SDKDemo calculator

/*
Faire marcher les 4 timers : ok
Show et Setup : gestion du clavier avec Mailbox dans dmcp.cpp
Utiliser la description du clavier de target.cc ligne 112
Gestion usb, segger à partir de 910.56, eth over usb, + client ftp ??
Tester un changement de version v9.9

// ----------------------------------------------------------------------------
//   Return the complete key ID from HP48-style Row/Column/Plane position
// ----------------------------------------------------------------------------
// HP48 key codes are given in the form rc.ph, where r, c, p and h
// are one decimal digit each.
// r = row
// c = column
// p = plane
// h = hold (i.e. shift + key held simultaneously)
//
// On DMCP, we reinterpret "h" as "transient alpha"
//
// The plane is documented as follows:
// 0: Like 1
// 1: Unshifted
// 2: Left shift
// 3: Right shift
// 4: Alpha
// 5: Alpha left shift
// 6: Alpha right shift
//
// DB48X adds 7, 8 and 9 for lowercase alpha


*/


#include "SEGGER_RTT.h"

#include "blitter.h"
#include "dmcp.h"
#include "expression.h"
#include "font.h"
#include "program.h"
#include "recorder.h"
#include "stack.h"
#include "sysmenu.h"
#include "target.h"
#include "user_interface.h"
#include "util.h"

#include "DBxxxx.h"

#if SIMULATOR
#  include "tests.h"
#endif


#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using std::max;
using std::min;


extern "C" {
}


// ============================================================================
//
// Those are put in the same file to guarantee initialization order
//
// ============================================================================

/* Initialize the screen
        surface(pixword *p, size w, size h, size scanline, size swap)
            : pixels(p),
              w(w),
              h(h),
              scanline(scanline),
              swapsize(swap-1),
              drawable(w, h)
*/

surface Screen((pixword *) LCD_GetFramebuffer(), LCD_W, LCD_H, LCD_SCANLINE, LCD_SCANLINE);

//                                                                 W si rotation ok
//surface Screen((pixword *) LCD_GetFramebuffer(), LCD_W, LCD_H, LCD_H, LCD_W);




// Reserve memory for db48x, 448ko / 512
#if (DBh743+DBu585+DBu5G9)
static uint8_t __attribute__((section(".AXI_RAM1"), aligned(32))) db48x_mem[1024*DB_MEM_SIZE] = {0};
#elif DBh7a3
static uint8_t __attribute__((section(".AXI_RAM1"), aligned(32))) db48x_mem[1024*DB_MEM_SIZE] = {0};
#endif



// Pre-built patterns for shades of grey
const pattern pattern::black   = pattern(0, 0, 0);
const pattern pattern::gray10  = pattern(32, 32, 32);
const pattern pattern::gray25  = pattern(64, 64, 64);
const pattern pattern::gray50  = pattern(128, 128, 128);
const pattern pattern::gray75  = pattern(192, 192, 192);
const pattern pattern::gray90  = pattern(224, 224, 224);
const pattern pattern::white   = pattern(255, 255, 255);
const pattern pattern::invert  = pattern(~0ULL);

// Settings depend on patterns
settings Settings;

// Runtime must be initialized before ser interface, which contains GC pointers
runtime::gcptr *runtime::GCSafe;
runtime rt(nullptr, 0);
user_interface ui;

uint32_t last_keystroke_time = 0;

char keymap_name[80] = { 0 };

// modif re tests !!!
int  last_key            = 0;

RECORDER(main,          16, "Main RPL thread");
RECORDER(main_error,    16, "Errors in the main RPL thread");
RECORDER(tests_rpl,    256, "Test request processing on RPL");
RECORDER(refresh,       16, "Refresh requests");


static byte *lcd_buffer = nullptr;
static uint  row_min    = ~0;
static uint  row_max    = 0;



void set_timer(uint  timerid, uint  period)
// ----------------------------------------------------------------------------
//   Conditiony set a timer based on period
// ----------------------------------------------------------------------------
{
   if (TIMER1 == timerid)
   {

#if TFT_LTDC
      if  (period <= 60000)
#else
      if  ((period <= 60000) && !usb_connected)
#endif

      {
         HAL_LPTIM_TimeOut_Stop_IT(&hlptim1);
         HAL_LPTIM_TimeOut_Start_IT(&hlptim1, (period*1024)/1000-1);
         RTT_vprintf_cr_time("Set lptim1 to : %d", period);
      }
      else
      {
         HAL_LPTIM_TimeOut_Stop_IT(&hlptim1);
         HAL_LPTIM_TimeOut_Start_IT(&hlptim1, (60000*1024)/1000-1);
         RTT_vprintf_cr_time("Set lptim1 to : %d", 60000);
      
      }   
   
   
   
   }
  /*      if (period >= 60000)
            CLR_ST(STAT_CLK_WKUP_SECONDS);
        else
            SET_ST(STAT_CLK_WKUP_SECONDS);
*/
      sys_timer_start(timerid, period);
      RTT_vprintf_cr_time("Set timer %d to : %d", timerid, period);

}



void mark_dirty(uint32_t x_min, uint32_t y_min, uint32_t x_max, uint32_t y_max)
// ----------------------------------------------------------------------------
//   Mark a screen range as dirty
// ----------------------------------------------------------------------------
{
   LCD_MarkLineModified(&hlcd, x_min, y_min, x_max, y_max);


}

void refresh_dirty(void)
// ----------------------------------------------------------------------------
//  Send an LCD refresh request for the area dirtied by drawing
// ----------------------------------------------------------------------------
{
    int32_t start = Get_Elapsed_Dual_Res(0);

   LCD_Status_t lcd_res = DisplayUpdate(&hlcd, false, true);
//   Disp_WaitForTransfer(&hlcd);
   if (lcd_res != LCD_OK)  RTT_vprintf_cr_time( "Lcd update err :%s", LCD_Status_Desc[lcd_res]);
//    row_min = ~0;
//    row_max = 0;

    program::refresh_time += Get_Elapsed_Dual_Res_64(start);   // refresh time is long long
}





void redraw_lcd(bool force)
// ----------------------------------------------------------------------------
//   Redraw the whole LCD
// ----------------------------------------------------------------------------
{
    uint start = sys_current_ms();

 //   SEGGER_RTT_printf(0,"\nBegin redraw at %u", start);

    // Draw the various components handled by the user interface
    ui.draw_start(force);

    ui.draw_header();               // rtc is read here
    ui.draw_battery();
//    ui.draw_battery(force);

    ui.draw_annunciators();

    ui.draw_menus();

    if (!ui.draw_help())
    {
        ui.draw_editor();
        ui.draw_cursor(true, ui.cursor_position());
        ui.draw_stack();
        if (!ui.draw_stepping_object())
            ui.draw_command();

    }
    ui.draw_error();

    // Refresh the screen
// 0.9.14
    ui.refresh();

//    Lcd_Update(force);

    // Compute next refresh
    uint end = sys_current_ms();
    uint period = ui.draw_refresh();
//    record(main,            "Refresh at %u (%u later), period %u", end, end - start, period);
 //   SEGGER_RTT_printf(0, "\nRefresh at %u (%u later), period %u", end, end - start, period);

    // Refresh screen moving elements after the requested period
    set_timer(TIMER1, period);
    program::display_time += end - start;

}


static void redraw_periodics()
// ----------------------------------------------------------------------------
//   Redraw the elements that move
// ----------------------------------------------------------------------------
{
    uint start       = program::read_time();
    uint dawdle_time = start - last_keystroke_time;

//SEGGER_RTT_printf(0,"\nPeriodics %u", start);
    ui.draw_start(false);
    ui.draw_header();
    ui.draw_battery();
    if (program::animated())
    {
        ui.draw_cursor(false, ui.cursor_position());
        ui.draw_menus();
    }

        ui.refresh();

//    refresh_dirty();

    // Slow things down if inactive for long enough
    uint period = ui.draw_refresh();
    if (!program::animated())
    {
        // Adjust refresh time based on time since last interaction
        // After 10s, update at most every 3s
        // After 1 minute, update at most every 10s
        // After 3 minutes, update at most once per minute
        if (dawdle_time > 180000 && period < 60000)
            period = 60000;
        else if (dawdle_time > 60000 && period < 10000)
            period = 10000;
        else if (dawdle_time > 10000 && period < 3000)
            period = 3000;
    }

    uint end = program::read_time();
//SEGGER_RTT_printf(0,"\nDawdling for %u at %u after %u", period, end, end-start);
#if TFT_LTDC
   period = 1000;
   set_timer(TIMER1, period);
#else
if (usb_connected) period = 200;
    // Refresh screen moving elements after 0.1s
    set_timer(TIMER1, period);
#endif
    program::display_time += end - start;
}


static void handle_key(int key, bool repeating, bool talpha)
// ----------------------------------------------------------------------------
//   Handle all user-interface keys
// ----------------------------------------------------------------------------
{
    sys_timer_disable(TIMER0);
    bool consumed = ui.key(key, repeating, talpha);
    if (!consumed)
        beep(1835, 125);

    // Key repeat timer
    if (ui.repeating())
        sys_timer_start(TIMER0, repeating ? KB_DB_REPEAT_PERIOD : KB_DB_REPEAT_FIRST);
}


void db48x_set_beep_mute(int val)
// ----------------------------------------------------------------------------
//   Set the beep flag (shared with firmware)
// ----------------------------------------------------------------------------
{
    Settings.BeepOff(val);
    Settings.SilentBeepOn(val);
}


int db48x_is_beep_mute()
// ----------------------------------------------------------------------------
//   Check the beep flag from our settings
// ----------------------------------------------------------------------------
{
    return Settings.BeepOff();
}


bool load_saved_keymap(cstring name)
// ----------------------------------------------------------------------------
//   Load the default system state file
// ----------------------------------------------------------------------------
{
    bool isdefault = false;
//    char keymap_name[80] = { 0 };
    if (name)
    {
        file kcfg("/config/keymap.cfg", file::WRITING);
        if (kcfg.valid())
            kcfg.write(name, strlen(name));
    }

    file kcfg("config/keymap.cfg", file::READING);
    if (kcfg.valid())
    {
        kcfg.read(keymap_name, sizeof(keymap_name)-1);
        for (size_t i = 0; i < sizeof(keymap_name); i++)
            if (keymap_name[i] == '\n')
                keymap_name[i] = 0;
    }
    else
    {
        strncpy(keymap_name, "/config/db48x.48k", sizeof(keymap_name));
        isdefault = true;
    }

    // Load default keymap
    if (!ui.load_keymap(keymap_name))
    {
        // Fail silently if we try to load a default file
        if (isdefault)
            rt.clear_error();
        else
            rt.command(command::static_object(object::ID_KeyMap));
        return false;
    }
    return true;
}


extern uint memory_size;
void program_init()
// ----------------------------------------------------------------------------
//   Initialize the program
// ----------------------------------------------------------------------------
{
   // Setup application menu callbacks
    run_menu_item_app = menu_item_run;
    menu_line_str_app = menu_item_description;
    is_beep_mute = db48x_is_beep_mute;
    set_beep_mute = db48x_set_beep_mute;

    lcd_buffer = LCD_GetFramebuffer();
// lcd_buffer = lcd_framebuffer;


    // Setup default fonts
    font_defaults();

#if (DBxxx)
    rt.memory(db48x_mem, sizeof(db48x_mem));
    RTT_vprintf_cr_time( "Db48x Memory : %dkb", sizeof(db48x_mem)>>10);
#elif SIMULATOR
    // Give 4K bytes to the runtime to stress-test the GC
    size_t size = 1024 * memory_size;
    byte *memory = (byte *) malloc(size);
    rt.memory(memory, size);

#else
    // Give as much as memory as possible to the runtime
    // Experimentally, this is the amount of memory we need to leave free
    size_t size = sys_free_mem() - 10 * 1024;
    byte *memory = (byte *) malloc(size);
    rt.memory(memory, size);


#endif

    // Check if we have a state file to load
    bool res = load_system_state();
    RTT_vprintf_cr_time( "Load state ==> %s", res ? "ok":"err");
     res = load_saved_keymap();
    RTT_vprintf_cr_time( "Load keymap ==> %s", res ? "ok":"err");

    // Enable wakeup each minute (for clock update)
    SET_ST(STAT_CLK_WKUP_ENABLE);
}

void power_check(bool running, bool showimage)
// when receiving running = true from command or graphics set flag
// several times per msec...
// Honor auto-off while waiting, do not erase drawn image
//       power_check(true);


{
   static uint64_t last_awake = 0;
   OS_TASKEVENT Db48xEvents;   
   SET_ST(STAT_RUNNING);

   int64_t tin = sys_current_ms_i64();
//   if (last_awake)
//       program::active_time += tin - last_awake;
   StopMode2_disable = false;

   unsigned long wake_up_mask = 0;
   if (running) wake_up_mask =  EV_DBx_KBD | EV_DBx_LPTIM1 | EV_DBx_PON | EV_DBx_USB_CON | EV_DBx_USB_DIS | EV_DBx_RESET;
   else wake_up_mask = EV_DBx_PON;
//   Db48xEvents = OS_TASKEVENT_GetSingleTimed( wake_up_mask, 15000);  // 15sec
   Db48xEvents = OS_TASKEVENT_GetSingleTimed( wake_up_mask, 120000);  // 120sec

   StopMode2_disable = true;
   int64_t tout = sys_current_ms_i64();
   last_awake = tout;
   program::sleeping_time += tout - tin;
   program::run_cycles++;

    if ( EV_DBx_LPTIM1 & Db48xEvents)  
    {
      RTT_vprintf_cr_T_F( power_check,  "wake up from lptim1");     

    }

   if ( EV_DBx_PON & Db48xEvents)  
   { // power on
      RTT_vprintf_cr_T_F( DBx_Task_App,  "Reset, redraw ");    
      ui.draw_header();   
      ui.draw_battery(true);
      redraw_lcd(true); // bug two times [exit] ???
// still missing batt level !
   }
}


void power_check_db48x(bool running, bool showimage)
// ----------------------------------------------------------------------------
//   Check power state, keep looping until it's safe to run
// VAL_ST() ???
// ----------------------------------------------------------------------------
// Status flags:
// ST(STAT_PGM_END)   - Program should go to off state (set by auto off timer)
// ST(STAT_SUSPENDED) - Program signals it is ready for off
// ST(STAT_OFF)       - Program in off state (only [EXIT] key can wake it up)
// ST(STAT_RUNNING)   - OS doesn't sleep in this mode
{
   while (true)
   {
      // Already in off mode and suspended
      if ((ST(STAT_PGM_END) && ST(STAT_SUSPENDED)) ||
         // Go to sleep if no keys available
         (!ST(STAT_PGM_END) && key_empty()))
      {
         CLR_ST(STAT_RUNNING);
         static uint last_awake = 0;
         uint tin = sys_current_ms();
         if (last_awake)
             program::active_time += tin - last_awake;
         sys_sleep();
         uint tout = sys_current_ms();
         last_awake = tout;
         program::sleeping_time += tout - tin;
         program::run_cycles++;
      }
      if (ST(STAT_PGM_END) || ST(STAT_SUSPENDED))
      {
         // Wakeup in off state or going to sleep
         if (!ST(STAT_SUSPENDED))
         {
             bool lowbat = !program::on_usb && program::low_battery();
             if (lowbat)
                 ui.draw_message("Switched off due to low power",
                                 "Connect to USB to avoid losing memory",
                                 "Replace the battery as soon as possible");
             else if (running)
                 ui.draw_message("Switched off to conserve battery",
                                 "Press the ON/EXIT key to resume");
             else if (showimage)
                 draw_power_off_image(0);
             else
                 lcd_refresh_wait();

             sys_critical_start();
             SET_ST(STAT_SUSPENDED);
             LCD_power_off(0);
             sys_timer_disable(TIMER0);
             sys_timer_disable(TIMER1);
            SET_ST(STAT_OFF);
            sys_critical_end();
         }
      // Already in OFF -> just continue to sleep above
      }
      else if (ST(STAT_CLK_WKUP_FLAG))
      {
         // Clock wakeup (once per second or per minute)
         CLR_ST(STAT_CLK_WKUP_FLAG);
         if (running)
            break;
         redraw_periodics();
      }
      else if (ST(STAT_POWER_CHANGE))
      {
         // Power state change (to/from USB)
         CLR_ST(STAT_POWER_CHANGE);
         sys_timer_disable(TIMER0);
         sys_timer_disable(TIMER1);
         // Force reload battery with correct value at next clock refresh.
         ui.draw_battery(true);
         program::last_interrupted -= Settings.BatteryRefresh() - 1000;
      }
      else
      {
            break;
      }
   }

   // Well, we are woken-up
   SET_ST(STAT_RUNNING);

    // Get up from OFF state
   if (ST(STAT_OFF))
   {
      LCD_power_on();

      // Ensure that RTC readings after power off will be OK
      rtc_wakeup_delay();

      CLR_ST(STAT_OFF);

      // Check if we need to redraw
      program::read_battery();
      if (ui.showing_graphics())
         ui.draw_graphics(true);
      else
         redraw_lcd(true);
   }
    // We definitely reached active state, clear suspended flag
    CLR_ST(STAT_SUSPENDED);
}

#ifndef SIMULATOR
extern const uint prog_build_id;
extern const uint qspi_build_id;
#endif



extern "C" void program_main()
// ----------------------------------------------------------------------------
//   DMCP main entry point and main loop
// ----------------------------------------------------------------------------
{
    int  key        = 0;
    bool transalpha = false;

#ifndef SIMULATOR
    if (prog_build_id != qspi_build_id)
    {
        msg_box(t24,
                "Incompatible " PROGRAM_NAME " build ID\n"
                "Please reload program and QSPI\n"
                "from the same build",
                true);
        lcd_refresh();
        wait_for_key_press();
        return;
    }
#endif

    // Initialization
    program_init();
    redraw_lcd(true);
    last_keystroke_time = program::read_time();

    // Main loop
    while (true)
    {
        // Check power state, and switch off if necessary
        power_check(false);

        // Key is ready -> clear auto off timer
        bool hadKey = false;

        if (!key_empty())
        {
            reset_auto_off();
            key    = key_pop();
            hadKey = true;
            record(main, "Got key %d", key);

#if !WASM
#if SIMULATOR
            // Process test-harness commands
            record(tests_rpl, "Processing key %d, last=%d, command=%u",
                   key, last_key, test_command);
            if (key == tests::EXIT_PGM || key == tests::SAVE_PGM)
            {
                cstring path = get_reset_state_file();
                printf("Exit: saving state to %s\n", path);
                if (path && *path)
                    save_state_file(path);
                if (key == tests::EXIT_PGM)
                    break;
            }
#else // Real hardware
//#define read_key __sysfn_read_key
#endif // SIMULATOR
#endif // !WASM

            // Check transient alpha mode
            if (key == KEY_UP || key == KEY_DOWN)
            {
                transalpha = true;
            }
            else if (transalpha)
            {
                int k1, k2;
//                int r = read_key(&k1, &k2);
                int r = 0;
                switch (r)
                {
                case 0:
                    transalpha = false;
                    break;
                case 1:
                    transalpha = k1 == KEY_UP || k1 == KEY_DOWN;
                    break;
                case 2:
                    transalpha = k1 == KEY_UP || k1 == KEY_DOWN
                        ||       k2 == KEY_UP || k2 == KEY_DOWN;
                    break;
                }
            }
        }
        bool repeating = key > 0
            && sys_timer_active(TIMER0)
            && sys_timer_timeout(TIMER0);
        if (repeating)
        {
            hadKey = true;
            record(main, "Repeating key %d", key);
        }

        // Check if we are displaying a graphic image - If so wait for key
        bool graphics = ui.showing_graphics();
        if (graphics && hadKey)
        {
            if (key > 0)
            {
                record(tests_rpl, "Clearing graphics from key %d", key);
                ui.show_graphics(false);
                redraw_lcd(true);
            }
        }

        // Fetch the key (<0: no key event, >0: key pressed, 0: key released)
        record(main, "Testing key %d (%+s)", key, hadKey ? "had" : "nope");
        if (key >= 0 && hadKey)
        {
#if SIMULATOR && !WASM
            if (process_test_key(key))
                graphics = false;
#endif // SIMULATOR && !WASM

            if (!graphics)
            {
                record(main, "Handle key %d last %d", key, last_key);
                handle_key(key, repeating, transalpha);
                record(main, "Did key %d last %d", key, last_key);

                // Redraw the LCD unless there is some type-ahead
                // key_empty = 1 si buffer vide
                if (key_empty() && !ui.showing_graphics())
                    redraw_lcd(false);
            }

            // Record the last keystroke
            last_keystroke_time = program::read_time();
            record(main, "Last keystroke time %u", last_keystroke_time);
        }
        else
        {
            // Blink the cursor
            if (!graphics && sys_timer_timeout(TIMER1))
                redraw_periodics();
            if (!key)
                sys_timer_disable(TIMER0);
        }
#if SIMULATOR && !WASM
        if (tests::running && test_command && key_empty())
            process_test_commands();
#endif // SIMULATOR && !WASM
    }
}





extern "C" void DBx_Task_App()
// ----------------------------------------------------------------------------
//   DMCP main entry point and main loop
// ----------------------------------------------------------------------------
{
   OS_TASKEVENT Db48xEvents;
   int64_t t_work;
   int64_t t_wait;
   bool repeating;
   char buff[80] = {"xxx"};
   st_key_data drcvd;
   int  key        = -1;
   uint32_t key_tmp, key_p1, key_p2, key_p3, keybdata;
   uint32_t wt_sleeping = 60000;        // each minute for time refresh
   bool transalpha = false;
   bool key_release = false;

   OS_EVENT_GetBlocked(&EV_db_lcd_kbd_ready);


//   bool res_init_sram = bkSRAM_Init();
//   RTT_vprintf_cr_time( "Check Sram Backup :  %s, struct kbd %d", res_init_sram ? "ok":"initialized", sizeof(drcvd));
//   OS_TASK_Delay( 500);
//   bkSRAM_ReadString(1, buff, sizeof(buff));
   RTT_vprintf_cr_time( "Bkp string n1 : %s\n", buff);

//   LCD_Status_t lcd_res = LCD_Sharp_Init(&hlcd, true);
//   RTT_vprintf_cr_time( "LCD status : %s, add : %08x", LCD_Status_Desc[lcd_res], LCD_GetFramebuffer());
//   snprintf(buff, sizeof(buff), "%s v%s %dMhz %dko Lcd %dx%d 49keys H",HARD_NAME, HARD_VERSION, SystemCoreClock/1000000, DB_MEM_SIZE, LCD_WIDTH, LCD_HEIGHT);
//   display_text_12x24(&hlcd, 2, 4, buff,1);

   // Initialization
   program_init();
   
int test = Settings.BLlevel();
   RTT_vprintf_cr_T_F( DBx_Task_App, "BLlevel %d ", test);
   Disp_Set_BL_Level(Settings.BLlevel());

   redraw_lcd(true);
   RTT_vprintf_cr_T_F( DBx_Task_App, "%s : init end", HARD_NAME);

   OS_EVENT_Set(&EV_db_app_ready);
   OS_EVENT_Set(&USB_Start);

   last_keystroke_time = program::read_time();

// Main loop
   t_wait = sys_current_ms_i64();
   while (true)
   {
      bool hadKey = false;
//      OS_TASKEVENT_Set( &TKBD, EV_KPo_EndOfTask);
      Db48xEvents = OS_TASKEVENT_GetTimed( 
//      Db48xEvents = OS_TASKEVENT_GetSingleTimed( 
         EV_DBx_KBD 
         | EV_DBx_LPTIM1
         | EV_DBx_WAKEUP 
         | EV_DBx_PON
         | EV_DBx_USB_CON 
         | EV_DBx_FORMAT 
         | EV_DBx_USB_DIS
         | EV_DBx_RESET, KB_SCRUT_PERIOD*10);  // eqivalent to timer0 repeat

      t_work = sys_current_ms_i64(); 
      // u64 !
      program::sleeping_time += t_work - t_wait;

      if ( EV_DBx_RESET & Db48xEvents)  
      { // reset
         char buff[80];
         snprintf(buff, sizeof(buff), "%s v%s %dMhz %dko Lcd %dx%d 49keys H",HARD_NAME, HARD_VERSION, SystemCoreClock/1000000, DB_MEM_SIZE, LCD_WIDTH, LCD_HEIGHT);
         RTT_vprintf_cr_T_F( DBx_Task_App,  "Db48x reset");
         ui.draw_message("F1 F6 EXIT : reboot", buff);
         redraw_periodics();
         refresh_dirty();
         while(1){
            OS_TASK_Delay(50);
         }
         break;
      }
      if ( EV_DBx_FORMAT & Db48xEvents)  
      { // 
         char buff[80];
         snprintf(buff, sizeof(buff), "%s v%s %dMhz %dko Lcd %dx%d 49keys H",HARD_NAME, HARD_VERSION, SystemCoreClock/1000000, DB_MEM_SIZE, LCD_WIDTH, LCD_HEIGHT);
         RTT_vprintf_cr_T_F( DBx_Task_App,  "Db48x F3 + F4 : format disk");
         ui.draw_message("F3 F4 : format", buff);
         redraw_periodics();
         refresh_dirty();
         while(1){
            OS_TASK_Delay(50);
         }
         
      }
      if ( EV_DBx_PON & Db48xEvents)  
      { // power on
         RTT_vprintf_cr_T_F( DBx_Task_App,  "Reset, redraw ");            
         key = -1;
         key_release = false;
         ui.draw_header();   
         ui.draw_battery(true);
         redraw_lcd(true); // bug two times [exit] ???
      }


      if ( EV_DBx_USB_CON & Db48xEvents)  
      { // usb connected
         redraw_periodics();   
      }
      if ( EV_DBx_USB_DIS & Db48xEvents)  
      { // usb disconnected
         redraw_periodics();   
      }

      if ( EV_DBx_LPTIM1 & Db48xEvents)  
      { 
            RTT_vprintf_cr_T_F( DBx_Task_App,  "wake up from lptim1");     
//         refresh_dirty();

             redraw_periodics();
      }

//      if ( EV_DBx_KBD & Db48xEvents)  
      if  ( 0 == OS_MAILBOX_Get(&Mb_Keyboard, &drcvd))
      { // event keyboard
         hadKey = true;
         key_tmp = keybdata & 0xff;
         if (drcvd.released == true) { // release
            key = 0;
            key_release = true;
         } else {
            key = key_DB_to_DM(drcvd.key);
            key_release = false;
         }
         key_p1 = key_DB_to_DM(drcvd.key1);
         key_p2 = key_DB_to_DM(drcvd.key2);
         key_p3 = key_DB_to_DM(drcvd.key3);         
RTT_vprintf_cr_T_F( DBx_Task_App, "key %02d %02d %02d %02d, %02d, %02d, %02d %s %02d",    drcvd.key3, drcvd.key2, drcvd.key1, drcvd.key, key_p1, key_p2, key_p3, key_release ?"Rl":"--", key );

// Check transient alpha mode
            if (key == KEY_UP || key == KEY_DOWN)
            {
                transalpha = true;
            }
            else if (transalpha)
            {
                    transalpha = key_p1 == KEY_UP || key_p1 == KEY_DOWN
                              || key_p2 == KEY_UP || key_p2 == KEY_DOWN;
            }
      } // end get key event
      
      // repeating after TIMER0
      repeating = key > 0
        && sys_timer_active(TIMER0)
        && sys_timer_timeout(TIMER0);

      if (repeating )
      {
         hadKey = true;
         record(main, "Repeating key %d", key);
      }

      // Check if we are displaying a graphic image - If so wait for key
      bool graphics = ui.showing_graphics();
      if (graphics && hadKey)
      {
         if (key > 0)
         {
             record(tests_rpl, "Clearing graphics from key %d", key);
             RTT_vprintf_cr_T_F(DBx_Task_App, "Clearing graphics from key %d", key);
             ui.show_graphics(false);
             redraw_lcd(true);
         }
      }

//    Fetch the key (<0: no key event, >0: key pressed, 0: key released)
//    record(main, "Testing key %d (%+s)", key, hadKey ? "had" : "nope");
      if (key >= 0 && hadKey)
      { // process key
         if (!graphics)
         {
            RTT_vprintf( " ==> %s %s", repeating ?"Rp":"--", transalpha ?"Ta":"--");
            record(main, "Handle key %d last %d", key, last_key);
            StopMode2_disable = true;
            // update rate for cursor
            set_timer(TIMER2, 10);
            handle_key(key, repeating, transalpha);
            hadKey = false;
   // allow power stop ?  
            record(main, "Did key %d last %d", key, last_key);
            RTT_vprintf_cr_T_F(DBx_Task_App, "Did key %d last %d", key, last_key);
            program::run_cycles++;
   //         program::active_time += sys_current_ms() - tin;
               // Redraw the LCD unless there is some type-ahead
            if (key_empty() && !ui.showing_graphics())
               redraw_lcd(false);
// les graphiques ne sont pas complets
            else  if (key_empty()) lcd_forced_refresh();

         }
            // Record the last keystroke
         last_key = key;
         last_keystroke_time = program::read_time();
         record(main, "Last keystroke time %u", last_keystroke_time);
      }
      else
      {  // kill auto-repeat timer
         if (!key)
             sys_timer_disable(TIMER0);
      }
      t_wait = sys_current_ms_i64();
      program::active_time +=  t_wait - t_work;
      if (!key)
         StopMode2_disable = false;

      sys_timer_disable(TIMER2);

#if SIMULATOR && !WASM
        if (tests::running && test_command && key_empty())
            process_test_commands();
#endif // SIMULATOR && !WASM
   }
}



