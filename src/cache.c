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

#include <fam.h>

// #############################################################################
// global variables
//

// Path to cache
char cacheDir[NNCMS_PATH_LEN_MAX] = "./cache/";

struct NNCMS_CACHE cachedb[NNCMS_CACHE_MEM_MAX];

static bool bCacheInit = false;

// If set to true, then cache system tries to hold data in memory, decreasing
// requests to harddrive
bool bCacheMemory = false;

pthread_mutex_t cache_mutex;

#ifdef HAVE_FAM
// File Alternation Monitor
FAMConnection g_fc;
pthread_t pFAMThread;
#endif

// #############################################################################
// functions

bool cache_init( )
{
    // Initialize cache if needed
    if( bCacheInit == false )
    {
        bCacheInit = true;

        if( bCacheMemory == true )
        {
            // Init cache db
            memset( cachedb, 0, sizeof(cachedb) );

            // Init pthread locking
            if( pthread_mutex_init( &cache_mutex, 0 ) != 0 ) {
                log_printf( 0, LOG_EMERGENCY, "Cache::Init: Lock init failed!!" );
                exit( -1 );
            }
        }

#ifdef HAVE_FAM
        // Create a connection to fam by calling FAMOpen. This routine will
        //  pass back a FAMConnection structure used in all fam procedures.
        //FAMConnection fc;
        int nResult = FAMOpen( &g_fc );
        if( nResult == -1 )
        {
            // The application was unable to open a connection to fam
            log_printf( 0, LOG_ALERT, "Cache::Init: The application was unable to open a connection to fam" );
            return false;
        }

        // Create the thread for monitoring file alternations
        pthread_create( &pFAMThread, 0, cache_fam, (void *) &g_fc );
#endif

        return true;
    }

    return false;
}

// #############################################################################

#ifdef HAVE_FAM
void cache_recursive_monitor( FAMConnection *fc, char *path )
{
    // Tell fam which files and directories to monitor by calling
    // FAMMonitorFile and FAMMonitorDirectory to express interest in files
    // and directories, respectively
    FAMRequest fr;
    int nResult = FAMMonitorDirectory( fc, path, &fr, path );
    if( nResult == -1 )
    {
        log_printf( 0, LOG_ALERT, "Cache::Fam: FAMMonitorDirectory failed for \"%s\"", path );
        return;
    }

    // List files in current directory
    unsigned int nDirCount = 0;
    struct NNCMS_FILE_ENTITY *lpEnts = file_get_directory( 0, path, &nDirCount, file_compare_name_asc );

    for( unsigned int i = 0; i < nDirCount; i++ )
    {
        // Skip . folder and hidden files
        if( lpEnts[i].fname[0] == '.' )
        {
            continue;
        }

        // Step inside directories
        if( !S_ISDIR( lpEnts[i].f_stat.st_mode ) )
        {
            continue;
        }

        char *fullpath = malloc( NNCMS_PATH_LEN_MAX );
        snprintf( fullpath, NNCMS_PATH_LEN_MAX, "%s%s/", path, lpEnts[i].fname );

        // Skip cache directory
        if( strcmp( cacheDir, fullpath ) == 0 )
        {
            continue;
        }

        cache_recursive_monitor( fc, fullpath );
        //FREE( fullpath );

        // Debug
        //log_printf( 0, LOG_DEBUG, "Cache::RecursiveMonitor: %s", fullpath );
    }

}

// #############################################################################

