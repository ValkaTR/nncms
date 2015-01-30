// #############################################################################
// Header file: i18n.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __i18n_h__
#define __i18n_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_LOCALES_TARGET_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *lid;
    char *language;
    char *translation;
    char *value[NNCMS_COLUMNS_MAX - 4];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_LOCALES_SOURCE_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *location;
    char *textgroup;
    char *source;
    char *value[NNCMS_COLUMNS_MAX - 4];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_LANGUAGE_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *name;
    char *native;
    char *prefix;
    char *value[NNCMS_COLUMNS_MAX - 4];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

char *i18n_string( struct NNCMS_THREAD_INFO *req, char *format, va_list args );
char *i18n_string_temp( struct NNCMS_THREAD_INFO *req, char *i18n_name, struct NNCMS_VARIABLE *vars );
void i18n_detect_language( struct NNCMS_THREAD_INFO *req );

// Modular functions
bool i18n_global_init( );
bool i18n_global_destroy( );
bool i18n_local_init( struct NNCMS_THREAD_INFO *req );
bool i18n_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void i18n_view( struct NNCMS_THREAD_INFO *req );
void i18n_edit( struct NNCMS_THREAD_INFO *req );

// #############################################################################

#endif // __i18n_h__

// #############################################################################
