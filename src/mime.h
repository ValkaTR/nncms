// #############################################################################
// Header file: mime.h

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

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

#include "sha2.h"

// #############################################################################

#ifndef __mime_h__
#define __mime_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_MIME {
    char *extension;
    char *mimetype;
};

// #############################################################################
// function declarations
//

// Module functions
bool mime_global_init( );
bool mime_global_destroy( );

char *get_extension( char *fpath );
char *get_mime( char *fpath );

// #############################################################################

#endif // __mime_h__

// #############################################################################
