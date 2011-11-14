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

// #################################################################################
// global variables
//

// Path to log file
char szLogFile[NNCMS_PATH_LEN_MAX] = "./files/logs/nncms.log";

// Data about priorities
struct NNCMS_PRIORITY_INFO piData[] =
    {
        {
            /* nPriority */ LOG_DEBUG,
            /* lpszPriorityANSI */ TERM_FG_WHITE "Debug" TERM_RESET,
            /* lpszPriority */ "Debug",
            /* lpszIcon */ "images/status/code/stock_macro-watch-variable.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#E0E0E0"
        },
        {
            /* nPriority */ LOG_INFO,
            /* lpszPriorityANSI */ TERM_FG_CYAN "Info" TERM_RESET,
            /* lpszPriority */ "Info",
            /* lpszIcon */ "images/status/dialog-information.png",
            /* lpszHtmlFgColor */ "#505070",
            /* lpszHtmlBgColor */ "#E0E0F0"
        },
        {
            /* nPriority */ LOG_ACTION,
            /* lpszPriorityANSI */ TERM_FG_GREEN "Action" TERM_RESET,
            /* lpszPriority */ "Action",
            /* lpszIcon */ "images/status/code/stock_script.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#C0F0C0"
        },
        {
            /* nPriority */ LOG_NOTICE,
            /* lpszPriorityANSI */ TERM_FG_RED "Notice" TERM_RESET,
            /* lpszPriority */ "Notice",
            /* lpszIcon */ "images/stock/stock_notes.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#C0F0C0"
        },
        {
            /* nPriority */ LOG_WARNING,
            /* lpszPriorityANSI */ TERM_FG_YELLOW "Warning" TERM_RESET,
            /* lpszPriority */ "Warning",
            /* lpszIcon */ "images/status/dialog-warning.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#F0F0C0"
        },
        {
            /* nPriority */ LOG_ERROR,
            /* lpszPriorityANSI */ TERM_FG_RED "Error" TERM_RESET,
            /* lpszPriority */ "Error",
            /* lpszIcon */ "images/status/dialog-error.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#F0C0C0"
        },
        {
            /* nPriority */ LOG_CRITICAL,
            /* lpszPriorityANSI */ TERM_BRIGHT TERM_FG_RED "Critical" TERM_RESET,
            /* lpszPriority */ "Critical",
            /* lpszIcon */ "images/status/dialog-error.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#F05050"
        },
        {
            /* nPriority */ LOG_ALERT,
            /* lpszPriorityANSI */ TERM_BRIGHT TERM_UNDERSCORE TERM_FG_RED "Alert" TERM_RESET,
            /* lpszPriority */ "Alert",
            /* lpszIcon */ "images/status/dialog-error.png",
            /* lpszHtmlFgColor */ "#000000",
            /* lpszHtmlBgColor */ "#F02020"
        },
        {
            /* nPriority */ LOG_EMERGENCY,
            /* lpszPriorityANSI */ TERM_BRIGHT TERM_UNDERSCORE TERM_BLINK TERM_FG_RED "Emergency" TERM_RESET,
            /* lpszPriority */ "Emergency",
            /* lpszIcon */ "images/status/dialog-error.png",
            /* lpszHtmlFgColor */ "#FFFFFF",
            /* lpszHtmlBgColor */ "#F00000"
        },
        {
            /* nPriority */ LOG_UNKNOWN,
            /* lpszPriorityANSI */ TERM_BRIGHT TERM_FG_MAGENTA "Unknown" TERM_RESET,
            /* lpszPriority */ "Unknown",
            /* lpszIcon */ "images/mimetypes/empty.png",
            /* lpszHtmlFgColor */ "#FFFFFF",
            /* lpszHtmlBgColor */ "#000000"
        },
        {
            /* nPriority */ 0,
            /* lpszPriorityANSI */ 0,
            /* lpszPriority */ 0,
            /* lpszIcon */ 0,
            /* lpszHtmlFgColor */ 0,
            /* lpszHtmlBgColor */ 0
        }
    };

// File
FILE *pFile = 0;

// Log module status
static bool bLogInit = false;

// Log file mutex
pthread_mutex_t log_mutex;

// #############################################################################
// functions

