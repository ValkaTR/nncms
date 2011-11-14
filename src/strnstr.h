// #############################################################################
// Header file: strnstr.h

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

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __strnstr_h__
#define __strnstr_h__

// #############################################################################
// type and constant definitions
//

// #############################################################################
// function declarations
//

char *strnstr( char *lpszHaystack, char *lpszNeedle, unsigned int nLen );

// #############################################################################

#endif // __strnstr_h__

// #############################################################################
