// #############################################################################
// Source file: asprintf.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "asprintf.h"

// #############################################################################
// includes of system headers
//

#include <stdarg.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

#ifndef HAVE_ASPRINTF
int asprintf( char **str, char *fmt, ... )
{
    va_list vl;
    char *allocated = NULL;
    int to_allocate = 0;

    va_start( vl, fmt );

    to_allocate = vsnprintf( NULL, 0, fmt, vl );

    allocated = (char *) malloc( to_allocate + 1 );

    if( !allocated )
    {
        return -1;
    }

    vsnprintf( allocated, to_allocate + 1, fmt, vl );

    va_end( vl );

    *str = allocated;
    return to_allocate;
}
#endif

// #############################################################################