static void *cache_fam( void *var )
{
    // a FAMConnection structure used in all fam procedures
    FAMConnection *fc = (FAMConnection *) var;

    // Watch fileDir folder recursively
    cache_recursive_monitor( fc, fileDir );

    // Loop
    while( 1 )
    {
        // FAMPending returns 1 if an event is waiting and 0 if no event is
        // waiting. It also returns 1 if an error has been encountered.
        // This routine returns immediately to the caller.
        int nResult = FAMPending( fc );
        if( nResult == 0 )
        {
            // We have to do this or the FAMClose will get blocked
            sleep( 1 );
            continue;
        }

        // FAMNextEvent will get the next FAM event.  If there are no FAM events
        // waiting, then the calling application blocks until a FAM event is
        // received.
        FAMEvent fe;
        nResult = FAMNextEvent( fc, &fe );
        if( nResult == -1 )
        {
            log_printf( 0, LOG_ALERT, "Cache::Fam: FAMNextEvent failed" );
            return NULL;
        }

        // Watch new directories
        if( fe.code == FAMCreated )
        {
            // Make fullpath
            char *fullpath = malloc( NNCMS_PATH_LEN_MAX );
            snprintf( fullpath, NNCMS_PATH_LEN_MAX, "%s%s", (char *) fe.userdata, (char *) &(fe.filename) );

            // Check if it is a directory
            struct stat f_stat;
            if( lstat( fullpath, &f_stat ) != 0 )
            {
                log_printf( 0, LOG_ERROR, "Cache::Fam: Could not get \"%s\" file status", fullpath );
            }

            if( S_ISDIR( f_stat.st_mode ) )
            {
                strlcat( fullpath, "/", NNCMS_PATH_LEN_MAX );
                cache_recursive_monitor( fc, fullpath );
                //log_printf( 0, LOG_DEBUG, "Cache::Fam: Watching new folder \"%s\"", fullpath );
            }

            //FREE( fullpath );
        }

        //
        // Degug
        //
        if( fe.code != FAMExists && fe.code != FAMEndExist )
        {
            // Get dir info
            struct NNCMS_FILE_INFO dirInfo;
            file_get_info( &dirInfo, &((char *) fe.userdata)[strlen(fileDir)] );
            if( dirInfo.bExists == false )
                continue;

            //
            // Delete cached files according to the changes
            //

            // Delete file browser and gallery cached html pages
            char fullpath[NNCMS_PATH_LEN_MAX];
            snprintf( fullpath, NNCMS_PATH_LEN_MAX, "%s%s%s", cacheDir, &dirInfo.lpszRelPath[1], "file_browse.html" );
            int nResult = remove( fullpath );
            snprintf( fullpath, NNCMS_PATH_LEN_MAX, "%s%s%s", cacheDir, &dirInfo.lpszRelPath[1], "file_gallery.html" );
            nResult = remove( fullpath );
            //if( nResult != -1 )
            //    log_printf( 0, LOG_DEBUG, "Cache::Fam: Delete \"%s\" ", fullpath );

            // Debug
            // *
#ifdef DEBUG
            char *szFAMCodes[] = { "FAMUnknown", "FAMChanged", "FAMDeleted",
                "FAMStartExecuting", "FAMStopExecuting", "FAMCreated", "FAMMoved",
                "FAMAcknowledge", "FAMExists", "FAMEndExist" };
            log_printf( 0, LOG_DEBUG, "Cache::Fam: Event %s occured with file \"%s\"", szFAMCodes[fe.code], &fe.filename );
            //*/
#endif
        }
    }


    return NULL;
}
#endif

// #############################################################################

bool cache_deinit( )
{
    if( bCacheInit == true )
    {
        bCacheInit = false;

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
                log_printf( 0, LOG_EMERGENCY, "Cache:Deinit: Mutex destroy failed!!" );
            }
        }

#ifdef HAVE_FAM
        // Kill the thread
        pthread_kill( pFAMThread, 0 );

        // Before the application exits, it should call FAMClose to free
        // resources associated with files still being monitored and to close
        // the connection to fam.
        int nResult = FAMClose( &g_fc );
        if( nResult == -1 )
        {
            log_printf( 0, LOG_ERROR, "Cache::DeInit: FAMClose unable to free resources associated with files still being monitored and closes a fam connection" );
            return false;
        }
#endif

        return true;
    }

    return false;
}

// #############################################################################

