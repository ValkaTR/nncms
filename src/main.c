// #############################################################################
// Source file: main.c

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

#include <glib.h>
#include <gmime/gmime.h>
//#include <glib-object.h>
//#include <gio/gio.h>

#include <locale.h>

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "user.h"
#include "group.h"
#include "file.h"
#include "template.h"
#include "cfg.h"
#include "log.h"
#include "ban.h"
#include "i18n.h"
#include "node.h"
#include "xxx.h"

#include "database.h"
#include "view.h"
#include "acl.h"
#include "post.h"
#include "cache.h"

#include "canonicalize.h"
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

// If set to false, then caching is disabled
extern bool bCache;

// Display page generation time
bool bPageGen = false;

// Enable Rewrite rules
bool bRewrite = false;

// No cache option
extern bool bTemplateCache;

// Settings
struct NNCMS_OPTION g_options[] =
    {
        { .name = "homeURL", /* nType */ NNCMS_STRING, /* lpMem */ &homeURL },
        { .name = "tSessionExpires", /* nType */ NNCMS_INTEGER, /* lpMem */ &tSessionExpires },
        { .name = "tCheckSessionInterval", /* nType */ NNCMS_INTEGER, /* lpMem */ &tCheckSessionInterval },
        { .name = "TimestampFormat", /* nType */ NNCMS_STRING, /* lpMem */ &szTimestampFormat },
        { .name = "databasePath", /* nType */ NNCMS_STRING, /* lpMem */ &szDatabasePath },
        { .name = "fileDir", /* nType */ NNCMS_STRING, /* lpMem */ &fileDir },
        { .name = "templateDir", /* nType */ NNCMS_STRING, /* lpMem */ &templateDir },
        { .name = "userDir", /* nType */ NNCMS_STRING, /* lpMem */ &userDir },
        { .name = "avatarDir", /* nType */ NNCMS_STRING, /* lpMem */ &avatarDir },
        { .name = "cacheDir", /* nType */ NNCMS_STRING, /* lpMem */ &cacheDir },
        { .name = "blockDir", /* nType */ NNCMS_STRING, /* lpMem */ &blockDir },
        { .name = "logFile", /* nType */ NNCMS_STRING, /* lpMem */ &szLogFile },
        { .name = "threads", /* nType */ NNCMS_INTEGER, /* lpMem */ &nThreads },
        { .name = "garbage_collector", /* nType */ NNCMS_INTEGER, /* lpMem */ &nGarbageCollector },
        { .name = "include_default_quiet", /* nType */ NNCMS_BOOL, /* lpMem */ &bIncludeDefaultQuiet },
        { .name = "cache_memory", /* nType */ NNCMS_BOOL, /* lpMem */ &bCacheMemory },
        { .name = "cache", /* nType */ NNCMS_BOOL, /* lpMem */ &bCache },
        { .name = "rewrite", /* nType */ NNCMS_BOOL, /* lpMem */ &bRewrite },
        { .name = "print_page_gen", /* nType */ NNCMS_BOOL, /* lpMem */ &bPageGen },
        { .name = "template_cache", /* nType */ NNCMS_BOOL, /* lpMem */ &bTemplateCache },
        { .name = "post_depth", /* nType */ NNCMS_INTEGER, /* lpMem */ &nPostDepth },
        { .name = 0, /* nType */ 0, /* lpMem */ 0 }
    };

// All pages
GHashTable *page_table;

// Setting this value to 1 will terminate the application
bool bDone = false;

// phreads data
GPtrArray *local_init;
GPtrArray *local_destroy;
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
//sleep( 1 );
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
    setlocale( LC_ALL, "" );
    //bindtextdomain( PACKAGE, LOCALEDIR );
    //textdomain( PACKAGE );

    // Initialize GMime.
    g_mime_init( 0 );

    // Register a function to be called at normal process termination
    if( atexit( main_atquit ) != 0 ) exit( EXIT_FAILURE );

    // Bind event handler for interruption signals
    signal( SIGINT, main_quit );

    local_init = g_ptr_array_new( );
    local_destroy = g_ptr_array_new( );
    page_table = g_hash_table_new( g_str_hash, g_str_equal );

    main_page_add( "index", &main_index );
    main_page_add( "info", &main_info );

    //
    // Initialize modules
    //

    if( cfg_global_init( ) == false )
    {
        log_print( 0, LOG_WARNING, "Main: Config file not found" );
    }

    if( database_global_init( ) == false )
    {
        log_print( 0, LOG_ERROR, "Main: Connecting to the database failed" );
        exit( EXIT_FAILURE );
    }
    
    if( cache_global_init( ) == false )
    {
        log_print( 0, LOG_ERROR, "Main: Cache initialization failed" );
        exit( EXIT_FAILURE );
    }

    memory_global_init( );
    file_global_init( );
    log_global_init( );
    i18n_global_init( );
    acl_global_init( );
    template_global_init( );
    user_global_init( );
    group_global_init( );
    post_global_init( );
    ban_global_init( );
    view_global_init( );
    node_global_init( );
    xxx_global_init( );

    // Init FastCGI stuff
    FCGX_Init( );

    // Init pthread struct
    pThreadList = MALLOC( nThreads * sizeof(void *) );

    // Create threads
    for( int i = 0; i < nThreads; i++ )
    {
        // This structure is associated with a thread
        struct NNCMS_THREAD_INFO *req = MALLOC( sizeof(struct NNCMS_THREAD_INFO) );
        memset( req, 0, sizeof(struct NNCMS_THREAD_INFO) );

        // Store thread
        pThreadList[i] = req;

        // Thread stuff
        req->nThreadID = i;
        //req->pThread = MALLOC( sizeof(pthread_t) );

        // Create the thread
        if( i != 0 )
            pthread_create( &req->pThread, 0, main_loop, (void *) req );
    }

    /*while( 1 )
    {
        sleep( 1 );
    }*/
    
    main_loop( (void *) pThreadList[0] );

    return 0;
}

// #############################################################################

void main_page_add( char *page, void (*func)(struct NNCMS_THREAD_INFO *) )
{
    g_hash_table_insert( page_table, page, func );
}

// #############################################################################

void main_local_init_add( bool (*func)(struct NNCMS_THREAD_INFO *) )
{
    g_ptr_array_add( local_init, func );
}

void main_local_destroy_add( bool (*func)(struct NNCMS_THREAD_INFO *) )
{
    g_ptr_array_add( local_destroy, func );
}

// #############################################################################

int main_variable_add( struct NNCMS_THREAD_INFO *req, GTree *tree, char *name, char *value)
{
    /*for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
    {
        if( req->variables[i].value.string == 0 )
        {
            req->variables[i].name = name; //g_strdup( name );
            req->variables[i].value.string = value; //g_strdup( value );

            return i;
        }
    }*/

    g_tree_insert( tree, name, value );

    return 0;
}

// #############################################################################

