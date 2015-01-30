// #############################################################################
// Header file: memory.h

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
#include <string.h>
#include <glib.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __memory_h__
#define __memory_h__

// #############################################################################
// type and constant definitions
//

extern GArray *global_garbage;

#define DUMP(lpStr) \
    FILE *pDumpFile = fopen( "/srv/http/logs/" #lpStr ".txt", "w+" ); \
    if( pDumpFile != 0 ) \
    { \
        fwrite( lpStr , strlen( lpStr ), 1, pDumpFile ); \
        fclose( pDumpFile ); \
    }

enum MEMORY_TYPE
{
    MEMORY_TYPE_UNKNOWN = 0,
    MEMORY_TYPE_MALLOC,
    MEMORY_TYPE_G_ALLOC,
};

struct ALLOC_ENTRY
{
    // Memory info
    void *lpMem;
    size_t nSize;

    // Source info
    const char *szString;
    const char *szFile;
    unsigned int nLine;
    const char *pretty_function;
    
    // If cached == true, then leak is already reported
    bool cached;
    
    // Type. Memory by g_alloc should be freed by g_free
    // Memory by malloc should be freed by free
    // This variable will help to detect mixing
    enum MEMORY_TYPE type;
};

enum MEMORY_GARBAGE_TYPE
{
    MEMORY_GARBAGE_UNKNOWN = 0,
    MEMORY_GARBAGE_FREE,
    MEMORY_GARBAGE_GFREE,
    MEMORY_GARBAGE_DB_FREE,
    MEMORY_GARBAGE_NODE_FREE,
    MEMORY_GARBAGE_GSTRING_FREE,
    MEMORY_GARBAGE_GARRAY_FREE,
    MEMORY_GARBAGE_GTREE_FREE,
    MEMORY_GARBAGE_MIME_STREAM_CLOSE,
    MEMORY_GARBAGE_MIME_PART_ITER_FREE,
    MEMORY_GARBAGE_OBJECT_UNREF,
    MEMORY_GARBAGE_BYTE_ARRAY_UNREF
};

struct MEMORY_GARBAGE
{
    //struct NNCMS_THREAD_INFO *req;
    void *data;
    enum MEMORY_GARBAGE_TYPE type;
};

// #############################################################################
// function declarations
//

GArray *garbage_create( );
void garbage_destroy( GArray *garbage );
void *garbage_add( GArray *garbage, void *data, enum MEMORY_GARBAGE_TYPE type );
void garbage_free( GArray *garbage );

// Modular functions
bool memory_global_init( );
bool memory_global_destroy( );
bool memory_local_init( struct NNCMS_THREAD_INFO *req );
bool memory_local_destroy( struct NNCMS_THREAD_INFO *req );

void memory_leaks( bool report );

// Duplicate memory
void *memdup( void *data, size_t n );
void *memdup_temp( struct NNCMS_THREAD_INFO *req, void *data, size_t n );

// #############################################################################

void memory_safe_free( void *lpMem, const char *lpname, const char *szFile, unsigned int nLine, const char *pretty_function );

// Replacements
void *memory_xmalloc(               size_t nSize,                           const char *szString, const char *szFile, unsigned int nLine, const char *pretty_function );
void memory_xfree(     void *lpMem,                                         const char *szString, const char *szFile, unsigned int nLine, const char *pretty_function );
void *memory_xrealloc( void *lpMem, size_t nSize, const char *szSizeString, const char *szString, const char *szFile, unsigned int nLine, const char *pretty_function );


#ifndef NDEBUG

#define MALLOC(nSize)         memory_xmalloc(         nSize, #nSize,         __FILE__, __LINE__, __PRETTY_FUNCTION__ );
#define FREE(lpMem)           memory_xfree(    lpMem,                #lpMem, __FILE__, __LINE__, __PRETTY_FUNCTION__ );
#define SAFE_FREE(lpMem) memory_safe_free(     lpMem,                #lpMem, __FILE__, __LINE__, __PRETTY_FUNCTION__ );
#define REALLOC(lpMem, nSize) memory_xrealloc( lpMem, nSize, #nSize, #lpMem, __FILE__, __LINE__, __PRETTY_FUNCTION__ );

#else

//#define MALLOC(nSize) g_slice_alloc( nSize );
//#define FREE(lpMem) g_slice_free1( lpMem );

#define MALLOC(nSize) g_malloc( nSize );
#define FREE(lpMem) g_free( lpMem );

#define REALLOC(lpMem, nSize) g_realloc( lpMem, nSize );
#define SAFE_FREE(lpMem) if( lpMem ) g_free( lpMem );

#endif // NDEBUG

gboolean memory_ghrfunc( gpointer key, gpointer value, gpointer user_data );
void memory_ghrfunc_report( gpointer key, gpointer value, gpointer user_data );

// #############################################################################

#endif // __memory_h__

// #############################################################################
