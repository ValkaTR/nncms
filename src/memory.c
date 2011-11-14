// #############################################################################
// Source file: memory.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "memory.h"
#include "log.h"

// #############################################################################
// includes of system headers
//

#include <stdlib.h>
#include <pthread.h>

// #############################################################################
// global variables
//

// Size of structure, once initialized should not be changed
int nGarbageCollectorSize = 256;

// Garbage collector main structure
struct ALLOC_ENTRY **garbage_collector = 0;

// Global memory usage count
int nMemCount = 0;

// Garbage collector
pthread_mutex_t garbage_mutex;

// #############################################################################
// functions

// Garbage collector initiliazation
bool memory_init( )
{
    #define TEST_GARBAGE_COLLECTOR  128
    //MALLOC( TEST_GARBAGE_COLLECTOR );

    // Init pthread locking
    if( pthread_mutex_init( &garbage_mutex, 0 ) != 0 ) {
        log_printf( 0, LOG_EMERGENCY, "Main: Garbage lock init failed!!" );
        exit( -1 );
    }

    garbage_collector = malloc( sizeof(struct ALLOC_ENTRY *) * nGarbageCollectorSize );
    if( garbage_collector == 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Main: Garbage collector unable to allocate!!" );
        exit( -1 );
    }

    // Zero the garbage collector structure
    memset( garbage_collector, 0, sizeof(struct ALLOC_ENTRY *) * nGarbageCollectorSize );

    return true;
}

// #############################################################################

// Destroing garbage collector
bool memory_deinit( )
{
#ifdef DEBUG
    memory_print_unfreed( );
    main_free_all( );
#endif

    // Deinit pthread locking
    if( pthread_mutex_destroy( &garbage_mutex ) != 0 ) {
        log_printf( 0, LOG_EMERGENCY, "Quit: Garbage mutex destroy failed!!" );
    }

    if( garbage_collector != 0 )
    {
        free( garbage_collector );
        garbage_collector = 0;
    }

    return true;
}

// #############################################################################

void memory_safe_free( void *lpMem, const char *lpszName, const char *szFile, unsigned int nLine )
{
    if( lpMem != 0 )
    {
        FREE( lpMem );
    }
    else
    {
        log_printf( 0, LOG_DEBUG, "Memory::SafeFree: %s is zero at %s:%u", lpszName, szFile, nLine );
    }
}

// #############################################################################

void memory_set_size( int nSize )
{
    nGarbageCollectorSize = nSize;
}

// #############################################################################

#ifdef DEBUG

// #############################################################################

unsigned int memory_hash( void *lpMem, unsigned int nMaxCount )
{
    return ((((unsigned int) lpMem) / 16) % nMaxCount);
}

// #############################################################################

void *xmalloc( size_t nSize, const char *szString, const char *szFile, unsigned int nLine )
{
    // The allocation
    void *lpMem = malloc( nSize );

    // Error check
    if( lpMem == 0 )
    {
        log_printf( 0, LOG_CRITICAL, "Main::Xmalloc: Unable to allocate memory block %s:%u: MALLOC( %s ): #%05i: lpMem = %08x, nSize = %u",
            szFile, nLine, szString, nMemCount, lpMem, nSize );
    }

    // If garbage collector is not ready, then die
    if( garbage_collector == 0 )
        return lpMem;

    // Get shared garbage collector lock
    if( pthread_mutex_lock( &garbage_mutex ) != 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Main::Xmalloc: Can't acquire lock!!" );
        exit( -1 );
    }

    /*if( nMemCount == nGarbageCollectorSize )
    {
        log_printf( 0, LOG_EMERGENCY, "Main::Xmalloc: Garbage structure will overflow!!" );
    }
    else if( nMemCount > nGarbageCollectorSize )
    {
        log_printf( 0, LOG_EMERGENCY, "Main::Xmalloc: Fuck you!! Garbage structure overflowed!!" );
        exit( -1 );
    }*/

    unsigned int i = memory_hash( lpMem, nGarbageCollectorSize );
    if( garbage_collector[i] == 0 )
    {
        garbage_collector[i] = malloc( sizeof(struct ALLOC_ENTRY) );
        memset( garbage_collector[i], 0, sizeof(struct ALLOC_ENTRY) );
    }
    struct ALLOC_ENTRY *alloc_entry = garbage_collector[i];
    if( alloc_entry->bUsed == true )
    {
        // Debug collision
        unsigned int nCollisionLevel = 1;

        // Recurse
        while( alloc_entry->next != 0 )
        {

            ++nCollisionLevel;
            alloc_entry = alloc_entry->next;
        }

        // Debug recurse collision
        //log_printf( 0, LOG_DEBUG, "Main::Xmalloc: Collision level: %u on #%03u", nCollisionLevel, i );

        // Malloc
        alloc_entry->next = malloc( sizeof(struct ALLOC_ENTRY) );
        alloc_entry = alloc_entry->next;
        if( alloc_entry == 0 )
        {
            log_printf( 0, LOG_EMERGENCY, "Main::Xmalloc: Unable to allocate memory for alloc entry!!" );
            exit( -1 );
        }
    }

    // Fill it
    alloc_entry->lpMem = lpMem;
    alloc_entry->nSize = nSize;
    alloc_entry->bUsed = true;

    alloc_entry->alloc.szString = szString;
    alloc_entry->alloc.szFile = szFile;
    alloc_entry->alloc.nLine = nLine;

    alloc_entry->next = 0;

    // Memory count
    nMemCount += 1;

    // Unlock
    pthread_mutex_unlock( &garbage_mutex );

#ifdef VERBOSE_DEBUG
    printf( "%s:%u: MALLOC( %s ): #%05i (#%03u): lpMem = %08x, nSize = %u\n",
        szFile, nLine, szString, nMemCount, i, lpMem, nSize );
#endif // VERBOSE_DEBUG

    return lpMem;
}

