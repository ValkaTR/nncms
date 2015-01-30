// #############################################################################
// Header file: group.h

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

#ifndef __group_h__
#define __group_h__

// #############################################################################
// type and constant definitions
//

// Special groups
extern char *admin_group_id;
extern char *user_group_id;
extern char *guest_group_id;
extern char *nobody_group_id;
extern char *owner_group_id;

struct NNCMS_GROUP_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *name;
    char *description;
    char *value[NNCMS_COLUMNS_MAX - 3];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_USERGROUP_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *user_id;
    char *group_id;
    char *value[NNCMS_COLUMNS_MAX - 3];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

// Modular functions
bool group_global_init( );
bool group_global_destroy( );
bool group_local_init( struct NNCMS_THREAD_INFO *req );
bool group_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void group_list( struct NNCMS_THREAD_INFO *req );
void group_add( struct NNCMS_THREAD_INFO *req );
void group_edit( struct NNCMS_THREAD_INFO *req );
void group_view( struct NNCMS_THREAD_INFO *req );
void group_delete( struct NNCMS_THREAD_INFO *req );
void ug_list( struct NNCMS_THREAD_INFO *req );
void ug_add( struct NNCMS_THREAD_INFO *req );
void ug_edit( struct NNCMS_THREAD_INFO *req );
void ug_view( struct NNCMS_THREAD_INFO *req );
void ug_delete( struct NNCMS_THREAD_INFO *req );

// Functions
char *ug_list_html( struct NNCMS_THREAD_INFO *req, char *user_id, char *group_id );

char *group_links( struct NNCMS_THREAD_INFO *req, char *group_id );
char *ug_links( struct NNCMS_THREAD_INFO *req, char *ug_id );

// #############################################################################

#endif // __group_h__

// #############################################################################
