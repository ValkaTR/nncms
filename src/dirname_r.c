// #############################################################################
// Source file: dirname_r.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "dirname_r.h"

// #############################################################################
// includes of system headers
//

#include <string.h>
#include <stdlib.h>

#ifdef WIN32

#include <io.h>
#include <windows.h>
#include <winbase.h>

#define ftruncate _chsize
#define srandom(a) srand(GetTickCount())
#define random()  rand()
#define DIR_SEPARATOR    '\\'

#else

#define DIR_SEPARATOR    '/'

#endif

// #############################################################################
// global variables
//

// #############################################################################
// functions

char *dirname_r( char *filename )
{
    char *lastsep;
    char *dname;
    long len;

    if( ( lastsep = strrchr( filename, DIR_SEPARATOR ) ) )
    {
        len = lastsep - filename;
        dname = malloc( len + 1 );
        memcpy( dname, filename, len );
        dname[len] = '\0';
    }
    else
    {
        dname = malloc( 2 );
        dname[0] = '.';
        dname[1] = '\0';
    }

    return dname;
}

// #############################################################################
