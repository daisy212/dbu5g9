ReadMe.txt for the ST STM32U5G9 start project

This project was built for SEGGER Embedded Studio V8.10d.

Supported hardware:
===================
The sample project for the STM32U5G9 is prepared to run on an ST STM32U5G9J-DK2 board.
Using different target hardware may require modifications.

Configurations:
===============
- Debug:
  This configuration is prepared for download into internal Flash using J-Link.
  An embOS debug and profiling library is used.

- Release:
  This configuration is prepared for download into internal Flash using J-Link.
  An embOS release library is used.

16/03/2026 : porting db48x to u5g9

16/03 : emfile ok en spi
17/03 : afficher ltdc ok
ram2 16k : low power ram

22/03/26 : passage en version v1b
Lcd 3.5'' 640x480 : ok, polarity Vsync & Hsync
Backlight PWM PE6, diminuer le courant maxi ? r=24ohms, 12.5mA 17v, 200mW
04/2026 : réglage valeur pwm : ok avec timer3, menu : ok, combinaison touches ?

To Do :

Bug :
   usb detect (   lptim1)
   couleurs manquantes ? ou se passe la convertion 16 ==> 24


multiple prevailing defs for '__ct_base ' : classe definie deux fois


15/04/2026 : V1.d : passage en spifi avec driver Segger, ok
04/206 : ip over usb, sntp, ftp server, ok
01/05/2026 : new keyboard
04/05/2026 : partial update using dma2d. Collatzbenchmark : 8160msec ==> 4580msec, 4436 with inline, not waiting end of transfert
correction bug sys_info

05/05/2026 : passage à version 1f
To do :
   partial refresh in setup : ok
   suppression rtc           : ok
   ajout lcd power off
   reinit lcd
 
Unification : VGA_3.5, Sharp_3.2 et Sharp_2.7
ou bien choix DMA, LTDC, puis taille ?
Nettoyage drastique dmcp.h, passage DBxxxx en .cpp

DBxxxx ---> Display ---> u5g_ltdc, include "ltdc_TFT035_7.h"
                    ---> u5_spi, include "Sharp2.7" ou "Sharp3.2"

ToDo : sharp

Local time :
London, Paris, Berlin, Caire, Moscou, Dubai, HongKong, Sisney, Noumea, Anchorage, LosAngeles, Denver, Chicago, NewYork, Rio de Janeiro


A directory has the following structure:
//        Directory { Name1 Value1 Name2 Value2 ... }

Directory {
   Physics
      Directory {
