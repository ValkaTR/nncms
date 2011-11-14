// #############################################################################
// Header file: asprintf.h

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __asprintf_h__
#define __asprintf_h__

// #############################################################################
// type and constant definitions
//

// #############################################################################
// function declarations
//

#ifndef HAVE_ASPRINTF
int asprintf(char **str, char *fmt, ...);
#endif

// #############################################################################

#endif // __asprintf_h__

// #############################################################################
