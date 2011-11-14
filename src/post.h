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
#include "smart.h"

// #############################################################################

#ifndef __post_h__
#define __post_h__

// #############################################################################
// type and constant definitions
//

#define POST_CHECK_PERM_DEPTH       100

// Maximum tree depth for "reply, modify, remove" actions
extern int nPostDepth;

// #############################################################################
// function declarations
//

// Module
bool post_init( struct NNCMS_THREAD_INFO *req );
bool post_deinit( struct NNCMS_THREAD_INFO *req );

// Pages
void post_add( struct NNCMS_THREAD_INFO *req );
void post_board( struct NNCMS_THREAD_INFO *req );
void post_edit( struct NNCMS_THREAD_INFO *req );
void post_modify( struct NNCMS_THREAD_INFO *req );
void post_news( struct NNCMS_THREAD_INFO *req );
void post_delete( struct NNCMS_THREAD_INFO *req );
void post_topics( struct NNCMS_THREAD_INFO *req );
void post_view( struct NNCMS_THREAD_INFO *req );
void post_remove( struct NNCMS_THREAD_INFO *req );
void post_reply( struct NNCMS_THREAD_INFO *req );
void post_root( struct NNCMS_THREAD_INFO *req );
void post_rss( struct NNCMS_THREAD_INFO *req );

// Functions
bool post_add_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, char *lpszPostParent );
bool post_reply_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, char *lpszPostParent );
bool post_check_perm( struct NNCMS_THREAD_INFO *req, char *lpszPostId, char *lpszPerm );
bool post_tree_check_perm( struct NNCMS_THREAD_INFO *req, char *lpszPostId, char *lpszPerm, int nDepth );
void post_get_post( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, enum TEMPLATE_INDEX tempNum, sqlite3_stmt *stmt );
void post_get_tree_post( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, enum TEMPLATE_INDEX tempNum, sqlite3_stmt *stmt );
size_t post_make_frame( struct NNCMS_THREAD_INFO *req, char *lpDest, struct NNCMS_BUFFER *smartBuffer, enum TEMPLATE_INDEX tempNum, sqlite3_stmt *stmt );

// #############################################################################

#endif // __post_h__

// #############################################################################
