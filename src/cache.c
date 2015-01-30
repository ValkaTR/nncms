// #############################################################################
// Source file: cache.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "cache.h"
#include "log.h"
#include "file.h"

#include "template.h"
#include "user.h"
#include "acl.h"

#include "strlcpy.h"
#include "strlcat.h"
#include "recursive_mkdir.h"

#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// #############################################################################
// global variables
//

// Path to cache
char cacheDir[NNCMS_PATH_LEN_MAX] = "./cache/";

struct NNCMS_CACHE cachedb[NNCMS_CACHE_MEM_MAX];

// If set to true, then cache system tries to hold data in memory, decreasing
// requests to harddrive
bool bCacheMemory = false;

// If set to false, then caching is disabled
bool bCache = false;

pthread_mutex_t cache_mutex;

// #############################################################################
// functions

bool cache_global_init( )
{
    if( bCacheMemory == true )
    {
        // Init cache db
        memset( cachedb, 0, sizeof(cachedb) );

        // Init pthread locking
        if( pthread_mutex_init( &cache_mutex, 0 ) != 0 ) {
            log_print( 0, LOG_EMERGENCY, "Lock init failed!!" );
            exit( -1 );
        }
    }

    main_page_add( "cache_admin", &cache_admin );

    return true;
}

// #############################################################################

bool cache_global_destroy( )
{
    if( bCacheMemory == true )
    {
        // Clear cache
        for( unsigned int i = 0; i < NNCMS_CACHE_MEM_MAX; i++ )
        {
            if( cachedb[i].lpData != 0 )
            {
                FREE( cachedb[i].lpData );
                cachedb[i].lpData = 0;
            }
        }
        memset( cachedb, 0, sizeof(cachedb) );

        // Deinit pthread locking
        if( pthread_mutex_destroy( &cache_mutex ) != 0 )
        {
            log_print( 0, LOG_EMERGENCY, "Cache:Deinit: Mutex destroy failed!!" );
        }
    }

    return true;
}

// #############################################################################

// Returns ZERO if nothing found
struct NNCMS_CACHE *cache_find_mem( struct NNCMS_THREAD_INFO *req, char *name )
{
    //
    // Find cached data in memory
    //

    if( pthread_mutex_lock( &cache_mutex ) != 0 )
    {
        log_print( req, LOG_EMERGENCY, "Can't acquire readlock!!" );
        exit( -1 );
    }
    for( unsigned int i = 0; i < NNCMS_CACHE_MEM_MAX; i++ )
    {
        if( strcmp( name, cachedb[i].name ) == 0 )
        {
            // Update time stamp
            cachedb[i].timeStamp = time( 0 );

            // Debug
            //log_printf( req, LOG_DEBUG, "\"%s\" acquired, data size %u", cachedb[i].name, cachedb[i].nSize );

            // Return cached data
            pthread_mutex_unlock( &cache_mutex );
            return &cachedb[i];
        }
    }

    // If data in memory was not found, then check in filesystem
    pthread_mutex_unlock( &cache_mutex );

    return NULL;
}

// Returns ZERO if nothing found
char *cache_find_file( struct NNCMS_THREAD_INFO *req, char *name )
{
    //
    // Find cached data in file system
    //
    if( bCache == false )
        return NULL;

    // Skip beginning slash
    if( *name == '/' )
        name++;

    // Create full path to file
    char szFullName[NNCMS_PATH_LEN_MAX];
    snprintf( szFullName, sizeof(szFullName), "%s%s", cacheDir, name );
    //log_printf( req, LOG_DEBUG, "%s", szFileName );

    // Try to open it
    FILE *pFile = fopen( szFullName, "rb" );
    if( pFile == NULL )
        return NULL;

    // Create buffer for that file
    fseek( pFile, 0, SEEK_END );
    unsigned long int nSize = ftell( pFile );
    rewind( pFile );
    char *lpData = MALLOC( nSize );
    if( lpData == NULL )
    {
        // ohshi..
        log_printf( req, LOG_CRITICAL, "Could not allocate %lu bytes (%s)", nSize, szFullName );
        return NULL;
    }

    //log_printf( req, LOG_DEBUG, "%lu bytes (%s)", nSize, szFullName );

    // Read and close it, then restore it to memory cache
    fread( lpData, 1, nSize, pFile );
    fclose( pFile );

    return lpData;
}