char *main_variable_get( struct NNCMS_THREAD_INFO *req, GTree *tree, char *name )
{
    char *value = g_tree_lookup( tree, name );

    return value;
}

// #############################################################################

struct main_glink_GTraverseFunc_t
{
    int i;
    struct NNCMS_VARIABLE *vars;
};

gboolean main_glink_GTraverseFunc( gpointer key, gpointer value, gpointer data )
{
    struct main_glink_GTraverseFunc_t *data_t = (struct main_glink_GTraverseFunc_t *) data;
    
    //data_t->vars[data_t->i].name = key;
    strncpy( data_t->vars[data_t->i].name, key, NNCMS_VAR_NAME_LEN_MAX );
    data_t->vars[data_t->i].value.string = value;
    data_t->vars[data_t->i].type = NNCMS_TYPE_STRING;
    data_t->i = 1 + data_t->i;

    // Return FALSE to continue the traversal
    return FALSE;
}

// LOL ?
char *main_glink( struct NNCMS_THREAD_INFO *req, char *function, char *title, GTree *tree )
{
    // Convert GTree to NNCMS_VARIABLE
    int n = g_tree_nnodes( tree ) + 1;
    
    if( n == 1 )
        return main_link( req, function, title, NULL );
    
    struct NNCMS_VARIABLE *vars = MALLOC( n * sizeof(struct NNCMS_VARIABLE) );
    memset( vars, 0, n * sizeof(struct NNCMS_VARIABLE) );
    struct main_glink_GTraverseFunc_t *data = MALLOC( sizeof(struct main_glink_GTraverseFunc_t) );
    data->i = 0;
    data->vars = vars;
    g_tree_foreach( tree, (GTraverseFunc) main_glink_GTraverseFunc, (gpointer) data );

    // Pass NNCMS_VARIABLE to a normal function
    char *result = main_link( req, function, title, vars );

    FREE( data );
    FREE( vars );
    
    return result;
}

// Makes a link
char *main_link( struct NNCMS_THREAD_INFO *req, char *function, char *title, struct NNCMS_VARIABLE *vars )
{
    GString *link = g_string_sized_new( 256 );
    
    // Make a HTML link if title is specified
    if( title )
    {
        g_string_append( link, "<a href=\"" );
    }
    
    g_string_append_printf( link, "/%s.fcgi", function );
    
    // Add variables
    if( vars != NULL )
    {
        bool first = true;
        for( unsigned int i = 0; vars[i].type != NNCMS_TYPE_NONE; i++ )
        {
            //if( vars[i].value.string == 0 )
            //    continue;

            char *txt_value = main_type( req, vars[i].value, vars[i].type );
            if( *txt_value == 0 )
            {
                g_free( txt_value );
                continue;
            }
            
            if( first == TRUE )
            {
                g_string_append( link, "?" );
                first = FALSE;
            }
            else
            {
                g_string_append( link, "&amp;" );
            }
            
            // Variable name
            g_string_append( link, vars[i].name );
            
            // Equal sign
            g_string_append( link, "=" );
            
            // Variable value
            g_string_append_uri_escaped( link, txt_value, NULL, TRUE );
            
            g_free( txt_value );
        }
    }

    // Make a HTML link if title is specified
    if( title )
    {
        g_string_append( link, "\">" );
        g_string_append( link, title );
        g_string_append( link, "</a>" );
    }
    
    return g_string_free( link, FALSE );
}

// Temporary link. String is freed at end of the loop automaticaly
char *main_temp_link( struct NNCMS_THREAD_INFO *req, char *function, char *title, struct NNCMS_VARIABLE *vars )
{
    char *link = main_link( req, function, title, vars );
    garbage_add( req->loop_garbage, link, MEMORY_GARBAGE_GFREE );
    return link;
}

// #############################################################################

int main_parse_query( struct NNCMS_THREAD_INFO *req, GTree *tree, char *query, char equalchar, char sepchar)
{
    int cnt = 0;
    
    if( query == NULL )
        return 0;
    
    // A string to be parsed
    //
    // {name}[{equalchar}[{value}]]{sepchar}...
    //

    // query                      name             value
    // ----------------------------------------------------------------
    // {name}{sepchar}{value}     {name}           {value}
    // {name}{sepchar}            {name}           "0"
    // {name}                     {name}           NULL
    // {sepchar}{name}...         "0"              NULL
    //

    int i = 0;
    bool bLoop;

    do
    {
        char *name = &query[i];
        char *value = 0;
        bLoop = false;
        
        // Find {equalchar}, but not after {sepchar}, it may be absent
        while( 1 )
        {
            if( query[i] == equalchar && value == 0 )
            {
                // Store value
                query[i] = 0;
                i = i + 1;
                value = &query[i];
            }
            
            if( query[i] == sepchar )
            {
                query[i] = 0;
                i = i + 1;
                bLoop = true;   // Remember that there was a {sepchar}
                break;
            }
            
            if( query[i] == 0 )
            {
                break;
            }
            
            i = i + 1;
        }
        
        if( *name != 0 && value != 0 )
        {
            _q_urldecode(name);
            _q_urldecode(value);
            
            // Store the variable
            //log_printf( req, LOG_INFO, "main_parse_query: name = %s, value = %s", name, value );
            //g_hash_table_insert(hash, name, value);
            main_variable_add( req, tree, name, value );
            cnt = cnt + 1;
        }

        // More variables
    }
    while( bLoop == true );

    return cnt;
}

// #############################################################################

char *_q_makeword(char *str, char stop)
{
    char *word;
    int  len, i;

    for (len = 0; ((str[len] != stop) && (str[len])); len++);
    word = (char *) g_malloc(sizeof(char) * (len + 1));

    for (i = 0; i < len; i++) word[i] = str[i];
    word[i] = '\0';

    if (str[len])len++;
    for (i = len; str[i]; i++) str[i - len] = str[i];
    str[i - len] = '\0';

    return word;
}

// #############################################################################

