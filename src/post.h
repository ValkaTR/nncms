// ##############################################################################
// Header file: post.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// ############################################################################
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

#include <sqlite3.h>

// ############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __post_h__
#define __post_h__

// #############################################################################
// type and constant definitions
//

// Maximum tree depth for "reply, modify, remove" actions
extern int nPostDepth;

enum NNCMS_POST_TYPE
{
    NNCMS_POST_NONE = 0,
    NNCMS_POST_MESSAGE,
    NNCMS_POST_TOPIC,
    NNCMS_POST_SUBFORUM,
    NNCMS_POST_GROUP,
    NNCMS_POST_FORUM,
};

struct NNCMS_POST_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *user_id;
    char *timestamp;
    char *parent_post_id;
    char *subject;
    char *body;
    char *type;
    char *group_id;
    char *mode;
    char *value[NNCMS_COLUMNS_MAX - 9];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

// Module
bool post_global_init( );
bool post_global_destroy( );
bool post_local_init( struct NNCMS_THREAD_INFO *req );
bool post_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void post_add( struct NNCMS_THREAD_INFO *req );
void post_list( struct NNCMS_THREAD_INFO *req );
void post_edit( struct NNCMS_THREAD_INFO *req );
void post_delete( struct NNCMS_THREAD_INFO *req );
void post_view( struct NNCMS_THREAD_INFO *req );

// Functions
void post_get_access( struct NNCMS_THREAD_INFO *req, char *user_id, char *post_id, bool *read_access_out, bool *write_access_out, bool *exec_access_out );
bool post_check_perm( struct NNCMS_THREAD_INFO *req, char *user_id, char *post_id, bool read_check, bool write_check, bool exec_check );

struct NNCMS_FORM *post_add_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_POST_ROW *post_row );
struct NNCMS_FORM *post_view_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_POST_ROW *post_row );
char *post_links( struct NNCMS_THREAD_INFO *req, char *post_id, char *post_parent_post_id );

// #############################################################################

#endif // __post_h__

// #############################################################################
