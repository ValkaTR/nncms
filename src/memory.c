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
#include "database.h"
#include "node.h"

// #############################################################################
// includes of system headers
//

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>
#include <gmime/gmime.h>

// #############################################################################
// global variables
//

// Memory table
GHashTable *memory_table = 0;

// This stores pointers which must be freed at the end of the loop
GArray *global_garbage = NULL;

// Anti recursion
bool memory_internal = false;

// Memory allocation vtable can only be set once at startup
bool memory_initialized = false;

// #############################################################################
// functions

static gpointer g_xmalloc( gsize n_bytes )
{
    return memory_xmalloc( n_bytes, "unk", "unk", 0, "unk" );
}

static gpointer g_xrealloc( gpointer mem, gsize n_bytes )
{
    return memory_xrealloc( mem, n_bytes, "unk", "unk", "unk", 0, "unk" );
}

static void g_xfree( gpointer mem )
{
    memory_xfree( mem, "unk", "unk", 0, "unk" );
}

static GMemVTable memvtable =
{
    /* malloc */ g_xmalloc,
    /* realloc */ g_xrealloc,
    /* free */ g_xfree,

    /* optional; set to NULL if not used */
    /* calloc */ NULL,
    /* try_malloc */ NULL,
    /* try_realloc */ NULL
};

// #############################################################################

// Garbage collector initiliazation
bool memory_global_init( )
{
    // Memory statistics
    // g_mem_profile() will output a summary g_malloc() memory usage,
    // if memory profiling has been enabled by calling g_mem_set_vtable
    // (glib_mem_profiler_table) upon startup. 

    //g_mem_set_vtable( glib_mem_profiler_table );
    //if( g_mem_is_system_malloc( ) == FALSE )
    if( memory_initialized == false )
    {
        memory_initialized = true;
        g_mem_set_vtable( &memvtable );
    }
     
    memory_table = g_hash_table_new( g_direct_hash, g_direct_equal );

    main_local_init_add( &memory_local_init );
    main_local_destroy_add( &memory_local_destroy );

    global_garbage = garbage_create( );

    return true;
}

// #############################################################################

// Destroing garbage collector
bool memory_global_destroy( )
{
    garbage_destroy( global_garbage );
    global_garbage = NULL;
    
    if( memory_table != 0 )
    {
        memory_internal = true;

#ifndef NDEBUG
        g_hash_table_foreach_remove( memory_table, memory_ghrfunc, NULL );
#endif // NDEBUG

        g_hash_table_destroy( memory_table );

        memory_internal = false;

        memory_table = 0;
    }

    return true;
}

// #############################################################################

bool memory_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Creates a new GArray for garbage
    if( req->loop_garbage == 0 )
    {
        //req->loop_garbage = g_array_sized_new( FALSE, FALSE, sizeof(struct MEMORY_GARBAGE), 100 );
        //g_array_set_clear_func( req->loop_garbage, (GDestroyNotify) memory_garbage_GDestroyNotify );
        
        req->loop_garbage = garbage_create( );
        req->thread_garbage = garbage_create( );
    }

    return true;
}

bool memory_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    if( req->loop_garbage != NULL )
    {
        //g_array_free( req->loop_garbage, TRUE );
        garbage_destroy( req->loop_garbage );
        garbage_destroy( req->thread_garbage );
        
        req->loop_garbage = NULL;
        req->thread_garbage = NULL;
    }

    return true;
}

// #############################################################################

void garbage_GDestroyNotify( gpointer data )
{
    struct MEMORY_GARBAGE *array = (struct MEMORY_GARBAGE *) data;
    switch( array->type )
    {
        case MEMORY_GARBAGE_FREE: FREE( array->data ); break;
        case MEMORY_GARBAGE_GFREE: g_free( array->data ); break;
        case MEMORY_GARBAGE_DB_FREE: database_free_rows( array->data ); break;
        case MEMORY_GARBAGE_NODE_FREE: node_free( array->data ); break;
        case MEMORY_GARBAGE_GSTRING_FREE: g_string_free( array->data, TRUE ); break;
        case MEMORY_GARBAGE_GARRAY_FREE: g_array_free( array->data, TRUE ); break;
        case MEMORY_GARBAGE_MIME_STREAM_CLOSE: g_mime_stream_close( array->data ); break;
        case MEMORY_GARBAGE_MIME_PART_ITER_FREE: g_mime_part_iter_free( array->data ); break;
        case MEMORY_GARBAGE_OBJECT_UNREF: g_object_unref( array->data ); break;
        case MEMORY_GARBAGE_BYTE_ARRAY_UNREF: g_byte_array_unref( array->data ); break;

    }
}

