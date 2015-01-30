// #################################################################################
// Source file: log.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <glib.h>

// #################################################################################
// includes of local headers
//

#include "log.h"
#include "main.h"
#include "database.h"
#include "user.h"
#include "filter.h"
#include "template.h"
#include "acl.h"
#include "smart.h"
#include "i18n.h"
#include "memory.h"

// #################################################################################
// global variables
//

// Path to log file
char szLogFile[NNCMS_PATH_LEN_MAX] = "./files/logs/nncms.log";

// Data about priorities
struct NNCMS_PRIORITY_INFO piData[] =
{
    { .priority = LOG_DEBUG, .lpszPriorityANSI = TERM_FG_WHITE "Debug" TERM_RESET, .lpszPriority = "Debug", .lpszIcon = "images/status/code/stock_macro-watch-variable.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#E0E0E0" },
    { .priority = LOG_INFO, .lpszPriorityANSI = TERM_FG_CYAN "Info" TERM_RESET, .lpszPriority = "Info", .lpszIcon = "images/status/dialog-information.png", .lpszHtmlFgColor = "#505070", .lpszHtmlBgColor = "#E0E0F0" },
    { .priority = LOG_ACTION, .lpszPriorityANSI = TERM_FG_GREEN "Action" TERM_RESET, .lpszPriority = "Action", .lpszIcon = "images/status/code/stock_script.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#C0F0C0" },
    { .priority = LOG_NOTICE, .lpszPriorityANSI = TERM_FG_RED "Notice" TERM_RESET, .lpszPriority = "Notice", .lpszIcon = "images/stock/stock_notes.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#C0F0C0" },
    { .priority = LOG_WARNING, .lpszPriorityANSI = TERM_FG_YELLOW "Warning" TERM_RESET, .lpszPriority = "Warning", .lpszIcon = "images/status/dialog-warning.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#F0F0C0" },
    { .priority = LOG_ERROR, .lpszPriorityANSI = TERM_FG_RED "Error" TERM_RESET, .lpszPriority = "Error", .lpszIcon = "images/status/dialog-error.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#F0C0C0" },
    { .priority = LOG_CRITICAL, .lpszPriorityANSI = TERM_BRIGHT TERM_FG_RED "Critical" TERM_RESET, .lpszPriority = "Critical", .lpszIcon = "images/status/dialog-error.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#F05050" },
    { .priority = LOG_ALERT, .lpszPriorityANSI = TERM_BRIGHT TERM_UNDERSCORE TERM_FG_RED "Alert" TERM_RESET, .lpszPriority = "Alert", .lpszIcon = "images/status/dialog-error.png", .lpszHtmlFgColor = "#000000", .lpszHtmlBgColor = "#F02020" },
    { .priority = LOG_EMERGENCY, .lpszPriorityANSI = TERM_BRIGHT TERM_UNDERSCORE TERM_BLINK TERM_FG_RED "Emergency" TERM_RESET, .lpszPriority = "Emergency", .lpszIcon = "images/status/dialog-error.png", .lpszHtmlFgColor = "#FFFFFF", .lpszHtmlBgColor = "#F00000" },
    { .priority = LOG_UNKNOWN, .lpszPriorityANSI = TERM_BRIGHT TERM_FG_MAGENTA "Unknown" TERM_RESET, .lpszPriority = "Unknown", .lpszIcon = "images/mimetypes/empty.png", .lpszHtmlFgColor = "#FFFFFF", .lpszHtmlBgColor = "#000000" },
    { .priority = 0, .lpszPriorityANSI = 0, .lpszPriority = 0, .lpszIcon = 0, .lpszHtmlFgColor = 0, .lpszHtmlBgColor = 0 }
};

// File
FILE *pFile = 0;

// Log file mutex
pthread_mutex_t log_mutex;

// Displaying log entries
GHashTable *session_table;