// #############################################################################

void xfree( void *lpMem, const char *szString, const char *szFile, unsigned int nLine )
{
    // The Free
    free( lpMem );

    // If garbage collector is not ready, then die
    if( garbage_collector == 0 )
        return;

    // Get shared garbage collector lock
    if( pthread_mutex_lock( &garbage_mutex ) != 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Main::Xfree: Can't acquire lock!!" );
        exit( -1 );
    }

    // Find alloc entry
    unsigned int i = memory_hash( lpMem, nGarbageCollectorSize );
    struct ALLOC_ENTRY **alloc_entry = &garbage_collector[i];
    struct ALLOC_ENTRY **prev_entry = 0;
    if( *alloc_entry == 0 )
    {
        log_printf( 0, LOG_CRITICAL, "Main::Xfree: %s:%u: FREE( %s ): #%05i (#%03u): Can't find lpMem %08x in garbage collector (step 1)",
            szFile, nLine, szString, nMemCount, i, lpMem ); // */
        goto mutex_unlock;
    }
    while( (*alloc_entry)->lpMem != lpMem )
    {
        prev_entry = alloc_entry;
        alloc_entry = &((*alloc_entry)->next);
        if( *alloc_entry == 0 )
        {
            log_printf( 0, LOG_CRITICAL, "Main::Xfree: %s:%u: FREE( %s ): #%05i (#%03u): Can't find lpMem %08x in garbage collector (step 2)",
                szFile, nLine, szString, nMemCount, i, lpMem ); // */
            goto mutex_unlock;
        }
    }

    // Disconnect found entry from chain
    //if( prev_entry != 0 )
    //    (*prev_entry)->next = (*alloc_entry)->next;
    struct ALLOC_ENTRY *temp_entry = (*alloc_entry)->next;

    // Unfill it
    (*alloc_entry)->bUsed = false;
    (*alloc_entry)->free.szString = szString;
    (*alloc_entry)->free.szFile = szFile;
    (*alloc_entry)->free.nLine = nLine;

#ifdef VERBOSE_DEBUG
    printf( "%s:%u: FREE( %s ): #%05i (#%03u): lpMem = %08x, nSize = %u\n",
        szFile, nLine, szString, nMemCount, i, lpMem, (*alloc_entry)->nSize );
#endif // VERBOSE_DEBUG

    // Free it
    free( *alloc_entry );
    *alloc_entry = temp_entry;

    //}
    //else
    //{
    //    log_printf( 0, LOG_WARNING, "FREE: Freeing unknown block %s:%u: FREE( %s ): lpMem = %08x",
    //        szFile, nLine, szString, lpMem );
    //}

    // Memory count
    nMemCount -= 1;

mutex_unlock:
    // Unlock
    pthread_mutex_unlock( &garbage_mutex );
}

// #############################################################################

