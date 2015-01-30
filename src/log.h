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
#include "template.h"

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

struct NNCMS_LOG_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    /* 01 */ char *id;
    /* 02 */ char *priority;
    /* 03 */ char *text;
    /* 04 */ char *timestamp;
    /* 05 */ char *user_id;
    /* 06 */ char *user_host;
    /* 07 */ char *user_agent;
    /* 08 */ char *uri;
    /* 09 */ char *referer;
    /* 10 */ char *function;
    /* 11 */ char *src_file;
    /* 12 */ char *src_line;
    char *value[NNCMS_COLUMNS_MAX - 12];

    struct NNCMS_ROW *next; // BC
};

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
    enum NNCMS_PRIORITY priority;
    char *lpszPriorityANSI;
    char *lpszPriority;
    char *lpszIcon;
    char *lpszHtmlFgColor;
    char *lpszHtmlBgColor;
};

struct NNCMS_LOG
{
    GString *text;
    enum NNCMS_PRIORITY priority;
    time_t timestamp;
};

// Modular
bool log_global_init( );
bool log_global_destroy( );
bool log_local_init( struct NNCMS_THREAD_INFO *req );
bool log_local_destroy( struct NNCMS_THREAD_INFO *req );

// Logging
struct NNCMS_PRIORITY_INFO *log_get_priority( enum NNCMS_PRIORITY priority );
void log_debug_printf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY priority, char *format, const char *function, const char *src_file, int src_line, ... );
void log_displayf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY priority, char *format, ... );
void log_vdisplayf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY priority, char *i18n_id, struct NNCMS_VARIABLE *vars );
struct NNCMS_LOG *log_session_get_messages( struct NNCMS_THREAD_INFO *req );
void log_session_clear_messages( struct NNCMS_THREAD_INFO *req );

// __FUNCTION__ is another name for __func__. Older versions of GCC recognize
// only this name. However, it is not standardized. For maximum portability, we
// recommend you use __func__, but provide a fallback definition with the
// preprocessor
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define log_print(req, priority, format) log_debug_printf( req, priority, format, __FUNCTION__, __FILE__, __LINE__ );
#define log_printf(req, priority, format, ...) log_debug_printf( req, priority, format, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__ );
//#define log_debug_print(req, priority, format) log_printf( req, priority, "[" __FUNCTION__ " at " __FILE__ ":" __LINE__ "]" format )
//#define log_debug_printf(req, priority, format, ...) log_printf( req, priority, "[" __FUNCTION__ " at " __FILE__ ":" __LINE__ "]" format, __VA_ARGS__ )

// Pages
void log_view( struct NNCMS_THREAD_INFO *req );
void log_clear( struct NNCMS_THREAD_INFO *req );
void log_list( struct NNCMS_THREAD_INFO *req );

char *log_links( struct NNCMS_THREAD_INFO *req, char *log_id );

// #################################################################################

#endif // __log_h__

// #################################################################################