GArray *garbage_create( )
{
    GArray *garbage = g_array_sized_new( FALSE, FALSE, sizeof(struct MEMORY_GARBAGE), 10 );
    g_array_set_clear_func( garbage, (GDestroyNotify) garbage_GDestroyNotify );

    return garbage;
}

void garbage_destroy( GArray *garbage )
{
    g_array_free( garbage, TRUE );
}

void *garbage_add( GArray *garbage, void *data, enum MEMORY_GARBAGE_TYPE type )
{
    if( garbage != NULL )
    {
        if( data == NULL )
            log_print( 0, LOG_CRITICAL, "Added zero pointer to garbage collector" );
        
        struct MEMORY_GARBAGE array =
        {
            .data = data,
            .type = type
        };
        g_array_append_vals( garbage, &array, 1 );
    }
    
    return data;
}

void garbage_free( GArray *garbage )
{
    if( garbage != 0 )
    {
        g_array_set_size( garbage, 0 );
    }
}

// #############################################################################

gboolean memory_ghrfunc( gpointer key, gpointer value, gpointer user_data )
{
    struct ALLOC_ENTRY *alloc_entry = value;

    log_printf( 0, LOG_WARNING, "(Leak) %s:%u:%s() %s is unfreed: lpMem = %08x, nSize = %u, \"%s\"",
        alloc_entry->szFile,
        alloc_entry->nLine,
        alloc_entry->pretty_function,
        alloc_entry->szString,
        alloc_entry->lpMem,
        alloc_entry->nSize,
        alloc_entry->lpMem );
    
    // Do not free "unk" entries, because they may belong to cache
    if( strcmp( alloc_entry->szFile, "unk" ) == 0 )
        return FALSE;
    else
        return TRUE;
}

// #############################################################################

void memory_ghrfunc_report( gpointer key, gpointer value, gpointer user_data )
{
    struct ALLOC_ENTRY *alloc_entry = value;
    
    bool report = *(bool *) user_data;
    
    /*if( alloc_entry->nSize > 30 )
    {
        ( (char *) alloc_entry->lpMem)[30] = 0;
    }*/
    
    if( report && alloc_entry->cached == false )
    log_printf( 0, LOG_WARNING, "(Leak) %s:%u:%s() %s is unfreed: lpMem = %08x, nSize = %u, \"%s\"",
        alloc_entry->szFile,
        alloc_entry->nLine,
        alloc_entry->pretty_function,
        alloc_entry->szString,
        alloc_entry->lpMem,
        alloc_entry->nSize,
        alloc_entry->lpMem );

    alloc_entry->cached = true;
}

// #############################################################################

void memory_leaks( bool report )
{
#ifndef NDEBUG
        g_hash_table_foreach( memory_table, memory_ghrfunc_report, &report );
#endif // NDEBUG
}

// #############################################################################

void memory_safe_free( void *lpMem, const char *lpname, const char *szFile, unsigned int nLine, const char *pretty_function )
{
    if( lpMem != 0 )
    {
        FREE( lpMem );
    }
    else
    {
        log_printf( 0, LOG_DEBUG, "%s is zero at %s:%u:%s()", lpname, szFile, nLine, pretty_function );
    }
}

// #############################################################################

void *memory_xmalloc( size_t nSize, const char *szString, const char *szFile, unsigned int nLine, const char *pretty_function )
{
    enum MEMORY_TYPE type = MEMORY_TYPE_UNKNOWN;
    if( strcmp( szString, "unk" ) == 0 )
        type = MEMORY_TYPE_G_ALLOC;
    else
        type = MEMORY_TYPE_MALLOC;

    // The allocation
    void *lpMem = malloc( nSize );

    // Error check
    if( lpMem == 0 )
    {
        log_printf( 0, LOG_CRITICAL, "Unable to allocate memory block %s:%u:%s() MALLOC( %s ): lpMem = %08x, nSize = %u",
            szFile, nLine, pretty_function, szString, lpMem, nSize );
    }

    // If garbage collector is not ready, then die
    if( memory_table == 0 || memory_internal == true )
        return lpMem;

    memory_internal = true;

    struct ALLOC_ENTRY *alloc_entry = malloc( sizeof(struct ALLOC_ENTRY) );

    // Fill it
    alloc_entry->lpMem = lpMem;
    alloc_entry->nSize = nSize;
    
    alloc_entry->szString = szString;
    alloc_entry->szFile = szFile;
    alloc_entry->nLine = nLine;
    alloc_entry->pretty_function = pretty_function;

    alloc_entry->cached = false;
    
    alloc_entry->type = type;


    g_hash_table_insert( memory_table, lpMem, alloc_entry );

    memory_internal = false;

#ifdef VERBOSE_DEBUG
    printf( "%s:%u:%s() MALLOC( %s ): lpMem = %08x, nSize = %u\n",
        szFile, nLine, pretty_function, szString, lpMem, nSize );
#endif // VERBOSE_DEBUG

    return lpMem;
}

