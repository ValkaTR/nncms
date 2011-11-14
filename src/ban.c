// #############################################################################
// Source file: ban.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "ban.h"

#include "database.h"
#include "template.h"
#include "user.h"
#include "acl.h"
#include "log.h"
#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #############################################################################
// global variables
//

// Log module status
static bool b_ban = false;

// Ban list
struct NNCMS_BAN_INFO *banList;

// #############################################################################
// functions

bool ban_init( struct NNCMS_THREAD_INFO *req )
{
    //if( b_ban == true ) return false;
    //b_ban = true;

    // Prepared statements
    req->stmtAddBan = database_prepare( "INSERT INTO `bans` VALUES(null, ?, ?, ?, ?, ?)" );
    req->stmtListBans = database_prepare( "SELECT `id`, `user_id`, `timestamp`, `ip`, `mask`, `reason` FROM `bans` ORDER BY `timestamp` ASC" );
    req->stmtFindBanById = database_prepare( "SELECT `id`, `user_id`, `timestamp`, `ip`, `mask`, `reason` FROM `bans` WHERE `id`=?" );
    req->stmtEditBan = database_prepare( "UPDATE `bans` SET `ip`=?, `mask`=?, `reason`=? WHERE `id`=?" );
    req->stmtDeleteBan = database_prepare( "DELETE FROM `bans` WHERE `id`=?" );

    // Load bans only once, not for every thread
    if( b_ban == false )
        //ban_load( req );
    b_ban = true;

    // Test
    //struct in_addr addr;
    //inet_aton( "194.168.1.55", &addr );
    //ban_check( req, &addr );

    return true;
}

// #############################################################################

bool ban_deinit( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req->stmtAddBan );
    database_finalize( req->stmtListBans );
    database_finalize( req->stmtFindBanById );
    database_finalize( req->stmtEditBan );
    database_finalize( req->stmtDeleteBan );


    if( b_ban == true )
        ban_unload( req );
    b_ban = false;

    return true;
}

// #############################################################################

void ban_load( struct NNCMS_THREAD_INFO *req )
{
    //sleep(3);
    // Find rows
    struct NNCMS_ROW *banRow = database_steps( req->stmtListBans );

    // No bans
    if( banRow == 0 )
        return;

    // Loop thru all rows
    struct NNCMS_ROW *curRow = banRow;
    struct NNCMS_BAN_INFO *firstStruct = 0;
    while( curRow )
    {
        // Prepare ban structure
        struct NNCMS_BAN_INFO *banStruct = MALLOC( sizeof(struct NNCMS_BAN_INFO) );
        banStruct->id = atoi( curRow->szColValue[0] );
        inet_aton( curRow->szColValue[3], &banStruct->ip );
        inet_aton( curRow->szColValue[4], &banStruct->mask );
        banStruct->next = 0;

        // Sorted insert
        if( firstStruct == 0 )
        {
            firstStruct = banStruct;
        }
        else
        {
            // Check if we just need to add at the beggining
            if( (unsigned int) firstStruct->ip.s_addr > (unsigned int) banStruct->ip.s_addr )
            {
                // We have to add to the beginning of the list
                banStruct->next = firstStruct;
                firstStruct = banStruct;
                goto next_row;
            }

            // Find a place somewhere in the middle or end
            struct NNCMS_BAN_INFO *curStruct = firstStruct;
            while( curStruct )
            {
                if( curStruct->next == 0 )
                {
                    // Position is at the end
                    curStruct->next = banStruct;
                    break;
                }

                // Position for structure
                if( (unsigned int) curStruct->next->ip.s_addr > (unsigned int) banStruct->ip.s_addr )
                {
                    // Add somewhere in the middle
                    banStruct->next = curStruct->next;
                    curStruct->next = banStruct;
                    break;
                }

                // Next
                curStruct = curStruct->next;
            }
        }

        // Test
        //struct NNCMS_BAN_INFO *curStruct;
next_row:
        //curStruct = firstStruct;
        //while( curStruct )
        //{
        //    printf( "%02u: %u\n", curStruct->id, curStruct->ip.s_addr );
        //
        //    // Next
        //    curStruct = curStruct->next;
        //}
        //printf("\n");

        // Select next row
        curRow = curRow->next;
    }

    // Free memory from query result
    database_freeRows( banRow );

    // Finished, save the chain
    banList = firstStruct;
}

// #############################################################################

void ban_unload( struct NNCMS_THREAD_INFO *req )
{
    // Remove bans from memory
    struct NNCMS_BAN_INFO *curStruct = banList;
    banList = 0;
    while( curStruct )
    {
        struct NNCMS_BAN_INFO *tempStruct = curStruct->next;
        FREE( curStruct );

        // Next
        curStruct = tempStruct;
    }
}

