// #############################################################################
// Header file: strlcpy.h

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

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __strlcpy_h__
#define __strlcpy_h__

// #############################################################################
// type and constant definitions
//

// #############################################################################
// function declarations
//

size_t strlcpy(char *dst, const char *src, size_t siz);

// #############################################################################

#endif // __strlcpy_h__

// #############################################################################