// #############################################################################

void memory_xfree( void *lpMem, const char *szString, const char *szFile, unsigned int nLine, const char *pretty_function )
{
    enum MEMORY_TYPE type = MEMORY_TYPE_UNKNOWN;
    if( strcmp( szString, "unk" ) == 0 )
        type = MEMORY_TYPE_G_ALLOC;
    else
        type = MEMORY_TYPE_MALLOC;
    
    // Error check
    if( lpMem == 0 )
    {
        log_printf( 0, LOG_CRITICAL, "Attempt to free NULL pointer %s:%u:%s() MALLOC( %s ): lpMem = %08x",
            szFile, nLine, pretty_function, szString, lpMem );
        return;
    }
    else
    {
        // The Free
        free( lpMem );
    }

    // If garbage collector is not ready, then die
    if( memory_table == 0 || memory_internal == true )
        return;

    memory_internal = true;

    // Find alloc entry
    struct ALLOC_ENTRY *alloc_entry = g_hash_table_lookup( memory_table, lpMem );
    if( alloc_entry != 0 )
    {
        if( type == MEMORY_TYPE_G_ALLOC )
        {
            if( alloc_entry->type == MEMORY_TYPE_MALLOC )
            {
                log_printf( 0, LOG_CRITICAL, 
                "Memory::Xfree: Freeing memory made by malloc() with g_free(), "
                "ALLOC[%s:%u:%s() %s: lpMem = %08x, nSize = %u, \"%s\"]; "
                "FREE[%s:%u:%s() %s: lpMem = %08x, \"%s\"]",
                    alloc_entry->szFile, alloc_entry->nLine, alloc_entry->pretty_function, alloc_entry->szString, alloc_entry->lpMem, alloc_entry->nSize, alloc_entry->lpMem,
                    szFile, nLine, pretty_function, szString, lpMem, lpMem );
            }
        }
        else if( type == MEMORY_TYPE_MALLOC )
        {
            if( alloc_entry->type == MEMORY_TYPE_G_ALLOC )
            {
                log_printf( 0, LOG_CRITICAL,
                "Memory::Xfree: Freeing memory made by g_malloc() with free(), "
                "ALLOC[%s:%u:%s() %s: lpMem = %08x, nSize = %u, \"%s\"]; "
                "FREE[%s:%u:%s() %s: lpMem = %08x, \"%s\"]",
                    alloc_entry->szFile, alloc_entry->nLine, alloc_entry->pretty_function, alloc_entry->szString, alloc_entry->lpMem, alloc_entry->nSize, alloc_entry->lpMem,
                    szFile, nLine, pretty_function, szString, lpMem, lpMem );
            }
        }

        g_hash_table_steal( memory_table, lpMem );
        free( alloc_entry );
    }
    else
    {
        if( type == MEMORY_TYPE_MALLOC )
        log_printf( 0, LOG_WARNING,
        "Memory::Xfree: Freeing unknown memory, "
        "FREE[%s:%u:%s() %s: lpMem = %08x, \"%s\"]",
            szFile, nLine, pretty_function, szString, lpMem, lpMem );
    }

    memory_internal = false;

#ifdef VERBOSE_DEBUG
    printf( "%s:%u:%s() FREE( %s ): #%05i (#%03u): lpMem = %08x, nSize = %u\n",
        szFile, nLine, pretty_function, szString, nMemCount, i, lpMem, (*alloc_entry)->nSize );
#endif // VERBOSE_DEBUG
}

// #############################################################################