// #############################################################################

bool ban_check( struct NNCMS_THREAD_INFO *req, struct in_addr *addr )
{
    // Check if ip address is in table
    struct NNCMS_BAN_INFO *curStruct = banList;
    while( curStruct )
    {
        if( (addr->s_addr & curStruct->mask.s_addr) ==
            (curStruct->ip.s_addr & curStruct->mask.s_addr) )
        {
            log_printf( req, LOG_WARNING, "Ban::Check: Ip %u.%u.%u.%u hits the ban #%u (%u.%u.%u.%u/%u.%u.%u.%u)",
                (addr->s_addr & (0x0FF << (8*0))) >> (8*0),
                (addr->s_addr & (0x0FF << (8*1))) >> (8*1),
                (addr->s_addr & (0x0FF << (8*2))) >> (8*2),
                (addr->s_addr & (0x0FF << (8*3))) >> (8*3),
                curStruct->id,
                (curStruct->ip.s_addr & (0x0FF << (8*0))) >> (8*0),
                (curStruct->ip.s_addr & (0x0FF << (8*1))) >> (8*1),
                (curStruct->ip.s_addr & (0x0FF << (8*2))) >> (8*2),
                (curStruct->ip.s_addr & (0x0FF << (8*3))) >> (8*3),
                (curStruct->mask.s_addr & (0x0FF << (8*0))) >> (8*0),
                (curStruct->mask.s_addr & (0x0FF << (8*1))) >> (8*1),
                (curStruct->mask.s_addr & (0x0FF << (8*2))) >> (8*2),
                (curStruct->mask.s_addr & (0x0FF << (8*3))) >> (8*3) );
            return true;
        }

        // Next
        curStruct = curStruct->next;
    }

    return false;
}

// #############################################################################

