// #############################################################################
// Source file: main.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// на память из http://en.wikipedia.org/wiki/C_preprocessor
//
//#define dumpme(x, fmt) printf("%s:%u: %s=" fmt, __FILE__, __LINE__, #x, x)
//
//int some_function() {
//    int foo;
//    /* [a lot of complicated code goes here] */
//    dumpme(foo, "%d");
//    /* [more complicated code goes here] */
//}

// ololo

// Test
//void database_xx(){}

// #############################################################################
// includes of system headers
//

#include "config.h"

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <process.h>
#else
extern char **environ;
#endif

#include <fcgiapp.h>
#include <pthread.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>
#include <errno.h>

#ifdef HAVE_IMAGEMAGICK
//#include <magick/MagickCore.h>
#include <wand/MagickWand.h>
#endif // HAVE_IMAGEMAGICK

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "user.h"
#include "file.h"
#include "template.h"
#include "cfg.h"
#include "log.h"
#include "ban.h"

#include "database.h"
#include "view.h"
#include "acl.h"
#include "post.h"
#include "cache.h"

#include "strnstr.h"
#include "strlcpy.h"
#include "strlcat.h"
#include "strdup.h"

#include "memory.h"

// sqlite test: char *zErrMsg = 0; if( sqlite3_exec( db, "select * from users", callback, 0, &zErrMsg ) != SQLITE_OK ) { fprintf( stderr, "SQL error: %s\n", zErrMsg ); sqlite3_FREE( zErrMsg ); }

// #############################################################################
// global variables
//

// Website Home URL
char homeURL[NNCMS_PATH_LEN_MAX] = "/";

// Global Timestamp Format
char szTimestampFormat[128] = "%H:%M:%S %d.%m.%Y";

// Path to cache
extern char cacheDir[NNCMS_PATH_LEN_MAX];

// Path to blocks
char blockDir[NNCMS_PATH_LEN_MAX] = "./blocks/";

// Number of threads
int nThreads = 1;

// Use garbage collector or not?
int nGarbageCollector = 0;

// Default value of quiet attribute in "include" tag
bool bIncludeDefaultQuiet = false;

// Use memory cache?
extern bool bCacheMemory;

// Display page generation time
bool bPageGen = false;

// Enable Rewrite rules
bool bRewrite = false;

// No cache option
extern bool bTemplateCache;

// Settings
struct NNCMS_OPTION g_options[] =
    {
        { /* szName */ "homeURL", /* nType */ NNCMS_STRING, /* lpMem */ &homeURL },
        { /* szName */ "tSessionExpires", /* nType */ NNCMS_INTEGER, /* lpMem */ &tSessionExpires },
        { /* szName */ "tCheckSessionInterval", /* nType */ NNCMS_INTEGER, /* lpMem */ &tCheckSessionInterval },
        { /* szName */ "TimestampFormat", /* nType */ NNCMS_STRING, /* lpMem */ &szTimestampFormat },
        { /* szName */ "databasePath", /* nType */ NNCMS_STRING, /* lpMem */ &szDatabasePath },
        { /* szName */ "fileDir", /* nType */ NNCMS_STRING, /* lpMem */ &fileDir },
        { /* szName */ "templateDir", /* nType */ NNCMS_STRING, /* lpMem */ &templateDir },
        { /* szName */ "cacheDir", /* nType */ NNCMS_STRING, /* lpMem */ &cacheDir },
        { /* szName */ "blockDir", /* nType */ NNCMS_STRING, /* lpMem */ &blockDir },
        { /* szName */ "logFile", /* nType */ NNCMS_STRING, /* lpMem */ &szLogFile },
        { /* szName */ "threads", /* nType */ NNCMS_INTEGER, /* lpMem */ &nThreads },
        { /* szName */ "garbage_collector", /* nType */ NNCMS_INTEGER, /* lpMem */ &nGarbageCollector },
        { /* szName */ "include_default_quiet", /* nType */ NNCMS_BOOL, /* lpMem */ &bIncludeDefaultQuiet },
        { /* szName */ "cache_memory", /* nType */ NNCMS_BOOL, /* lpMem */ &bCacheMemory },
        { /* szName */ "rewrite", /* nType */ NNCMS_BOOL, /* lpMem */ &bRewrite },
        { /* szName */ "print_page_gen", /* nType */ NNCMS_BOOL, /* lpMem */ &bPageGen },
        { /* szName */ "template_cache", /* nType */ NNCMS_BOOL, /* lpMem */ &bTemplateCache },
        { /* szName */ "post_depth", /* nType */ NNCMS_INTEGER, /* lpMem */ &nPostDepth },
        { /* szName */ 0, /* nType */ 0, /* lpMem */ 0 }
    };

// All pages
struct NNCMS_FUNCTION funcPages[] =
{
    // szName           szDesc              lpFunc
    { "index",          "Main",             main_index },
    { "info",           "Information",      main_info },
    { "acl_add",        "Add ACL",          acl_add },
    { "acl_edit",       "Edit ACL",         acl_edit },
    { "acl_delete",     "Delete ACL",       acl_delete },
    { "acl_view",       "View ACL",         acl_view },
    { "ban_add",        "Add Ban",          ban_add },
    { "ban_view",       "View Ban",         ban_view },
    { "ban_edit",       "Edit Ban",         ban_edit },
    { "ban_delete",     "Delete Ban",       ban_delete },
    { "cache_admin",    "Cache admin",      cache_admin },
    { "cfg_view",       "View config",      cfg_view },
    { "cfg_edit",       "Edit config",      cfg_edit },
    { "log_view",       "View log",         log_view },
    { "log_clear",      "Clear log",        log_clear },
    { "post_add",       "Add post",         post_add },
    { "post_board",     "Message board",    post_board },
    { "post_edit",      "Edit post",        post_edit },
    { "post_modify",    "Modify post",      post_modify },
    { "post_news",      "News",             post_news },
    { "post_delete",    "Delete post",      post_delete },
    { "post_view",      "View post",        post_view },
    { "post_remove",    "Remove post",      post_remove },
    { "post_reply",     "Reply post",       post_reply },
    { "post_root",      "All posts",        post_root },
    { "post_rss",       "Syndication",      post_rss },
    { "post_topics",    "Topics",           post_topics },
    { "user_register",  "Add user",         user_register },
    { "user_login",     "Log in",           user_login },
    { "user_logout",    "Log out",          user_logout },
    { "user_sessions",  "User sessions",    user_sessions },
    { "user_view",      "View user",        user_view },
    { "user_profile",   "User profile",     user_profile },
    { "user_edit",      "Edit user",        user_edit },
    { "user_delete",    "Delete user",      user_delete },
    { "view",           "Viewer®",          view_show },
    { "thumbnail",      "Thumbnail",        view_thumbnail },
    { "file_browse",    "Browse files",     file_browse },
    { "file_rename",    "Rename file",      file_rename },
    { "file_edit",      "Edit file",        file_edit },
    { "file_gallery",   "Gallery",          file_gallery },
    { "file_upload",    "Upload file",      file_upload },
    { "file_delete",    "Delete file",      file_delete },
    { "file_mkdir",     "Make directory",   file_mkdir },
    { "file_add",       "Add file",         file_add },
    { 0, 0, 0 } // Null terminating row
};