// Returns ZERO if nothing found
struct NNCMS_CACHE *cache_find( struct NNCMS_THREAD_INFO *req, char *lpszName )
{
    //
    // Find cached data in memory
    //

    if( bCacheMemory == true )
    {
        if( pthread_mutex_lock( &cache_mutex ) != 0 )
        {
            log_printf( req, LOG_EMERGENCY, "Cache::Find: Can't acquire readlock!!" );
            exit( -1 );
        }
        for( unsigned int i = 0; i < NNCMS_CACHE_MEM_MAX; i++ )
        {
            if( strcmp( lpszName, cachedb[i].lpszName ) == 0)
            {
                // Update time stamp
                cachedb[i].timeStamp = time( 0 );

                // Debug
                //log_printf( req, LOG_DEBUG, "Cache::Find: \"%s\" acquired, data size %u", cachedb[i].lpszName, cachedb[i].nSize );

                // Return cached data
                pthread_mutex_unlock( &cache_mutex );
                return &cachedb[i];
            }
        }

        //
        // If data in memory was not found, then check in filesystem
        //
        pthread_mutex_unlock( &cache_mutex );
    }

    // Skip beginning slash
    if( *lpszName == '/' )
        lpszName++;

    // Create full path to file
    char szFullName[NNCMS_PATH_LEN_MAX];
    snprintf( szFullName, sizeof(szFullName), "%s%s", cacheDir, lpszName );
    //log_printf( req, LOG_DEBUG, "Cache::Find: %s", szFileName );

    // Try to open it
    FILE *pFile = fopen( szFullName, "rb" );
    if( pFile == 0 )
        goto done;

    // Create buffer for that file
    fseek( pFile, 0, SEEK_END );
    unsigned long int nSize = ftell( pFile );
    rewind( pFile );
    char *lpData = MALLOC( nSize );
    if( lpData == 0 )
    {
        // ohshi..
        log_printf( req, LOG_CRITICAL, "Cache::Find: Could not allocate %lu bytes (%s)", nSize, szFullName );
        goto done;
    }

    //log_printf( req, LOG_DEBUG, "Cache::Find: %lu bytes (%s)", nSize, szFullName );

    // Read and close it, then restore it to memory cache
    fread( lpData, 1, nSize, pFile );
    fclose( pFile );


    return cache_store( req, lpszName, lpData, nSize );

done:
    return 0;
}

// #############################################################################

struct NNCMS_CACHE *cache_store( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpData, size_t nSize )
{
    //
    // Store data in memory
    //

