/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : FS_Conf.h
Purpose : File system configuration
*/

#ifndef FS_CONF_H           // Avoid multiple inclusion.
#define FS_CONF_H

#ifdef DEBUG
  #if (DEBUG)
    #define FS_DEBUG_LEVEL               5
    #define FS_SUPPORT_PROFILE           1
    #define FS_SUPPORT_PROFILE_END_CALL  1
  #endif
#endif

#define FS_OS_LOCKING          (1)
//
// Necessary for SEGGER Eval Software
//
#define FS_SUPPORT_JOURNAL     (1)
#define FS_SUPPORT_ENCRYPTION  (1)
//
// 20221021: updated
//
#define FS_MMC_SUPPORT_UHS                 1
#define FS_NOR_SUPPORT_CRC                 1
#define FS_SUPPORT_SECTOR_BUFFER_CACHE     1
#define FS_SUPPORT_CHECK_MEMORY            1
#define FS_SUPPORT_DEINIT                  1
#define FS_SUPPORT_EXT_ASCII               1
#define FS_SUPPORT_FILE_NAME_ENCODING      1
#define FS_SUPPORT_MBCS                    1


#endif                      // Avoid multiple inclusion.

/*************************** End of file ****************************/