bool cache_access_file( struct NNCMS_THREAD_INFO *req, char *name )
{
    //
    // Store data to file system
    //

    // Skip beginning slash
    if( *name == '/' )
        name++;

    // Create full path to file
    char szFullName[NNCMS_PATH_LEN_MAX];
    snprintf( szFullName, sizeof(szFullName), "%s%s", cacheDir, name );
    //log_printf( req, LOG_DEBUG, "%s", szFileName );

    // Check if already exists
    //if( access( szFullName, F_OK ) == 0 )
    //    goto done;
    struct stat stFile;
    if( stat( szFullName, &stFile ) == 0 )
    {
        if( S_ISREG(stFile.st_mode) )
        {
            return true;
        }
    }
    
    return false;
}

// Returns ZERO if nothing found
char *cache_find( struct NNCMS_THREAD_INFO *req, char *name )
{
    //
    // Find cached data in memory or file system
    //
    struct NNCMS_CACHE *cache;

    if( bCache == false )
        return NULL;

    if( bCacheMemory == true )
    {
        cache = cache_find_mem( req, name );
        if( cache != NULL )
            return cache->lpData;
    }

    char *data = cache_find_file( req, name );

    return data;
}

// #############################################################################

struct NNCMS_CACHE *cache_store_mem( struct NNCMS_THREAD_INFO *req, char *name, char *lpData, size_t nSize )
{
    //
    // Store data in memory
    //

    struct NNCMS_CACHE *lpCache = &cachedb[0];

    // Acquire write lock
    if( pthread_mutex_lock( &cache_mutex ) != 0 )
    {
        log_print( req, LOG_EMERGENCY, "Can't acquire writelock!!" );
        exit( -1 );
    }

    // Find free cache entry
    time_t currentTime = time( 0 );
    time_t tempTime = currentTime;
    for( unsigned int i = 0; i < NNCMS_CACHE_MEM_MAX; i++ )
    {
        if( cachedb[i].timeStamp < tempTime )
        {
            tempTime = cachedb[i].timeStamp;
            lpCache = &cachedb[i];
        }
    }

    // Delete data
    if( lpCache->lpData != NULL )
    {
        FREE( lpCache->lpData );
        lpCache->lpData = NULL;
    }

    // Replace with new data
    strlcpy( lpCache->name, name, NNCMS_PATH_LEN_MAX );
    lpCache->lpData = lpData;
    lpCache->nSize = nSize;
    lpCache->timeStamp = currentTime;

    // Unlock
    pthread_mutex_unlock( &cache_mutex );

    return lpCache;
}

bool cache_store_file( struct NNCMS_THREAD_INFO *req, char *name, char *lpData, size_t nSize )
{
    //
    // Store data to file system
    //

    // Skip beginning slash
    if( *name == '/' )
        name++;

    // Create full path to file
    char szFullName[NNCMS_PATH_LEN_MAX];
    snprintf( szFullName, sizeof(szFullName), "%s%s", cacheDir, name );
    //log_printf( req, LOG_DEBUG, "%s", szFileName );

    // Check if already exists
    //if( access( szFullName, F_OK ) == 0 )
    //    goto done;
    struct stat stFile;
    if( stat( szFullName, &stFile ) == 0 )
    {
        if( S_ISREG(stFile.st_mode) )
        {
            return false;
        }
    }

    // Create path to file
    char *lpszFileName = rindex( szFullName, '/' );
    if( lpszFileName == NULL )
    {
        // No path in name
        lpszFileName = szFullName;
    }
    else
    {
        // Separate path and filename
        *lpszFileName = 0;

        // Create path recursively
        recursive_mkdir( szFullName );

        // Join path and file name
        *lpszFileName = '/';
    }

    // Store data in file
    //
    //  w       Truncate  file  to  zero  length  or create text file
    //          for writing.  The stream is positioned at the
    //          beginning of the file.
    FILE *pFile = fopen( szFullName, "wb" );
	if( pFile == NULL )
	{
		log_printf( req, LOG_CRITICAL, "Could not create cache file \"%s\" (data size %u)", szFullName, nSize );
        return false;
	}
	else
	{
		// Write data
		fwrite( lpData, 1, nSize, pFile );

		// Close file
		fclose( pFile );
	}

    return true;
}

struct NNCMS_CACHE *cache_store( struct NNCMS_THREAD_INFO *req, char *name, char *lpData, size_t nSize )
{
    //
    // Store data in memory and file system
    //

    if( bCache == false )
        return NULL;

    struct NNCMS_CACHE *cache;

    if( bCacheMemory == true )
    {
        cache = cache_store_mem( req, name, lpData, nSize );
    }

