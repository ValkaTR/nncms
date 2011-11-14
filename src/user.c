// #############################################################################
// Source file: user.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <time.h>

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "threadinfo.h"
#include "user.h"
#include "filter.h"
#include "database.h"
#include "template.h"
#include "log.h"
#include "acl.h"
#include "smart.h"

#include "strlcpy.h"
#include "strlcat.h"

// #############################################################################
// global variables
//

// How often check and remove old sessions
int tSessionExpires = 2 * 60;
int tCheckSessionInterval = 1 * 60;

// #############################################################################
// functions

bool user_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmtFindUserByName = database_prepare( "SELECT `id`, `name` FROM `users` WHERE `name`=?" );
    req->stmtFindUserById = database_prepare( "SELECT `id`, `name`, `nick`, `group` FROM `users` WHERE `id`=?" );
    req->stmtListUsers = database_prepare( "SELECT `id`, `name`, `nick`, `group` FROM `users`" );
    req->stmtAddUser = database_prepare( "INSERT INTO `users` VALUES(null, ?, ?, '@user', ?)" );
    req->stmtLoginUserByName = database_prepare( "SELECT `id` FROM `users` WHERE `name`=? AND `hash`=? LIMIT 1" );
    req->stmtLoginUserById = database_prepare( "SELECT `name` FROM `users` WHERE `id`=? AND `hash`=? LIMIT 1" );
    req->stmtFindSession = database_prepare( "SELECT `user_id`, `user_name`, `user_agent`, `remote_addr` FROM `sessions` WHERE `id`=?" );
    req->stmtAddSession = database_prepare( "INSERT INTO `sessions` VALUES(?, ?, ?, ?, ?, ?)" );
    req->stmtDeleteSession = database_prepare( "DELETE FROM `sessions` WHERE `id`=?" );
    req->stmtCleanSessions = database_prepare( "DELETE FROM `sessions` WHERE `timestamp` < ?" );
    req->stmtListSessions = database_prepare( "SELECT `id`, `user_id`, `user_name`, `timestamp`, `user_agent`, `remote_addr` FROM `sessions` ORDER BY `timestamp` DESC" );
    req->stmtEditUserHash = database_prepare( "UPDATE `users` SET `hash`=? WHERE `id`=?" );
    req->stmtEditUserNick = database_prepare( "UPDATE `users` SET `nick`=? WHERE `id`=?" );
    req->stmtEditUserGroup = database_prepare( "UPDATE `users` SET `group`=? WHERE `id`=?" );
    req->stmtDeleteUser = database_prepare( "DELETE FROM `users` WHERE `id`=?" );

    return true;
}

// #############################################################################

bool user_deinit( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req->stmtFindUserByName );
    database_finalize( req->stmtFindUserById );
    database_finalize( req->stmtListUsers );
    database_finalize( req->stmtAddUser );
    database_finalize( req->stmtLoginUserByName );
    database_finalize( req->stmtLoginUserById );
    database_finalize( req->stmtFindSession );
    database_finalize( req->stmtAddSession );
    database_finalize( req->stmtDeleteSession );
    database_finalize( req->stmtCleanSessions );
    database_finalize( req->stmtListSessions );
    database_finalize( req->stmtEditUserHash );
    database_finalize( req->stmtEditUserNick );
    database_finalize( req->stmtEditUserGroup );
    database_finalize( req->stmtDeleteUser );

    return true;
}

// #############################################################################