struct NNCMS_LOG_CLEAR_FIELDS
{
    struct NNCMS_FIELD log_clear_db;
    struct NNCMS_FIELD log_clear_file;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD log_clear;
    struct NNCMS_FIELD none;
}
log_clear_fields =
{
    { .name = "log_clear_db", .value = "on", .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_clear_file", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "log_clear", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_LOG_FIELDS
{
    struct NNCMS_FIELD log_id;
    struct NNCMS_FIELD log_priority;
    struct NNCMS_FIELD log_timestamp;
    struct NNCMS_FIELD log_text;
    struct NNCMS_FIELD log_user_id;
    struct NNCMS_FIELD log_user_agent;
    struct NNCMS_FIELD log_user_host;
    struct NNCMS_FIELD log_uri;
    struct NNCMS_FIELD log_referer;
    struct NNCMS_FIELD log_function;
    struct NNCMS_FIELD log_src_file;
    struct NNCMS_FIELD log_src_line;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    //struct NNCMS_FIELD log_clear;
    struct NNCMS_FIELD none;
}
log_fields =
{
    { .name = "log_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_priority", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_timestamp", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_text", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_user_agent", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_user_host", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_uri", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_function", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_src_file", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "log_src_line", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    //{ .name = "log_clear", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// #############################################################################
// functions

// Session key destroy
void log_key_destroy_func( gpointer data )
{
    gchar *session_id = (gchar *) data;
    g_free( session_id );
}

// Session value destroy
void log_value_destroy_func( gpointer data )
{
    GArray *array = (GArray *) data;
    g_array_free( array, TRUE );
}

// Log element destroy
void log_array_destroy_func( gpointer data )
{
    struct NNCMS_LOG *log_entry = (struct NNCMS_LOG *) data;
    g_string_free( log_entry->text, TRUE );
}

// #############################################################################

bool log_global_init( )
{
    // Open file log file
    pFile = fopen( szLogFile, "a" );
    if( pFile == 0 )
        log_printf( 0, LOG_ERROR, "Could not open log file %s", szLogFile );

    // Init pthread locking
    if( pthread_mutex_init( &log_mutex, 0 ) != 0 )
    {
        log_print( 0, LOG_EMERGENCY, "Mutex init failed!!" );
        exit( -1 );
    }

    main_local_init_add( &log_local_init );
    main_local_destroy_add( &log_local_destroy );

    main_page_add( "log_view", &log_view );
    main_page_add( "log_clear", &log_clear );
    main_page_add( "log_list", &log_list );

    session_table = g_hash_table_new_full( g_str_hash, g_str_equal, log_key_destroy_func, log_value_destroy_func );

    // Test logging system
    /*log_print( 0, LOG_UNKNOWN, testing );
    log_print( 0, LOG_INFO, "testing" );
    log_print( 0, LOG_NOTICE, "testing" );
    log_print( 0, LOG_WARNING, "testing" );
    log_print( 0, LOG_ERROR, "testing" );
    log_print( 0, LOG_CRITICAL, "testing" );
    log_print( 0, LOG_ALERT, "testing" );
    log_print( 0, LOG_EMERGENCY, "testing" );//*/

    return true;
}

// #############################################################################

bool log_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_list_logs = database_prepare( req, "SELECT `id`, `priority`, `text`, `timestamp`, `user_id`, `user_host`, `user_agent`, `uri`, `referer`, src_file, src_line FROM `log` ORDER BY `id` DESC LIMIT ? OFFSET ?" );
    req->stmt_count_logs = database_prepare( req, "SELECT COUNT(*) FROM `log`" );
    req->stmt_view_log = database_prepare( req, "SELECT `id`, `priority`, `text`, `timestamp`, `user_id`, `user_host`, `user_agent`, `uri`, `referer`, function, src_file, src_line FROM `log` WHERE `id`=?" );
    req->stmt_add_log_req = database_prepare( req, "INSERT INTO `log` VALUES(null, ?, ?, ?, ?, ? || ':' || ?, ?, ?, ?, ?, ?, ?)" );
    req->stmt_clear_log = database_prepare( req, "DELETE FROM `log`" );

    return true;
}

// #############################################################################

bool log_global_destroy( )
{
    // Close log file
    fclose( pFile );
    pFile = 0;

    // Deinit pthread locking
    if( pthread_mutex_destroy( &log_mutex ) != 0 )
    {
        log_print( 0, LOG_EMERGENCY, "Log:Deinit: Mutex destroy failed!!" );
    }

    g_hash_table_destroy( session_table );

    return true;
}

// #############################################################################


bool log_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_list_logs );
    database_finalize( req, req->stmt_view_log );
    database_finalize( req, req->stmt_add_log_req );
    database_finalize( req, req->stmt_clear_log );

    return true;
}

// #############################################################################

struct NNCMS_PRIORITY_INFO *log_get_priority( enum NNCMS_PRIORITY priority )
{
    for( unsigned int i = 0; i < sizeof(piData); i++ )
    {
        if( piData[i].priority == priority )
            return &piData[i];
    }
    return log_get_priority( LOG_UNKNOWN );
}

// #################################################################################

void log_displayf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY priority, char *format, ... )
{
    GString *text = g_string_sized_new( 50 );
    
    // Parse variable number of arguments
    va_list args;
    va_start( args, format );
    g_string_vprintf( text, format, args );
    va_end( args );

    // Get Time string
    time_t timestamp = time( NULL );

    // Check session id
    char *session_id = req->session_id;
    if( session_id == NULL )
        session_id = "null";

    // Get array from hash table
    GArray *array = g_hash_table_lookup( session_table, session_id );
    if( array == NULL )
    {
        // Create new array if not found
        array = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_LOG) );
        g_array_set_clear_func( array, log_array_destroy_func );

        // Inserts a new key and value into a GHashTable
        g_hash_table_insert( session_table, g_strdup(session_id), array );
    }

    // Add log entry to the array
    struct NNCMS_LOG log_entry =
    {
        .text = text,
        .timestamp = timestamp,
        .priority = priority
    };    
    g_array_append_vals( array, &log_entry, 1 );

    return;
}