    cache_store_file( req, name, lpData, nSize );

    return cache;
}

// #################################################################################

#define NNCMS_CACHE_CLEAR_HTML          (1 << 0)
#define NNCMS_CACHE_CLEAR_THUMBNAILS    (1 << 1)

void cache_clear( char *path, char fClearType )
{
    // List files in current directory
    unsigned int nDirCount = 0;
    struct NNCMS_FILE_ENTITY *lpEnts = 0;//file_get_directory( 0, path, &nDirCount, file_compare_name_asc );
    if( lpEnts == 0 )
    {
        log_printf( 0, LOG_ERROR, "Cache directory not found: %s", path );
        return;
    }

    if( fClearType == 0 )
        return;

    for( unsigned int i = 0; i < nDirCount; i++ )
    {
        // Skip . folder and hidden files
        if( lpEnts[i].fname[0] == '.' )
            continue;

        // Create a fullpath to file
        char fullpath[NNCMS_PATH_LEN_MAX];
        snprintf( fullpath, NNCMS_PATH_LEN_MAX, "%s%s", path, lpEnts[i].fname );

        size_t nLen = strlen( lpEnts[i].fname );
        if( S_ISREG( lpEnts[i].f_stat.st_mode ) )
        {
            //struct NNCMS_FILE_INFO = fileInfo;
            //file_get_info( &fileInfo, fullpath );

            if( nLen > 5 )
            {
                if( strcasecmp( &lpEnts[i].fname[nLen - 5], ".html" ) == 0 )
                {
                    // If file is a html and flag html is set, then delete
                    if( fClearType & NNCMS_CACHE_CLEAR_HTML )
                        remove( fullpath );
                    //log_printf( 0, LOG_DEBUG, "Remove: %s", fullpath );
                }
                else
                {
                    // If file is not a html and flag thumbnail is set, then delete
                    if( fClearType & NNCMS_CACHE_CLEAR_THUMBNAILS )
                        remove( fullpath );
                }
            }
        }

        // Step inside directories
        else if( S_ISDIR( lpEnts[i].f_stat.st_mode ) )
        {
            // Do the recursion
            strlcat( fullpath, "/", NNCMS_PATH_LEN_MAX );
            cache_clear( fullpath, fClearType );

            // Debug
            //log_printf( 0, LOG_DEBUG, "Dir: %s", fullpath );
        }
    }
}

// #################################################################################

void cache_admin( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Cache admin";

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = szHeader, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = req->lpszBuffer, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/mimetypes/package-x-generic.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    struct NNCMS_VARIABLE log_template[] =
        {
            { .name = "referer", .value.string = req->referer, .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "fkey", .value.string = req->session_id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // First we check what permissions do user have
    if( acl_check_perm( req, "cache", NULL, "clear" ) == false )
    {
        frame_template[0].value.string = "Error";
        frame_template[1].value.string = "Not allowed";
        log_printf( req, LOG_NOTICE, "%s", frame_template[1].value.string );
        goto output;
    }

    //
    // Check if user pushed big gray shiny "Clear" button
    //
    char *httpVarClear = main_variable_get( req, req->get_tree, "cache_clear" );
    if( httpVarClear != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frame_template[0].value.string = "Error";
            frame_template[1].value.string = "Unequal keys";
            goto output;
        }

        char *httpVarClearHtml = main_variable_get( req, req->get_tree, "cache_clear_html" );
        char *httpVarClearThumbnail = main_variable_get( req, req->get_tree, "cache_clear_thumbnail" );

        // Buffer for logging
        char szResult[16]; *szResult = 0;

        char fClearType = 0;
        if( httpVarClearHtml != 0 && strcasecmp( httpVarClearHtml, "on" ) == 0 )
        {
            // Delete HTML data
            strcat( szResult, " HTML" );
            fClearType |= NNCMS_CACHE_CLEAR_HTML;
        }

        if( httpVarClearThumbnail != 0 && strcasecmp( httpVarClearThumbnail, "on" ) == 0 )
        {
            // Delete thumbnails
            strcat( szResult, " Thumbnails" );
            fClearType |= NNCMS_CACHE_CLEAR_THUMBNAILS;
        }
        cache_clear( cacheDir, fClearType );

        // Log results
        if( *szResult != 0 )
            log_printf( req, LOG_INFO, "Cache deleted:%s", szResult );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Load rename page
    //template_iparse( req, TEMPLATE_CACHE_ADMIN, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, log_template );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    //template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frame_template );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #################################################################################