    struct NNCMS_CACHE *lpCache = &cachedb[0];
    if( bCacheMemory == true )
    {
        // Acquire write lock
        if( pthread_mutex_lock( &cache_mutex ) != 0 )
        {
            log_printf( req, LOG_EMERGENCY, "Cache::Store: Can't acquire writelock!!" );
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
        if( lpCache->lpData != 0 )
        {
            FREE( lpCache->lpData );
            lpCache->lpData = 0;
        }

        // Replace with new data
        strlcpy( lpCache->lpszName, lpszName, NNCMS_PATH_LEN_MAX );
        lpCache->lpData = lpData;
        lpCache->nSize = nSize;
        lpCache->timeStamp = currentTime;
    }

    // Debug
    //log_printf( req, LOG_DEBUG, "Cache::Store: \"%s\" stored, data size %u", lpszName, nSize );

    //
    // Store data to file system
    //

    // Skip beginning slash
    if( *lpszName == '/' )
        lpszName++;

    // Create full path to file
    char szFullName[NNCMS_PATH_LEN_MAX];
    snprintf( szFullName, sizeof(szFullName), "%s%s", cacheDir, lpszName );
    //log_printf( req, LOG_DEBUG, "Cache::Store: %s", szFileName );

    // Check if already exists
    //if( access( szFullName, F_OK ) == 0 )
    //    goto done;
    struct stat stFile;
    if( stat( szFullName, &stFile ) == 0 )
    {
        if( S_ISREG(stFile.st_mode) )
        {
            goto done;
        }
    }

    // Create path to file
    char *lpszFileName = rindex( szFullName, '/' );
    if( lpszFileName == 0 )
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

    // NOW WE CAN STORE DATA IN FILE!!!1
    // (try to) Create file
    //  w       Truncate  file  to  zero  length  or create text file
    //          for writing.  The stream is positioned at the
    //          beginning of the file.
    FILE *pFile = fopen( szFullName, "wb" );

    // Write data
    fwrite( lpData, 1, nSize, pFile );

    // Close file
    fclose( pFile );


    struct NNCMS_CACHE *lpCacheTemp;
done:

    lpCacheTemp = MALLOC( sizeof(struct NNCMS_CACHE) );
    if( bCacheMemory == true )
    {
        // Unlock
        pthread_mutex_unlock( &cache_mutex );

        memcpy( lpCacheTemp, lpCache, sizeof(struct NNCMS_CACHE) );
    }
    else
    {
        strlcpy( lpCacheTemp->lpszName, lpszName, sizeof(lpCacheTemp->lpszName) );
        lpCacheTemp->lpData = lpData;
        lpCacheTemp->nSize = nSize;
    }
    lpCacheTemp->fileTimeStamp = stFile.st_mtime;

    return lpCacheTemp;
}

// #################################################################################

#define NNCMS_CACHE_CLEAR_HTML          (1 << 0)
#define NNCMS_CACHE_CLEAR_THUMBNAILS    (1 << 1)

void cache_clear( char *path, char fClearType )
{
    // List files in current directory
    unsigned int nDirCount = 0;
    struct NNCMS_FILE_ENTITY *lpEnts = file_get_directory( 0, path, &nDirCount, file_compare_name_asc );
    if( lpEnts == 0 )
    {
        log_printf( 0, LOG_ERROR, "Cache::Clear: Cache directory not found: %s", path );
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
                    //log_printf( 0, LOG_DEBUG, "Cache::Clear: Remove: %s", fullpath );
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
            //log_printf( 0, LOG_DEBUG, "Cache::Clear: Dir: %s", fullpath );
        }
    }
}

// #################################################################################

void cache_admin( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Cache admin";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/mimetypes/package-x-generic.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS logTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    logTemplate[2].szValue = req->g_sessionid;

    // First we check what permissions do user have
    if( acl_check_perm( req, "cache", req->g_username, "clear" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Cache::Clear: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user pushed big gray shiny "Clear" button
    //
    struct NNCMS_VARIABLE *httpVarClear = main_get_variable( req, "cache_clear" );
    if( httpVarClear != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        struct NNCMS_VARIABLE *httpVarClearHtml = main_get_variable( req, "cache_clear_html" );
        struct NNCMS_VARIABLE *httpVarClearThumbnail = main_get_variable( req, "cache_clear_thumbnail" );

        // Buffer for logging
        char szResult[16]; *szResult = 0;

        char fClearType = 0;
        if( httpVarClearHtml != 0 && strcasecmp( httpVarClearHtml->lpszValue, "on" ) == 0 )
        {
            // Delete HTML data
            strcat( szResult, " HTML" );
            fClearType |= NNCMS_CACHE_CLEAR_HTML;
        }

        if( httpVarClearThumbnail != 0 && strcasecmp( httpVarClearThumbnail->lpszValue, "on" ) == 0 )
        {
            // Delete thumbnails
            strcat( szResult, " Thumbnails" );
            fClearType |= NNCMS_CACHE_CLEAR_THUMBNAILS;
        }
        cache_clear( cacheDir, fClearType );

        // Log results
        if( *szResult != 0 )
            log_printf( req, LOG_INFO, "Cache::Clear: Cache deleted:%s", szResult );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Load rename page
    template_iparse( req, TEMPLATE_CACHE_ADMIN, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, logTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #################################################################################
