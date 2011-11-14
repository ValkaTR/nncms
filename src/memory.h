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

#include "config.h"

#include <stdbool.h>
#include <string.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __memory_h__
#define __memory_h__

// #############################################################################
// type and constant definitions
//

#define DUMP(lpStr) \
    FILE *pDumpFile = fopen( "/var/www/localhost/htdocs/nncms/logs/" #lpStr ".txt", "w+" ); \
    if( pDumpFile != 0 ) \
    { \
        fwrite( lpStr , strlen( lpStr ), 1, pDumpFile ); \
        fclose( pDumpFile ); \
    }

struct SOURCE_INFO
{
    // Source info
    const char *szString;
    const char *szFile;
    unsigned int nLine;
};

struct ALLOC_ENTRY
{
    // Memory info
    void *lpMem;
    size_t nSize;

    bool bUsed;

    struct SOURCE_INFO alloc;
    struct SOURCE_INFO free;

    struct ALLOC_ENTRY *next;
};

// #############################################################################
// function declarations
//

// Modular functions
bool memory_init( );
bool memory_deinit( );

// Set size of garbage collector
void memory_set_size( int nSize );

// #############################################################################

#ifdef DEBUG

// #############################################################################

void memory_safe_free( void *lpMem, const char *lpszName, const char *szFile, unsigned int nLine );

// Hash function
unsigned int memory_hash( void *lpMem, unsigned int nMaxCount );

// Replacements
void *xmalloc( size_t nSize, const char *szString, const char *szFile, unsigned int nLine );
void xfree( void *lpMem, const char *szString, const char *szFile, unsigned int nLine );
void *xrealloc( void *lpMem, size_t nSize, const char *szSizeString, const char *szMemString, const char *szFile, unsigned int nLine );

// Debugging tool
void memory_print_unfreed( );
void main_free_all( );

#define MALLOC(nSize) xmalloc( nSize, #nSize, __FILE__, __LINE__ );
#define FREE(lpMem) xfree( lpMem, #lpMem, __FILE__, __LINE__ );
#define REALLOC(lpMem, nSize) xrealloc( lpMem, nSize, #nSize, #lpMem, __FILE__, __LINE__ );
#define SAFE_FREE(lpMem) memory_safe_free(lpMem, #lpMem, __FILE__, __LINE__ );

// #############################################################################

#else

// #############################################################################

#define MALLOC(nSize) malloc( nSize );
#define FREE(lpMem) free( lpMem );
#define REALLOC(lpMem, nSize) realloc( lpMem, nSize );
#define SAFE_FREE(lpMem) if( lpMem ) free( lpMem );

// #############################################################################

#endif // DEBUG

// #############################################################################

#endif // __memory_h__

// #############################################################################