void ban_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Add Ban";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/status/security-medium.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS addTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ 0 },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    addTemplate[2].szValue = req->g_sessionid;

    // Check user permission to edit Bans
    if( acl_check_perm( req, "ban", req->g_username, "add" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarAdd = main_get_variable( req, "ban_add" );
    if( httpVarAdd != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get POST data
        struct NNCMS_VARIABLE *httpVarIp = main_get_variable( req, "ban_ip" );
        struct NNCMS_VARIABLE *httpVarMask = main_get_variable( req, "ban_mask" );
        struct NNCMS_VARIABLE *httpVarReason = main_get_variable( req, "ban_reason" );
        if( httpVarIp == 0 ||
            httpVarMask == 0 ||
            httpVarReason == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_ERROR, "Ban::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Check if address written correctly
        struct in_addr addr;
        if( inet_aton( httpVarIp->lpszValue, &addr ) == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Invalid ip address";
            log_printf( req, LOG_ERROR, "Ban::Add: %s", frameTemplate[1].szValue );
            goto output;
        }
        if( inet_aton( httpVarMask->lpszValue, &addr ) == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Invalid mask address";
            log_printf( req, LOG_ERROR, "Ban::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Query Database
        database_bind_text( req->stmtAddBan, 1, req->g_userid );
        database_bind_int( req->stmtAddBan, 2, time( 0 ) );
        database_bind_text( req->stmtAddBan, 3, httpVarIp->lpszValue );
        database_bind_text( req->stmtAddBan, 4, httpVarMask->lpszValue );
        database_bind_text( req->stmtAddBan, 5, httpVarReason->lpszValue );
        database_steps( req->stmtAddBan );
        log_printf( req, LOG_ACTION, "Ban::Add: Added ban for ip %s", httpVarIp->lpszValue );

        // Recache bans
        ban_load( req );
        ban_unload( req );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Referer
    addTemplate[0].szValue = FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp );

    // Load editor
    template_iparse( req, TEMPLATE_BAN_ADD, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, addTemplate );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void ban_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Ban view";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/status/security-medium.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS banTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "ban_id", /* szValue */ 0 },
            { /* szName */ "ban_user_id", /* szValue */ 0 },
            { /* szName */ "ban_user_name", /* szValue */ 0 },
            { /* szName */ "ban_user_nick", /* szValue */ 0 },
            { /* szName */ "ban_timestamp", /* szValue */ 0 },
            { /* szName */ "ban_ip", /* szValue */ 0 },
            { /* szName */ "ban_mask", /* szValue */ 0 },
            { /* szName */ "ban_reason", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS structureTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "entries", /* szValue */ req->lpszBuffer },
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
    if( acl_check_perm( req, "ban", req->g_username, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Ban::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get log entry id
    struct NNCMS_ROW *banRow = 0;
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "ban_id" );
    if( httpVarId == 0 )
    {
        // Find rows
        banRow = database_steps( req->stmtListBans );
    }
    else
    {
        // Find row associated with our object by 'id'
        database_bind_text( req->stmtFindBanById, 1, httpVarId->lpszValue );
        banRow = database_steps( req->stmtFindBanById );
    }

    // Is log empty ?
    if( banRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No ban data";
        log_printf( req, LOG_ERROR, "Ban::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Loop thru all rows
    struct NNCMS_ROW *curRow = banRow;
    unsigned int i = 100;
    while( --i )
    {
        // Fill template values
        banTemplate[1].szValue = curRow->szColValue[0];
        banTemplate[6].szValue = curRow->szColValue[3];
        banTemplate[7].szValue = curRow->szColValue[4];
        banTemplate[8].szValue = curRow->szColValue[5];

        // Get time string
        char szTime[64];
        time_t rawTime = atoi( curRow->szColValue[2] );
        struct tm *timeInfo = localtime( &rawTime );
        strftime( szTime, 64, szTimestampFormat, timeInfo );
        banTemplate[5].szValue = szTime;

        // Get user
        database_bind_text( req->stmtFindUserById, 1, curRow->szColValue[1] ); // id
        struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );
        if( userRow != 0 )
        {
            banTemplate[2].szValue = curRow->szColValue[1];
            banTemplate[3].szValue = userRow->szColValue[1];
            banTemplate[4].szValue = userRow->szColValue[2];
        }
        else
        {
            banTemplate[2].szValue = "0";
            banTemplate[3].szValue = "guest";
            banTemplate[4].szValue = "Anonymous";
        }

        // Use different template for one log entry and list of log entries
        if( httpVarId == 0 )
            // Parse one row
            template_iparse( req, TEMPLATE_BAN_VIEW_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, banTemplate );
        else
            template_iparse( req, TEMPLATE_BAN_VIEW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, banTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );

        // Free rows after parsing the template
        database_freeRows( userRow );

        // Select next row
        curRow = curRow->next;
        if( curRow == 0 )
            break;
    }

    // Free memory from query result
    database_freeRows( banRow );

    // Make a frame around rows if no id specified
    if( httpVarId == 0 )
    {
        template_iparse( req, TEMPLATE_BAN_VIEW_STRUCTURE, req->lpszTemp, NNCMS_PAGE_SIZE_MAX, structureTemplate );
        frameTemplate[1].szValue = req->lpszTemp;
    }

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void ban_edit( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Edit Ban";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/status/security-medium.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS editTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "ban_id", /* szValue */ 0 },
            { /* szName */ "ban_user_id", /* szValue */ 0 },
            { /* szName */ "ban_user_name", /* szValue */ 0 },
            { /* szName */ "ban_user_nick", /* szValue */ 0 },
            { /* szName */ "ban_timestamp", /* szValue */ 0 },
            { /* szName */ "ban_ip", /* szValue */ 0 },
            { /* szName */ "ban_mask", /* szValue */ 0 },
            { /* szName */ "ban_reason", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    editTemplate[10].szValue = req->g_sessionid;

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "ban", req->g_username, "edit" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Ban::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get file name
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "ban_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No id specified";
        log_printf( req, LOG_ERROR, "Ban::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Find rows associated with our object by 'id'
    database_bind_text( req->stmtFindBanById, 1, httpVarId->lpszValue );
    struct NNCMS_ROW *banRow = database_steps( req->stmtFindBanById );
    if( banRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Ban not found";
        log_printf( req, LOG_ERROR, "Ban::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "ban_edit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get POST data
        struct NNCMS_VARIABLE *httpVarIp = main_get_variable( req, "ban_ip" );
        struct NNCMS_VARIABLE *httpVarMask = main_get_variable( req, "ban_mask" );
        struct NNCMS_VARIABLE *httpVarReason = main_get_variable( req, "ban_reason" );
        if( httpVarIp == 0 ||
            httpVarMask == 0 ||
            httpVarReason == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_ERROR, "Ban::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Check if address written correctly
        struct in_addr addr;
        if( inet_aton( httpVarIp->lpszValue, &addr ) == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Invalid ip address";
            log_printf( req, LOG_ERROR, "Ban::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }
        if( inet_aton( httpVarMask->lpszValue, &addr ) == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Invalid mask address";
            log_printf( req, LOG_ERROR, "Ban::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Query Database
        database_bind_text( req->stmtEditBan, 1, httpVarIp->lpszValue );
        database_bind_text( req->stmtEditBan, 2, httpVarMask->lpszValue );
        database_bind_text( req->stmtEditBan, 3, httpVarReason->lpszValue );
        database_bind_text( req->stmtEditBan, 4, httpVarId->lpszValue );
        database_steps( req->stmtEditBan );
        log_printf( req, LOG_ACTION, "Ban::Edit: Edited ban where id=\"%s\"", httpVarId->lpszValue );

        // Redirect back
        redirect_to_referer( req );
        database_freeRows( banRow );
        return;
    }

    // Fill template values
    editTemplate[2].szValue = banRow->szColValue[0];
    editTemplate[7].szValue = banRow->szColValue[3];
    editTemplate[8].szValue = banRow->szColValue[4];
    editTemplate[9].szValue = banRow->szColValue[5];

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( banRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, 64, szTimestampFormat, timeInfo );
    editTemplate[6].szValue = szTime;

    // Get user
    database_bind_text( req->stmtFindUserById, 1, banRow->szColValue[1] ); // id
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );
    if( userRow != 0 )
    {
        editTemplate[3].szValue = banRow->szColValue[1];
        editTemplate[4].szValue = userRow->szColValue[1];
        editTemplate[5].szValue = userRow->szColValue[2];
    }
    else
    {
        editTemplate[3].szValue = "0";
        editTemplate[4].szValue = "guest";
        editTemplate[5].szValue = "Anonymous";
    }

    // Load editor
    template_iparse( req, TEMPLATE_BAN_EDIT, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, editTemplate );

    // Clean up
    database_freeRows( banRow );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void ban_delete( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Delete Ban";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/edit-delete.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS delTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "ban_id", /* szValue */ 0 },
            { /* szName */ "ban_user_id", /* szValue */ 0 },
            { /* szName */ "ban_user_name", /* szValue */ 0 },
            { /* szName */ "ban_user_nick", /* szValue */ 0 },
            { /* szName */ "ban_timestamp", /* szValue */ 0 },
            { /* szName */ "ban_ip", /* szValue */ 0 },
            { /* szName */ "ban_mask", /* szValue */ 0 },
            { /* szName */ "ban_reason", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    delTemplate[10].szValue = req->g_sessionid;

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "ban", req->g_username, "delete" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Ban::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Try to get post id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "ban_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No data";
        log_printf( req, LOG_NOTICE, "Ban::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter evil data
    unsigned int uBanId = atoi( httpVarId->lpszValue );

    // Ok, try to find selected post id
    database_bind_int( req->stmtFindBanById, 1, uBanId );
    struct NNCMS_ROW *banRow = database_steps( req->stmtFindBanById );
    if( banRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Ban not found";
        log_printf( req, LOG_NOTICE, "Ban::Delete: %s (id = %u)", frameTemplate[1].szValue, uBanId );
        goto output;
    }

    // Did user pressed button?
    struct NNCMS_VARIABLE *httpVarDelete = main_get_variable( req, "ban_delete" );
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
        database_bind_int( req->stmtDeleteBan, 1, uBanId );
        database_steps( req->stmtDeleteBan );
        log_printf( req, LOG_ACTION, "Ban::Delete: Ban #%u deleted", uBanId );

        // Redirect back
        redirect_to_referer( req );
        database_freeRows( banRow );
        return;
    }

    // Fill template values
    delTemplate[2].szValue = banRow->szColValue[0];
    delTemplate[7].szValue = banRow->szColValue[3];
    delTemplate[8].szValue = banRow->szColValue[4];
    delTemplate[9].szValue = banRow->szColValue[5];

    // Get time string
    char szTime[64];
    time_t rawTime = atoi( banRow->szColValue[2] );
    struct tm *timeInfo = localtime( &rawTime );
    strftime( szTime, 64, szTimestampFormat, timeInfo );
    delTemplate[6].szValue = szTime;

    // Get user
    database_bind_text( req->stmtFindUserById, 1, banRow->szColValue[1] ); // id
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUserById );
    if( userRow != 0 )
    {
        delTemplate[3].szValue = banRow->szColValue[1];
        delTemplate[4].szValue = userRow->szColValue[1];
        delTemplate[5].szValue = userRow->szColValue[2];
    }
    else
    {
        delTemplate[3].szValue = "0";
        delTemplate[4].szValue = "guest";
        delTemplate[5].szValue = "Anonymous";
    }

    // Load editor
    template_iparse( req, TEMPLATE_BAN_DELETE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, delTemplate );

    database_freeRows( banRow );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################