// #################################################################################

void log_vdisplayf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY priority, char *i18n_id, struct NNCMS_VARIABLE *vars )
{
    // Prepare output buffer
    GString *gtext = g_string_sized_new( 50 );
    
    // Get string from the database
    char *template_data = i18n_string( req, i18n_id, NULL );
    
    // Parse the string
    template_sparse( req, i18n_id, template_data, gtext, vars );
    
    // String from database is not needed anymore
    g_free( template_data );

    // Get Time string
    time_t timestamp = time( NULL );

    log_displayf( req, priority, gtext->str );

    // Copy of the string is stored in database
    g_string_free( gtext, TRUE );
    
    return;
}

// #################################################################################

struct NNCMS_LOG *log_session_get_messages( struct NNCMS_THREAD_INFO *req )
{
    // Prepare session id
    char *session_id = req->session_id;
    if( session_id == NULL )
        session_id = "null";

    // Get array from hash table
    GArray *array;
    gchar *orig_session_id;
    gboolean result = g_hash_table_lookup_extended( session_table, session_id, (gpointer *) &orig_session_id, (gpointer *) &array );
    if( result == FALSE )
        return NULL;

    struct NNCMS_LOG *log_entry = (struct NNCMS_LOG *) array->data;

    return log_entry;
}

void log_session_clear_messages( struct NNCMS_THREAD_INFO *req )
{
    // Prepare session id
    char *session_id = req->session_id;
    if( session_id == NULL )
        session_id = "null";

    // Get array from hash table
    GArray *array;
    gchar *orig_session_id;
    gboolean result = g_hash_table_lookup_extended( session_table, session_id, (gpointer *) &orig_session_id, (gpointer *) &array );
    if( result == FALSE )
        return;

    struct NNCMS_LOG *log_entry = (struct NNCMS_LOG *) array->data;

    //// Clear hash table right away
    //g_hash_table_steal( session_table, session_id );
    //g_free( orig_session_id );
    //log_entry = g_array_free( array, FALSE );
    g_array_set_size( array, 0 );
}

// #################################################################################

