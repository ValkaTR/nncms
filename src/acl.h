// #############################################################################
// Header file: acl.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//



#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#  include <getopt.h>
#else
#  include <sys/time.h>
#endif

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __acl_h__
#define __acl_h__

// #############################################################################
// type and constant definitions
//
#define NNCMS_aclNAME_LEN_MAX   128
#define NNCMS_aclPERM_LEN_MAX   128

struct NNCMS_ACL_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *name;
    char *subject;
    char *perm;
    char *grant;
    char *value[NNCMS_COLUMNS_MAX - 5];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

// Modular functions
bool acl_global_init( );
bool acl_global_destroy( );
bool acl_local_init( struct NNCMS_THREAD_INFO *req );
bool acl_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void acl_add( struct NNCMS_THREAD_INFO *req );
void acl_edit( struct NNCMS_THREAD_INFO *req );
void acl_delete( struct NNCMS_THREAD_INFO *req );
void acl_list( struct NNCMS_THREAD_INFO *req );
void acl_view( struct NNCMS_THREAD_INFO *req );

// Check if 'subject' has permission
bool acl_test_perm( struct NNCMS_THREAD_INFO *req, char *name, char *subject, char *permission );
bool acl_check_perm( struct NNCMS_THREAD_INFO *req, char *lpname, char *subject, char *permission );

// #############################################################################

#endif // __acl_h__

// #############################################################################
