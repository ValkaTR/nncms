// #############################################################################
// Header file: user.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// Best viewed without word wrap, cuz this file contains very long lines.
// #############################################################################
// rofl includes of system headers
//

#include <config.h>

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

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "threadinfo.h"
#include "sha2.h"

// #############################################################################

#ifndef __user_h__
#define __user_h__

// #############################################################################
// type and constant definitions
//

#define NNCMS_NICKNAME_LEN          128
#define NNCMS_PASSWORD_LEN          256
#define NNCMS_EMAIL_LEN             128
#define NNCMS_SESSION_ID_LEN        128

// #############################################################################
// function declarations
//

// Module
bool user_init( struct NNCMS_THREAD_INFO *req );
bool user_deinit( struct NNCMS_THREAD_INFO *req );

// Functions
char *user_block( struct NNCMS_THREAD_INFO *req );

// Pages
void user_register( struct NNCMS_THREAD_INFO *req ); // Registration page
void user_login( struct NNCMS_THREAD_INFO *req ); // Login page
void user_logout( struct NNCMS_THREAD_INFO *req );
void user_sessions( struct NNCMS_THREAD_INFO *req );
void user_view( struct NNCMS_THREAD_INFO *req );
void user_profile( struct NNCMS_THREAD_INFO *req );
void user_edit( struct NNCMS_THREAD_INFO *req );
void user_delete( struct NNCMS_THREAD_INFO *req );

// Session
void user_check_session( struct NNCMS_THREAD_INFO *req );
void user_reset_session( struct NNCMS_THREAD_INFO *req );
bool user_xsrf( struct NNCMS_THREAD_INFO *req );

// #############################################################################
// variables

// How often check and remove old sessions
extern int tSessionExpires;
extern int tCheckSessionInterval;

// #############################################################################

#endif // __user_h__

// #############################################################################
