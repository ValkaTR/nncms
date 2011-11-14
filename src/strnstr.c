// #############################################################################
// Source file: strnstr.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "strnstr.h"

// #############################################################################
// includes of system headers
//

#include <string.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

char *strnstr( char *lpszHaystack, char *lpszNeedle, unsigned int nLen )
{
    unsigned int nNeedleLen = strlen( lpszNeedle );
    while( nLen > 0 )
    {
        if( strncmp( lpszHaystack, lpszNeedle, nNeedleLen ) == 0 )
            return lpszHaystack;

        nLen--; lpszHaystack++;
    }

    return 0;
}

// #############################################################################