bool log_init( struct NNCMS_THREAD_INFO *req )
{
    // Initialize cache if needed
    if( bLogInit == false )
    {
        bLogInit = true;

        // Open file log file
        pFile = fopen( szLogFile, "a" );
        if( pFile == 0 )
            log_printf( 0, LOG_ERROR, "Log::Printf: Could not open log file %s", szLogFile );

        // Init pthread locking
        if( pthread_mutex_init( &log_mutex, 0 ) != 0 ) {
            log_printf( 0, LOG_EMERGENCY, "Log::Init: Mutex init failed!!" );
            exit( -1 );
        }

        // Test logging system
        /*log_printf( 0, LOG_UNKNOWN, "testing" );
        log_printf( 0, LOG_INFO, "testing" );
        log_printf( 0, LOG_NOTICE, "testing" );
        log_printf( 0, LOG_WARNING, "testing" );
        log_printf( 0, LOG_ERROR, "testing" );
        log_printf( 0, LOG_CRITICAL, "testing" );
        log_printf( 0, LOG_ALERT, "testing" );
        log_printf( 0, LOG_EMERGENCY, "testing" );//*/
    }

    // Prepared statements
    req->stmtListLogs = database_prepare( "SELECT `id`, `priority`, `text`, `timestamp`, `user_id`, `host`, `uri`, `referer`, `user-agent` from `log` ORDER BY `id` DESC LIMIT 50" );
    req->stmtViewLog = database_prepare( "SELECT `id`, `priority`, `text`, `timestamp`, `user_id`, `host`, `uri`, `referer`, `user-agent` from `log` WHERE `id`=?" );
    req->stmtUser = database_prepare( "SELECT `name`, `nick` FROM `users` WHERE `id`=?" );
    req->stmtAddLogReq = database_prepare( "INSERT INTO `log` VALUES(null, ?, ?, ?, ?, ? || ':' || ?, ?, ?, ?, ?, ?)" );
    req->stmtAddLog = database_prepare( "INSERT INTO `log` VALUES(null, ?, ?, ?, null, null, null, null, null, null, null)" );
    req->stmtClearLog = database_prepare( "DELETE FROM `log`" );

    return true;
}

// #############################################################################

bool log_deinit( struct NNCMS_THREAD_INFO *req )
{
    if( bLogInit == true )
    {
        bLogInit = false;

        // Close log file
        fclose( pFile );
        pFile = 0;

        // Deinit pthread locking
        if( pthread_mutex_destroy( &log_mutex ) != 0 ) {
            log_printf( 0, LOG_EMERGENCY, "Log:Deinit: Mutex destroy failed!!" );
        }
    }

    // Free prepared statements
    database_finalize( req->stmtListLogs );
    database_finalize( req->stmtViewLog );
    database_finalize( req->stmtUser );
    database_finalize( req->stmtAddLogReq );
    database_finalize( req->stmtAddLog );
    database_finalize( req->stmtClearLog );

    return true;
}

// #############################################################################

struct NNCMS_PRIORITY_INFO *log_get_priority( enum NNCMS_PRIORITY nPriority )
{
    for( unsigned int i = 0; i < sizeof(piData); i++ )
    {
        if( piData[i].nPriority == nPriority )
            return &piData[i];
    }
    return log_get_priority( LOG_UNKNOWN );
}

// #################################################################################