char *_q_strtrim(char *str)
{
    int i, j;

    if (str == NULL) return NULL;
    for (j = 0; str[j] == ' ' || str[j] == '\t' || str[j] == '\r' || str[j] == '\n'; j++);
    for (i = 0; str[j] != '\0'; i++, j++) str[i] = str[j];
    for (i--; (i >= 0) && (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n'); i--);
    str[i+1] = '\0';

    return str;
}

// #############################################################################

size_t _q_urldecode(char *str)
{
    if (str == NULL) {
        return 0;
    }

    char *pEncPt, *pBinPt = str;
    for (pEncPt = str; *pEncPt != '\0'; pEncPt++) {
        switch (*pEncPt) {
            case '+': {
                *pBinPt++ = ' ';
                break;
            }
            case '%': {
                *pBinPt++ = _q_x2c(*(pEncPt + 1), *(pEncPt + 2));
                pEncPt += 2;
                break;
            }
            default: {
                *pBinPt++ = *pEncPt;
                break;
            }
        }
    }
    *pBinPt = '\0';

    return (pBinPt - str);
}

// #############################################################################

// Change two hex character to one hex value.
char _q_x2c(char hex_up, char hex_low)
{
    char digit;

    digit = 16 * (hex_up >= 'A' ? ((hex_up & 0xdf) - 'A') + 10 : (hex_up - '0'));
    digit += (hex_low >= 'A' ? ((hex_low & 0xdf) - 'A') + 10 : (hex_low - '0'));

    return digit;
}

// #############################################################################

// RFC1867: Form-based File Upload in HTML
void main_store_data_rfc1867( struct NNCMS_THREAD_INFO *req, char *content_data, size_t content_lenght, char *content_type )
{
    req->files = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FILE) );

    // About memory leaks
    // http://list-archives.org/2012/11/21/gmime-devel-list-gnome-org/gmime-devel-memory-leaks-while-compiling-a-test-gmime-c-code/f/2647923728
    //
    // stedfast@comcast.net                                23.11.2012 21:11 UTC
    // Hi Paul,
    //
    // I think that it probably did not help that GMime used to be inconsistent
    // (at least in the 2.2 days) in that sometimes foo_get_bar() would ref and
    // sometimes it wouldn't.
    //
    // These days, at least with 2.6 (and I think 2.4 as well), foo_get_bar()
    // APIs should never ref their returned object.
    //
    // The only time a function should ever return a ref'd object is if it
    // creates a new one.
    //
    // The other place objects get ref'd is when they get added to a parent
    // object (e.g. adding a GMimePart to a GMimeMultipart - the GMimeMultipart
    // will take a ref on the GMimePart).
    //
    // Hope that helps clear up any confusion,
    //
    // Jeff 

	// Create a new GMimeStreamMem object for MIME data from browser
	GMimeStream *mime_stream = g_mime_stream_mem_new( );
    garbage_add( req->loop_garbage, mime_stream, MEMORY_GARBAGE_MIME_STREAM_CLOSE );
    garbage_add( req->loop_garbage, mime_stream, MEMORY_GARBAGE_OBJECT_UNREF );

    // Write MIME data to the stream
    g_mime_stream_printf( mime_stream, "Content-Type: %s\r\n", content_type );
    g_mime_stream_printf( mime_stream, "Content-Length: %lu\r\n", content_lenght );
    g_mime_stream_write( mime_stream, content_data, content_lenght );
    g_mime_stream_seek( mime_stream, 0, GMIME_STREAM_SEEK_SET );

    // Create a new parser object preset to parse stream
    GMimeParser *mime_parser = g_mime_parser_new_with_stream( mime_stream );
    garbage_add( req->loop_garbage, mime_parser, MEMORY_GARBAGE_OBJECT_UNREF );
    
    // Construct a MIME part from parser
	GMimeObject *mime_parts = g_mime_parser_construct_part( mime_parser );
    garbage_add( req->loop_garbage, mime_parts, MEMORY_GARBAGE_OBJECT_UNREF );

    // Create a new GMimePartIter for iterating over toplevel's subparts
    GMimePartIter *mime_iter = g_mime_part_iter_new( mime_parts );
    garbage_add( req->loop_garbage, mime_iter, MEMORY_GARBAGE_MIME_PART_ITER_FREE );

    while( 1 )
    {
        // Get the GMimeObject at the current GMimePartIter position
        GMimeObject *mime_part = g_mime_part_iter_get_current( mime_iter );
        if( mime_part == NULL )
            break;

        // Get the name of the specificed mime part
        const char *name = g_mime_object_get_content_disposition_parameter( mime_part, "name" );
        const char *filename = g_mime_part_get_filename( (GMimePart *) mime_part );
        
        // Get the internal data-wrapper of the specified mime part
        GMimeDataWrapper *content_object = g_mime_part_get_content_object( (GMimePart *) mime_part );
        
        // Create a new GMimeStreamMem object
        GMimeStream *content_stream = g_mime_stream_mem_new( );
        garbage_add( req->loop_garbage, content_stream, MEMORY_GARBAGE_MIME_STREAM_CLOSE );
        garbage_add( req->loop_garbage, content_stream, MEMORY_GARBAGE_OBJECT_UNREF );
        
        // Write the raw (decoded) data to the output stream
        ssize_t content_size = g_mime_data_wrapper_write_to_stream( content_object, content_stream );

        // Append zero terminating character
        g_mime_stream_write( content_stream, "\0", 1 );

        // Get the byte array from the memory stream
        GByteArray *content_byte = g_mime_stream_mem_get_byte_array( (GMimeStreamMem *) content_stream );

        if( name != NULL && name[0] != 0 )
        {
            //printf( "variable: %s (%i bytes) = %s\n", name, (int) content_size, (char *) content_byte->data );
            main_variable_add( req, req->post_tree, (char *) name, (char *) content_byte->data );
        }

        if( filename != NULL && filename[0] != 0 )
        {
            //printf( "file: %s, %s (%i bytes) = %s\n", name, filename, (int) content_size, (char *) content_byte->data );
            //main_variable_add( req, req->post_tree, (char *) filename, (char *) content_byte->data );
            
            struct NNCMS_FILE file =
            {
                .var_name = (char *) name,
                .file_name = (char *) filename,
                .data = content_byte->data,
                .size = content_size
            };
            
            g_array_append_vals( req->files, &file, 1 );
        }
        
        gboolean result = g_mime_part_iter_next( mime_iter );
        if( result == FALSE )
            break;
    }
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
    struct NNCMS_THREAD_INFO *req = var;
    static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

    //
    // Here goes initialization of FastCGI
    //
    req->fcgi_request = MALLOC( sizeof(FCGX_Request) );
    FCGX_InitRequest( req->fcgi_request, 0, 0 );

    // Init modules
    for( unsigned int i = 0; i < local_init->len; i++ )
    {
        bool (*init_func)(struct NNCMS_THREAD_INFO *) = local_init->pdata[i];
        bool result = init_func( req );
        
        if( result == false )
        {
            log_printf( req, LOG_ERROR, "Local initialization failed (i = %u)", i );
            exit( EXIT_FAILURE );
            return NULL;
        }
    }
    
    if( req->nThreadID == 0 )
    {
#ifdef NDEBUG
        log_printf( req, LOG_INFO, "nncms started, created %u threads", nThreads );
#else
        log_printf( req, LOG_INFO, "nncms started in debug mode, created %u threads", nThreads );
#endif
    }
    
    // Workaround. Need to remove user_check_session everywhere
    user_reset_session( req );

    // Mark all memory allocations as not leaks
    // Comment to trace init/deinit leaks
    memory_leaks( false );

    //
    // Accept loop
    //
    while( 1 )
    {
        // ################################
        // Request accept
        // ################################
       
        // Some platforms require accept() serialization, some don't..
        pthread_mutex_lock( &accept_mutex );
        int result = FCGX_Accept_r( req->fcgi_request );
        pthread_mutex_unlock( &accept_mutex );

        // Check for error and global die signal
        if( result < 0 || bDone == true )
            break;

        // Page generation measurements
        //struct timespec tp1;
        unsigned long long tp1;
        if( bPageGen == true )
        {
            //clock_gettime( CLOCK_REALTIME, &tp1 );
            tp1 = rdtsc( );
        }

        // ################################
        // Initialization
        // ################################

        // Check if this user is banned
        req->user_address = FCGX_GetParam( "REMOTE_ADDR", req->fcgi_request->envp );
        struct in_addr addr;
        inet_aton( req->user_address, &addr );
        if( ban_check( req, &addr ) == true )
        //if( 0 )
        {
            // User is banned
            main_set_response( req, "403 Forbidden" );

            // Send generated html to client
            main_send_headers( req, 16, 0 );
            FCGX_PutS( "You are banned.\n", req->fcgi_request->out );

            // Finish request
            FCGX_Finish_r( req->fcgi_request );

            // Use less as possible CPU power for banned users
            continue;
        }

        // Fetch HTTPd software type
        if( req->httpd_type == HTTPD_NONE )
        {
            req->server_software = FCGX_GetParam( "SERVER_SOFTWARE", req->fcgi_request->envp );
            req->httpd_type = HTTPD_UNKNOWN;
            if( req->server_software != 0 )
            {
                if( strncasecmp( req->server_software, "lighttpd", 8 ) == 0 )
                    req->httpd_type = HTTPD_LIGHTTPD;
                else if( strncasecmp( req->server_software, "nginx", 5 ) == 0 )
                    req->httpd_type = HTTPD_NGINX;
            }
        }

        // Thread debug
        //log_printf( req, LOG_DEBUG, "thread №%i got request", nThreadID );

        // Get some important information
        //
        // FCGI_ROLE=RESPONDER
        // SERVER_SOFTWARE=lighttpd/1.4.32
        // SERVER_NAME=127.0.0.1
        // GATEWAY_INTERFACE=CGI/1.1
        // SERVER_PORT=80
        // SERVER_ADDR=127.0.0.1
        // REMOTE_PORT=47011
        // REMOTE_ADDR=127.0.0.1
        // SCRIPT_NAME=/info
        // PATH_INFO=
        // SCRIPT_FILENAME=//info.fcgi
        // DOCUMENT_ROOT=/
        // REQUEST_URI=/info.fcgi
        // QUERY_STRING=
        // REQUEST_METHOD=GET
        // REDIRECT_STATUS=200
        // SERVER_PROTOCOL=HTTP/1.1
        // HTTP_HOST=127.0.0.1
        // HTTP_USER_AGENT=Mozilla/5.0 (X11; Linux x86_64; rv:20.0) Gecko/20100101 Firefox/20.0
        // HTTP_ACCEPT=text/html,application/xhtml+xml,application/xml;q=0.9,*/ *;q=0.8
        // HTTP_ACCEPT_LANGUAGE=de-de,de;q=0.8,en-us;q=0.5,en;q=0.3
        // HTTP_ACCEPT_ENCODING=gzip, deflate
        // HTTP_COOKIE=session_id
        // HTTP_CONNECTION=keep-alive
        //
        req->request_method = FCGX_GetParam( "REQUEST_METHOD", req->fcgi_request->envp );
        req->user_agent = FCGX_GetParam( "HTTP_USER_AGENT", req->fcgi_request->envp );
        req->script_name = FCGX_GetParam( "SCRIPT_NAME", req->fcgi_request->envp );
        req->query_string = FCGX_GetParam( "QUERY_STRING", req->fcgi_request->envp );
        req->cookie = FCGX_GetParam( "HTTP_COOKIE", req->fcgi_request->envp );
        req->content_type = FCGX_GetParam( "CONTENT_TYPE", req->fcgi_request->envp );
        char *content_lenght = FCGX_GetParam("CONTENT_LENGTH", req->fcgi_request->envp);
        req->content_lenght = strtol( (content_lenght != 0 ? content_lenght : "0"), 0, 10 );
        req->referer = FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp );

        // Prepare tree
        req->get_tree = g_tree_new( (GCompareFunc) g_ascii_strcasecmp );
        req->post_tree = g_tree_new( (GCompareFunc) g_ascii_strcasecmp );
        req->cookie_tree = g_tree_new( (GCompareFunc) g_ascii_strcasecmp );
        req->files = NULL;

        // Store query data
        main_parse_query( req, req->get_tree, req->query_string, '=', '&' );

        // And even more data (from cookies)
        main_parse_query( req,  req->cookie_tree, req->cookie, '=', ';' );

        // Get data if available
        if( req->content_type != 0 && req->content_lenght > 0 )
        {
                req->content_data = MALLOC( req->content_lenght + 1 );
                if( req->content_data == 0 )
                {
                    log_printf( req, LOG_ALERT, "Unable to allocate %u bytes for content (max: %u)", req->content_lenght, NNCMS_UPLOAD_LEN_MAX );
                }
                else
                {
                    int result = FCGX_GetStr( req->content_data, req->content_lenght, req->fcgi_request->in );
                    req->content_data[result] = 0;
                    if( result < req->content_lenght )
                        log_printf( req, LOG_WARNING, "Received %u, required %u. Not enough bytes received on standard input", result, req->content_lenght );

                    if( strncmp( req->content_type, "application/x-www-form-urlencoded", 33 ) == 0 )
                        main_parse_query( req, req->post_tree, req->content_data, '=', '&' );

                    if( strncmp( req->content_type, "multipart/form-data", 19 ) == 0 )
                        main_store_data_rfc1867( req, req->content_data, req->content_lenght, req->content_type );
                }
        }

        // WARNING:
        // Headers should be reset before user check session

        // Prepare default response headers
        strcpy( req->response_headers, "Server: " CMAKE_PROJECT_NAME " " CMAKE_VERSION_STRING "\r\n" );
        strcpy( req->response_content_type, "text/html" );
        strcpy( req->response_code,"200 OK" );
        req->headers_sent = 0;

        // Who is requesting page?
        user_check_session( req );

        // Detect language
        i18n_detect_language( req );

        // What page was requested?
        char *lpszTempVar = strchr( req->script_name, '.' );
        if( lpszTempVar != 0 ) *lpszTempVar = 0;
        req->function_name = &(req->script_name[1]);
        
        // ################################
        // Page function call
        // ################################

        // Search function for requested page
        void (*func)(struct NNCMS_THREAD_INFO *) = g_hash_table_lookup( page_table, req->function_name );
        if( func == 0 )
            // Function not found, show default page or 404
            main_index( req );
        else
            func( req );

        // ################################
        // Deinitialization
        // ################################

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
            log_printf( req, LOG_DEBUG, "Page generation time: %.09f seconds.", tics );
        }

        // Finish request
        FCGX_Finish_r( req->fcgi_request );

        // Reset session
        user_reset_session( req );

        // Reset variables
        g_tree_destroy( req->get_tree );
        g_tree_destroy( req->post_tree );
        g_tree_destroy( req->cookie_tree );

        // Restart temporary buffers
        g_string_assign( req->buffer, "" );
        g_string_assign( req->frame, "" );
        g_string_assign( req->temp, "" );
        g_string_assign( req->output, "" );

        // Free content memory block
        if( req->content_data != NULL ) { FREE( req->content_data ); }
        if( req->files != NULL ) { g_array_free( req->files, TRUE ); }
        
        req->content_data = NULL;
        req->files = NULL;

        // Deinitialize garbage collector
        garbage_free( req->loop_garbage );
        
        // Leaks?
        memory_leaks( true );
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
        log_print( req, LOG_CRITICAL, "gmtime() returned null (year does not fit into an integer?)" );
}

