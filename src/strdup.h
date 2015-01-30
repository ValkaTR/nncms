// #############################################################################
// Header file: strdup.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//



#include <stdbool.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __strdup_h__
#define __strdup_h__

// #############################################################################
// type and constant definitions
//

#ifdef strdup
#undef strdup
#endif // strdup

// #############################################################################
// function declarations
//

char *strdup_d( char *lpszString );

// #############################################################################

#endif // __strdup_h__

// #############################################################################