void log_printf( struct NNCMS_THREAD_INFO *req, enum NNCMS_PRIORITY nPriority, char *lpszFormat, ... )
{
    va_list vArgs;

    // This should hold sqlite error message
    char *zErrMsg = 0;

    // Log text buffer
    char szLogText[NNCMS_MAX_LOG_TEXT];

    // Buffer for full log text18
    char szText[NNCMS_MAX_LOG_TEXT + 64];

    va_start( vArgs, lpszFormat );
    vsnprintf( szLogText, NNCMS_MAX_LOG_TEXT, lpszFormat, vArgs );
    va_end( vArgs );

    // Get priority text
    struct NNCMS_PRIORITY_INFO *priInfo = log_get_priority( nPriority );

    // Get Time string
    char szTime[64];
    time_t rawtime;
    time_t curTime = time( &rawtime );
    struct tm *timeinfo = localtime( &rawtime );
    strftime( szTime, 64, szTimestampFormat, timeinfo );

    // Print to console
    snprintf( szText, sizeof(szText), "%s [%-9s] %s\n", szTime, priInfo->lpszPriorityANSI, szLogText );
    fputs( szText, stdout ); // Don't use puts( ); here, it appends \n at the end of line

#ifndef DEBUG
    // Do not add debug info into file log
    if( nPriority == LOG_DEBUG ) return;
#endif // DEBUG

    // Write text to file
    if( pFile != 0 )
    {
        if( pthread_mutex_lock( &log_mutex ) != 0 )
        {
            log_printf( req, LOG_EMERGENCY, "Log::Printf: Can't acquire lock!!" );
            exit( -1 );
        }
        fputs( szText, pFile );
        pthread_mutex_unlock( &log_mutex );
    }

    // Do not add debug info into sqlite database
    if( nPriority == LOG_DEBUG ) return;

    // Write to sqlite database
    filter_table_replace( szLogText, (unsigned int) strlen( szLogText ), queryFilter );
    if( req != 0 )
    {
        user_check_session( req );

        database_bind_int( req->stmtAddLogReq, 1, nPriority );       // Priority
        database_bind_text( req->stmtAddLogReq, 2, szLogText );      // Text
        database_bind_int( req->stmtAddLogReq, 3, curTime );         // Timestamp
        database_bind_text( req->stmtAddLogReq, 4, req->g_userid );  // User ID
        database_bind_text( req->stmtAddLogReq, 5, FCGX_GetParam( "REMOTE_ADDR", req->fcgi_request->envp ) );  // Host
        database_bind_text( req->stmtAddLogReq, 6, FCGX_GetParam( "REMOTE_PORT", req->fcgi_request->envp ) );  // Host
        database_bind_text( req->stmtAddLogReq, 7, FCGX_GetParam( "REQUEST_URI", req->fcgi_request->envp ) );  // URI
        database_bind_text( req->stmtAddLogReq, 8, FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) );  // Referer
        database_bind_text( req->stmtAddLogReq, 9, FCGX_GetParam( "HTTP_USER_AGENT", req->fcgi_request->envp ) );  // User-Agent
        database_bind_text( req->stmtAddLogReq, 10, __FILE__ );      // Source File
        database_bind_int( req->stmtAddLogReq, 11, __LINE__ );       // Source Line
        database_steps( req->stmtAddLogReq );
    }
    /*else
    {
        database_bind_int( req->stmtAddLog, 1, nPriority );       // Priority
        database_bind_text( req->stmtAddLog, 2, szLogText );      // Text
        database_bind_int( req->stmtAddLog, 3, curTime );         // Timestamp
        database_steps( req->stmtAddLog );
    }*/

    return;
}

// #################################################################################