// #############################################################################

// Convert typed data to a string
char *main_type( struct NNCMS_THREAD_INFO *req, union NNCMS_VARIABLE_UNION value, enum NNCMS_VARIABLE_TYPE type )
{
    // FIXME: Short cut for strings
    //if( type == NNCMS_TYPE_STRING )
    //    return value.string;
    
    GString *buffer = g_string_sized_new( 32 );
    
    switch( type )
    {
        case NNCMS_TYPE_UNSIGNED_INTEGER:
        {
            int unsigned_integer = value.unsigned_integer;
            g_string_printf( buffer, "%u", unsigned_integer );
            break;
        }

        case NNCMS_TYPE_INTEGER:
        {
            int integer = value.integer;
            g_string_printf( buffer, "%i", integer );
            break;
        }

        case NNCMS_TYPE_BOOL:
        {
            bool bval = value.boolean;
            if( bval == true )
            {
                char *bool_txt = i18n_string( req, "true", NULL );
                g_string_assign( buffer, bool_txt );
                g_free( bool_txt );
            }
            else
            {
                char *bool_txt = i18n_string( req, "false", NULL );
                g_string_assign( buffer, bool_txt );
                g_free( bool_txt );
            }
            break;
        }

        case NNCMS_TYPE_STR_BOOL:
        {
            char *pstring = value.string;
            if( strcmp( pstring, "1" ) == 0 ||
                strcmp( pstring, "on" ) == 0 ||
                strcmp( pstring, "true" ) == 0 )
            {
                char *bool_txt = i18n_string( req, "true", NULL );
                g_string_assign( buffer, bool_txt );
                g_free( bool_txt );
            }
            else
            {
                char *bool_txt = i18n_string( req, "false", NULL );
                g_string_assign( buffer, bool_txt );
                g_free( bool_txt );
            }
            break;
        }

        case NNCMS_TYPE_PMEM:
        {
            void **pval = (void **) value.pmem;
            g_string_printf( buffer, "%08Xh", *pval );
            break;
        }

        case NNCMS_TYPE_MEM:
        {
            void *pval = value.pmem;
            g_string_printf( buffer, "%08Xh", pval );
            break;
        }

        case NNCMS_TYPE_STRING:
        {
            char *pstring = value.string;
            if( pstring != 0 )
                g_string_assign( buffer, pstring );
            else
                g_string_assign( buffer, "" );
            break;
        }

        case NNCMS_TYPE_DUP_STRING:
        {
            char *pstring = value.string;
            if( pstring != 0 )
                g_string_assign( buffer, pstring );
            else
                g_string_assign( buffer, "" );
            break;
        }

        case TEMPLATE_TYPE_LINKS:
        {
            struct NNCMS_LINK *pval = (struct NNCMS_LINK *) value.pmem;
            char *tmp = template_links( req, pval );
            g_string_assign( buffer, tmp );
            g_free( tmp );
            break;
        }
        
        case TEMPLATE_TYPE_UNSAFE_STRING:
        {
            char *pstring = value.string;
            if( pstring != 0 )
            {
                char *escaped = template_escape_html( pstring );
                g_string_assign( buffer, escaped );
                g_free( escaped );
            }
            else
            {
                g_string_assign( buffer, "" );
            }
            break;
        }

        case TEMPLATE_TYPE_USER:
        {
            char *user_id = value.string;
            database_bind_text( req->stmt_find_user_by_id, 1, user_id );
            struct NNCMS_ROW *user_row = database_steps( req, req->stmt_find_user_by_id );
            
            char *user_name, *user_nick;
            if( user_row == 0 )
            {
                user_name = guest_user_name;
                user_nick = guest_user_nick;
            }
            else
            {
                user_name = user_row->value[1];
                user_nick = user_row->value[2];
            }

            g_string_printf( buffer, "<a href=\"%suser_view.fcgi?user_id=%s\">%s</a>", homeURL, user_id, user_nick );

            database_free_rows( user_row );
            break;
        }

        case TEMPLATE_TYPE_GROUP:
        {
            char *group_id = value.string;
            database_bind_text( req->stmt_find_group, 1, group_id );
            struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_find_group );
            if( group_row != NULL )
            {
                g_string_printf( buffer, "<a href=\"%sgroup_view.fcgi?group_id=%s\">%s</a>", homeURL, group_row->id, group_row->description );
                database_free_rows( (struct NNCMS_ROW *) group_row );
            }
            
            break;
        }

        case TEMPLATE_TYPE_FIELD_CONFIG:
        {
            database_bind_text( req->stmt_find_field_config, 1, value.string );
            struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_find_field_config );
            struct NNCMS_VARIABLE field_vars[] =
            {
                { .name = "field_id", .value.string = value.string, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };

            if( field_config_row != NULL )
            {
                g_string_append( buffer, field_config_row->name );
                database_free_rows( (struct NNCMS_ROW *) field_config_row );
            }

            break;
        }

        case NNCMS_TYPE_STR_TIMESTAMP:
        {
            char *timestamp = value.string;
            
            // Convert string to integer and then format it
            gint64 i_dt = g_ascii_strtoll( timestamp, NULL, 10 );
            GDateTime *g_dt = g_date_time_new_from_unix_local( i_dt );
            if( g_dt != 0 )
            {
                char *sz_dt = g_date_time_format( g_dt, szTimestampFormat );
                g_date_time_unref( g_dt );
                
                if( sz_dt != 0 )
                {
                    g_string_assign( buffer, sz_dt );
                    g_free( sz_dt );
                }
            }

            break;
        }

        case NNCMS_TYPE_TIMESTAMP:
        {
            // Convert string to integer and then format it
            gint64 i_dt = value.time;
            GDateTime *g_dt = g_date_time_new_from_unix_local( i_dt );
            if( g_dt != 0 )
            {
                char *sz_dt = g_date_time_format( g_dt, szTimestampFormat );
                g_date_time_unref( g_dt );
                
                if( sz_dt != 0 )
                {
                    g_string_assign( buffer, sz_dt );
                    g_free( sz_dt );
                }
            }

            break;
        }

        case TEMPLATE_TYPE_LANGUAGE:
        {
            char *lang_prefix = value.string;
            
            struct NNCMS_ROW *lang_row = database_query( req, "select id, name, native, prefix, weight from languages where prefix='%q'", lang_prefix );
            if( lang_row != 0 )
            {
                char *lang_id = lang_row->value[0];
                char *lang_name = lang_row->value[1];
                char *lang_native = lang_row->value[2];
                //char *lang_prefix = lang_row->value[3];
                char *lang_weight = lang_row->value[4];

                g_string_assign( buffer, lang_native );

                database_free_rows( lang_row );
            }
            else
            {
                // Not found
                g_string_assign( buffer, "" );
            }

            break;
        }

        case TEMPLATE_TYPE_ICON:
        {
            char *icon_path = value.string;
            g_string_printf( buffer, "<img src=\"%s%s\" width=\"16\" height=\"16\" alt=\"icon\" border=\"0\">", homeURL, icon_path );

            break;
        }

        default:
        {
            char *pstring = value.string;
            g_string_assign( buffer, pstring );
            break;
        }
    }
    
    // Return pointer to memory containing the string
    char *pbuffer = g_string_free( buffer, FALSE );
    return pbuffer;
}

