// #############################################################################
// Header file: cache.h

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

#include <string.h>
#ifdef _WIN32
#  include <getopt.h>
#else
#  include <sys/time.h>
#endif

#include <stdbool.h>
#include <fam.h>

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "threadinfo.h"

// #############################################################################

#ifndef __cache_h__
#define __cache_h__

// #############################################################################
// type and constant definitions
//

#define NNCMS_CACHE_MEM_MAX         128

struct NNCMS_CACHE
{
    // Name of cache entry
    char lpszName[NNCMS_PATH_LEN_MAX];
    time_t timeStamp;

    // Data info
    char *lpData;
    size_t nSize;
    time_t fileTimeStamp;
};

// #############################################################################
// function declarations
//

bool cache_init( );
bool cache_deinit( );

void cache_recursive_monitor( FAMConnection *fc, char *path );
static void *cache_fam( void *var );

struct NNCMS_CACHE *cache_find( struct NNCMS_THREAD_INFO *req, char *lpszName );
struct NNCMS_CACHE *cache_store( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpData, size_t nSize );

// Pages
void cache_admin( struct NNCMS_THREAD_INFO *req );

// #############################################################################

#endif // __cache_h__

// #############################################################################