void log_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Log view";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/utilities-system-monitor.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS logTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "log_id", /* szValue */ 0 },
            { /* szName */ "log_priority", /* szValue */ 0 },
            { /* szName */ "log_text", /* szValue */ 0 },
            { /* szName */ "log_timestamp", /* szValue */ 0 },
            { /* szName */ "log_user_id", /* szValue */ 0 },
            { /* szName */ "log_user_name", /* szValue */ 0 },
            { /* szName */ "log_user_nick", /* szValue */ 0 },
            { /* szName */ "log_user_host", /* szValue */ 0 },
            { /* szName */ "log_uri", /* szValue */ 0 },
            { /* szName */ "log_fgcolor", /* szValue */ 0 },
            { /* szName */ "log_bgcolor", /* szValue */ 0 },
            { /* szName */ "log_icon", /* szValue */ 0 },
            { /* szName */ "log_referer", /* szValue */ 0 },
            { /* szName */ "log_useragent", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS footTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
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

    // Check session
    user_check_session( req );

    // First we check what permissions do user have
    if( acl_check_perm( req, "log", req->g_username, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Log::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get log entry id
    struct NNCMS_ROW *logRow = 0;
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "log_id" );
    if( httpVarId == 0 )
    {
        // Find rows
        logRow = database_steps( req->stmtListLogs );

        // Load Log header
        template_iparse( req, TEMPLATE_LOG_VIEW_HEAD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
        smart_cat( &smartBuffer, req->lpszFrame );
    }
    else
    {
        // Find row associated with our object by 'id'
        database_bind_text( req->stmtViewLog, 1, httpVarId->lpszValue );
        logRow = database_steps( req->stmtViewLog );
    }

    // Is log empty ?
    if( logRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No log data";
        log_printf( req, LOG_ERROR, "Log::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Loop thru all rows
    struct NNCMS_ROW *curRow = logRow;
    while( 1 )
    {
        // Fill template values
        enum NNCMS_PRIORITY nPriority = atoi( curRow->szColValue[1] );
        struct NNCMS_PRIORITY_INFO *priInfo = log_get_priority( nPriority );
        logTemplate[1].szValue = curRow->szColValue[0];
        logTemplate[2].szValue = priInfo->lpszPriority;
        logTemplate[3].szValue = curRow->szColValue[2];

        // Get time string
        char szTime[64];
        time_t rawTime = atoi( curRow->szColValue[3] );
        struct tm *timeInfo = localtime( &rawTime );
        strftime( szTime, 64, szTimestampFormat, timeInfo );
        logTemplate[4].szValue = szTime;

        // Get user
        database_bind_text( req->stmtUser, 1, curRow->szColValue[4] ); // id
        struct NNCMS_ROW *userRow = database_steps( req->stmtUser );
        if( userRow != 0 )
        {
            logTemplate[5].szValue = curRow->szColValue[4];
            logTemplate[6].szValue = userRow->szColValue[0];
            logTemplate[7].szValue = userRow->szColValue[1];
        }
        else
        {
            logTemplate[5].szValue = "0";
            logTemplate[6].szValue = "guest";
            logTemplate[7].szValue = "Anonymous";
        }

        // Host and URI
        logTemplate[8].szValue = curRow->szColValue[5];
        logTemplate[9].szValue = curRow->szColValue[6];

        // Other fancy stuff
        logTemplate[10].szValue = priInfo->lpszHtmlFgColor;
        logTemplate[11].szValue = priInfo->lpszHtmlBgColor;
        logTemplate[12].szValue = priInfo->lpszIcon;
        logTemplate[13].szValue = curRow->szColValue[7];
        logTemplate[14].szValue = curRow->szColValue[8];

        // Use different template for one log entry and list of log entries
        if( httpVarId == 0 )
            // Parse one row
            template_iparse( req, TEMPLATE_LOG_VIEW_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, logTemplate );
        else
            template_iparse( req, TEMPLATE_LOG_VIEW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, logTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );

        // Free rows after parsing the template
        database_freeRows( userRow );

        // Select next row
        curRow = curRow->next;
        if( curRow == 0 )
            break;
    }

    // Free memory from query result
    database_freeRows( logRow );

    // Add footer if no Id specified
    if( httpVarId == 0 )
    {
        template_iparse( req, TEMPLATE_LOG_VIEW_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, footTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );
    }

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #################################################################################

void log_clear( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Clear log";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/utilities-system-monitor.png" },
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
    if( acl_check_perm( req, "log", req->g_username, "clear" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Log::Clear: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user pushed big gray shiny "Clear" button
    //
    struct NNCMS_VARIABLE *httpVarClear = main_get_variable( req, "log_clear" );
    if( httpVarClear != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        struct NNCMS_VARIABLE *httpVarClearDb = main_get_variable( req, "log_clear_db" );
        struct NNCMS_VARIABLE *httpVarClearFile = main_get_variable( req, "log_clear_file" );

        // Buffer for logging
        char szResult[16]; *szResult = 0;

        if( httpVarClearDb != 0 && strcasecmp( httpVarClearDb->lpszValue, "on" ) == 0 )
        {
            // Delete data from database
            strcat( szResult, " DB" );
            database_steps( req->stmtClearLog );
        }

        if( httpVarClearFile != 0 && strcasecmp( httpVarClearFile->lpszValue, "on" ) == 0 )
        {
            // Delete data from file
            strcat( szResult, " File" );
            remove( szLogFile );
        }

        // Log results
        if( *szResult != 0 )
            log_printf( req, LOG_INFO, "Log::Clear: Log cleared:%s", szResult );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Load rename page
    template_iparse( req, TEMPLATE_LOG_CLEAR, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, logTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #################################################################################