void *xrealloc( void *lpMem, size_t nSize, const char *szSizeString, const char *szMemString, const char *szFile, unsigned int nLine )
{
    // Get some memory for reallocation
    char *lpNewMem = realloc( lpMem, nSize );

    // If garbage collector is not ready, then die
    if( garbage_collector == 0 )
        return lpNewMem;

    // Error check
    if( lpNewMem == 0 )
    {
        log_printf( 0, LOG_CRITICAL, "Main::Xrealloc: Unable to reallocate memory block %s:%u: REALLOC( %s, %s ): #%05i: lpMem = %08x, nSize = %u",
            szFile, nLine, szMemString, szSizeString, nMemCount, lpMem, nSize );
    }

    // Get shared garbage collector lock
    if( pthread_mutex_lock( &garbage_mutex ) != 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Main::Xfree: Can't acquire lock!!" );
        exit( -1 );
    }

    {
        // Find alloc entry
        unsigned int i = memory_hash( lpMem, nGarbageCollectorSize );
        struct ALLOC_ENTRY **alloc_entry = &garbage_collector[i];
        struct ALLOC_ENTRY **prev_entry = 0;
        if( *alloc_entry == 0 )
        {
            log_printf( 0, LOG_CRITICAL, "Main::Xrealloc: %s:%u: REALLOC( %s, %s ): #%05i (#%03u): Can't find lpMem %08x in garbage collector (step 1)",
                szFile, nLine, szMemString, szSizeString, nMemCount, i, lpMem ); // */
            goto mutex_unlock;
        }
        while( (*alloc_entry)->lpMem != lpMem )
        {
            prev_entry = alloc_entry;
            alloc_entry = &((*alloc_entry)->next);
            if( *alloc_entry == 0 )
            {
                log_printf( 0, LOG_CRITICAL, "Main::Xrealloc: %s:%u: REALLOC( %s, %s ): #%05i (#%03u): Can't find lpMem %08x in garbage collector (step 2)",
                    szFile, nLine, szMemString, szSizeString, nMemCount, i, lpMem ); // */
                goto mutex_unlock;
            }
        }

        // Disconnect found entry from chain
        struct ALLOC_ENTRY *temp_entry = (*alloc_entry)->next;

        // Unfill it
        (*alloc_entry)->bUsed = false;
        (*alloc_entry)->free.szString = szMemString;
        (*alloc_entry)->free.szFile = szFile;
        (*alloc_entry)->free.nLine = nLine;

#ifdef VERBOSE_DEBUG
    printf( "%s:%u: REALLOC( %s, %s ): #%05i (#%03u): lpMem = %08x, nOldSize = %u, nNewSize = %u\n",
        szFile, nLine, szMemString, szSizeString, nMemCount, i, lpMem, (*alloc_entry)->nSize, nSize );
#endif // VERBOSE_DEBUG

        // Free allocation entry
        free( *alloc_entry );
        *alloc_entry = temp_entry;
    }

    {
        // Get alloc entry for new memory
        unsigned int i = memory_hash( lpNewMem, nGarbageCollectorSize );
        if( garbage_collector[i] == 0 )
        {
            garbage_collector[i] = malloc( sizeof(struct ALLOC_ENTRY) );
            memset( garbage_collector[i], 0, sizeof(struct ALLOC_ENTRY) );
        }
        struct ALLOC_ENTRY *alloc_entry = garbage_collector[i];
        if( alloc_entry->bUsed == true )
        {
            // Debug collision
            unsigned int nCollisionLevel = 1;

            // Recurse
            while( alloc_entry->next != 0 )
            {

                ++nCollisionLevel;
                alloc_entry = alloc_entry->next;
            }

            // Malloc
            alloc_entry->next = malloc( sizeof(struct ALLOC_ENTRY) );
            alloc_entry = alloc_entry->next;
            if( alloc_entry == 0 )
            {
                log_printf( 0, LOG_EMERGENCY, "Main::Xrealloc: Unable to allocate memory for alloc entry!!" );
                exit( -1 );
            }
        }

        // Fill it
        alloc_entry->lpMem = lpNewMem;
        alloc_entry->nSize = nSize;
        alloc_entry->bUsed = true;

        alloc_entry->alloc.szString = szMemString;
        alloc_entry->alloc.szFile = szFile;
        alloc_entry->alloc.nLine = nLine;

        alloc_entry->next = 0;
    }

    // Memory count is unchanged

mutex_unlock:
    // Unlock
    pthread_mutex_unlock( &garbage_mutex );

    return lpNewMem;
}

// #############################################################################