// Registration page
void user_register( struct NNCMS_THREAD_INFO *req )
{
    // Page caption
    char *szHeader = "User registration";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/system-users.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS registerTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    registerTemplate[2].szValue = req->g_sessionid;

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", req->g_username, "register" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "User::Register: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check if submit button was pressed
    struct NNCMS_VARIABLE *httpVarReg = main_get_variable( req, "user_register" );
    if( httpVarReg != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        //
        // Obtain the data
        //
        struct NNCMS_VARIABLE *username = main_get_variable( req, "user_name" );
        struct NNCMS_VARIABLE *nickname = main_get_variable( req, "user_nick" );
        struct NNCMS_VARIABLE *password = main_get_variable( req, "user_password" );
        struct NNCMS_VARIABLE *password_retry = main_get_variable( req, "user_password_retry" );

        //
        // Check if all forms were filled
        //
        if( username == 0 || nickname == 0 ||
            password == 0 || password_retry == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_ERROR, "User::Register: %s", frameTemplate[1].szValue );
            goto output;
        }
        if( *username->lpszValue == 0 || *nickname->lpszValue == 0 ||
            *password->lpszValue == 0 || *password_retry->lpszValue == 0 )
        {
            // Tell user that he is <s>stupid</s> not paying attention
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "You were not registered, fill all forms";
            log_printf( req, LOG_INFO, "User::Register: %s", frameTemplate[1].szValue );
            goto output;
        }

        //
        // Here we have EVIL data, when it's come from user it is always EVIL!
        // We should filter it, so EVIL data from user won't crash or
        // exploit our glamourous content managment system. First, trimm the data,
        // we don't want unlimited size data.
        //
        filter_truncate_string( username->lpszValue, NNCMS_USERNAME_LEN );
        filter_truncate_string( nickname->lpszValue, NNCMS_NICKNAME_LEN );
        filter_truncate_string( password->lpszValue, NNCMS_PASSWORD_LEN );
        filter_truncate_string( password_retry->lpszValue, NNCMS_PASSWORD_LEN );

        //
        // Now use table replace for this.
        //
        filter_table_replace( username->lpszValue, (unsigned int) strlen( username->lpszValue ), usernameFilter_cp1251 );
        filter_table_replace( nickname->lpszValue, (unsigned int) strlen( nickname->lpszValue ), nicknameFilter_cp1251 );
        filter_table_replace( password->lpszValue, (unsigned int) strlen( password->lpszValue ), passwordFilter_cp1251 );
        filter_table_replace( password_retry->lpszValue, (unsigned int) strlen( password_retry->lpszValue ), passwordFilter_cp1251 );

        //
        // Compare passwords
        //
        if( strcmp( password->lpszValue, password_retry->lpszValue ) != 0 )
        {
            // Passwords are different
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Passwords doesn't match";
            log_printf( req, LOG_ERROR, "User::Register: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Generate a hash value for password
        unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
        unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
        sha512( password->lpszValue, strlen( (char *) password->lpszValue ), digest );
        hash[2 * SHA512_DIGEST_SIZE] = '\0';
        for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
            sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

        //
        // Almost there, check if username exists in database
        //
        database_bind_text( req->stmtFindUserByName, 1, username->lpszValue );
        struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserByName );
        if( userRow != 0 )
        {
            // If user with same user name was found, then disallow registration
            database_freeRows( userRow );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "You were not registered. Selected username is already in use.";
            log_printf( req, LOG_NOTICE, "User::Register: %s", frameTemplate[1].szValue );
            goto output;
        }

        //
        // Add user in database
        //
        database_bind_text( req->stmtAddUser, 1, username->lpszValue );
        database_bind_text( req->stmtAddUser, 2, nickname->lpszValue );
        database_bind_text( req->stmtAddUser, 3, hash );
        database_steps( req->stmtAddUser );

        // Log action
        log_printf( req, LOG_ACTION, "User::Register: User \"%s\" registered", username->lpszValue );

        // Tell user that he was successfully registered
        frameTemplate[0].szValue = "Registered";
        frameTemplate[1].szValue = "You were successfully registered, now u can login on <a href=\"/\">main page</a>.";
        goto output;
    }

    // Load register page
    template_iparse( req, TEMPLATE_USER_REGISTER, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, registerTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );

    return;
}

// #############################################################################

