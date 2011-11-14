// #############################################################################
// Header file: cfg.h

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

#include "threadinfo.h"
#include "sha2.h"

// #############################################################################

#ifndef __cfg_h__
#define __cfg_h__

// #############################################################################
// type and constant definitions
//

#define NNCMS_CFG_OPTIONS_MAX       128

enum NNCMS_TYPES
{
    NNCMS_INTEGER = 1,
    NNCMS_STRING,
    NNCMS_BOOL
};

struct NNCMS_OPTION
{
    char szName[64];        // Option name
    enum NNCMS_TYPES nType; // Option type
    void *lpMem;            // Location of variable in memory
    //void *lpDefault;      // Default value if option isn't in config
};

// #############################################################################
// function declarations
//

// Pages
void cfg_view( struct NNCMS_THREAD_INFO *req );
void cfg_edit( struct NNCMS_THREAD_INFO *req );

// Parser
bool cfg_parse_file( const char *lpszFileName, struct NNCMS_OPTION *lpOptions );

// #############################################################################

#endif // __cfg_h__

// #############################################################################
