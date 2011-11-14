// #################################################################################
// Header file: log.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #################################################################################
// includes of system headers
//

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#  include <getopt.h>
#else
#  include <sys/time.h>
#endif

// #################################################################################
// includes of local headers
//

#include "main.h"

// #################################################################################

#ifndef __log_h__
#define __log_h__

// #################################################################################
// type and constant definitions
//

// ANSI/VT100 Terminal Standart Escape Codes
#define TERM_ESCAPE     "" // Hex 0x1B // Dec: 27

#define TERM_RESET      TERM_ESCAPE "[0m"
#define TERM_BRIGHT     TERM_ESCAPE "[1m"
#define TERM_UNDERSCORE TERM_ESCAPE "[4m"
#define TERM_BLINK      TERM_ESCAPE "[5m"
#define TERM_REVERSE    TERM_ESCAPE "[7m"

#define TERM_FG_BLACK   TERM_ESCAPE "[30m"
#define TERM_FG_RED     TERM_ESCAPE "[31m"
#define TERM_FG_GREEN   TERM_ESCAPE "[32m"
#define TERM_FG_YELLOW  TERM_ESCAPE "[33m"
#define TERM_FG_BLUE    TERM_ESCAPE "[34m"
#define TERM_FG_MAGENTA TERM_ESCAPE "[35m"
#define TERM_FG_CYAN    TERM_ESCAPE "[36m"
#define TERM_FG_WHITE   TERM_ESCAPE "[37m"

#define TERM_BG_BLACK   TERM_ESCAPE "[40m"
#define TERM_BG_RED     TERM_ESCAPE "[41m"
#define TERM_BG_GREEN   TERM_ESCAPE "[42m"
#define TERM_BG_YELLOW  TERM_ESCAPE "[43m"
#define TERM_BG_BLUE    TERM_ESCAPE "[44m"
#define TERM_BG_MAGENTA TERM_ESCAPE "[45m"
#define TERM_BG_CYAN    TERM_ESCAPE "[46m"
#define TERM_BG_WHITE   TERM_ESCAPE "[47m"

// Path to log file
extern char szLogFile[NNCMS_PATH_LEN_MAX];

// #################################################################################
// function declarations
//

#define NNCMS_MAX_LOG_TEXT      256

// Priorities
enum NNCMS_PRIORITY
{
    LOG_UNKNOWN = 0,
    LOG_DEBUG = 10,
    LOG_INFO = 20,
    LOG_ACTION = 25,
    LOG_NOTICE = 30,
    LOG_WARNING = 40,
    LOG_ERROR = 50,
    LOG_CRITICAL = 60,
    LOG_ALERT = 70,
    LOG_EMERGENCY = 80
};

// This hold info about priority
struct NNCMS_PRIORITY_INFO
{
    enum NNCMS_PRIORITY nPriority;
    char *lpszPriorityANSI;
    char *lpszPriority;
    char *lpszIcon;
    char *lpszHtmlFgColor;
    char *lpszHtmlBgColor;
};

// Modular
bool log_init( struct NNCMS_THREAD_INFO *req );
bool log_deinit( struct NNCMS_THREAD_INFO *req );

// Logging
struct NNCMS_PRIORITY_INFO *log_get_priority( enum NNCMS_PRIORITY nPriority );
void log_printf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY nPriority, char *lpszFormat, ... );

// Pages
void log_view( struct NNCMS_THREAD_INFO *req );
void log_clear( struct NNCMS_THREAD_INFO *req );

// #################################################################################

#endif // __log_h__

// #################################################################################
