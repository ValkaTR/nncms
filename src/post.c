// #############################################################################
// Source file: post.c

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
#include "log.h"
#include "acl.h"
#include "user.h"
#include "template.h"
#include "database.h"
#include "post.h"
#include "filter.h"
#include "smart.h"
#include "file.h"

#include "strlcpy.h"
#include "strlcat.h"
#include "strdup.h"

#include "memory.h"

// #############################################################################
// global variables
//

// Maximum tree depth for "reply, modify, remove" actions
int nPostDepth = 2;

// #############################################################################
// functions

bool post_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmtAddPost = database_prepare( "INSERT INTO `posts` VALUES(null, ?, ?, ?, ?, ?)" );
    req->stmtFindPost = database_prepare( "SELECT `id`, `user_id`, `timestamp`, `parent_post_id`, `subject`, `body` FROM `posts` WHERE `id`=? LIMIT 1" );
    req->stmtPowerEditPost = database_prepare( "UPDATE `posts` SET `subject`=?, `body`=?, `parent_post_id`=? WHERE `id`=?" );
    req->stmtEditPost = database_prepare( "UPDATE `posts` SET `subject`=?, `body`=? WHERE `id`=?" );
    req->stmtDeletePost = database_prepare( "DELETE FROM `posts` WHERE `id`=?" );
    req->stmtGetParentPost = database_prepare( "SELECT `parent_post_id` FROM `posts` WHERE `id`=?" );
    req->stmtRootPosts = database_prepare( "SELECT `parent_post_id` FROM `posts`" );

    req->stmtTopicPosts = database_prepare( "SELECT `id`, `user_id`, `timestamp`, `parent_post_id`, `subject`, `body` FROM `posts` WHERE `parent_post_id`=? ORDER BY `timestamp` DESC" );

    req->stmtCountChildPosts = database_prepare( "SELECT COUNT() FROM `posts` WHERE `parent_post_id`=?" );

    return true;
}

// #############################################################################

bool post_deinit( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req->stmtAddPost );
    database_finalize( req->stmtFindPost );
    database_finalize( req->stmtPowerEditPost );
    database_finalize( req->stmtEditPost );
    database_finalize( req->stmtDeletePost );
    database_finalize( req->stmtGetParentPost );
    database_finalize( req->stmtRootPosts );

    database_finalize( req->stmtTopicPosts );

    database_finalize( req->stmtCountChildPosts );

    return true;
}

// #############################################################################

