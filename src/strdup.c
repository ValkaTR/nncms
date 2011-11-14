// #############################################################################
// Source file: strdup.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "strdup.h"
#include "main.h"
#include "memory.h"
#include "strlcpy.h"

// #############################################################################
// includes of system headers
//

// #############################################################################
// global variables
//

// #############################################################################
// functions

char *strdup_d( char *lpszString )
{
    if( lpszString == 0 )
        return 0;

    size_t nTextSize = strlen( lpszString ) + 1;
    char *lpszOutput = MALLOC( nTextSize + 1 );
    strlcpy( lpszOutput, lpszString, nTextSize );
    return lpszOutput;
}

// #############################################################################
