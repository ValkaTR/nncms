// #############################################################################
// Header file: array.h

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

#ifndef __array_h__
#define __array_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_ARRAY
{
    void *data;
    size_t len;

    size_t alloc;
    size_t elt_size;
    unsigned int zero_terminated : 1;
    unsigned int clear : 1;
};

#define array_elt_len(array,i) ((array)->elt_size * (i))
#define array_elt_pos(array,i) ((array)->data + g_array_elt_len((array),(i)))

// #############################################################################
// function declarations
//

struct NNCMS_ARRAY *array_new( size_t elt_size, size_t reserved_size );

// #############################################################################

#endif // __array_h__

// #############################################################################
