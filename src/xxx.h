// #############################################################################
// Header file: xxx.h

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
#include "database.h"

// #############################################################################

#ifndef __xxx_h__
#define __xxx_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_XXX_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *data;
    char *value[NNCMS_COLUMNS_MAX - 2];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

// Modular functions
bool xxx_global_init( );
bool xxx_global_destroy( );
bool xxx_local_init( struct NNCMS_THREAD_INFO *req );
bool xxx_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void xxx_list( struct NNCMS_THREAD_INFO *req );
void xxx_add( struct NNCMS_THREAD_INFO *req );
void xxx_edit( struct NNCMS_THREAD_INFO *req );
void xxx_view( struct NNCMS_THREAD_INFO *req );
void xxx_delete( struct NNCMS_THREAD_INFO *req );

// Functions
char *xxx_links( struct NNCMS_THREAD_INFO *req, char *xxx_id );

// #############################################################################

#endif // __xxx_h__

// #############################################################################