// #############################################################################

void main_send_headers( struct NNCMS_THREAD_INFO *req, int nContentLength, time_t nModTime )
{
    char szTimeBuf[NNCMS_TIME_STR_LEN_MAX];

    if( req->headers_sent ) return;

    req->headers_sent = 1;

    main_format_time_string( req, szTimeBuf, 0 );
    FCGX_FPrintF( req->fcgi_request->out,
        "Status: %s\r\n"          // Response
        "%s"                      // Response headers
        "Date: %s\r\n"            // Date
        "Connection: close\r\n"   // Connection
        "Content-Type: %s\r\n",   // Content Type
        req->response_code, req->response_headers, szTimeBuf,
        req->response_content_type );

    if( nContentLength > 0 )
    {
        main_format_time_string( req, szTimeBuf, nModTime );
        FCGX_FPrintF( req->fcgi_request->out,
            "Content-Length: %d\r\n"  // Content-Lenght
            "Last-Modified: %s\r\n",  // Last-Modified
            nContentLength, szTimeBuf );
    }

    // Заключительный перевод строки, после него идут данные, вместо
    // заголовков
    FCGX_PutS( "\r\n", req->fcgi_request->out );
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
    strlcpy( req->response_code, lpszMsg, sizeof(req->response_code) );
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
    strlcpy( req->response_content_type, type, sizeof(req->response_content_type) );
}

