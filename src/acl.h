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

#include "config.h"

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
#define NNCMS_ACLNAME_LEN_MAX   128
#define NNCMS_ACLPERM_LEN_MAX   128

// #############################################################################
// function declarations
//

// Modular functions
bool acl_init( struct NNCMS_THREAD_INFO *req );
bool acl_deinit( struct NNCMS_THREAD_INFO *req );

// Pages
void acl_add( struct NNCMS_THREAD_INFO *req );
void acl_edit( struct NNCMS_THREAD_INFO *req );
void acl_delete( struct NNCMS_THREAD_INFO *req );
void acl_view( struct NNCMS_THREAD_INFO *req );

// Check if 'subject' has permission
bool acl_check_perm( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszSubject, char *lpszPerm );
bool acl_check_prim_perm( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszSubject, char *lpszPerm );

// #############################################################################

#endif // __acl_h__

// #############################################################################