void log_debug_printf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY priority, char *format, const char *function, const char *src_file, int src_line, ... )
{
    va_list vArgs;

    // This should hold sqlite error message
    char *zErrMsg = 0;

    // Log text buffer
    char log_text[NNCMS_MAX_LOG_TEXT];

    // Buffer for full log text18
    char text[NNCMS_MAX_LOG_TEXT + 64];

    va_start( vArgs, src_line );
    vsnprintf( log_text, NNCMS_MAX_LOG_TEXT, format, vArgs );
    va_end( vArgs );

    // Get priority text
    struct NNCMS_PRIORITY_INFO *priority_info = log_get_priority( priority );

    // Get Time string
    char str_time[64];
    time_t timestamp = time( 0 );
    struct tm *timeinfo = localtime( &timestamp );
    strftime( str_time, 64, szTimestampFormat, timeinfo );

    // Print to console
    snprintf( text, sizeof(text), "%s [%-9s] [%s at %s:%i] %s\n", str_time, priority_info->lpszPriorityANSI, function, src_file, src_line, log_text );
    fputs( text, stdout ); // Don't use puts( ); here, it appends \n at the end of line

#ifndef DEBUG
    // Do not add debug info into file log
    if( priority == LOG_DEBUG ) return;
#endif // DEBUG

    // Write text to file
    if( pFile != 0 )
    {
        if( pthread_mutex_lock( &log_mutex ) != 0 )
        {
            log_print( req, LOG_EMERGENCY, "Can't acquire lock!!" );
            exit( -1 );
        }
        fputs( text, pFile );
        pthread_mutex_unlock( &log_mutex );
    }

    // Do not add debug info into sqlite database
    if( priority == LOG_DEBUG ) return;

    // Write to sqlite database
    filter_table_replace( log_text, (unsigned int) strlen( log_text ), queryFilter );
    if( req != 0 )
    {
        database_bind_int( req->stmt_add_log_req, 1, priority );       // Priority
        database_bind_text( req->stmt_add_log_req, 2, log_text );      // Text
        database_bind_int( req->stmt_add_log_req, 3, timestamp );         // Timestamp
        database_bind_text( req->stmt_add_log_req, 4, req->user_id );  // User ID
        database_bind_text( req->stmt_add_log_req, 5, FCGX_GetParam( "REMOTE_ADDR", req->fcgi_request->envp ) );  // Host
        database_bind_text( req->stmt_add_log_req, 6, FCGX_GetParam( "REMOTE_PORT", req->fcgi_request->envp ) );  // Host
        database_bind_text( req->stmt_add_log_req, 7, FCGX_GetParam( "HTTP_USER_AGENT", req->fcgi_request->envp ) );  // User-Agent
        database_bind_text( req->stmt_add_log_req, 8, FCGX_GetParam( "REQUEST_URI", req->fcgi_request->envp ) );  // URI
        database_bind_text( req->stmt_add_log_req, 9, req->referer );  // Referer
        database_bind_text( req->stmt_add_log_req, 10, (char *) function );      // Function
        database_bind_text( req->stmt_add_log_req, 11, (char *) src_file );      // Source File
        database_bind_int( req->stmt_add_log_req, 12, src_line );       // Source Line
        database_steps( req, req->stmt_add_log_req );
    }

    return;
}

// #################################################################################

