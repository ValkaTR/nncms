// #############################################################################
// Source file: array.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "memory.h"
#include "array.h"

// #############################################################################
// includes of system headers
//

// #############################################################################
// global variables
//

#define MIN_ARRAY_SIZE  16

#undef	MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// #############################################################################
// functions

struct NNCMS_ARRAY *array_new( size_t elt_size, size_t reserved_size )
{
    struct NNCMS_ARRAY *array = malloc( sizeof(struct NNCMS_ARRAY) );

    array->data = NULL;
    array->len = 0;

    array->alloc = 0;
    array->elt_size = elt_size;
    if( reserved_size != 0 )
        array_maybe_expand( array, reserved_size );

    return array;  
}

// #############################################################################

void array_destroy( struct NNCMS_ARRAY *array )
{
    free( array );
}

// #############################################################################

/* Returns the smallest power of 2 greater than n, or n if
 * such power does not fit in a guint
 */
static size_t g_nearest_pow( size_t num )
{
    size_t n = 1;

    while( n < num && n > 0 )
        n <<= 1;

    return n ? n : num;
}

// #############################################################################

static void array_maybe_expand( struct NNCMS_ARRAY *array, size_t len )
{
    size_t want_alloc = array_elt_len( array, array->len + len );

    if( want_alloc > array->alloc )
    {
        want_alloc = nearest_pow( want_alloc );
        want_alloc = MAX( want_alloc, MIN_ARRAY_SIZE );

        array->data = realloc( array->data, want_alloc );
        
        memset( array->data + array->alloc, 0, want_alloc - array->alloc );

        array->alloc = want_alloc;
    }
}

// #############################################################################
