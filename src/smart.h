// #############################################################################
// Header file: smart.h

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
#include <stdbool.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __smart_h__
#define __smart_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_BUFFER
{
    // Buffer info
    char *lpBuffer;
    size_t nSize;

    // This variable is the main purpose of smart buffers:
    // we save position of null terminating character
    // so we can catcenate really fast and safer than strncat.
    size_t nPos;
};

// #############################################################################
// function declarations
//

bool smart_cat( struct NNCMS_BUFFER *buffer, char *lpszString );
bool smart_copy( struct NNCMS_BUFFER *dst, struct NNCMS_BUFFER *src );
void smart_reset( struct NNCMS_BUFFER *buffer );
bool smart_append_file( struct NNCMS_BUFFER *buffer, char *szFileName );

// #############################################################################

#endif // __smart_h__

// #############################################################################