void log_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "log", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *log_id = main_variable_get( req, req->get_tree, "log_id" );
    if( log_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_view_log, 1, log_id );
    struct NNCMS_LOG_ROW *log_row = (struct NNCMS_LOG_ROW *) database_steps( req, req->stmt_view_log );
    if( log_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, log_row, MEMORY_GARBAGE_DB_FREE );

    // Priority info
    enum NNCMS_PRIORITY log_priority = atoi( log_row->priority );
    struct NNCMS_PRIORITY_INFO *log_priority_info = log_get_priority( log_priority );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "log_id", .value.string = log_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "log_view_header", vars );

    //
    // Form
    //
    struct NNCMS_LOG_FIELDS *fields = memdup_temp( req, &log_fields, sizeof(log_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->log_id.value = log_row->id;
    fields->log_priority.value = log_priority_info->lpszPriority;
    fields->log_text.value = log_row->text;
    fields->log_timestamp.value = log_row->timestamp;
    fields->log_user_id.value = log_row->user_id;
    fields->log_user_host.value = log_row->user_host;
    fields->log_user_agent.value = log_row->user_agent;
    fields->log_uri.value = log_row->uri;
    fields->log_referer.value = log_row->referer;
    fields->log_function.value = log_row->function;
    fields->log_src_file.value = log_row->src_file;
    fields->log_src_line.value = log_row->src_line;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "log_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Colorize the cell
    GString *style = g_string_sized_new( 64 );
    garbage_add( req->loop_garbage, style, MEMORY_GARBAGE_GSTRING_FREE );
    g_string_printf( style, "background-color: %s; color: %s;",
        log_priority_info->lpszHtmlBgColor,
        log_priority_info->lpszHtmlFgColor );
    struct NNCMS_VARIABLE options[] =
    {
        { .name = "style", .value.string = style->str, .type = NNCMS_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Generate links
    char *links = log_links( req, log_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-system-monitor.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );


    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

void log_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "log", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "log_list_header", NULL );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_logs );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_logs, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_logs, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_LOG_ROW *log_row = (struct NNCMS_LOG_ROW *) database_steps( req, req->stmt_list_logs );
    if( log_row != NULL )
    {
        garbage_add( req->loop_garbage, log_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "log_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "log_timestamp_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "log_priority_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "log_text_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; log_row != NULL && log_row[i].id != NULL; i = i + 1 )
    {
        enum NNCMS_PRIORITY log_priority = atoi( log_row[i].priority );
        struct NNCMS_PRIORITY_INFO *log_priority_info = log_get_priority( log_priority );
        
        // Colorize the cell
        GString *style = g_string_sized_new( 64 );
        garbage_add( req->loop_garbage, style, MEMORY_GARBAGE_GSTRING_FREE );
        g_string_printf( style, "background-color: %s; color: %s;",
            log_priority_info->lpszHtmlBgColor,
            log_priority_info->lpszHtmlFgColor );
        struct NNCMS_VARIABLE options[] =
        {
            { .name = "style", .value.string = style->str, .type = NNCMS_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        
        // Actions
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "log_id", .value.string = log_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "log_view", i18n_string_temp( req, "view", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = log_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = log_row[i].timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP, .options = NULL },
            { .value.string = log_priority_info->lpszPriority, .type = NNCMS_TYPE_STRING, .options = memdup_temp( req, options, sizeof(options) ) },
            { .value.string = log_row[i].text, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
    }

    // Create a table
    struct NNCMS_TABLE table =
    {
        .caption = NULL,
        .header_html = NULL, .footer_html = NULL,
        .options = NULL,
        .cellpadding = NULL, .cellspacing = NULL,
        .border = NULL, .bgcolor = NULL,
        .width = NULL, .height = NULL,
        .row_count = atoi( row_count->value[0] ),
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );

    // Generate links
    char *links = log_links( req, NULL );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-system-monitor.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

void log_clear( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "log", NULL, "clear" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "log_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "log_clear_header", vars );

    //
    // Form
    //
    struct NNCMS_LOG_CLEAR_FIELDS *fields = memdup_temp( req, &log_clear_fields, sizeof(log_clear_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->log_clear.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "log_clear", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *log_clear = main_variable_get( req, req->post_tree, "log_clear" );
    if( log_clear != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Get POST data
        form_post_data( req, (struct NNCMS_FIELD *) fields );
        
        // Validate
        bool result = true;//log_validate( req, fields );
        if( result == true )
        {
            if( fields->log_clear_db.value != NULL &&
                g_ascii_strcasecmp( fields->log_clear_db.value, "on" ) == 0 )
            {
                // Delete data from database
                struct NNCMS_ROW *result = database_steps( req, req->stmt_clear_log );
                if( result != &empty_row )
                {
                    log_print( req, LOG_ACTION, "Cleared database log rows" );
                    log_vdisplayf( req, LOG_ACTION, "log_clear_db_success", vars );
                }
                else
                {
                    log_print( req, LOG_ERROR, "Unable to clear database log rows" );
                    log_vdisplayf( req, LOG_ERROR, "log_clear_db_fail", vars );
                }
            }

            if( fields->log_clear_file.value != NULL &&
                g_ascii_strcasecmp( fields->log_clear_file.value, "on" ) == 0 )
            {
                // Delete data from file
                int result = remove( szLogFile );
                if( result == 0 )
                {
                    log_print( req, LOG_ACTION, "Removed log file" );
                    log_vdisplayf( req, LOG_ACTION, "log_clear_file_success", vars );
                }
                else
                {
                    log_printf( req, LOG_ERROR, "Unable to remove log file: %s", g_strerror( errno ) );
                    log_vdisplayf( req, LOG_ERROR, "log_clear_file_fail", vars );
                }
            }

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = log_links( req, NULL );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-system-monitor.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );


    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

char *log_links( struct NNCMS_THREAD_INFO *req, char *log_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "log_id", .value.string = log_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Create array for links
    
    struct NNCMS_LINK link =
    {
        .function = NULL,
        .title = NULL,
        .vars = NULL
    };
    
    GArray *links = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_LINK) );

    // Fill the link array with links

    link.function = "log_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    link.function = "log_clear";
    link.title = i18n_string_temp( req, "clear_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( log_id != NULL )
    {
        link.function = "log_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );
    }

    // Convert arrays to HTML code
    char *html = template_links( req, (struct NNCMS_LINK *) links->data );
    garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GFREE );

    // Free array
    g_array_free( links, TRUE );
    
    return html;
}

// #############################################################################