// #############################################################################

// If you need to add a header line to the set of headers being returned to the
// client it can be done using httpdAddHeader.
// Example:
//
//      main_set_response( req, "307 Temporary Redirect" );
//      main_header_add( req, "Location: http://www.foo.com/some/new/location" );
void main_header_add( struct NNCMS_THREAD_INFO *req, char *lpszMsg )
{
    strcat( req->response_headers, lpszMsg );
    if( lpszMsg[strlen( lpszMsg ) - 1] != '\n' )
        strcat( req->response_headers, "\r\n" );
}

// #############################################################################

// To add a cookie to the HTTP response, simply call this routine with the name
// and value of the cookie. There is no httpdGetCookie( ) equivalent routines as
// cookies are automatically presented as variables in the symbol table.
void main_set_cookie( struct NNCMS_THREAD_INFO *req, char *name, char *value )
{
    char buff[NNCMS_HEADERS_LEN_MAX];

    snprintf( buff, sizeof(req->response_headers), "Set-Cookie: %s=%s; path=/;", name, value );
    main_header_add( req, buff );
}

// #############################################################################

void main_atquit( )
{
    //printf( "atquit\n" );
    
    // Signal all thread to die and wait
    bDone = true;
    FCGX_ShutdownPending( );
    for( int i = 0; i < nThreads; i++ )
    {
        // Kill the thread
        if( i > 0 )
            pthread_kill( pThreadList[i]->pThread, 0 );

        // Deinit modules
        for( unsigned int j = 0; j < local_destroy->len; j++ )
        {
            bool (*deinit_func)(struct NNCMS_THREAD_INFO *) = local_destroy->pdata[j];
            deinit_func( pThreadList[i] );
        }

        SAFE_FREE( pThreadList[i]->fcgi_request );
        SAFE_FREE( pThreadList[i] );
    }

    // Mem free
    SAFE_FREE( pThreadList );

    file_global_destroy( );
    log_global_destroy( );
    i18n_global_destroy( );
    acl_global_destroy( );
    template_global_destroy( );
    user_global_destroy( );
    group_global_destroy( );
    post_global_destroy( );
    ban_global_destroy( );
    view_global_destroy( );
    node_global_destroy( );
    xxx_global_destroy( );

    // Flush and close database
    database_global_destroy( );
    cache_global_destroy( );

    g_ptr_array_free( local_init, TRUE );
    g_ptr_array_free( local_destroy, TRUE );
    g_hash_table_unref( page_table );

    // Free internally allocated tables created in g_mime_global_init().
    g_mime_shutdown( );

    memory_global_destroy( );

    // Water
    log_print( 0, LOG_INFO, "nncms stopped" );

    exit( EXIT_SUCCESS );
}

// #############################################################################

void main_quit( int nSignal )
{
    if( nSignal == SIGINT )
        //printf( "\n" );
        printf( " <- rofl\n" );

    //printf( "signal: %i\n", nSignal );

    exit( nSignal );
}

// #############################################################################

void main_vmessage( struct NNCMS_THREAD_INFO *req, char *i18n_id )
{
    char *msg = i18n_string( req, i18n_id, NULL );
    main_message( req, msg );
    g_free( msg );
}