// Setting this value to 1 will terminate the application
bool bDone = false;

// phreads data
struct NNCMS_THREAD_INFO **pThreadList;

#ifdef HAVE_IMAGEMAGICK
ExceptionInfo *lpException = 0;
#endif // HAVE_IMAGEMAGICK

// #############################################################################
// functions
//

int main( int argc, char *argv[] )
{
    char szWorkingDir[128]; // Holds current working directory
    struct timeval timeout; // Some strange timeout structure
    int result;             // Temporary variable for return values

    //
    // Ensure that PIPE signals are either handled or ignored.
    // If a client connection breaks while the server is using
    // it then the application will be sent a SIGPIPE.  If you
    // don't handle it then it'll terminate your application.
    //
    signal( SIGPIPE, SIG_IGN );

    //
    // The random algorithm should be initialized to different
    // starting points using function srand to generate more
    // realistic random numbers.
    //
    srand( time( 0 ) );

    //
    // Initialization of locale data
    //
    //setlocale( LC_ALL, "" );
    //bindtextdomain( PACKAGE, LOCALEDIR );
    //textdomain( PACKAGE );

    // Register a function to be called at normal process termination
    if( atexit( atquit ) != 0 ) quit( EXIT_FAILURE );

    // Bind event handler for interruption signals
    signal( SIGINT, quit );

    //
    // Load config
    //

    char bCfgLoaded = 0;

    bCfgLoaded |= cfg_parse_file( "/etc/nncms.conf", g_options );
    bCfgLoaded |= cfg_parse_file( "nncms.conf", g_options );

    if( bCfgLoaded == 1 )
    {
#ifdef VERBOSE_DEBUG
        log_printf( 0, LOG_DEBUG, "Cfg::Init: Config file loaded" );
#endif // VERBOSE_DEBUG
    }
    else
    {
        log_printf( 0, LOG_WARNING, "Cfg::Init: Config file not found" );
    }

    //
    // Initialize modules
    //

    // Hardcore macro
    #ifdef VERBOSE_DEBUG
        #define VERBOSE_LOAD(xmodule) \
            log_printf( 0, LOG_DEBUG, "Main: Module " #xmodule " has been successfully initialized" );
    #else
        #define VERBOSE_LOAD(xmodule) // #xmodule loaded
    #endif // VERBOSE_DEBUG
    #define INIT_MODULE(xmodule) \
        if( xmodule##_##init() == 0 ) \
        { \
            log_printf( 0, LOG_ERROR, "Main: Unable to initialize " #xmodule " module" ); \
            quit( EXIT_FAILURE ); \
        } \
        else \
        { \
            VERBOSE_LOAD(xmodule) \
        };

    if( nGarbageCollector != 0 )
    {
        memory_set_size( nGarbageCollector );
        INIT_MODULE(memory)
    }
    //sleep(5);
    INIT_MODULE(database)
    //INIT_MODULE(log)
    //INIT_MODULE(acl)
    INIT_MODULE(cache)
    //INIT_MODULE(template)

    #undef INIT_MODULE
    #undef VERBOSE_LOAD

    // Check homeURL
    size_t nHomeUrlLen = strlen( homeURL );
    nHomeUrlLen--;
    if( homeURL[nHomeUrlLen] == '/' )
        homeURL[nHomeUrlLen] = 0;

    // Initialize ImageMagick
#ifdef HAVE_IMAGEMAGICK
    // Initialize the MagickWand environment
    MagickWandGenesis();

    //MagickCoreGenesis( *argv, MagickTrue );
    //lpException = AcquireExceptionInfo( );
#endif // HAVE_IMAGEMAGICK

    // Init FastCGI stuff
    FCGX_Init();

    // Init pthread struct
    pThreadList = MALLOC( nThreads * sizeof(void *) );

    // Create threads
    for( int i = 0; i < nThreads; i++ )
    {
        // This structure is associated with a thread
        struct NNCMS_THREAD_INFO *threadInfo = MALLOC( sizeof(struct NNCMS_THREAD_INFO) );
        memset( threadInfo, 0, sizeof(struct NNCMS_THREAD_INFO) );

        // Store thread
        pThreadList[i] = threadInfo;

        // Thread stuff
        threadInfo->nThreadID = i;
        threadInfo->pThread = MALLOC( sizeof(pthread_t) );

        //
        // Here goes initialization of FastCGI
        //
        threadInfo->fcgi_request = MALLOC( sizeof(FCGX_Request) );
        FCGX_InitRequest( threadInfo->fcgi_request, 0, 0 );

        // Trying to avoid allocating large chunks of memory every request
        #define ALLOC_CHUNK(chunk_name,chunk_size) \
            chunk_name = MALLOC( chunk_size ); \
            if( chunk_name == 0 ) \
            { \
                log_printf( threadInfo, LOG_ALERT, "Main::Loop: Unable to allocate memory for buffer " #chunk_name ", required %u bytes, thread №%i dropped!", chunk_size, i ); \
                break; \
            }

        ALLOC_CHUNK( threadInfo->lpszBuffer, NNCMS_PAGE_SIZE_MAX + 1 )
        ALLOC_CHUNK( threadInfo->lpszOutput, NNCMS_PAGE_SIZE_MAX + 1 )
        ALLOC_CHUNK( threadInfo->lpszTemp, NNCMS_PAGE_SIZE_MAX + 1 )
        ALLOC_CHUNK( threadInfo->lpszFrame, NNCMS_PAGE_SIZE_MAX + 1 )
        ALLOC_CHUNK( threadInfo->lpszBlockContent, NNCMS_BLOCK_LEN_MAX + 1 )
        ALLOC_CHUNK( threadInfo->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX + 1 )
        #undef ALLOC_CHUNK

        // Init modules
        log_init( threadInfo );
        acl_init( threadInfo );
        template_init( threadInfo );
        user_init( threadInfo );
        post_init( threadInfo );
        ban_init( threadInfo );

        // Create the thread
        pthread_create( threadInfo->pThread, 0, main_loop, (void *) threadInfo );
    }

    log_printf( pThreadList[0], LOG_INFO, "Main: nncms started, created %u threads", nThreads );
    //main_loop( 0 );

    while( 1 )
    {
        sleep( 1 );
    }

    return 0;
}

// #############################################################################

int main_add_variable( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszValue)
{
    for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
    {
        if( req->variables[i].lpszValue == 0 )
        {
            req->variables[i].lpszName = strdup_d( lpszName );
            req->variables[i].lpszValue = strdup_d( lpszValue );

            return i;
        }
    }

    return 0;
}

// #############################################################################

struct NNCMS_VARIABLE *main_get_variable( struct NNCMS_THREAD_INFO *req, char *lpszName )
{
    for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
    {
        if( req->variables[i].lpszName != 0 && req->variables[i].lpszValue != 0 )
        {
            if( strcmp( req->variables[i].lpszName, lpszName ) == 0 )
                return &req->variables[i];
        }
    }

    return 0;
}

// #############################################################################

char main_from_hex( char c )
{
    return  c >= '0' && c <= '9' ?  c - '0'
            : c >= 'A' && c <= 'F' ? c - 'A' + 10
            : c - 'a' + 10;     /* accept small letters just in case */
}

// #############################################################################

char *main_unescape( char *str )
{
    char *p = str;
    char *q = str;
    static char blank[] = "";

    if( !str ) return blank;
    while( *p )
    {
        if( *p == '%' )
        {
            p++;
            if( *p ) *q = main_from_hex( *p++ ) * 16;
            if( *p ) *q = (*q + main_from_hex( *p++ ));
            q++;
        }
        else
        {
            if( *p == '+' )
            {
                *q++ = ' ';
                p++;
            }
            else
            {
                *q++ = *p++;
            }
        }
    }

    *q++ = 0;
    return str;
}

// #############################################################################

char *main_escape( const char *s, int len, int *new_length )
{
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;
    unsigned char const hexchars[] = "0123456789ABCDEF";

    from = s;
    end = s + len;
    start = to = (unsigned char *) malloc( 3 * len + 1 );

    while( from < end )
    {
        c = *from++;

        if( c == ' ' )
        {
            *to++ = '+';
        }
        else if( ( c < '0' && c != '-' && c != '.' ) ||
                 ( c < 'A' && c > '9' ) ||
                 ( c > 'Z' && c < 'a' && c != '_' ) ||
                 ( c > 'z' ) )
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }
    *to = 0;
    if( new_length )
    {
        *new_length = to - start;
    }
    return (char *) start;
}

// #############################################################################

void main_store_data( struct NNCMS_THREAD_INFO *req, char *query )
{
    char *cp, *cp2, var[50], *val, *tmpVal;

    if( !query ) return;

    cp = query;
    cp2 = var;
    memset( var, 0, sizeof(var) );
    val = 0;
    while( *cp )
    {
        if( *cp == '=' )
        {
            cp++;
            *cp2 = 0;
            val = cp;
            continue;
        }
        if( *cp == '&' )
        {
            *cp = 0;
            tmpVal = main_unescape( val );
            main_add_variable( req, var, tmpVal );
            cp++;
            cp2 = var;
            val = 0;
            continue;
        }
        if( val )
        {
            cp++;
        }
        else
        {
            *cp2 = *cp++;
            if( *cp2 == '.' )
            {
                strcpy( cp2, "_dot_" );
                cp2 += 5;
            }
            else
            {
                cp2++;
            }
        }
    }
    *cp = 0;
    tmpVal = main_unescape( val );
    main_add_variable( req, var, tmpVal );
}

// #############################################################################

char *main_get_var( char *lpBuf, char *lpszDest, unsigned int nDestSize, char *lpszVarName )
{
    unsigned int i; // Standart iterations counter
    unsigned int nVarNameLen = strlen( lpszVarName );

    // Sample:
    //
    // "Content-Disposition: form-data; name=\"upload_file\"; filename=\".gtkrc\"\r\n
    // Content-Type: application/octet-stream\r\n
    // \r\n
    // gtk-fallback-icon-theme = \"gnome\"\n
    // \r\n",
    // '-' <repeats 29 times>, "1330065118175668826815"...

    if( strncasecmp( lpBuf, lpszVarName, nVarNameLen ) == 0 )
    {
        lpBuf += nVarNameLen;

        // Copy value to destination buffer
        for( i = 0; i < nDestSize; i++ )
        {
            if( (lpBuf[i] == '\r' && lpBuf[i + 1] == '\n') ||
                lpBuf[i] == ';' )
            {
                break;
            }

            // wtf?
            if( lpBuf[i] == 0 ) return 0;

            lpszDest[i] = lpBuf[i];
        }

        // Terminate the string
        if( lpszVarName[nVarNameLen - 1] == '"' ) lpszDest[i - 1] = 0;
        else lpszDest[i] = 0;

        // wtf?
        if( i == nDestSize ) return 0;

        // Skip (';' or CR or LF or CRLF) and ' '
        lpBuf += i; lpBuf++;
        if( *lpBuf == '\n' ) lpBuf++;
        if( *lpBuf == ' ' ) lpBuf++;
    }

    return lpBuf;
}

// #############################################################################

// RFC1867: Form-based File Upload in HTML
void main_store_data_rfc1867( struct NNCMS_THREAD_INFO *req, char *query )
{
    // Boundary header values
    char szContentDisposition[NNCMS_VAR_NAME_LEN_MAX];
    char szContentType[NNCMS_VAR_NAME_LEN_MAX];
    char szFileName[NNCMS_VAR_NAME_LEN_MAX];
    char szName[NNCMS_VAR_NAME_LEN_MAX];
    char *lpBuf = query;

find_boundary:

    // Find a boundary and skip it
    while( *lpBuf )
    {
        if( strncmp( lpBuf, req->szBoundary, req->nBoundaryLen ) == 0 )
        {
            lpBuf += req->nBoundaryLen + 2 /* \r\n */;
            goto parse_header;
        }
        lpBuf++;
    }

    // No more boundaries left
    return;

parse_header:
    // Parse boundary header
    memset( szContentDisposition, 0, sizeof(szContentDisposition) );
    memset( szContentType, 0, sizeof(szContentType) );
    memset( szFileName, 0, sizeof(szFileName) );
    memset( szName, 0, sizeof(szName) );
    while( *lpBuf )
    {
        if( lpBuf[0] == '\r' && lpBuf[1] == '\n' )
        {
            // Variable content starts here
            lpBuf += 2;

            // Find next boundary
            char *szNextBoundary = strnstr( lpBuf, req->szBoundary, req->nContentLenght - (lpBuf - query) );
            if( szNextBoundary == 0 )
            {
                //log_printf( req, LOG_ALERT, "Main::StoreDataRFC1867: Next boundary not found!" );
                return;
            }
            unsigned int nValueLen = (szNextBoundary - 3 /* wtf? */) - lpBuf;

            // Create buffer for value
            char *szValue = 0;
            if( nValueLen < NNCMS_PAGE_SIZE_MAX && *szFileName != 0 )
            {
                szValue = MALLOC( nValueLen );
            }
            else if( nValueLen < NNCMS_UPLOAD_LEN_MAX )
            {
                szValue = MALLOC( nValueLen );
            }

            // Copy boundary data to it
            if( szValue != 0 )
            {
                memcpy( szValue, lpBuf, nValueLen );
                nValueLen--; lpBuf += nValueLen;
                szValue[nValueLen] = 0;

                if( *szFileName == 0 )
                {
                    // Add to global variables if it's not a file
                    main_add_variable( req, szName, szValue );
                    FREE( szValue );
                }
                else
                {
                    // Add it to a list of uploaded files
                    struct NNCMS_FILE *curFile = 0;
                    if( req->lpUploadedFiles == 0 )
                    {
                        req->lpUploadedFiles = MALLOC( sizeof(struct NNCMS_FILE) );
                        curFile = req->lpUploadedFiles;
                    }
                    else
                    {
                        curFile = req->lpUploadedFiles;
                        while( curFile->lpNext ) curFile = curFile->lpNext;
                        curFile->lpNext = MALLOC( sizeof(struct NNCMS_FILE) );
                        curFile = curFile->lpNext;
                    }

                    if( curFile != 0 )
                    {
                        strlcpy( curFile->szName, szName, sizeof(curFile->szName) );
                        strlcpy( curFile->szFileName, szFileName, sizeof(curFile->szFileName) );
                        curFile->lpData = szValue;
                        curFile->nSize = nValueLen;
                        curFile->lpNext = 0;
                    }
                }
            }
            else
            {
                log_printf( req, LOG_WARNING, "Main::StoreDataRFC1867: Data value (%u bytes) is too big or memory block was not allocated", nValueLen );
            }

            //printf( "%s; %s; %s; %s\n", szContentDisposition, szContentType, szName, szFileName );

            // Try to find next boundary
            goto find_boundary;
        }

        char *lpResult;

        lpResult = main_get_var( lpBuf, szContentDisposition, sizeof(szContentDisposition), "Content-Disposition: " ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }
        lpResult = main_get_var( lpBuf, szContentType, sizeof(szContentType), "Content-Type: " ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }
        lpResult = main_get_var( lpBuf, szFileName, sizeof(szFileName), "filename=\"" ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }
        lpResult = main_get_var( lpBuf, szName, sizeof(szName), "name=\"" ); if( lpResult == 0 ) { return; } else if( lpResult > lpBuf ) { lpBuf = lpResult; continue; }

        lpBuf = strstr( lpBuf, "\r\n\r\n" );
        if( lpBuf == 0 )
        {
            // No variable no content found, looks like syntaxis error, quit
            log_printf( req, LOG_ALERT, "Main::StoreDataRFC1867: Content of part not found" );
            break;
        }
        lpBuf += 2;
    }

    // wtf?
    log_printf( req, LOG_ALERT, "Main::StoreDataRFC1867: Unexpected null terminating character in standart input" );
    return;
}

// #############################################################################

unsigned long long rdtsc( )
{
    unsigned int tickl, tickh;
    __asm__ __volatile__( "rdtsc" : "=a"(tickl), "=d"(tickh) );
    return ((unsigned long long) tickh << 32) | tickl;
}

// #############################################################################

static void *main_loop( void *var )
{
    // This structure is passed to all functions
    struct NNCMS_THREAD_INFO *threadInfo = var;
    static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

    // Accept loop
    while( 1 )
    {
        /* Some platforms require accept() serialization, some don't.. */
        pthread_mutex_lock( &accept_mutex );
        int result = FCGX_Accept_r( threadInfo->fcgi_request );
        pthread_mutex_unlock( &accept_mutex );

        // Page generation measurements
        //struct timespec tp1;
        unsigned long long tp1;
        if( bPageGen == true )
        {
            //clock_gettime( CLOCK_REALTIME, &tp1 );
            tp1 = rdtsc( );
        }

        // Fetch HTTPd software type
        if( threadInfo->httpd_type == HTTPD_NONE )
        {
            threadInfo->lpszHTTPd = FCGX_GetParam( "SERVER_SOFTWARE", threadInfo->fcgi_request->envp );
            threadInfo->httpd_type = HTTPD_UNKNOWN;
            if( threadInfo->lpszHTTPd != 0 )
            {
                if( strncasecmp( threadInfo->lpszHTTPd, "lighttpd", 8 ) == 0 )
                    threadInfo->httpd_type = HTTPD_LIGHTTPD;
                else if( strncasecmp( threadInfo->lpszHTTPd, "nginx", 5 ) == 0 )
                    threadInfo->httpd_type = HTTPD_NGINX;
            }
        }

        // Check if this user is banned
        threadInfo->g_useraddr = FCGX_GetParam( "REMOTE_ADDR", threadInfo->fcgi_request->envp );
        threadInfo->g_useragent = FCGX_GetParam( "HTTP_USER_AGENT", threadInfo->fcgi_request->envp );
        struct in_addr addr;
        inet_aton( threadInfo->g_useraddr, &addr );
        if( ban_check( threadInfo, &addr ) == true )
        //if( 0 )
        {
            // User is banned
            main_set_response( threadInfo, "403 Forbidden" );

            // Send generated html to client
            main_send_headers( threadInfo, 16, 0 );
            FCGX_PutS( "You are banned.\n", threadInfo->fcgi_request->out );

            // Finish request
            FCGX_Finish_r( threadInfo->fcgi_request );

            // Use less as possible CPU power for banned users
            continue;
        }

        // Check for error and global die signal
        if( result < 0 || bDone == true )
            break;

        // Thread debug
        //log_printf( threadInfo, LOG_DEBUG, "Main::Loop: thread №%i got request", nThreadID );

        // Get some important information
        threadInfo->lpszRequestURI = FCGX_GetParam( "SCRIPT_NAME", threadInfo->fcgi_request->envp );
        threadInfo->lpszRoot = FCGX_GetParam( "DOCUMENT_ROOT", threadInfo->fcgi_request->envp );
        threadInfo->lpszContentType = FCGX_GetParam( "CONTENT_TYPE", threadInfo->fcgi_request->envp );
        char *lpszContentLenght = FCGX_GetParam("CONTENT_LENGTH", threadInfo->fcgi_request->envp);
        threadInfo->nContentLenght = strtol( (lpszContentLenght != 0 ? lpszContentLenght : "0"), 0, 10 );

        // Get boundary data if available
        if( threadInfo->lpszContentType != 0 )
        {
            char *lpszTemp = index( threadInfo->lpszContentType, ';' );
            if( lpszTemp != 0 )
            {
                *lpszTemp = 0;
                if( strncasecmp( lpszTemp + 2, "boundary=", 9 ) == 0 )
                {
                    strlcpy( threadInfo->szBoundary, lpszTemp + 11, NNCMS_BOUNDARY_LEN_MAX );
                    threadInfo->nBoundaryLen = strlen( threadInfo->szBoundary );
                }
            }
        }

        // Store query data
        main_store_data( threadInfo, FCGX_GetParam( "QUERY_STRING", threadInfo->fcgi_request->envp ) );

        // Get more data if available (from standart input)
        if( threadInfo->lpszContentType != 0 && threadInfo->nContentLenght > 0 )
        {
            // Check lenght
            if( threadInfo->nContentLenght > NNCMS_UPLOAD_LEN_MAX )
            {
                log_printf( threadInfo, LOG_WARNING, "Main::Loop: Content lenght is too big (%u > %u)", threadInfo->nContentLenght, NNCMS_UPLOAD_LEN_MAX );
            }
            else
            {
                // Retreive data
                threadInfo->lpszContentData = MALLOC( threadInfo->nContentLenght + 1 );
                if( threadInfo->lpszContentData == 0 )
                {
                    log_printf( threadInfo, LOG_ALERT, "Main::Loop: Unable to allocate %u bytes for content (max: %u)", threadInfo->nContentLenght, NNCMS_UPLOAD_LEN_MAX );
                }
                else
                {
                    int nResult = FCGX_GetStr( threadInfo->lpszContentData, threadInfo->nContentLenght, threadInfo->fcgi_request->in );
                    threadInfo->lpszContentData[nResult] = 0;
                    if( nResult < threadInfo->nContentLenght )
                        log_printf( threadInfo, LOG_WARNING, "Main::Loop: Received %u, required %u. Not enough bytes received on standard input", nResult, threadInfo->nContentLenght );

                    if( strcmp( threadInfo->lpszContentType, "application/x-www-form-urlencoded" ) == 0 )
                        main_store_data( threadInfo, threadInfo->lpszContentData );

                    // RFC1867: Form-based File Upload in HTML
                    if( strcmp( threadInfo->lpszContentType, "multipart/form-data" ) == 0 )
                        main_store_data_rfc1867( threadInfo, threadInfo->lpszContentData );
                }
            }
        }

        // And even more data (from cookies)
        main_store_data( threadInfo, FCGX_GetParam( "HTTP_COOKIE", threadInfo->fcgi_request->envp ) );

        // Prepare default response headers
        strcpy( threadInfo->szResponseHeaders, "Server: " PACKAGE_NAME " " PACKAGE_VERSION "\n" );
        strcpy( threadInfo->szResponseContentType, "text/html" );
        strcpy( threadInfo->szResponse ,"200 OK\n" );
        threadInfo->bHeadersSent = 0;

        // What page was requested?
        char *lpszTempVar = strchr( threadInfo->lpszRequestURI, '.' );
        if( lpszTempVar != 0 ) *lpszTempVar = 0;

        // Search function for requested page
        unsigned int i = 0;
        while( funcPages[i].lpFunc != 0 )
        {
            if( strcmp( &(threadInfo->lpszRequestURI[1]), funcPages[i].szName ) == 0 )
            {
                // Call the function for requested page and break the loop
                funcPages[i].lpFunc( threadInfo );
                goto exit_loop;
            }
            i = i + 1;  // Next function
        }

        // If we get here, then function was not found, show default page or 404
        main_index( threadInfo );

exit_loop:

        // Page generation measurements
        //tp1 = rdtsc( );
        //usleep( 1000*1000 );
        if( bPageGen == true )
        {
            //struct timespec tp2;
            //clock_gettime( CLOCK_REALTIME, &tp2 );
            //double tics = (double) (tp2.tv_sec-tp1.tv_sec) + ((double) (tp2.tv_nsec-tp1.tv_nsec)) / (1000.0*1000.0*1000.0);

            unsigned long long tp2 = rdtsc( );

            unsigned long long freq = 2673458266ull;
            //unsigned long long freq = 1600273409ull;
            double tics = (double) tp2/freq - (double) tp1/freq;
            log_printf( threadInfo, LOG_DEBUG, "Main::Loop: Page generation time: %.09f seconds.", tics );
        }

        // Finish request
        FCGX_Finish_r( threadInfo->fcgi_request );

        // Reset session
        user_reset_session( threadInfo );

        // Reset variables
        for( unsigned int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
        {
            // Debug variables
            //printf("[%2u] name=%s\nval=%s\n\n", i, threadInfo->variables[i].lpszName, threadInfo->variables[i].lpszValue );

            if( threadInfo->variables[i].lpszName != 0 )
                FREE( threadInfo->variables[i].lpszName );

            if( threadInfo->variables[i].lpszValue != 0 )
                FREE( threadInfo->variables[i].lpszValue );

            threadInfo->variables[i].lpszName = 0;
            threadInfo->variables[i].lpszValue = 0;
        }

        // Clear uploaded files
        struct NNCMS_FILE *lpCurFile = threadInfo->lpUploadedFiles;
        while( lpCurFile )
        {
            struct NNCMS_FILE *lpNextFile = lpCurFile->lpNext;
            FREE( lpCurFile->lpData );
            FREE( lpCurFile );
            lpCurFile = lpNextFile;
        }
        threadInfo->lpUploadedFiles = 0;

        // Free content memory block
        if( threadInfo->lpszContentData != 0 )
            FREE( threadInfo->lpszContentData );
        threadInfo->lpszContentData = 0;
    }

exit_thread:

    return 0;
}

// #############################################################################

void main_format_time_string( struct NNCMS_THREAD_INFO *req, char *ptr, time_t clock)
{
    struct tm *timePtr;

    if( clock == 0 ) clock = time( 0 );
    timePtr = gmtime( &clock );
    *ptr = 0;
    if( timePtr != 0 )
        strftime( ptr, NNCMS_TIME_STR_LEN_MAX, "%a, %d %b %Y %T GMT", timePtr );
    else
        log_printf( req, LOG_CRITICAL, "Main::FormatTimeString: gmtime() returned null (year does not fit into an integer?)" );
}

// #############################################################################

void main_send_headers( struct NNCMS_THREAD_INFO *req, int nContentLength, time_t nModTime )
{
    char szTimeBuf[NNCMS_TIME_STR_LEN_MAX];

    if( req->bHeadersSent ) return;

    req->bHeadersSent = 1;

    main_format_time_string( req, szTimeBuf, 0 );
    FCGX_FPrintF( req->fcgi_request->out,
        "Status: %s"            // Response
        "%s"                    // Response headers
        "Date: %s\n"            // Date
        "Connection: close\n"   // Connection
        "Content-Type: %s\n",   // Content Type
        req->szResponse, req->szResponseHeaders, szTimeBuf,
        req->szResponseContentType );

    if( nContentLength > 0 )
    {
        main_format_time_string( req, szTimeBuf, nModTime );
        FCGX_FPrintF( req->fcgi_request->out,
            "Content-Length: %d\n"  // Content-Lenght
            "Last-Modified: %s\n",  // Last-Modified
            nContentLength, szTimeBuf );
    }

    // Очень важно. Здесь можно было бы передавать два перевода строки сразу,
    // однако при передачи данных/рисунка — один лишний перевод строки попадает
    // в поток данных. А если всегда один перевод строки, то lighttpd съедает
    // половину страницы, считаю её частью заголовков.
    if( req->httpd_type == HTTPD_LIGHTTPD )
    {
        // СУКА!!1
        // Не менять, после заголовков должно быть '\r\n\r\n',
        // а не '\n\n' !!!1
        FCGX_PutS( "\r\n\r\n", req->fcgi_request->out );
    }
    else //if( req->httpd_type == HTTPD_NGINX )
    {
        // БЛЯДЬ!!11
        // Для nginx надо чтоб было лишь один раз \r\n
        FCGX_PutS( "\r\n", req->fcgi_request->out );
    }

}

// #############################################################################

// This routine is used to override the default response code returned to a
// client browser. By default, an HTTP Response code of 200 (successful request)
// is returned to the browser. If you wish to return a different response code
// then this routine must be called with the new response information before the
// headers are sent to the client.
// Example:
//
//      main_set_response( req, "301 Moved Permanently" );
void main_set_response( struct NNCMS_THREAD_INFO *req, char *lpszMsg )
{
    strlcpy( req->szResponse, lpszMsg, sizeof(req->szResponse) );
}

// #############################################################################

// If you need to generate a response containing something other than HTML text
// then you will need to use this routine. A call to this routine will override
// the default content type for the current request. When the HTTP headers are
// generated, the specified content type information will be used.
// Example:
//
//      main_set_content_type( req, "image/jpeg" );
void main_set_content_type( struct NNCMS_THREAD_INFO *req, char *type )
{
    strlcpy( req->szResponseContentType, type, sizeof(req->szResponseContentType) );
}

// #############################################################################

// If you need to add a header line to the set of headers being returned to the
// client it can be done using httpdAddHeader.
// Example:
//
//      main_set_response( req, "307 Temporary Redirect" );
//      main_add_header( req, "Location: http://www.foo.com/some/new/location" );
void main_add_header( struct NNCMS_THREAD_INFO *req, char *lpszMsg )
{
    strcat( req->szResponseHeaders, lpszMsg );
    if( lpszMsg[strlen( lpszMsg ) - 1] != '\n' )
        strcat( req->szResponseHeaders, "\n" );
}

// #############################################################################

// To add a cookie to the HTTP response, simply call this routine with the name
// and value of the cookie. There is no httpdGetCookie( ) equivalent routines as
// cookies are automatically presented as variables in the symbol table.
void main_set_cookie( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszValue )
{
    char buff[NNCMS_HEADERS_LEN_MAX];

    snprintf( buff, sizeof(req->szResponseHeaders), "Set-Cookie: %s=%s; path=/;", lpszName, lpszValue );
    main_add_header( req, buff );
}

// #############################################################################

void atquit( )
{
    quit( EXIT_SUCCESS );
}

// #############################################################################

void quit( int nSignal )
{
    if( nSignal == SIGINT )
        //printf( "\n" );
        printf( " <- rofl\n" );

    if( nSignal != SIGINT )
    {
        // Signal all thread to die and wait
        bDone = true;
        FCGX_ShutdownPending( );
        for( int i = 0; i < nThreads; i++ )
        {
            // Kill the thread
            pthread_kill( *(pThreadList[i]->pThread), 0 );

            // Deinit modules
            ban_deinit( pThreadList[i] );
            post_deinit( pThreadList[i] );
            user_deinit( pThreadList[i] );
            template_deinit( pThreadList[i] );
            acl_deinit( pThreadList[i] );
            log_deinit( pThreadList[i] );

            // Free temporary buffers
            SAFE_FREE( pThreadList[i]->lpszBuffer );
            SAFE_FREE( pThreadList[i]->lpszOutput );
            SAFE_FREE( pThreadList[i]->lpszTemp );
            SAFE_FREE( pThreadList[i]->lpszFrame );
            SAFE_FREE( pThreadList[i]->lpszBlockContent );
            SAFE_FREE( pThreadList[i]->lpszBlockTemp );
            SAFE_FREE( pThreadList[i]->pThread );
            SAFE_FREE( pThreadList[i]->fcgi_request );
            SAFE_FREE( pThreadList[i] );
        }

        // Mem free
        SAFE_FREE( pThreadList );

        // Flush and close database
        database_deinit( );
        cache_deinit( );

#ifdef HAVE_IMAGEMAGICK
        // Cleanup ImageMagick stuff
        //lpException = DestroyExceptionInfo( lpException );
        //MagickCoreTerminus( );
        MagickWandTerminus();
#endif

        // Water
        log_printf( 0, LOG_INFO, "Main: nncms stopped" );

        // Free memory garbage
        if( nGarbageCollector != 0 )
            memory_deinit( );
    }

    exit( nSignal );
}

// #############################################################################

// This fuckin function shall make valid HTML text, add banners, footer,
// header, menu blocks and other useless shit like app name prefix to
// title. And after that it should output whole dog's breakfast to
// browser (client).
unsigned int main_output( struct NNCMS_THREAD_INFO *req, char *lpszTitle, char *lpszContent, time_t nModTime )
{
    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS mainTags[] =
        {
            { /* szName */ "title", /* szValue */ lpszTitle },
            { /* szName */ "content",  /* szValue */ lpszContent },
            { /* szName */ "copyleft",  /* szValue */ PACKAGE_STRING " <!-- Copyleft (C) 2005 - 2008 ValkaTR &lt;valkur@gmail.com&gt; -->" },
            { /* szName */ "user_block",  /* szValue */ user_block( req ) },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Parse template file
    unsigned int nRetVal = template_iparse( req, TEMPLATE_STRUCTURE,
        req->lpszOutput, NNCMS_PAGE_SIZE_MAX, mainTags );
    if( nRetVal == 0 )
    {
        // Error happened, memory not allocated or file not found
        log_printf( 0, LOG_CRITICAL, "Main::Output: Could not parse template file: file not found or memory could not be allocated" );
        return 0;
    }

    // Output
    main_send_headers( req, nRetVal, nModTime );
    //char *temp = strdup( req->lpszOutput );
    //FCGX_PutS( temp, req->fcgi_request->out );
    //FCGX_PutS( req->lpszOutput, req->fcgi_request->out );
    //free( temp );
    //FCGX_PutS( req->lpszOutput, req->fcgi_request->out );
    FCGX_PutStr( req->lpszOutput, nRetVal, req->fcgi_request->out );
    //FCGX_PutStr( req->lpszOutput, strlen( req->lpszOutput ), req->fcgi_request->out );
    //DUMP( req->lpszOutput );
}

// #############################################################################

void redirect_to_referer( struct NNCMS_THREAD_INFO *req )
{
    char *lpszReferer = FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp );
    struct NNCMS_VARIABLE *httpVarReferer = main_get_variable( req, "referer" );
    if( httpVarReferer != 0 && *httpVarReferer->lpszValue != 0 )
            redirect( req, httpVarReferer->lpszValue );
    else if( lpszReferer != 0 )
        if( *lpszReferer != 0 )
            redirect( req, lpszReferer );
        else
            redirect( req, "/" );
    else
        redirect( req, "/" );
}

// #############################################################################

void redirectf( struct NNCMS_THREAD_INFO *req, char *lpszUrlFormat, ... )
{
    // Preformat URL
    va_list vArgs;
    char szURL[NNCMS_PATH_LEN_MAX];
    va_start( vArgs, lpszUrlFormat );
    vsnprintf( szURL, sizeof(szURL), lpszUrlFormat, vArgs );
    va_end( vArgs );

    // Call safe redirect
    redirect( req, szURL );
}

// #############################################################################

// Cheap and vaginality
void redirect( struct NNCMS_THREAD_INFO *req, char *lpszURL )
{
    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS redirectTemplate[] =
        {
            { /* szName */ "redirect",  /* szValue */ lpszURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };


    // Response
    // WTF MOZILLA FIREFOX 3.0.10 with 307 response a message
    // popup appears:
    //
    // "This web page is being redirected to a new location. Would
    //  you like to resend the form data you have typed to the
    //  new location?"
    //                                         [ Ok ]   [ Cancel ]
    //
    //main_set_response( req, "307 Temporary Redirect" );
    //
    // Have to use 302 response to avoid this WTF
    //
    main_set_response( req, "302 Found" );

    // Generate the page
    char szLocation[NNCMS_PATH_LEN_MAX];
    snprintf( szLocation, sizeof(szLocation), "Location: %s", lpszURL );
    main_add_header( req, szLocation );
    unsigned int nRetVal = template_iparse( req, TEMPLATE_REDIRECT, req->lpszOutput, NNCMS_PAGE_SIZE_MAX, redirectTemplate );

    // Send generated html to client
    main_send_headers( req, nRetVal, 0 );
    FCGX_PutS( req->lpszOutput, req->fcgi_request->out );
}

// #############################################################################

// Index page
void main_index( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Index";

    // Make path to index.html
    char szFileName[NNCMS_PATH_LEN_MAX];
    strlcpy( szFileName, fileDir, NNCMS_PATH_LEN_MAX );
    strlcat( szFileName, "/main.html", NNCMS_PATH_LEN_MAX );

    // Try to open index file for reading
    FILE *pFile = fopen( szFileName, "rb" );
    if( pFile == 0 )
    {
        // Oops, no file
        return;
    }

    // Obtain file size.
    fseek( pFile, 0, SEEK_END );
    unsigned int lSize = ftell( pFile );
    rewind( pFile );

    if( lSize >= NNCMS_PAGE_SIZE_MAX )
    {
        lSize = NNCMS_PAGE_SIZE_MAX - 1;
    }

    // Copy the file into the buffer.
    fread( req->lpszFrame, 1, lSize, pFile );

    // Close the file associated with the specified pFile stream after flushing
    // all buffers associated with it.
    fclose (pFile);

    // Terminate string
    req->lpszFrame[lSize] = 0;

    //size_t nSize = NNCMS_PAGE_SIZE_MAX;
    //main_get_file( szFileName, &nSize );

    // Put data in main template
    main_output( req, szHeader, req->lpszFrame, 0 );

    return;
}

// #############################################################################

static void main_print_env( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, char *label, char **envp )
{
    snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<hr>%s:<br>\n<pre>\n", label );
    smart_cat( smartBuffer, req->lpszTemp );
    for( ; *envp != 0; envp++ ) {
        snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "%s\n", *envp );
        smart_cat( smartBuffer, req->lpszTemp );
    }
    smart_cat( smartBuffer, "</pre><p>\n" );
}

// #############################################################################

// Info page
void main_info( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Info";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/utilities-system-monitor.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Create smart buffers
    struct NNCMS_BUFFER smartBuffer =
        {
            /* lpBuffer */ req->lpszBuffer,
            /* nSize */ NNCMS_PAGE_SIZE_MAX,
            /* nPos */ 0
        };
    *smartBuffer.lpBuffer = 0;

    // Standart input
    if( req->nContentLenght <= 0 )
    {
        smart_cat( &smartBuffer, "No data from standard input.<p>\n" );
    }
    else
    {
        smart_cat( &smartBuffer, "Standard input:<br>\n<pre>\n" );
        smart_cat( &smartBuffer, req->lpszContentData );
        smart_cat( &smartBuffer, "\n</pre><p>\n" );
    }

    // Get some info
    main_print_env( req, &smartBuffer, "Request environment", req->fcgi_request->envp );
    main_print_env( req, &smartBuffer, "Initial environment", environ );

    // Variables
    snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<hr>%s:<br>\n<pre>\n", "Variables" );
    smart_cat( &smartBuffer, req->lpszTemp );
    for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
    {
        if( req->variables[i].lpszName != 0 && req->variables[i].lpszValue != 0 )
        {
            snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<b>%s</b> = <font color=\"gray\">%s</font>\n",
                req->variables[i].lpszName, req->variables[i].lpszValue );
            smart_cat( &smartBuffer, req->lpszTemp );
        }
    }
    smart_cat( &smartBuffer, "</pre><p>\n" );

    // Uploaded files
    snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<hr>%s:<br>\n", "Uploaded files" );
    smart_cat( &smartBuffer, req->lpszTemp );
    struct NNCMS_FILE *lpCurFile = req->lpUploadedFiles;
    while( lpCurFile )
    {
        snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX,
            "<b>szName</b> = <font color=\"darkgreen\">%s</font><br>\n"
            "<b>szFileName</b> = <font color=\"darkgreen\">%s</font><br>\n"
            "<b>lpData</b> = <pre>%s</pre><br>\n"
            "<b>nSize</b> = <font color=\"red\">%u</font><br>---<br>\n",
            lpCurFile->szName, lpCurFile->szFileName, lpCurFile->lpData,
            lpCurFile->nSize );
        smart_cat( &smartBuffer, req->lpszTemp );
        lpCurFile = lpCurFile->lpNext;
    }
    smart_cat( &smartBuffer, "<p>\n" );

    // Generate the block
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Put data in main template
    main_output( req, szHeader, req->lpszFrame, 0 );

    return;
}

// #############################################################################

char *main_get_file( char *lpszFileName, size_t *nLimit )
{

    // Open template
    FILE *pFile = fopen( lpszFileName, "rb" );
    if( pFile == 0 )
    {
        // File not found
        log_printf( 0, LOG_ERROR, "Main::GetFile: Could not open \"%s\" file", lpszFileName );
        return 0;
    }

    // Get file size
    fseek( pFile, 0, SEEK_END );
    size_t nSize = ftell( pFile );
    rewind( pFile );

    // Do not accept zero files
    if( nSize == 0 )
        return 0;

    // Truntucate (limit) the size
    if( nSize > *nLimit )
    {
        nSize = *nLimit;
    }

    // Allocate memory
    char *lpszBuffer = MALLOC( nSize + 1 );
    if( lpszBuffer == 0 )
    {
        // Out of memory
        log_printf( 0, LOG_ALERT, "Main::GetFile: Could not allocate %u bytes of memory for \"%s\" file", nSize, lpszFileName );
        return 0;
    }

    // Read and close file
    *nLimit = fread( lpszBuffer, 1, nSize, pFile );
    fclose( pFile );

    // Null terminating character
    lpszBuffer[*nLimit] = 0;

    return lpszBuffer;
}

// #############################################################################

// The test array of structures

/*typedef struct TEST
{
    int nValue;
} TEST, *PTEST;

void
printarray( TEST *arg[], int length )
{
    int n;
    for( n = 0; n < length; n++ )
    {
        printf( "%u ", &arg[n]->nValue );
    }
    printf( "\n" );
}

int
main( int argc, char *argv[] )
{
  TEST firstarray[] = { {5}, {10}, {15} };
  TEST secondarray[] = { {2}, {4}, {6}, {8}, {10} };
  printarray( firstarray, 3 );
  printarray( secondarray, 5 );
  return 0;
}*/

// #############################################################################