void *memory_xrealloc( void *lpMem, size_t nSize, const char *szSizeString, const char *szString, const char *szFile, unsigned int nLine, const char *pretty_function )
{
    enum MEMORY_TYPE type = MEMORY_TYPE_UNKNOWN;
    if( strcmp( szString, "unk" ) == 0 )
        type = MEMORY_TYPE_G_ALLOC;
    else
        type = MEMORY_TYPE_MALLOC;

    // Get some memory for reallocation
    char *lpNewMem = realloc( lpMem, nSize );

    // If garbage collector is not ready, then die
    if( memory_table == 0 || memory_internal == true )
        return lpNewMem;

    // Error check
    if( lpNewMem == 0 )
    {
        log_printf( 0, LOG_CRITICAL, "Unable to reallocate memory block %s:%u:%s() REALLOC( %s, %s ): lpMem = %08x, nSize = %u",
            szFile, nLine, pretty_function, szString, szSizeString, lpMem, nSize );
    }

    memory_internal = true;

    // Find alloc entry
    
    struct ALLOC_ENTRY *alloc_entry = g_hash_table_lookup( memory_table, lpMem );

    if( alloc_entry != 0 )
    {
        if( type == MEMORY_TYPE_G_ALLOC )
        {
            if( alloc_entry->type == MEMORY_TYPE_MALLOC )
            {
                log_printf( 0, LOG_CRITICAL, 
                "Memory::Xrealloc: Reallocating memory made by malloc() with g_free(), "
                "ALLOC[%s:%u:%s() %s: lpMem = %08x, nSize = %u, \"%s\"]; "
                "REALLOC[%s:%u:%s() %s: lpMem = %08x, \"%s\"]",
                    alloc_entry->szFile, alloc_entry->nLine, alloc_entry->pretty_function, alloc_entry->szString, alloc_entry->lpMem, alloc_entry->nSize, alloc_entry->lpMem,
                    szFile, nLine, pretty_function, szString, lpMem, lpMem );
            }
        }
        else if( type == MEMORY_TYPE_MALLOC )
        {
            if( alloc_entry->type == MEMORY_TYPE_G_ALLOC )
            {
                log_printf( 0, LOG_CRITICAL,
                "Memory::Xrealloc: Reallocating memory made by g_malloc() with free(), "
                "ALLOC[%s:%u:%s() %s: lpMem = %08x, nSize = %u, \"%s\"]; "
                "REALLOC[%s:%u:%s() %s: lpMem = %08x, \"%s\"]",
                    alloc_entry->szFile, alloc_entry->nLine, alloc_entry->pretty_function, alloc_entry->szString, alloc_entry->lpMem, alloc_entry->nSize, alloc_entry->lpMem,
                    szFile, nLine, pretty_function, szString, lpMem, lpMem );
            }
        }
        
        g_hash_table_steal( memory_table, lpMem );

        // Free it
        free( alloc_entry );
    }
    else
    {
        if( type == MEMORY_TYPE_MALLOC )
        log_printf( 0, LOG_WARNING,
        "Memory::Xfree: Reallocating unknown memory, "
        "FREE[%s:%u:%s() %s: lpMem = %08x, \"%s\"]",
            szFile, nLine, pretty_function, szString, lpMem, lpMem );
    }


    // Add new memory
    alloc_entry = malloc( sizeof(struct ALLOC_ENTRY) );

    // Fill it
    alloc_entry->lpMem = lpNewMem;
    alloc_entry->nSize = nSize;

    alloc_entry->szString = szString;
    alloc_entry->szFile = szFile;
    alloc_entry->nLine = nLine;
    alloc_entry->pretty_function = pretty_function;
    
    alloc_entry->type = type;

    g_hash_table_insert( memory_table, lpNewMem, alloc_entry );

    memory_internal = false;

#ifdef VERBOSE_DEBUG
    printf( "%s:%u:%s() REALLOC( %s, %s ): #%05i (#%03u): lpMem = %08x, nOldSize = %u, nNewSize = %u\n",
        szFile, nLine, pretty_function, szMemString, szSizeString, nMemCount, i, lpMem, (*alloc_entry)->nSize, nSize );
#endif // VERBOSE_DEBUG

    return lpNewMem;
}

// #############################################################################

void *memdup( void *data, size_t n )
{
    void *dup = MALLOC( n );
    memcpy( dup, data, n );
    return dup;
}

void *memdup_temp( struct NNCMS_THREAD_INFO *req, void *data, size_t n )
{
    void *dup = memdup( data, n );
    garbage_add( req->loop_garbage, dup, MEMORY_GARBAGE_FREE );
    return dup;
}

// #############################################################################