// Login page
void user_login( struct NNCMS_THREAD_INFO *req )
{
    // Page caption
    char *szHeader = "Login";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/status/dialog-password.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS loginTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "fkey", /* szValue */ req->g_sessionid },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Check if submit button was pressed
    struct NNCMS_VARIABLE *httpVarLogin = main_get_variable( req, "user_login" );
    if( httpVarLogin == 0 )
    {
        // Load login page
        template_iparse( req, TEMPLATE_USER_LOGIN, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, loginTemplate );
        //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );
        goto output;
    }

    //
    // Obtain the data
    //
    struct NNCMS_VARIABLE *httpVarUserName = main_get_variable( req, "user_name" );
    struct NNCMS_VARIABLE *httpVarPassword = main_get_variable( req, "user_password" );

    //
    // Check if all forms were filled
    //
    if( httpVarUserName == 0 || httpVarPassword == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data.";
        log_printf( req, LOG_ERROR, "User::Login: %s", frameTemplate[1].szValue );
        goto output;
    }
    if( httpVarUserName->lpszValue[0] == '\0' || httpVarPassword->lpszValue[0] == '\0' )
    {
        // User didnt filled all forms
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "You were not logged, because you are did'nt filled all forms.";
        log_printf( req, LOG_INFO, "User::Login: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter data exactly the same way as in register page
    // unfortunately same code duplicating occurs here
    //
    filter_truncate_string( httpVarUserName->lpszValue, NNCMS_USERNAME_LEN );
    filter_truncate_string( httpVarPassword->lpszValue, NNCMS_PASSWORD_LEN );
    filter_table_replace( httpVarUserName->lpszValue, (unsigned int) strlen( httpVarUserName->lpszValue ), usernameFilter_cp1251 );
    filter_table_replace( httpVarPassword->lpszValue, (unsigned int) strlen( httpVarPassword->lpszValue ), queryFilter );

    // Generate a hash value for password
    unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
    unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
    sha512( httpVarPassword->lpszValue, strlen( (char *) httpVarPassword->lpszValue ), digest );
    hash[2 * SHA512_DIGEST_SIZE] = '\0';
    for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
    {
        sprintf( (char *) hash + (2 * i), "%02x", digest[i] );
    }

    //
    // Find user in database
    //
    database_bind_text( req->stmtLoginUserByName, 1, httpVarUserName->lpszValue );
    database_bind_text( req->stmtLoginUserByName, 2, hash );
    struct NNCMS_ROW *userRow = database_steps( req->stmtLoginUserByName );
    if( userRow == 0 )
    {
        // Wrong password or user name, we won't tell which one :p
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Wrong password or user name.";
        log_printf( req, LOG_ALERT, "User::Login: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Login approved, here we start login operation
    // we open a session with random id, save user id in that
    // session, then tell to user-agent to remember session id in cookies
    //
    char session_id[NNCMS_SESSION_ID_LEN + 1];
    static char session_charmap[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
        'z', 'x', 'c', 'v', 'b', 'n', 'm',
        'Z', 'X', 'C', 'V', 'B', 'N', 'M'
    };

login_regenerate_session_id:

    // Generate and check session id
    for( int i = 0; i < NNCMS_SESSION_ID_LEN; i++ )
    {
        session_id[i] = session_charmap[(unsigned int) rand( ) % sizeof(session_charmap)];
        session_id[i + 1] = 0;
    }

    // Check if session number is already in use.
    // There's (sessions_in_use / (sizeof(session_charmap)^NNCMS_SESSION_ID_LEN))
    // chance that randomly generated session will be the same one as already
    // in use. (About [3,74 * 10^(-228)]%, that is very low chance, but we can not say
    // that there is no chance at all)
    database_bind_text( req->stmtFindSession, 1, session_id );
    struct NNCMS_ROW *sessionRow = database_steps( req->stmtFindSession );
    if( sessionRow != 0 )
    {
        database_freeRows( sessionRow );
        log_printf( req, LOG_CRITICAL, "User::Login: Regenerating session" );
        goto login_regenerate_session_id;
    }

    //
    // Fill selected session id with some data
    //
    database_bind_text( req->stmtAddSession, 1, session_id );
    database_bind_text( req->stmtAddSession, 2, userRow->szColValue[0] );
    database_bind_text( req->stmtAddSession, 3, httpVarUserName->lpszValue );
    database_bind_text( req->stmtAddSession, 4, req->g_useragent );
    database_bind_text( req->stmtAddSession, 5, req->g_useraddr );
    database_bind_int( req->stmtAddSession, 6, time( 0 ) );
    database_steps( req->stmtAddSession );

    // User row is not needed anymore
    database_freeRows( userRow );

    // Log action
    log_printf( req, LOG_ACTION, "User::Login: User \"%s\" logged in", httpVarUserName->lpszValue );

    // Set Cookie and Redirect user to homepage
    main_set_cookie( req, "session_id", session_id );
    redirect_to_referer( req );
    return;

    //
    // Here might be your advert/banner/spam/weblink and etc. for
    // crackers who is watching this part of code in order to crack, exploit
    // the application.
    // 1 line = $1.00
    // Call: +372 555 64 327 (Estonia)
    // Email: valka@sonic-world.ru
    //

    //
    // Login Output
    //

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );

    return;
}

// #############################################################################

// This function verifies data in cookies with database
// and fills some global variables with neccessary login information
void user_check_session( struct NNCMS_THREAD_INFO *req )
{
    if( req->g_bSessionChecked == true ) return;

    // Before anything - reset the global data to guest account, so
    // if something fails down there, then the guest account will be used
    // instead of last logged in person
    user_reset_session( req );
    req->g_bSessionChecked = true;

    //
    // Get session_id from cookies
    //
    struct NNCMS_VARIABLE *session_id = main_get_variable( req, "session_id" );
    if( session_id == 0 ) return;
    filter_truncate_string( session_id->lpszValue, 256 );
    filter_table_replace( session_id->lpszValue, (unsigned int) strlen( session_id->lpszValue ), usernameFilter_cp1251 );

    // Find session in db
    database_bind_text( req->stmtFindSession, 1, session_id->lpszValue );
    struct NNCMS_ROW *sessionRow = database_steps( req->stmtFindSession );
    if( sessionRow == 0 ) return; // No sesssion found, assume guest account

    // Check for cookie stealing
    if( strcmp( req->g_useragent, sessionRow->szColValue[2] ) != 0 ||
        strcmp( req->g_useraddr, sessionRow->szColValue[3] ) != 0 )
    {
        log_printf( req, LOG_WARNING, "User::CheckSession: Cookie stealing detected" );
        return;
    }

    // Session found, copy data from it and free row data
    strlcpy( req->g_userid, sessionRow->szColValue[0], sizeof(req->g_userid) );
    strlcpy( req->g_username, sessionRow->szColValue[1], sizeof(req->g_username) );
    req->g_isGuest = false;
    req->g_sessionid = session_id->lpszValue;
    database_freeRows( sessionRow );
}

// ############################################################################

bool user_xsrf( struct NNCMS_THREAD_INFO *req )
{
    //
    // Get session_id from cookies
    //
    struct NNCMS_VARIABLE *session_id = main_get_variable( req, "session_id" );
    if( session_id == 0 ) goto xsrf_attack;
    filter_truncate_string( session_id->lpszValue, 256 );
    filter_table_replace( session_id->lpszValue, (unsigned int) strlen( session_id->lpszValue ), usernameFilter_cp1251 );

    // Get POST data
    struct NNCMS_VARIABLE *httpVarFKey = main_get_variable( req, "fkey" );
    if( httpVarFKey == 0 ) goto xsrf_attack;
    filter_truncate_string( httpVarFKey->lpszValue, 256 );
    filter_table_replace( httpVarFKey->lpszValue, (unsigned int) strlen( httpVarFKey->lpszValue ), queryFilter );

    // Compare
    if( strcmp( session_id->lpszValue, httpVarFKey->lpszValue ) == 0 )
    {
        return true;
    }

xsrf_attack:
    log_printf( req, LOG_ALERT, "User::Xsrf: XSRF attack detected" );

    return false;
}

// ############################################################################

void user_logout( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Logout";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ 0 },
            { /* szName */ "icon",  /* szValue */ "images/status/dialog-password.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    //
    // Get session_id from cookies
    //
    struct NNCMS_VARIABLE *session_id = main_get_variable( req, "session_id" );
    if( session_id != 0 )
    {
        // Remove unused session from db
        filter_truncate_string( session_id->lpszValue, 256 );
        filter_table_replace( session_id->lpszValue, (unsigned int) strlen( session_id->lpszValue ), usernameFilter_cp1251 );

        // Log action
        user_check_session( req );
        log_printf( req, LOG_ACTION, "User::Logout: User \"%s\" logged out", req->g_username );

        if( *session_id->lpszValue != 0 )
        {
            database_bind_text( req->stmtDeleteSession, 1, session_id->lpszValue );
            database_steps( req->stmtDeleteSession );
        }
    }

    // Reset cookie and redirect to homeURL
    frameTemplate[1].szValue = "Logged out";
    main_set_cookie( req, "session_id", "0" );
    redirectf( req, "%s/index.fcgi", homeURL );
    return;

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// ############################################################################

time_t session_timer = 0;

// Reset session to initial status
void user_reset_session( struct NNCMS_THREAD_INFO *req )
{
    // Start a timer to check sessions
    if( session_timer == 0 )
        session_timer = time( 0 ) + 10;

    // Check timer time out
    time_t current_time = time( 0 );
    if( current_time >= session_timer )
    {
        // Update timer
        session_timer = current_time + tCheckSessionInterval;

        // Remove old sessions
        database_bind_int( req->stmtCleanSessions, 1, current_time - tSessionExpires );
        database_steps( req->stmtCleanSessions );
    }

    // Reset current session to "guest" account
    memset( req->g_userid, 0, sizeof(req->g_userid) );
    memset( req->g_username, 0, sizeof(req->g_username) );
    strcpy( req->g_userid, "1" );
    strcpy( req->g_username, "guest" );
    req->g_isGuest = true;
    req->g_sessionid = 0;
    req->g_bSessionChecked = false;
}

// ############################################################################

// This function will give one of two tags, for logged in users or for guests
char *user_block( struct NNCMS_THREAD_INFO *req )
{
    // Block title
    char *szBlockTitle;

    // Find out if user is logged in or not
    user_check_session( req );
    if( req->g_isGuest == true )
    {
        // Specify values for template
        struct NNCMS_TEMPLATE_TAGS loginTags[] =
            {
                { /* szName */ "homeURL", /* szValue */ homeURL },
                { /* szName */ "referer", /* szValue */ FCGX_GetParam( "REQUEST_URI", req->fcgi_request->envp ) },
                { /* szName */ 0, /* szValue */0 } // Terminating row
            };

        szBlockTitle = "Login";

        // Generate the block
        template_iparse( req, TEMPLATE_USER_LOGIN, req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX, loginTags );
    }
    else
    {
        // Specify values for template
        struct NNCMS_TEMPLATE_TAGS userTags[] =
            {
                { /* szName */ "homeURL", /* szValue */ homeURL },
                { /* szName */ "user_name", /* szValue */ req->g_username },
                { /* szName */ "user_id",  /* szValue */ req->g_userid },
                { /* szName */ 0, /* szValue */0 } // Terminating row
            };

        szBlockTitle = "User info";

        // Generate the block
        template_iparse( req, TEMPLATE_USER_BLOCK, req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX, userTags );
    }

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS blockTags[] =
        {
            { /* szName */ "header", /* szValue */ szBlockTitle },
            { /* szName */ "content",  /* szValue */ req->lpszBlockTemp },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Generate the block
    template_iparse( req, TEMPLATE_BLOCK, req->lpszBlockContent, NNCMS_BLOCK_LEN_MAX, blockTags );

    return req->lpszBlockContent;
}

// ############################################################################

void user_sessions( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Session list";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/text/stock_list_enum.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS sessionTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "session_id", /* szValue */ 0 },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "session_timestamp", /* szValue */ 0 },
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
    if( acl_check_perm( req, "user", req->g_username, "view_sessions" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "User::Sessions: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Header template
    template_iparse( req, TEMPLATE_USER_SESSIONS_HEAD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
    smart_cat( &smartBuffer, req->lpszFrame );

    // Find rows associated with our object by 'name'
    struct NNCMS_ROW *sessionRow = database_steps( req->stmtListSessions );
    if( sessionRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No sessions data";
        log_printf( req, LOG_ERROR, "User::Sessions: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Loop thru all rows
    struct NNCMS_ROW *curRow = sessionRow;
    while( 1 )
    {
        // Fill template tags
        sessionTemplate[1].szValue = curRow->szColValue[0];
        sessionTemplate[2].szValue = curRow->szColValue[1];
        sessionTemplate[3].szValue = curRow->szColValue[2];

        // Get time string
        char szTime[64];
        time_t rawTime = atoi( curRow->szColValue[3] );
        struct tm *timeInfo = localtime( &rawTime );
        strftime( szTime, 64, szTimestampFormat, timeInfo );
        sessionTemplate[4].szValue = szTime;

        // Parse one row
        template_iparse( req, TEMPLATE_USER_SESSIONS_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, sessionTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );

        // Select next row
        curRow = curRow->next;
        if( curRow == 0 )
            break;
    }

    // Free memory from query result
    database_freeRows( sessionRow );

    // Footer template
    template_iparse( req, TEMPLATE_USER_SESSIONS_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
    smart_cat( &smartBuffer, req->lpszFrame );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// ############################################################################

void user_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "View users";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/system-users.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS userTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "user_group", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS footerTemplate[] =
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
    if( acl_check_perm( req, "user", req->g_username, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "User::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get log entry id
    struct NNCMS_ROW *userRow = 0;
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "user_id" );
    if( httpVarId == 0 )
    {
        // List all users
        userRow = database_steps( req->stmtListUsers );

        // Load ACL header
        template_iparse( req, TEMPLATE_USER_VIEW_HEAD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
        smart_cat( &smartBuffer, req->lpszFrame );
    }
    else
    {
        // Change header and icon for individual user
        szHeader = "View user";
        frameTemplate[0].szValue = szHeader;
        frameTemplate[2].szValue = "images/apps/user-info.png";

        // Find user by id
        database_bind_int( req->stmtFindUserById, 1, atoi( httpVarId->lpszValue ) );
        userRow = database_steps( req->stmtFindUserById );
        *req->lpszBuffer = 0;
    }

    // Found users?
    if( userRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No users data";
        log_printf( req, LOG_ERROR, "User::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Loop thru all rows
    struct NNCMS_ROW *curRow = userRow;
    while( 1 )
    {
        // Fill template values
        userTemplate[1].szValue = curRow->szColValue[0];
        userTemplate[2].szValue = curRow->szColValue[1];
        userTemplate[3].szValue = curRow->szColValue[2];
        userTemplate[4].szValue = curRow->szColValue[3];

        // Use different template for one log entry and list of log entries
        if( httpVarId == 0 )
            // Parse one row
            template_iparse( req, TEMPLATE_USER_VIEW_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, userTemplate );
        else
            template_iparse( req, TEMPLATE_USER_VIEW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, userTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );

        // Select next row
        curRow = curRow->next;
        if( curRow == 0 )
            break;
    }

    // Free memory from query result
    database_freeRows( userRow );

    // Add footer if no Id specified
    if( httpVarId == 0 )
    {
        template_iparse( req, TEMPLATE_USER_VIEW_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, footerTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );
    }

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// ############################################################################

void user_profile( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "User profile";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/gtk-edit.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS userTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "user_group", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    userTemplate[6].szValue = req->g_sessionid;

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", req->g_username, "profile" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "User::Profile: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get log entry id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "user_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "User::Profile: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Find row associated with our object by 'name'
    unsigned int nUserId = atoi( httpVarId->lpszValue );
    database_bind_int( req->stmtFindUserById, 1, nUserId );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );
    if( userRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "User not found";
        log_printf( req, LOG_ERROR, "User::Profile: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Only own profile can be changed
    user_check_session( req );
    if( strcmp( req->g_username, userRow->szColValue[1] ) != 0 )
    {
        // But if user has 'edit' permission, it can change profile anyway
        if( acl_check_perm( req, "user", req->g_username, "edit" ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Can't change another's profile";
            log_printf( req, LOG_ERROR, "User::Profile: %s", frameTemplate[1].szValue );
            goto output;
        }
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "user_profile" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get new data
        struct NNCMS_VARIABLE *httpVarNick = main_get_variable( req, "user_nick" );
        if( httpVarNick == 0 )
        {
            database_freeRows( userRow );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_NOTICE, "User::Profile: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Passwords
        struct NNCMS_VARIABLE *httpVarPass = main_get_variable( req, "user_pass" );
        struct NNCMS_VARIABLE *httpVarNewPass = main_get_variable( req, "user_new_pass" );
        struct NNCMS_VARIABLE *httpVarNewPassRetry = main_get_variable( req, "user_new_pass_retry" );
        if( httpVarPass == 0 || httpVarNewPass == 0 || httpVarNewPassRetry == 0 )
            goto skip_pass_change;
        if( httpVarPass->lpszValue[0] == 0 || httpVarNewPass->lpszValue[0] == 0 || httpVarNewPassRetry->lpszValue[0] == 0 )
            goto skip_pass_change;

        filter_truncate_string( httpVarPass->lpszValue, NNCMS_PASSWORD_LEN );
        filter_truncate_string( httpVarNewPass->lpszValue, NNCMS_PASSWORD_LEN );
        filter_truncate_string( httpVarNewPassRetry->lpszValue, NNCMS_PASSWORD_LEN );
        filter_table_replace( httpVarPass->lpszValue, (unsigned int) strlen( httpVarPass->lpszValue ), queryFilter );
        filter_table_replace( httpVarNewPass->lpszValue, (unsigned int) strlen( httpVarNewPass->lpszValue ), queryFilter );
        filter_table_replace( httpVarNewPassRetry->lpszValue, (unsigned int) strlen( httpVarNewPassRetry->lpszValue ), queryFilter );

        //
        // Compare passwords
        //
        if( strcmp( httpVarNewPass->lpszValue, httpVarNewPassRetry->lpszValue ) != 0 )
        {
            // Passwords are different
            database_freeRows( userRow );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Passwords doesn't match";
            log_printf( req, LOG_ERROR, "User::Profile: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Generate a hash value for password
        unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
        unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
        sha512( httpVarPass->lpszValue, strlen( (char *) httpVarPass->lpszValue ), digest );
        hash[2 * SHA512_DIGEST_SIZE] = '\0';
        for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
            sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

        //
        // Find user in database
        //
        database_bind_int( req->stmtLoginUserById, 1, nUserId );
        database_bind_text( req->stmtLoginUserById, 2, hash );
        struct NNCMS_ROW *tempRow = database_steps( req->stmtLoginUserById );
        if( tempRow == 0 )
        {
            database_freeRows( userRow );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Wrong password";
            log_printf( req, LOG_ALERT, "User::Profile: %s", frameTemplate[1].szValue );
            goto output;
        }
        database_freeRows( tempRow );

        // Okay then, update the password
        sha512( httpVarNewPass->lpszValue, strlen( (char *) httpVarNewPass->lpszValue ), digest );
        hash[2 * SHA512_DIGEST_SIZE] = '\0';
        for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
            sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

        // Query database
        database_bind_text( req->stmtEditUserHash, 1, hash );
        database_bind_int( req->stmtEditUserHash, 2, nUserId );
        database_steps( req->stmtEditUserHash );

        // Log action
        log_printf( req, LOG_ACTION, "User::Profile: User #%u \"%s\" (%s) password updated", nUserId, httpVarNick->lpszValue, userRow->szColValue[1] );
skip_pass_change:

        // Filter evil data
        filter_truncate_string( httpVarNick->lpszValue, NNCMS_NICKNAME_LEN );

        // Query database
        database_bind_text( req->stmtEditUserNick, 1, httpVarNick->lpszValue );
        database_bind_int( req->stmtEditUserNick, 2, nUserId );
        database_steps( req->stmtEditUserNick );

        // Log action
        log_printf( req, LOG_ACTION, "User::Profile: User #%u \"%s\" (%s) was modified", nUserId, httpVarNick->lpszValue, userRow->szColValue[1] );
        database_freeRows( userRow );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Fill template values
    userTemplate[2].szValue = userRow->szColValue[0];
    userTemplate[3].szValue = userRow->szColValue[1];
    userTemplate[4].szValue = userRow->szColValue[2];
    userTemplate[5].szValue = userRow->szColValue[3];

    // Load editor template
    template_iparse( req, TEMPLATE_USER_PROFILE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, userTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

    // Free memory from query result
    database_freeRows( userRow );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// ############################################################################

void user_edit( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Edit user";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/gtk-edit.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS userTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "user_group", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    userTemplate[6].szValue = req->g_sessionid;

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", req->g_username, "edit" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "User::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get log entry id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "user_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "User::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Find row associated with our object by 'name'
    unsigned int nUserId = atoi( httpVarId->lpszValue );
    database_bind_int( req->stmtFindUserById, 1, nUserId );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );
    if( userRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "User not found";
        log_printf( req, LOG_ERROR, "User::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "user_edit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get new data
        struct NNCMS_VARIABLE *httpVarNick = main_get_variable( req, "user_nick" );
        struct NNCMS_VARIABLE *httpVarGroup = main_get_variable( req, "user_group" );
        if( httpVarNick == 0 || httpVarGroup == 0 )
        {
            database_freeRows( userRow );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_NOTICE, "User::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Passwords
        struct NNCMS_VARIABLE *httpVarNewPass = main_get_variable( req, "user_new_pass" );
        struct NNCMS_VARIABLE *httpVarNewPassRetry = main_get_variable( req, "user_new_pass_retry" );
        if( httpVarNewPass == 0 || httpVarNewPassRetry == 0 )
            goto skip_pass_change;
        if( httpVarNewPass->lpszValue[0] == 0 || httpVarNewPassRetry->lpszValue[0] == 0 )
            goto skip_pass_change;

        filter_truncate_string( httpVarNewPass->lpszValue, NNCMS_PASSWORD_LEN );
        filter_truncate_string( httpVarNewPassRetry->lpszValue, NNCMS_PASSWORD_LEN );
        filter_table_replace( httpVarNewPass->lpszValue, (unsigned int) strlen( httpVarNewPass->lpszValue ), queryFilter );
        filter_table_replace( httpVarNewPassRetry->lpszValue, (unsigned int) strlen( httpVarNewPassRetry->lpszValue ), queryFilter );

        //
        // Compare passwords
        //
        if( strcmp( httpVarNewPass->lpszValue, httpVarNewPassRetry->lpszValue ) != 0 )
        {
            // Passwords are different
            database_freeRows( userRow );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Passwords doesn't match";
            log_printf( req, LOG_ERROR, "User::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Generate a hash value for password
        unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
        unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
        sha512( httpVarNewPass->lpszValue, strlen( (char *) httpVarNewPass->lpszValue ), digest );
        hash[2 * SHA512_DIGEST_SIZE] = '\0';
        for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
            sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

        // Query database
        database_bind_text( req->stmtEditUserHash, 1, hash );
        database_bind_int( req->stmtEditUserHash, 2, nUserId );
        database_steps( req->stmtEditUserHash );

        // Log action
        log_printf( req, LOG_ACTION, "User::Edit: User \"%s\" password updated", userRow->szColValue[1] );
skip_pass_change:

        // Filter evil data
        filter_truncate_string( httpVarNick->lpszValue, NNCMS_NICKNAME_LEN );

        // Query database
        database_bind_text( req->stmtEditUserNick, 1, httpVarNick->lpszValue );
        database_bind_int( req->stmtEditUserNick, 2, nUserId );
        database_steps( req->stmtEditUserNick );
        database_bind_text( req->stmtEditUserGroup, 1, httpVarGroup->lpszValue );
        database_bind_int( req->stmtEditUserGroup, 2, nUserId );
        database_steps( req->stmtEditUserGroup );

        // Log action
        log_printf( req, LOG_ACTION, "User::Edit: User #%u \"%s\" (%s) was modified", nUserId, httpVarNick->lpszValue, userRow->szColValue[1] );
        database_freeRows( userRow );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Fill template values
    userTemplate[2].szValue = userRow->szColValue[0];
    userTemplate[3].szValue = userRow->szColValue[1];
    userTemplate[4].szValue = userRow->szColValue[2];
    userTemplate[5].szValue = userRow->szColValue[3];

    // Load editor template
    template_iparse( req, TEMPLATE_USER_EDIT, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, userTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

    // Free memory from query result
    database_freeRows( userRow );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// ############################################################################

void user_delete( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Delete user";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/edit-delete.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS userTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    userTemplate[5].szValue = req->g_sessionid;

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", req->g_username, "delete" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "User::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get log entry id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "user_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "User::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Find row associated with our object by 'name'
    unsigned int nUserId = atoi( httpVarId->lpszValue );
    database_bind_int( req->stmtFindUserById, 1, nUserId );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );
    if( userRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "User not found";
        log_printf( req, LOG_ERROR, "User::Delete: %s", frameTemplate[1].szValue );
        goto cleanup;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "user_delete" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Query database
        database_bind_int( req->stmtDeleteUser, 1, nUserId );
        database_steps( req->stmtDeleteUser );

        // Log action
        log_printf( req, LOG_ACTION, "User::Delete: User \"%s\" deleted", userRow->szColValue[1] );

        // Redirect back
        redirect_to_referer( req );
        database_freeRows( userRow );
        return;
    }

    // Fill template values
    userTemplate[2].szValue = userRow->szColValue[0];
    userTemplate[3].szValue = userRow->szColValue[1];
    userTemplate[4].szValue = userRow->szColValue[2];

    // Load editor template
    template_iparse( req, TEMPLATE_USER_DELETE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, userTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );

    return;

cleanup:
    // Free memory from query result
    database_freeRows( userRow );

    goto output;
}

// ############################################################################
