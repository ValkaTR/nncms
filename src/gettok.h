// #############################################################################
// Header file: gettok.h

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

#ifndef __gettok_h__
#define __gettok_h__

// #############################################################################
// type and constant definitions
//

// #############################################################################
// function declarations
//

void gettok( char *lpszSrc, char *lpszDst, int nIndex, char chDelimiter );

// #############################################################################

#endif // __gettok_h__

// #############################################################################