void post_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post add";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/mail-message-new.png" },
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

    // Check session
    user_check_session( req );

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_parent" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Add: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Check if user can add post
    if( post_tree_check_perm( req, httpVarId->lpszValue, "add", POST_CHECK_PERM_DEPTH ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Add: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarAdd = main_get_variable( req, "post_add" );
    if( httpVarAdd != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get other data
        struct NNCMS_VARIABLE *httpVarSubject = main_get_variable( req, "post_subject" );
        struct NNCMS_VARIABLE *httpVarBody = main_get_variable( req, "post_body" );
        if( httpVarSubject == 0 || httpVarBody == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_NOTICE, "Post::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Filter evil data
        filter_truncate_string( httpVarSubject->lpszValue, NNCMS_PATH_LEN_MAX );
        filter_truncate_string( httpVarBody->lpszValue, NNCMS_PAGE_SIZE_MAX );

        // Query Database
        database_bind_text( req->stmtAddPost, 1, req->g_userid );
        database_bind_int( req->stmtAddPost, 2, time( 0 ) );
        database_bind_int( req->stmtAddPost, 3, uPostId );
        database_bind_text( req->stmtAddPost, 4, httpVarSubject->lpszValue );
        database_bind_text( req->stmtAddPost, 5, httpVarBody->lpszValue );
        database_steps( req->stmtAddPost );
        log_printf( req, LOG_ACTION, "Post::Add: Post added for post #%u", uPostId );

        // Redirect back
        //redirectf( req, "%s/post_view.fcgi?post_id=%u", homeURL, uPostId );
        redirect_to_referer( req );
        return;
    }
    else
    {
        // Get post for reference
        database_bind_int( req->stmtFindPost, 1, uPostId );
        post_get_post( req, &smartBuffer, TEMPLATE_POST_VIEW, req->stmtFindPost );

        // Check if user can add post
        post_add_form( req, &smartBuffer, httpVarId->lpszValue );
    }

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_reply( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post reply";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/mail-message-new.png" },
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

    // Check session
    user_check_session( req );

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_parent" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Reply: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Check if user can add post
    if( post_tree_check_perm( req, httpVarId->lpszValue, "reply", nPostDepth + 1 ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Reply: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarReply = main_get_variable( req, "post_reply" );
    if( httpVarReply != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get other data
        struct NNCMS_VARIABLE *httpVarSubject = main_get_variable( req, "post_subject" );
        struct NNCMS_VARIABLE *httpVarBody = main_get_variable( req, "post_body" );
        if( httpVarSubject == 0 || httpVarBody == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_NOTICE, "Post::Reply: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Filter evil data
        filter_truncate_string( httpVarSubject->lpszValue, NNCMS_PATH_LEN_MAX );
        filter_truncate_string( httpVarBody->lpszValue, NNCMS_PAGE_SIZE_MAX );

        // Query Database
        database_bind_text( req->stmtAddPost, 1, req->g_userid );
        database_bind_int( req->stmtAddPost, 2, time( 0 ) );
        database_bind_int( req->stmtAddPost, 3, uPostId );
        database_bind_text( req->stmtAddPost, 4, httpVarSubject->lpszValue );
        database_bind_text( req->stmtAddPost, 5, httpVarBody->lpszValue );
        database_steps( req->stmtAddPost );
        log_printf( req, LOG_ACTION, "Post::Reply: Post added for post #%u", uPostId );

        // Redirect back
        //redirectf( req, "%s/post_view.fcgi?post_id=%u", homeURL, uPostId );
        redirect_to_referer( req );
        return;
    }
    else
    {
        // Get post for reference
        database_bind_int( req->stmtFindPost, 1, uPostId );
        post_get_post( req, &smartBuffer, TEMPLATE_POST_VIEW, req->stmtFindPost );

        // Check if user can add post
        post_reply_form( req, &smartBuffer, httpVarId->lpszValue );
    }

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

bool post_add_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, char *lpszPostParent )
{
    // Page header
    char *szHeader = "Add post";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszFrame },
            { /* szName */ "icon",  /* szValue */ "images/actions/mail-message-new.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS addTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "post_parent", /* szValue */ 0 },
            { /* szName */ "post_subject", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ req->g_sessionid },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, req->g_userid );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill add template
    if( userRow == 0 )
    {
        addTemplate[2].szValue = 0; // User Id
        addTemplate[3].szValue = "Anonymous"; // User Nick
        addTemplate[4].szValue = "guest"; // User Name
    }
    else
    {
        addTemplate[2].szValue = userRow->szColValue[0]; // User Id
        addTemplate[3].szValue = userRow->szColValue[2]; // User Nick
        addTemplate[4].szValue = userRow->szColValue[1]; // User Name
    }
    addTemplate[5].szValue = lpszPostParent;

    // Load subject
    database_bind_text( req->stmtFindPost, 1, lpszPostParent );
    struct NNCMS_ROW *postRow = database_steps( req->stmtFindPost );
    if( postRow != 0 )
        addTemplate[6].szValue = postRow->szColValue[4];

    // Load template
    template_iparse( req, TEMPLATE_POST_ADD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, addTemplate );

    // Put form in frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszTemp, NNCMS_PAGE_SIZE_MAX, frameTemplate );
    smart_cat( smartBuffer, req->lpszTemp );

    // Free rows after parsing the template
    database_freeRows( postRow );
    database_freeRows( userRow );

    return true;
}

// #############################################################################

bool post_reply_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, char *lpszPostParent )
{
    // Page header
    char *szHeader = "Reply post";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszFrame },
            { /* szName */ "icon",  /* szValue */ "images/actions/mail-message-new.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS replyTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "post_parent", /* szValue */ 0 },
            { /* szName */ "post_subject", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ req->g_sessionid },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, req->g_userid );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill add template
    if( userRow == 0 )
    {
        replyTemplate[2].szValue = 0; // User Id
        replyTemplate[3].szValue = "Anonymous"; // User Nick
        replyTemplate[4].szValue = "guest"; // User Name
    }
    else
    {
        replyTemplate[2].szValue = userRow->szColValue[0]; // User Id
        replyTemplate[3].szValue = userRow->szColValue[2]; // User Nick
        replyTemplate[4].szValue = userRow->szColValue[1]; // User Name
    }
    replyTemplate[5].szValue = lpszPostParent;

    // Load subject
    database_bind_text( req->stmtFindPost, 1, lpszPostParent );
    struct NNCMS_ROW *postRow = database_steps( req->stmtFindPost );
    if( postRow != 0 )
        replyTemplate[6].szValue = postRow->szColValue[4];

    // Load template
    template_iparse( req, TEMPLATE_POST_REPLY, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, replyTemplate );

    // Put form in frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszTemp, NNCMS_PAGE_SIZE_MAX, frameTemplate );
    smart_cat( smartBuffer, req->lpszTemp );

    // Free rows after parsing the template
    database_freeRows( postRow );
    database_freeRows( userRow );

    return true;
}

// #############################################################################

void post_edit( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post edit";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/gtk-edit.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS editTemplate[] =
        {
            /* 00 */ { /* szName */ "homeURL", /* szValue */ homeURL },
            /* 01 */ { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            /* 02 */ { /* szName */ "post_id", /* szValue */ 0 },
            /* 03 */ { /* szName */ "user_id", /* szValue */ 0 },
            /* 04 */ { /* szName */ "user_name", /* szValue */ 0 },
            /* 05 */ { /* szName */ "user_nick", /* szValue */ 0 },
            /* 06 */ { /* szName */ "post_timestamp", /* szValue */ 0 },
            /* 07 */ { /* szName */ "post_date", /* szValue */ 0 },
            /* 08 */ { /* szName */ "post_parent", /* szValue */ 0 },
            /* 09 */ { /* szName */ "post_subject", /* szValue */ 0 },
            /* 10 */ { /* szName */ "post_body", /* szValue */ 0 },
            /* 11 */ { /* szName */ "fkey", /* szValue */ 0 },
            /* 12 */ { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    editTemplate[11].szValue = req->g_sessionid;

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Ok, try to find selected post id
    database_bind_int( req->stmtFindPost, 1, uPostId );
    struct NNCMS_ROW *postRow = database_steps( req->stmtFindPost );
    if( postRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Post not found";
        log_printf( req, LOG_NOTICE, "Post::Edit: %s (id = %u)", frameTemplate[1].szValue, uPostId );
        goto output;
    }

    // Check if user can add post
    if( acl_check_perm( req, "post", req->g_username, "edit" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "post_edit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get other data
        struct NNCMS_VARIABLE *httpVarSubject = main_get_variable( req, "post_subject" );
        struct NNCMS_VARIABLE *httpVarBody = main_get_variable( req, "post_body" );
        struct NNCMS_VARIABLE *httpVarParent = main_get_variable( req, "post_parent" );
        if( httpVarSubject == 0 || httpVarBody == 0 || httpVarParent == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_NOTICE, "Post::Edit: %s", frameTemplate[1].szValue );
            goto cleanup;
        }

        // Filter evil data
        filter_truncate_string( httpVarSubject->lpszValue, NNCMS_PATH_LEN_MAX );
        filter_truncate_string( httpVarBody->lpszValue, NNCMS_PAGE_SIZE_MAX );
        filter_truncate_string( httpVarParent->lpszValue, NNCMS_PATH_LEN_MAX );

        // Query Database
        database_bind_text( req->stmtPowerEditPost, 1, httpVarSubject->lpszValue );
        database_bind_text( req->stmtPowerEditPost, 2, httpVarBody->lpszValue );
        database_bind_int( req->stmtPowerEditPost, 3, atoi(httpVarParent->lpszValue) );
        database_bind_int( req->stmtPowerEditPost, 4, uPostId );
        database_steps( req->stmtPowerEditPost );
        log_printf( req, LOG_ACTION, "Post::Edit: Post #%u editted", uPostId );

        // Redirect back
        //redirectf( req, "%s/post_view.fcgi?post_id=%u", homeURL, uPostId );
        redirect_to_referer( req );
        database_freeRows( postRow );
        return;
    }

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, postRow->szColValue[0] );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill template values
    editTemplate[2].szValue = httpVarId->lpszValue; // Post Id
    editTemplate[3].szValue = postRow->szColValue[1]; // User Id
    if( userRow == 0 )
    {
        editTemplate[4].szValue = "guest"; // User Name
        editTemplate[5].szValue = "Anonymous"; // User Nick
    }
    else
    {
        editTemplate[4].szValue = userRow->szColValue[1]; // User Name
        editTemplate[5].szValue = userRow->szColValue[2]; // User Nick
    }

    editTemplate[6].szValue = postRow->szColValue[2]; // Timestamp
    editTemplate[8].szValue = postRow->szColValue[3]; // Parent Post Id
    editTemplate[9].szValue = postRow->szColValue[4]; // Subject
    editTemplate[10].szValue = postRow->szColValue[5]; // Body

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( postRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
    editTemplate[7].szValue = szTime; // Date

    // Show edit form
    template_iparse( req, TEMPLATE_POST_EDIT, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, editTemplate );

    // Free memory after use
    database_freeRows( userRow );

cleanup:
    database_freeRows( postRow );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_modify( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post modify";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/gtk-edit.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS modifyTemplate[] =
        {
            /* 00 */ { /* szName */ "homeURL", /* szValue */ homeURL },
            /* 01 */ { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            /* 02 */ { /* szName */ "post_id", /* szValue */ 0 },
            /* 03 */ { /* szName */ "user_id", /* szValue */ 0 },
            /* 04 */ { /* szName */ "user_name", /* szValue */ 0 },
            /* 05 */ { /* szName */ "user_nick", /* szValue */ 0 },
            /* 06 */ { /* szName */ "post_timestamp", /* szValue */ 0 },
            /* 07 */ { /* szName */ "post_date", /* szValue */ 0 },
            /* 08 */ { /* szName */ "post_parent", /* szValue */ 0 },
            /* 09 */ { /* szName */ "post_subject", /* szValue */ 0 },
            /* 10 */ { /* szName */ "post_body", /* szValue */ 0 },
            /* 11 */ { /* szName */ "fkey", /* szValue */ 0 },
            /* 12 */ { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    modifyTemplate[11].szValue = req->g_sessionid;

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Modify: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Ok, try to find selected post id
    database_bind_int( req->stmtFindPost, 1, uPostId );
    struct NNCMS_ROW *postRow = database_steps( req->stmtFindPost );
    if( postRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Post not found";
        log_printf( req, LOG_NOTICE, "Post::Modify: %s (id = %u)", frameTemplate[1].szValue, uPostId );
        goto output;
    }

    // Check if user can add post
    if( post_tree_check_perm( req, httpVarId->lpszValue, "modify", nPostDepth + 1 ) == false ||
        strcmp( postRow->szColValue[1], req->g_userid ) != 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Modify: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarModify = main_get_variable( req, "post_modify" );
    if( httpVarModify != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get other data
        struct NNCMS_VARIABLE *httpVarSubject = main_get_variable( req, "post_subject" );
        struct NNCMS_VARIABLE *httpVarBody = main_get_variable( req, "post_body" );
        if( httpVarSubject == 0 || httpVarBody == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_NOTICE, "Post::Modify: %s", frameTemplate[1].szValue );
            goto cleanup;
        }

        // Filter evil data
        filter_truncate_string( httpVarSubject->lpszValue, NNCMS_PATH_LEN_MAX );
        filter_truncate_string( httpVarBody->lpszValue, NNCMS_PAGE_SIZE_MAX );

        // Query Database
        database_bind_text( req->stmtEditPost, 1, httpVarSubject->lpszValue );
        database_bind_text( req->stmtEditPost, 2, httpVarBody->lpszValue );
        database_bind_int( req->stmtEditPost, 3, uPostId );
        database_steps( req->stmtEditPost );
        log_printf( req, LOG_ACTION, "Post::Modify: Post #%u editted", uPostId );

        // Redirect back
        //redirectf( req, "%s/post_view.fcgi?post_id=%u", homeURL, uPostId );
        redirect_to_referer( req );
        database_freeRows( postRow );
        return;
    }

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, postRow->szColValue[0] );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill template values
    modifyTemplate[2].szValue = httpVarId->lpszValue; // Post Id
    modifyTemplate[3].szValue = postRow->szColValue[1]; // User Id
    if( userRow == 0 )
    {
        modifyTemplate[4].szValue = "guest"; // User Name
        modifyTemplate[5].szValue = "Anonymous"; // User Nick
    }
    else
    {
        modifyTemplate[4].szValue = userRow->szColValue[1]; // User Name
        modifyTemplate[5].szValue = userRow->szColValue[2]; // User Nick
    }

    modifyTemplate[6].szValue = postRow->szColValue[2]; // Timestamp
    modifyTemplate[8].szValue = postRow->szColValue[3]; // Parent Post Id
    modifyTemplate[9].szValue = postRow->szColValue[4]; // Subject
    modifyTemplate[10].szValue = postRow->szColValue[5]; // Body

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( postRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
    modifyTemplate[7].szValue = szTime; // Date

    // Show edit form
    template_iparse( req, TEMPLATE_POST_MODIFY, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, modifyTemplate );

    // Free memory after use
    database_freeRows( userRow );

cleanup:
    database_freeRows( postRow );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_delete( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post delete";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/edit-delete.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS deleteTemplate[] =
        {
            /* 00 */ { /* szName */ "homeURL", /* szValue */ homeURL },
            /* 01 */ { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            /* 02 */ { /* szName */ "post_id", /* szValue */ 0 },
            /* 03 */ { /* szName */ "user_id", /* szValue */ 0 },
            /* 04 */ { /* szName */ "user_name", /* szValue */ 0 },
            /* 05 */ { /* szName */ "user_nick", /* szValue */ 0 },
            /* 06 */ { /* szName */ "post_timestamp", /* szValue */ 0 },
            /* 07 */ { /* szName */ "post_date", /* szValue */ 0 },
            /* 08 */ { /* szName */ "post_parent", /* szValue */ 0 },
            /* 09 */ { /* szName */ "post_subject", /* szValue */ 0 },
            /* 10 */ { /* szName */ "post_body", /* szValue */ 0 },
            /* 11 */ { /* szName */ "fkey", /* szValue */ 0 },
            /* 12 */ { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    deleteTemplate[11].szValue = req->g_sessionid;

    // Check if user can add post
    if( acl_check_perm( req, "post", req->g_username, "delete" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Ok, try to find selected post id
    database_bind_int( req->stmtFindPost, 1, uPostId );
    struct NNCMS_ROW *postRow = database_steps( req->stmtFindPost );
    if( postRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Post not found";
        log_printf( req, LOG_NOTICE, "Post::Delete: %s (id = %u)", frameTemplate[1].szValue, uPostId );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarDelete = main_get_variable( req, "post_delete" );
    if( httpVarDelete != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Query Database
        database_bind_int( req->stmtDeletePost, 1, uPostId );
        database_steps( req->stmtDeletePost );
        log_printf( req, LOG_ACTION, "Post::Delete: Post #%u deleted", uPostId );

        // Redirect back
        redirect_to_referer( req );
        database_freeRows( postRow );
        return;
    }

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, postRow->szColValue[0] );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill template values
    deleteTemplate[2].szValue = httpVarId->lpszValue; // Post Id
    deleteTemplate[3].szValue = postRow->szColValue[1]; // User Id
    if( userRow == 0 )
    {
        deleteTemplate[4].szValue = "guest"; // User Name
        deleteTemplate[5].szValue = "Anonymous"; // User Nick
    }
    else
    {
        deleteTemplate[4].szValue = userRow->szColValue[1]; // User Name
        deleteTemplate[5].szValue = userRow->szColValue[2]; // User Nick
    }

    deleteTemplate[6].szValue = postRow->szColValue[2]; // Timestamp
    deleteTemplate[8].szValue = postRow->szColValue[3]; // Parent Post Id
    deleteTemplate[9].szValue = postRow->szColValue[4]; // Subject
    deleteTemplate[10].szValue = postRow->szColValue[5]; // Body

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( postRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
    deleteTemplate[7].szValue = szTime; // Date

    // Show delete form
    template_iparse( req, TEMPLATE_POST_DELETE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, deleteTemplate );

    // Free rows after use
    database_freeRows( userRow );

cleanup:
    database_freeRows( postRow );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_remove( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post remove";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/edit-delete.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS removeTemplate[] =
        {
            /* 00 */ { /* szName */ "homeURL", /* szValue */ homeURL },
            /* 01 */ { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            /* 02 */ { /* szName */ "post_id", /* szValue */ 0 },
            /* 03 */ { /* szName */ "user_id", /* szValue */ 0 },
            /* 04 */ { /* szName */ "user_name", /* szValue */ 0 },
            /* 05 */ { /* szName */ "user_nick", /* szValue */ 0 },
            /* 06 */ { /* szName */ "post_timestamp", /* szValue */ 0 },
            /* 07 */ { /* szName */ "post_date", /* szValue */ 0 },
            /* 08 */ { /* szName */ "post_parent", /* szValue */ 0 },
            /* 09 */ { /* szName */ "post_subject", /* szValue */ 0 },
            /* 10 */ { /* szName */ "post_body", /* szValue */ 0 },
            /* 11 */ { /* szName */ "fkey", /* szValue */ 0 },
            /* 12 */ { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    removeTemplate[11].szValue = req->g_sessionid;

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Remove: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Ok, try to find selected post id
    database_bind_int( req->stmtFindPost, 1, uPostId );
    struct NNCMS_ROW *postRow = database_steps( req->stmtFindPost );
    if( postRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Post not found";
        log_printf( req, LOG_NOTICE, "Post::Remove: %s (id = %u)", frameTemplate[1].szValue, uPostId );
        goto output;
    }

    // Check if user can add post
    if( post_tree_check_perm( req, httpVarId->lpszValue, "remove", nPostDepth + 1 ) == false ||
        strcmp( postRow->szColValue[1], req->g_userid ) != 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Remove: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarRemove = main_get_variable( req, "post_remove" );
    if( httpVarRemove != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Query Database
        database_bind_int( req->stmtDeletePost, 1, uPostId );
        database_steps( req->stmtDeletePost );
        log_printf( req, LOG_ACTION, "Post::Remove: Post #%u removed", uPostId );

        // Redirect back
        redirect_to_referer( req );
        database_freeRows( postRow );
        return;
    }

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, postRow->szColValue[0] );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill template values
    removeTemplate[2].szValue = httpVarId->lpszValue; // Post Id
    removeTemplate[3].szValue = postRow->szColValue[1]; // User Id
    if( userRow == 0 )
    {
        removeTemplate[4].szValue = "guest"; // User Name
        removeTemplate[5].szValue = "Anonymous"; // User Nick
    }
    else
    {
        removeTemplate[4].szValue = userRow->szColValue[1]; // User Name
        removeTemplate[5].szValue = userRow->szColValue[2]; // User Nick
    }

    removeTemplate[6].szValue = postRow->szColValue[2]; // Timestamp
    removeTemplate[8].szValue = postRow->szColValue[3]; // Parent Post Id
    removeTemplate[9].szValue = postRow->szColValue[4]; // Subject
    removeTemplate[10].szValue = postRow->szColValue[5]; // Body

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( postRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
    removeTemplate[7].szValue = szTime; // Date

    // Show edit form
    template_iparse( req, TEMPLATE_POST_REMOVE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, removeTemplate );

    // Free rows after use
    database_freeRows( userRow );

cleanup:
    database_freeRows( postRow );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_topics( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Topics";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszTemp },
            { /* szName */ "icon",  /* szValue */ "images/stock/net/stock_mail-open.png" },
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

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Topics: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check session
    user_check_session( req );

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, (unsigned int) strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Check if user can view posts
    if( post_tree_check_perm( req, httpVarId->lpszValue, "view", POST_CHECK_PERM_DEPTH ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Topics: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get posts
    database_bind_int( req->stmtTopicPosts, 1, uPostId );
    post_get_post( req, &smartBuffer, TEMPLATE_POST_TOPICS_ROW, req->stmtTopicPosts );

    // Make a cute frame
    database_bind_int( req->stmtFindPost, 1, uPostId );
    post_make_frame( req, req->lpszTemp, &smartBuffer, TEMPLATE_POST_TOPICS_STRUCTURE, req->stmtFindPost );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_news( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "News";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszTemp },
            { /* szName */ "icon",  /* szValue */ "images/stock/net/stock_mail-open.png" },
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

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    struct NNCMS_VARIABLE *httpVarTemplate = main_get_variable( req, "template" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::News: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check session
    user_check_session( req );

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Check if user can view posts
    if( post_tree_check_perm( req, httpVarId->lpszValue, "view", POST_CHECK_PERM_DEPTH ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::News: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Prepare output buffer
    *req->lpszBuffer = 0;

    // Get posts
    database_bind_int( req->stmtTopicPosts, 1, uPostId );
    post_get_post( req, &smartBuffer, TEMPLATE_POST_NEWS_ROW, req->stmtTopicPosts );

    // Make a cute frame
    database_bind_int( req->stmtFindPost, 1, uPostId );
    post_make_frame( req, req->lpszTemp, &smartBuffer, TEMPLATE_POST_NEWS_STRUCTURE, req->stmtFindPost );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Post view";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/net/stock_mail-open.png" },
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

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check session
    user_check_session( req );

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, strlen( httpVarId->lpszValue ), numericFilter );

    // Check if user can view posts
    if( post_tree_check_perm( req, httpVarId->lpszValue, "view", POST_CHECK_PERM_DEPTH ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Prepare output buffer
    *req->lpszBuffer = 0;

    // Get posts
    database_bind_int( req->stmtFindPost, 1, atoi( httpVarId->lpszValue ) );
    post_get_post( req, &smartBuffer, TEMPLATE_POST_VIEW, req->stmtFindPost );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_rss( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Message board";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/net/stock_mail-open.png" },
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

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Rss: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check session
    user_check_session( req );

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Check if user can view posts
    if( post_tree_check_perm( req, httpVarId->lpszValue, "view", POST_CHECK_PERM_DEPTH ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Rss: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Prepare output buffer
    *req->lpszBuffer = 0;

    // Get topics
    database_bind_int( req->stmtTopicPosts, 1, uPostId );
    post_get_tree_post( req, &smartBuffer, TEMPLATE_POST_RSS_ENTRY, req->stmtTopicPosts );

    // Make a cute frame
    database_bind_int( req->stmtFindPost, 1, uPostId );
    size_t nRetVal = post_make_frame( req, req->lpszFrame, &smartBuffer, TEMPLATE_POST_RSS_STRUCTURE, req->stmtFindPost );

    // Send generated rss to client
    main_send_headers( req, nRetVal, 0 );
    FCGX_PutStr( req->lpszFrame, nRetVal, req->fcgi_request->out );

    return;

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void post_board( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Message board";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszTemp },
            { /* szName */ "icon",  /* szValue */ "images/stock/net/stock_mail-open.png" },
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

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "post_id" );
    struct NNCMS_VARIABLE *httpVarTemplate = main_get_variable( req, "template" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Post::Board: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check session
    user_check_session( req );

    // Filter evil data
    filter_truncate_string( httpVarId->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_table_replace( httpVarId->lpszValue, strlen( httpVarId->lpszValue ), numericFilter );
    unsigned int uPostId = atoi( httpVarId->lpszValue );

    // Check if user can view posts
    if( post_tree_check_perm( req, httpVarId->lpszValue, "view", POST_CHECK_PERM_DEPTH ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Board: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Prepare output buffer
    *req->lpszBuffer = 0;

    // Get posts
    database_bind_int( req->stmtTopicPosts, 1, uPostId );
    post_get_tree_post( req, &smartBuffer, TEMPLATE_POST_BOARD_ROW, req->stmtTopicPosts );

    // Make a cute frame
    database_bind_int( req->stmtFindPost, 1, uPostId );
    post_make_frame( req, req->lpszTemp, &smartBuffer, TEMPLATE_POST_BOARD_STRUCTURE, req->stmtFindPost );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

bool post_check_perm( struct NNCMS_THREAD_INFO *req, char *lpszPostId, char *lpszPerm )
{
    // Temp buffer
    char szPermObject[NNCMS_PATH_LEN_MAX];

    // Check session
    user_check_session( req );

    // Название в базе
    snprintf( szPermObject, sizeof(szPermObject), "post_%s", lpszPostId );

    // Проверка доступа
    if( acl_check_perm( req, szPermObject, req->g_username, lpszPerm ) == true )
    {
        return true;
    }

    return false;
}

// #############################################################################

bool post_tree_check_perm( struct NNCMS_THREAD_INFO *req, char *lpszPostId, char *lpszPerm, int nDepth )
{
    // Check session
    user_check_session( req );

    // Check if user can view posts
    if( acl_check_perm( req, "post", req->g_username, lpszPerm ) == true )
        return true;

    // Give one more chance
    char szCurId[32]; strlcpy( szCurId, lpszPostId, sizeof(szCurId) );
    struct NNCMS_ROW *postRow = 0;
    int i = 0;
    do
    {
        // Get next parent row
        if( postRow != 0 )
        {
            strlcpy( szCurId, postRow->szColValue[0], sizeof(szCurId) );
            database_freeRows( postRow );
        }

        // Perm check
        if( post_check_perm( req, szCurId, lpszPerm ) == true )
            return true;

        // Check for accident cyclic parent=>id=>parent=>id
        i = 1 + i;
        if( i >= nDepth )
        {
            //log_printf( req, LOG_DEBUG, "Post::CheckPerm: Too nested" );
            return false;
        }

        // Query for parent post
        database_bind_text( req->stmtGetParentPost, 1, szCurId );
        postRow = database_steps( req->stmtGetParentPost );
    } while( postRow != 0 );

    return false;
}

// #############################################################################

// Uses req->lpszFrame, add result to req->lpszBuffer
void post_get_tree_post( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, enum TEMPLATE_INDEX tempNum, sqlite3_stmt *stmt )
{
    struct NNCMS_TEMPLATE_TAGS postTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "post_id", /* szValue */ 0 },
            { /* szName */ "user_id", /* szValue */ 0 },
            { /* szName */ "user_name", /* szValue */ 0 },
            { /* szName */ "user_nick", /* szValue */ 0 },
            { /* szName */ "post_timestamp", /* szValue */ 0 },
            { /* szName */ "post_parent", /* szValue */ 0 },
            { /* szName */ "post_subject", /* szValue */ 0 },
            { /* szName */ "post_body", /* szValue */ 0 },
            { /* szName */ "post_actions", /* szValue */ 0 },
            { /* szName */ "post_internet_timestamp", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Find posts
    //va_list vArgs;
    //va_start( vArgs, lpszQuery );
    struct NNCMS_ROW *firstRow = database_steps( stmt ); //database_vquery( req, lpszQuery, vArgs );
    //printf( TERM_BRIGHT TERM_FG_YELLOW "GetPostQuery:" TERM_RESET " %s\n", lpszQuery, vArgs ); // Debug
    //va_end( vArgs );

    // If not found then return zero
    if( firstRow == 0 )
    {
        //smart_cat( smartBuffer, "n/a\n" );
        return;
    }

    // Check session
    user_check_session( req );

    // Loop thru all rows
    struct NNCMS_ROW *curRow = firstRow;
    while( curRow != 0 )
    {
        // Get user name and user nick
        database_bind_text( req->stmtFindUserById, 1, curRow->szColValue[1] );
        struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

        // Fill template values
        postTemplate[1].szValue = curRow->szColValue[0]; // Post ID
        postTemplate[2].szValue = curRow->szColValue[1]; // User ID
        if( userRow == 0 )
        {
            postTemplate[3].szValue = "guest"; // User Name
            postTemplate[4].szValue = "Anonymous"; // User Nick
        }
        else
        {
            postTemplate[3].szValue = userRow->szColValue[1]; // User Name
            postTemplate[4].szValue = userRow->szColValue[2]; // User Nick
        }
        postTemplate[6].szValue = curRow->szColValue[3]; // Parent Post ID
        postTemplate[7].szValue = curRow->szColValue[4]; // Post Subject
        postTemplate[8].szValue = curRow->szColValue[5]; // Post Body
        postTemplate[9].szValue = 0; //post_get_actions( req, curRow->szColValue[0] ); // Post Actions

        // Get time string
        char szTime[64];
        time_t rawTime = atoi( curRow->szColValue[2] );
        struct tm *timeInfo = localtime( &rawTime );
        strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
        postTemplate[5].szValue = szTime;

        // Get internet time string
        char szInternetTime[64];
        timeInfo = gmtime( &rawTime );
        strftime( szInternetTime, sizeof(szInternetTime), "%Y-%m-%dT%H:%M:%SZ", timeInfo );
        postTemplate[10].szValue = szInternetTime;

        // Load template
        template_iparse( req, tempNum, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, postTemplate );
        smart_cat( smartBuffer, req->lpszFrame );

        // Free memory from useless info
        if( postTemplate[9].szValue != 0 )
        {
            FREE( postTemplate[9].szValue );
            postTemplate[9].szValue = 0;
        }

next_row:
        // Free rows after parsing the template
        database_freeRows( userRow );

        // Look for child posts
        //smart_cat( smartBuffer, "<ul>\n" );
        database_bind_text( req->stmtTopicPosts, 1, curRow->szColValue[0] );
        post_get_tree_post( req, smartBuffer, tempNum, req->stmtTopicPosts );
        //smart_cat( smartBuffer, "</ul>\n" );

        // Select next row
        struct NNCMS_ROW *nextRow = curRow->next;

        // Update current row
        curRow = nextRow;
    }

    // Free memory from query result
    database_freeRows( firstRow );

    // Ok
    return;
}

// #############################################################################

// Uses req->lpszFrame, add result to req->lpszBuffer
void post_get_post( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *smartBuffer, enum TEMPLATE_INDEX tempNum, sqlite3_stmt *stmt )
{
    struct NNCMS_TEMPLATE_TAGS postTemplate[] =
        {
            /* 00 */ { /* szName */ "homeURL", /* szValue */ homeURL },
            /* 01 */ { /* szName */ "post_id", /* szValue */ 0 },
            /* 02 */ { /* szName */ "user_id", /* szValue */ 0 },
            /* 03 */ { /* szName */ "user_name", /* szValue */ 0 },
            /* 04 */ { /* szName */ "user_nick", /* szValue */ 0 },
            /* 05 */ { /* szName */ "post_timestamp", /* szValue */ 0 },
            /* 06 */ { /* szName */ "post_parent", /* szValue */ 0 },
            /* 07 */ { /* szName */ "post_subject", /* szValue */ 0 },
            /* 08 */ { /* szName */ "post_body", /* szValue */ 0 },
            /* 09 */ { /* szName */ "post_actions", /* szValue */ 0 },
            /* 10 */ { /* szName */ "post_child_count", /* szValue */ 0 },
            /* 11 */ { /* szName */ "post_depth", /* szValue */ 0 },
            /* 12 */ { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Find posts
    //va_list vArgs;
    //va_start( vArgs, lpszQuery );
    struct NNCMS_ROW *firstRow = database_steps( stmt ); //database_vquery( req, lpszQuery, vArgs );
    //printf( TERM_BRIGHT TERM_FG_YELLOW "GetPostQuery:" TERM_RESET " %s\n", lpszQuery, vArgs ); // Debug
    //va_end( vArgs );

    // If not found then return zero
    if( firstRow == 0 )
    {
        smart_cat( smartBuffer, "n/a\n" );
        return;
    }

    // Makes string out of depth
    char szDepth[20];
    snprintf( szDepth, sizeof(szDepth), "%i", nPostDepth );
    postTemplate[11].szValue = szDepth;

    // Check session
    user_check_session( req );

    // Loop thru all rows
    struct NNCMS_ROW *curRow = firstRow;
    while( curRow != 0 )
    {
        // Get user name and user nick
        database_bind_text( req->stmtFindUserById, 1, curRow->szColValue[1] );
        struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

        // Fill template values
        postTemplate[1].szValue = curRow->szColValue[0]; // Post ID
        postTemplate[2].szValue = curRow->szColValue[1]; // User ID
        if( userRow == 0 )
        {
            postTemplate[3].szValue = "guest"; // User Name
            postTemplate[4].szValue = "Anonymous"; // User Nick
        }
        else
        {
            postTemplate[3].szValue = userRow->szColValue[1]; // User Name
            postTemplate[4].szValue = userRow->szColValue[2]; // User Nick
        }
        postTemplate[6].szValue = curRow->szColValue[3]; // Parent Post ID
        postTemplate[7].szValue = curRow->szColValue[4]; // Post Subject
        postTemplate[8].szValue = curRow->szColValue[5]; // Post Body
        postTemplate[9].szValue = 0; //post_get_actions( req, curRow->szColValue[0] ); // Post Actions

        // Cound child posts
        database_bind_text( req->stmtCountChildPosts, 1, curRow->szColValue[0] );
        struct NNCMS_ROW *countRow = database_steps( req->stmtCountChildPosts );
        postTemplate[10].szValue = countRow->szColValue[0];

        // Get time string
        char szTime[64];
        time_t rawTime = atoi( curRow->szColValue[2] );
        struct tm *timeInfo = localtime( &rawTime );
        strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
        postTemplate[5].szValue = szTime;

        // Load template
        template_iparse( req, tempNum, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, postTemplate );
        smart_cat( smartBuffer, req->lpszFrame );

        // Free memory from useless info
        if( postTemplate[9].szValue != 0 )
        {
            FREE( postTemplate[9].szValue );
            postTemplate[9].szValue = 0;
        }

next_row:
        // Free rows after parsing the template
        database_freeRows( userRow );
        database_freeRows( countRow );

        // Select next row
        struct NNCMS_ROW *nextRow = curRow->next;

        // Update current row
        curRow = nextRow;
    }

    // Free memory from query result
    database_freeRows( firstRow );

    // Ok
    return;
}

// #############################################################################

// Loads a post and buffer into one template
size_t post_make_frame( struct NNCMS_THREAD_INFO *req, char *lpDest, struct NNCMS_BUFFER *smartBuffer, enum TEMPLATE_INDEX tempNum, sqlite3_stmt *stmt )
{
    struct NNCMS_TEMPLATE_TAGS postTemplate[] =
        {
            /* 00 */ { /* szName */ "homeURL", /* szValue */ homeURL },
            /* 01 */ { /* szName */ "post_id", /* szValue */ 0 },
            /* 02 */ { /* szName */ "user_id", /* szValue */ 0 },
            /* 03 */ { /* szName */ "user_name", /* szValue */ 0 },
            /* 04 */ { /* szName */ "user_nick", /* szValue */ 0 },
            /* 05 */ { /* szName */ "post_timestamp", /* szValue */ 0 },
            /* 06 */ { /* szName */ "post_parent", /* szValue */ 0 },
            /* 07 */ { /* szName */ "post_subject", /* szValue */ 0 },
            /* 08 */ { /* szName */ "post_body", /* szValue */ 0 },
            /* 09 */ { /* szName */ "post_actions", /* szValue */ 0 },
            /* 10 */ { /* szName */ "entries", /* szValue */ smartBuffer->lpBuffer },
            /* 11 */ { /* szName */ "post_depth", /* szValue */ 0 },
            /* 12 */ { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Find post
    struct NNCMS_ROW *postRow = database_steps( stmt );

    // If not found then return zero
    if( postRow == 0 )
    {
        strcpy( lpDest, "n/a\n" );
        return 4;
    }

    // Makes string out of depth
    char szDepth[20];
    snprintf( szDepth, sizeof(szDepth), "%i", nPostDepth );
    postTemplate[11].szValue = szDepth;

    // Check session
    user_check_session( req );

    // Get user name and user nick
    database_bind_text( req->stmtFindUserById, 1, postRow->szColValue[1] );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );

    // Fill template values
    postTemplate[1].szValue = postRow->szColValue[0]; // Post ID
    postTemplate[2].szValue = postRow->szColValue[1]; // User ID
    if( userRow == 0 )
    {
        postTemplate[3].szValue = "guest"; // User Name
        postTemplate[4].szValue = "Anonymous"; // User Nick
    }
    else
    {
        postTemplate[3].szValue = userRow->szColValue[1]; // User Name
        postTemplate[4].szValue = userRow->szColValue[2]; // User Nick
    }
    postTemplate[6].szValue = postRow->szColValue[3]; // Parent Post ID
    postTemplate[7].szValue = postRow->szColValue[4]; // Post Subject
    postTemplate[8].szValue = postRow->szColValue[5]; // Post Body

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( postRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, sizeof(szTime), szTimestampFormat, timeInfo );
    postTemplate[5].szValue = szTime;

    // Load template
    size_t nRetVal = template_iparse( req, tempNum, lpDest, NNCMS_PAGE_SIZE_MAX, postTemplate );

    // Free rows after parsing the template
    database_freeRows( userRow );

    // Free memory from query result
    database_freeRows( postRow );

    // Ok
    return nRetVal;
}

// #############################################################################

void post_root( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "All posts";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/net/stock_mail-open-multiple.png" },
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

    // Check session
    user_check_session( req );

    // Check if user can view posts
    if( acl_check_perm( req, "post", req->g_username, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Post::Root: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get list of all root posts
    struct NNCMS_ROW *firstRow = database_steps( req->stmtRootPosts );

    // If not found then return zero
    if( firstRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Posts not found";
        log_printf( req, LOG_ERROR, "Post::Root: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Loop thru all rows
    struct NNCMS_ROW *curRow = firstRow;
    unsigned int *uRootPosts = MALLOC( sizeof(unsigned int) * 1024 );
    unsigned int nRootPostsCount = 0;
    memset( uRootPosts, 0, sizeof(unsigned int) * 1024 );
    while( curRow != 0 )
    {
        // Save unique parent posts
        unsigned int uPostId = atoi( curRow->szColValue[0] );

        // Check if post id is in list
        for( unsigned int i = 0; i < nRootPostsCount; i++ )
        {
            if( uRootPosts[i] == uPostId )
            {
                goto next_row;
            }
        }

        // If we are here, then post id is not in list
        // Add it
        uRootPosts[nRootPostsCount] = uPostId;
        nRootPostsCount++;

        // Select next row
        struct NNCMS_ROW *nextRow;
next_row:
        nextRow = curRow->next;

        // Update current row
        curRow = nextRow;
    }

    // Print post headers for every unique parent post id
    for( unsigned int i = 0; i < nRootPostsCount; i++ )
    {
        //Debug
        //printf( TERM_FG_CYAN "Post ID:" TERM_RESET " %u\n", uRootPosts[i] );

        database_bind_int( req->stmtFindPost, 1, uRootPosts[i] );
        post_get_post( req, &smartBuffer, TEMPLATE_POST_ROOT, req->stmtFindPost );
        database_bind_int( req->stmtTopicPosts, 1, uRootPosts[i] );
        post_get_post( req, &smartBuffer, TEMPLATE_POST_PARENT, req->stmtTopicPosts );
        //smart_cat( &smartBuffer, "<hr>\n" );
    }

    // Free memory from query result and other
    database_freeRows( firstRow );
    FREE( uRootPosts );

    // Check if user can add post
    post_add_form( req, &smartBuffer, "0" );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################