void memory_print_unfreed( )
{
#ifdef VERBOSE_DEBUG
    printf( "\n*** Checking unfreed memory\n" );
#endif // VERBOSE_DEBUG

    unsigned int nTotal = 0;
    unsigned int nRealMemCount = 0;

    for( unsigned int i = 0; i < nGarbageCollectorSize; i++ )
    {
        if( garbage_collector[i] != 0 )
        {
#ifdef VERBOSE_DEBUG
            /*log_printf( 0, LOG_WARNING, "Memory leak: %s:%u: #%05i (#%03u): %s is unfreed: lpMem = %08x, nSize = %u",
                garbage_collector[i]->alloc.szFile,
                garbage_collector[i]->alloc.nLine,
                nRealMemCount, i,
                garbage_collector[i]->alloc.szString,
                garbage_collector[i]->lpMem,
                garbage_collector[i]->nSize );*/
            log_printf( 0, LOG_WARNING, "Memory leak: %s:%u: #%05i (#%03u): %s is unfreed: lpMem = %08x, nSize = %u, \"%s\"",
                garbage_collector[i]->alloc.szFile,
                garbage_collector[i]->alloc.nLine,
                nRealMemCount, i,
                garbage_collector[i]->alloc.szString,
                garbage_collector[i]->lpMem,
                garbage_collector[i]->nSize,
                garbage_collector[i]->lpMem );
#endif // VERBOSE_DEBUG
            nTotal += garbage_collector[i]->nSize;
            nRealMemCount += 1;

            // Recurse
            struct ALLOC_ENTRY *alloc_entry = garbage_collector[i]->next;
            while( alloc_entry != 0 )
            {
#ifdef VERBOSE_DEBUG
                log_printf( 0, LOG_WARNING, "Memory leak: %s:%u: #%05i (#%03u): %s is unfreed: lpMem = %08x, nSize = %u",
                    alloc_entry->alloc.szFile,
                    alloc_entry->alloc.nLine,
                    nRealMemCount, i,
                    alloc_entry->alloc.szString,
                    alloc_entry->lpMem,
                    alloc_entry->nSize );
#endif // VERBOSE_DEBUG
                nTotal += alloc_entry->nSize;
                nRealMemCount += 1;

                alloc_entry = alloc_entry->next;
            }
        }
    }

    if( nRealMemCount > 0 )
        log_printf( 0, LOG_WARNING, "Found memory leaks: %u bytes (%i/%i pointers)", nTotal, nMemCount, nRealMemCount );
#ifdef VERBOSE_DEBUG
    else
        printf( "*** Found total unfreed: %u (%i/%i pointers)\n", nTotal, nMemCount, nRealMemCount );
#endif // VERBOSE_DEBUG
}

// #############################################################################

void main_free_all( )
{
#ifdef VERBOSE_DEBUG
    printf( "\n*** Freeing all unfreed memory\n" );
#endif // VERBOSE_DEBUG

    unsigned int nTotal = 0;

    for( unsigned int i = 0; i < nGarbageCollectorSize; i++ )
    {
        if( garbage_collector[i] != 0 )
        {
            garbage_collector[i]->bUsed = false;

#ifdef VERBOSE_DEBUG
            printf( "%s:%u: was MALLOC( %s ): lpMem = %08x, nSize = %u\n",
                garbage_collector[i]->alloc.szFile,
                garbage_collector[i]->alloc.nLine,
                garbage_collector[i]->alloc.szString,
                garbage_collector[i]->lpMem,
                garbage_collector[i]->nSize );
#endif // VERBOSE_DEBUG
            nTotal += garbage_collector[i]->nSize;
            free( garbage_collector[i]->lpMem );

            // Recurse
            struct ALLOC_ENTRY *alloc_entry = garbage_collector[i]->next;

            while( alloc_entry != 0 )
            {
#ifdef VERBOSE_DEBUG
                printf( "%s:%u: was MALLOC( %s ): lpMem = %08x, nSize = %u\n",
                    alloc_entry->alloc.szFile,
                    alloc_entry->alloc.nLine,
                    alloc_entry->alloc.szString,
                    alloc_entry->lpMem,
                    alloc_entry->nSize );
#endif // VERBOSE_DEBUG
                nTotal += alloc_entry->nSize;

                struct ALLOC_ENTRY *temp_entry = alloc_entry;
                alloc_entry = alloc_entry->next;

                // Free
                free( temp_entry );

            }

            // Free
            free( garbage_collector[i] );
            garbage_collector[i] = 0;
        }
    }

#ifdef VERBOSE_DEBUG
    printf( "*** Unfreed total: %u bytes\n\n", nTotal );
#endif // VERBOSE_DEBUG
}

// #############################################################################

#else

// #############################################################################

// Nothing here
void xinit( ) { }
void xdestroy( ) { }

#endif // DEBUG

// #############################################################################