void main_message( struct NNCMS_THREAD_INFO *req, char *msg )
{
    // Page header
    char *header_str = "Message";
    
    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = msg, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/categories/applications-internet.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

// This function makes valid HTML text, add banners, footer,
// header, menu blocks
unsigned int main_output( struct NNCMS_THREAD_INFO *req, char *lpszTitle, char *lpszContent, time_t nModTime )
{
    // Specify values for template
    struct NNCMS_VARIABLE main_tags[] =
        {
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "title", .value.string = lpszTitle, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = lpszContent, .type = NNCMS_TYPE_STRING },
            { .name = "copyleft", .value.string = CMAKE_PROJECT_NAME " <!-- Copyleft (C) 2005 - 2013 ValkaTR &lt;valkur@gmail.com&gt; -->" },
            { .name = "user_id", .value.string = req->user_id, .type = NNCMS_TYPE_STRING },
            { .name = "user_name", .value.string = req->user_name, .type = NNCMS_TYPE_STRING },
            { .name = "user_nick", .value.string = req->user_nick, .type = NNCMS_TYPE_STRING },
            { .name = "user_avatar", .value.string = req->user_avatar, .type = NNCMS_TYPE_STRING },
            { .name = "user_folder_id", .value.string = req->user_folder_id, .type = NNCMS_TYPE_STRING },
            { .name = "messages", .value.string = 0, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Load messages
    struct NNCMS_LOG *msg_array = log_session_get_messages( req );
    if( msg_array != NULL && msg_array->text != NULL )
    {
        GString *msg_html = g_string_new( NULL );
        
        // Parse every message
        for( unsigned int i = 0; msg_array[i].text != NULL; i = i + 1 )
        {
            struct NNCMS_PRIORITY_INFO *priority_info = log_get_priority( msg_array[i].priority );
            struct NNCMS_VARIABLE msg_tags[] =
                {
                    { .name = "msg_timestamp", .value.time = msg_array[i].timestamp, .type = NNCMS_TYPE_TIMESTAMP },
                    { .name = "msg_priority_id", .value.integer = msg_array[i].priority, .type = NNCMS_TYPE_INTEGER },
                    { .name = "msg_priority", .value.string = priority_info->lpszPriority, .type = NNCMS_TYPE_STRING },
                    { .name = "msg_icon", .value.string = priority_info->lpszIcon, .type = NNCMS_TYPE_STRING },
                    { .name = "msg_fg_color", .value.string = priority_info->lpszHtmlFgColor, .type = NNCMS_TYPE_STRING },
                    { .name = "msg_bg_color", .value.string = priority_info->lpszHtmlBgColor, .type = NNCMS_TYPE_STRING },
                    { .name = "msg_text", .value.string = msg_array[i].text->str, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE } // Terminating row
                };

            template_hparse( req, "log_message.html", msg_html, msg_tags );
        }

        char *pmsg_html = g_string_free( msg_html, FALSE );
        garbage_add( req->loop_garbage, pmsg_html, MEMORY_GARBAGE_GFREE );
        main_tags[3].value.string = pmsg_html;
        
        log_session_clear_messages( req );
    }

    // Parse template file
    size_t nRetVal = template_hparse( req, "structure.html", req->output, main_tags );
    if( nRetVal == 0 )
    {
        // Error happened, memory not allocated or file not found
        log_print( 0, LOG_CRITICAL, "Could not parse template file: file not found or memory could not be allocated" );
        return 0;
    }

    // Output
    main_send_headers( req, req->output->len, nModTime );
    FCGX_PutStr( req->output->str, req->output->len, req->fcgi_request->out );
    //DUMP( req->lpszOutput );
}

// #############################################################################

void redirect_to_referer( struct NNCMS_THREAD_INFO *req )
{
    char *lpszReferer = req->referer;
    char *httpPostReferer = main_variable_get( req, req->post_tree, "referer" );
    char *httpGetReferer = main_variable_get( req, req->get_tree, "referer" );
    if( httpGetReferer != 0 && *httpGetReferer != 0 )
            redirect( req, httpGetReferer );
    else if( httpPostReferer != 0 && *httpPostReferer != 0 )
            redirect( req, httpPostReferer );
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
    struct NNCMS_VARIABLE redirect_template[] =
        {
            { .name = "redirect", .value.string = lpszURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
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
    main_set_response( req, "302 Found" );

    // Generate the page
    char szLocation[NNCMS_PATH_LEN_MAX];
    snprintf( szLocation, sizeof(szLocation), "Location: %s", lpszURL );
    main_header_add( req, szLocation );
    //main_header_add( req, "Location: index.fcgi" );
    
    unsigned int nRetVal = template_hparse( req, "redirect.html", req->output, redirect_template );

    // Send generated html to client
    main_send_headers( req, nRetVal, 0 );
    FCGX_PutStr( req->output->str, req->output->len, req->fcgi_request->out );
}

// #############################################################################

// Index page
void main_index( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = "";

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
    fclose( pFile );

    // Terminate string
    req->lpszFrame[lSize] = 0;

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = req->lpszFrame, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "/images/categories/applications-internet.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = NULL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Put data in main template
    main_output( req, header_str, req->frame->str, 0 );

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
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = szHeader, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = req->lpszBuffer, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-system-monitor.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
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
    if( req->content_lenght <= 0 )
    {
        smart_cat( &smartBuffer, "No data from standard input.<p>\n" );
    }
    else
    {
        smart_cat( &smartBuffer, "Standard input:<br>\n<pre>\n" );
        smart_cat( &smartBuffer, req->content_data );
        smart_cat( &smartBuffer, "\n</pre><p>\n" );
    }

    // Get some info
    main_print_env( req, &smartBuffer, "Request environment", req->fcgi_request->envp );
    main_print_env( req, &smartBuffer, "Initial environment", environ );

    // Variables
    /*snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<hr>%s:<br>\n<pre>\n", "Variables" );
    smart_cat( &smartBuffer, req->lpszTemp );
    for( int i = 0; i < NNCMS_VARIABLES_MAX; i++ )
    {
        if( req->variables[i].name != 0 && req->variables[i].value.string != 0 )
        {
            snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<b>%s</b> = <font color=\"gray\">%s</font>\n",
                req->variables[i].name, req->variables[i].value.string );
            smart_cat( &smartBuffer, req->lpszTemp );
        }
    }
    smart_cat( &smartBuffer, "</pre><p>\n" );*/

    // Uploaded files
    /*snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX, "<hr>%s:<br>\n", "Uploaded files" );
    smart_cat( &smartBuffer, req->lpszTemp );
    struct NNCMS_FILE *lpCurFile = req->lpUploadedFiles;
    while( lpCurFile )
    {
        snprintf( req->lpszTemp, NNCMS_PAGE_SIZE_MAX,
            "<b>name</b> = <font color=\"darkgreen\">%s</font><br>\n"
            "<b>szFileName</b> = <font color=\"darkgreen\">%s</font><br>\n"
            "<b>lpData</b> = <pre>%s</pre><br>\n"
            "<b>nSize</b> = <font color=\"red\">%u</font><br>---<br>\n",
            lpCurFile->name, lpCurFile->szFileName, lpCurFile->lpData,
            lpCurFile->nSize );
        smart_cat( &smartBuffer, req->lpszTemp );
        lpCurFile = lpCurFile->lpNext;
    }
    smart_cat( &smartBuffer, "<p>\n" );*/

    // Generate the block
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Put data in main template
    main_output( req, szHeader, req->frame->str, 0 );

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
        log_printf( 0, LOG_ERROR, "Could not open \"%s\" file", lpszFileName );
        return NULL;
    }

    // Get file size
    fseek( pFile, 0, SEEK_END );
    size_t nSize = ftell( pFile );
    rewind( pFile );

    // Do not accept zero files
    if( nSize == 0 )
        return NULL;

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
        log_printf( 0, LOG_ALERT, "Could not allocate %u bytes of memory for \"%s\" file", nSize, lpszFileName );
        fclose( pFile );
        return NULL;
    }

    // Read and close file
    *nLimit = fread( lpszBuffer, 1, nSize, pFile );
    fclose( pFile );

    // Null terminating character
    lpszBuffer[*nLimit] = 0;

    return lpszBuffer;
}

// #############################################################################

#include "recursive_mkdir.h"
bool main_store_file( char *lpszFileName, char *lpszData, size_t *nSize )
{
    // Create path to file
    char *lpszTempFileName = rindex( lpszFileName, '/' );
    if( lpszTempFileName != NULL )
    {
        // Separate path and filename
        *lpszTempFileName = 0;

        // Create path recursively
        recursive_mkdir( lpszFileName );

        // Join path and file name
        *lpszTempFileName = '/';
    }

    // Open for reading and writing.  The file is created  if  it  does not
    // exist, otherwise it is truncated.  The stream is positioned at the
    // beginning of the file.
    FILE *pFile = fopen( lpszFileName, "w+b" );
    if( pFile == NULL )
    {
        log_printf( 0, LOG_ALERT, "Could not open \"%s\" file for writing", lpszFileName );
        return false;
    }

    if( lpszData != NULL )
    {
        size_t nWrite = 0;
        size_t nResult = 0;
        
        // Get size from argument
        if( nSize != NULL )
            nWrite = *nSize;
        
        // If argument size is zero, then calculate
        if( nWrite == 0 )
            nWrite = strlen( lpszData );

        // Write changes to file
        if( nWrite != 0 )
            nResult = fwrite( lpszData, nWrite, 1, pFile );
        
        if( nSize != NULL )
            *nSize = nResult;
    }

    // Tests the error indicator for the stream
    int nErrNo = ferror( pFile );
    if( nErrNo != 0 )
    {
        char szErrorBuf[100];
        strerror_r( nErrNo, szErrorBuf, sizeof(szErrorBuf) );
        log_printf( 0, LOG_ALERT, "Error writing \"%s\" file (%s)", lpszFileName, &szErrorBuf );
        fclose( pFile );
        return false;
    }

    // Close the file associated with the specified pFile stream after
    // flushing all buffers associated with it.
    fclose( pFile );
    return true;
}

// #############################################################################

bool main_remove( struct NNCMS_THREAD_INFO *req, char *file )
{
    bool ret_val = false;
    
    // Canonicalize file name
    char *canon_file = canonicalize_filename_mode( file, CAN_EXISTING | CAN_NOLINKS );
    
    // Check if file resides within base path
    size_t file_dir_len = strlen( fileDir );
    if( strncmp( canon_file, fileDir, file_dir_len ) != 0 )
    {
        log_printf( req, LOG_ALERT, "Attempt to remove file outside base path (File = %s, Canonical = %s)", file, canon_file );
        goto main_remove_exit;
    }
    
    // Get info
    struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, canon_file );
    
    // Exists?
    if( file_info->exists == false )
        goto main_remove_exit;
    
    if( file_info->is_file == true )
    {
        // If it's file, then just remove it
        int result = unlink( canon_file );
        if( result != 0 )
        {
            char szErrorBuf[100];
            strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
            log_printf( req, LOG_ALERT, "Error removing \"%s\" file (%s)", canon_file, &szErrorBuf );
            goto main_remove_exit;
        }
    }
    else if( file_info->is_dir == true )
    {
        // Recursive
        GPtrArray *gstack = g_ptr_array_new( );
        g_ptr_array_add( gstack, g_string_new( canon_file ) );

        // Stack will increase as new directories found
        for( unsigned int i = 0; i < gstack->len; i = i + 1 )
        {
            GString *gfile = (GString *) (gstack->pdata[i]);
            
            //
            // List files in current directory
            //
            DIR *dir_stream = opendir( gfile->str );
            if( dir_stream == 0 )
            {
                char szErrorBuf[100];
                strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                log_printf( req, LOG_ALERT, "Error openning \"%s\" directory (%s)", file, &szErrorBuf );
                ret_val = false;
                continue;
            }

            //
            // Read the directory
            //
            rewinddir( dir_stream );
            while( 1 )
            {
                struct dirent *dir_entity = readdir( dir_stream );
                if( dir_entity == NULL )
                    break;
                
                // Skip . and ..
                if( strcmp( dir_entity->d_name, "." ) == 0 ||
                    strcmp( dir_entity->d_name, ".." ) == 0 )
                    continue;
                
                // Make full path to file
                GString *full_path = g_string_sized_new( 32 );
                g_string_assign( full_path, gfile->str );
                g_string_append_c( full_path, '/' );
                g_string_append( full_path, dir_entity->d_name );
                
                // Stat file
                struct stat f_stat;
                int result = lstat( full_path->str, &f_stat );
                if( result != 0 )
                {
                    char szErrorBuf[100];
                    strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                    log_printf( req, LOG_ALERT, "Error stat \"%s\" file (%s)", full_path->str, &szErrorBuf );
                    ret_val = false;
                    continue;
                }
                
                // Recurse into directories
                if( S_ISDIR( f_stat.st_mode ) )
                {
                    // Add to stack, which will free the pointers later
                    g_ptr_array_add( gstack, full_path );
                }
                
                // Unlink the file
                else if( S_ISREG( f_stat.st_mode ) )
                {
                    int result = unlink( full_path->str );
                    if( result != 0 )
                    {
                        char szErrorBuf[100];
                        strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                        log_printf( req, LOG_ERROR, "Error removing file %s (%s)", full_path->str, &szErrorBuf );
                        ret_val = false;
                    }
                    g_string_free( full_path, TRUE );
                }
            }
            
            // Close directory stream
            int result = closedir( dir_stream );
            if( result != 0 )
            {
                char szErrorBuf[100];
                strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                log_printf( req, LOG_ERROR, "Could not close stream (%s)", &szErrorBuf );
            }
        }
        
        // Traverse stack array backwards and remove directories
        for( unsigned int i = gstack->len; i != 0; i = i - 1 )
        {
            GString *gfile = (GString *) (gstack->pdata[i - 1]);
            
            int result = rmdir( gfile->str );
            if( result != 0 )
            {
                char szErrorBuf[100];
                strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                log_printf( req, LOG_ERROR, "Error removing directory %s (%s)", gfile->str, &szErrorBuf );
                ret_val = false;
            }

            g_string_free( gfile, TRUE );
        }

        g_ptr_array_free( gstack, TRUE );
    }
    else
    {
        log_printf( req, LOG_ALERT, "Tried to remove \"%s\", which is not a file, nor a directory", canon_file );
        goto main_remove_exit;
    }

    // Ok
    ret_val = true;
    
main_remove_exit:
    free( canon_file );
    return true;
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
