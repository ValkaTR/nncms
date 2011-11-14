// #############################################################################
// Source file: acl.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "acl.h"
#include "main.h"
#include "database.h"
#include "filter.h"
#include "user.h"
#include "template.h"
#include "log.h"
#include "smart.h"

#include "gettok.h"

// #############################################################################
// global variables
//

// #############################################################################
// functions

bool acl_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmtAddACL = database_prepare( "INSERT INTO `acl` VALUES(null, ?, ?, ?)" );
    req->stmtFindACLById = database_prepare( "SELECT `id`, `name`, `subject`, `perm` FROM `acl` WHERE `id`=?" );
    req->stmtFindACLByName = database_prepare( "SELECT `id`, `subject`, `perm` FROM `acl` WHERE `name`=?" );
    req->stmtEditACL = database_prepare( "UPDATE `acl` SET `name`=?, `subject`=?, `perm`=? WHERE `id`=?" );
    req->stmtDeleteACL = database_prepare( "DELETE FROM `acl` WHERE `id`=?" );
    req->stmtDeleteObject = database_prepare( "DELETE FROM `acl` WHERE `name`=?" );
    req->stmtGetACLId = database_prepare( "SELECT `id` FROM `acl` WHERE `name`=?" );
    req->stmtGetObject = database_prepare( "SELECT `subject`, `perm` FROM `acl` WHERE `name`=?" );
    req->stmtListACL = database_prepare( "SELECT `name` FROM `acl` ORDER BY `name` asc" );
    req->stmtFindUser = database_prepare( "SELECT `group` FROM `users` WHERE `name`=?" );

    return true;
}

// #############################################################################

bool acl_deinit( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req->stmtAddACL );
    database_finalize( req->stmtFindACLById );
    database_finalize( req->stmtFindACLByName );
    database_finalize( req->stmtEditACL );
    database_finalize( req->stmtDeleteACL );
    database_finalize( req->stmtDeleteObject );
    database_finalize( req->stmtGetACLId );
    database_finalize( req->stmtGetObject );
    database_finalize( req->stmtListACL );
    database_finalize( req->stmtFindUser );

    return true;
}

// #############################################################################

void acl_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Add ACL";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/preferences-system-authentication.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS addTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ 0 },
            { /* szName */ "homeURL", /* szValue */ 0 },
            { /* szName */ "acl_object", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    addTemplate[3].szValue = req->g_sessionid;

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "acl", req->g_username, "add" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        goto output;
    }

    // Get file name
    struct NNCMS_VARIABLE *httpVarObject = main_get_variable( req, "acl_object" );
    if( httpVarObject != 0 )
        filter_table_replace( httpVarObject->lpszValue, (unsigned int) strlen( httpVarObject->lpszValue ), queryFilter );

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "acl_add" );
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
        struct NNCMS_VARIABLE *httpVarObject = main_get_variable( req, "acl_object" );
        struct NNCMS_VARIABLE *httpVarSubject = main_get_variable( req, "acl_subject" );
        struct NNCMS_VARIABLE *httpVarPerm = main_get_variable( req, "acl_perm" );
        if( httpVarObject == 0 ||
            httpVarSubject == 0 ||
            httpVarPerm == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data (You fail)";
            goto output;
        }

        // Filter
        filter_table_replace( httpVarObject->lpszValue, (unsigned int) strlen( httpVarObject->lpszValue ), queryFilter );
        filter_table_replace( httpVarSubject->lpszValue, (unsigned int) strlen( httpVarSubject->lpszValue ), queryFilter );
        filter_table_replace( httpVarPerm->lpszValue, (unsigned int) strlen( httpVarPerm->lpszValue ), queryFilter );

        // Query Database
        database_bind_text( req->stmtAddACL, 1, httpVarObject->lpszValue );
        database_bind_text( req->stmtAddACL, 2, httpVarSubject->lpszValue );
        database_bind_text( req->stmtAddACL, 3, httpVarPerm->lpszValue );
        database_steps( req->stmtAddACL );
        log_printf( req, LOG_ACTION, "ACL::Add: Added acl where object=\"%s\"", httpVarObject->lpszValue );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Some static form info
    addTemplate[1].szValue = homeURL;
    if( httpVarObject != 0 )
        addTemplate[2].szValue = httpVarObject->lpszValue;

    // Referer
    addTemplate[0].szValue = FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp );

    // Load editor
    template_iparse( req, TEMPLATE_ACL_ADD, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, addTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void acl_edit( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Edit ACL";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/preferences-system-authentication.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS editTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ 0 },
            { /* szName */ "homeURL", /* szValue */ 0 },
            { /* szName */ "acl_id", /* szValue */ 0 },
            { /* szName */ "acl_object", /* szValue */ 0 },
            { /* szName */ "acl_subject", /* szValue */ 0 },
            { /* szName */ "acl_perm", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    editTemplate[6].szValue = req->g_sessionid;

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "acl", req->g_username, "edit" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "ACL::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get file name
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "acl_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No id specified";
        log_printf( req, LOG_ERROR, "ACL::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Convert id to integer
    //unsigned int nId = atoi( httpVarId->lpszValue );

    // Find rows associated with our object by 'id'
    database_bind_text( req->stmtFindACLById, 1, httpVarId->lpszValue );
    struct NNCMS_ROW *aclRow = database_steps( req->stmtFindACLById );
    if( aclRow == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "ACL not found";
        log_printf( req, LOG_ERROR, "ACL::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "acl_edit" );
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
        struct NNCMS_VARIABLE *httpVarObject = main_get_variable( req, "acl_object" );
        struct NNCMS_VARIABLE *httpVarSubject = main_get_variable( req, "acl_subject" );
        struct NNCMS_VARIABLE *httpVarPerm = main_get_variable( req, "acl_perm" );
        if( httpVarObject == 0 ||
            httpVarSubject == 0 ||
            httpVarPerm == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data (You fail)";
            log_printf( req, LOG_ERROR, "ACL::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Filter
        filter_table_replace( httpVarObject->lpszValue, strlen( httpVarObject->lpszValue ), queryFilter );
        filter_table_replace( httpVarSubject->lpszValue, strlen( httpVarSubject->lpszValue ), queryFilter );
        filter_table_replace( httpVarPerm->lpszValue, strlen( httpVarPerm->lpszValue ), queryFilter );

        // Query Database
        database_bind_text( req->stmtEditACL, 1, httpVarObject->lpszValue );
        database_bind_text( req->stmtEditACL, 2, httpVarSubject->lpszValue );
        database_bind_text( req->stmtEditACL, 3, httpVarPerm->lpszValue );
        database_bind_text( req->stmtEditACL, 4, httpVarId->lpszValue );
        database_steps( req->stmtEditACL );
        log_printf( req, LOG_ACTION, "ACL::Edit: Edited acl where id=\"%s\"", httpVarId->lpszValue );

        // Redirect back
        redirect_to_referer( req );
        database_freeRows( aclRow );
        return;
    }

    // Retreive data of acl
    editTemplate[2].szValue = aclRow->szColValue[0];
    editTemplate[3].szValue = aclRow->szColValue[1];
    editTemplate[4].szValue = aclRow->szColValue[2];
    editTemplate[5].szValue = aclRow->szColValue[3];

    // Referer
    editTemplate[0].szValue = FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp );

    // homeURL
    editTemplate[1].szValue = homeURL;

    // Load editor
    template_iparse( req, TEMPLATE_ACL_EDIT, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, editTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

    // Clean up
    database_freeRows( aclRow );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void acl_delete( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Delete ACL";

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
            { /* szName */ "referer", /* szValue */ 0 },
            { /* szName */ "homeURL", /* szValue */ 0 },
            { /* szName */ "acl_id", /* szValue */ 0 },
            { /* szName */ "acl_object", /* szValue */ 0 },
            { /* szName */ "acl_subject", /* szValue */ 0 },
            { /* szName */ "acl_perm", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    delTemplate[6].szValue = req->g_sessionid;

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "acl", req->g_username, "delete" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "ACL::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get id or object
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "acl_id" );
    struct NNCMS_VARIABLE *httpVarObject = main_get_variable( req, "acl_object" );
    struct NNCMS_ROW *aclRow = 0;
    if( httpVarId != 0 && *httpVarId->lpszValue != 0 )
    {
        //
        // Deleting by Id
        //

        // Find rows associated with our object by 'id'
        database_bind_text( req->stmtFindACLById, 1, httpVarId->lpszValue );
        aclRow = database_steps( req->stmtFindACLById );
        if( aclRow == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "ACL not found";
            log_printf( req, LOG_ERROR, "ACL::Delete: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Check if user commit changes
        struct NNCMS_VARIABLE *httpVarDelete = main_get_variable( req, "acl_delete" );
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
            database_bind_text( req->stmtDeleteACL, 1, httpVarId->lpszValue );
            database_steps( req->stmtDeleteACL );
            log_printf( req, LOG_ACTION, "ACL::Delete: Deleted acl where id=\"%s\"", httpVarId->lpszValue );

            // Redirect back
            redirect_to_referer( req );
            database_freeRows( aclRow );
            return;
        }

        // Retreive data of acl
        delTemplate[2].szValue = aclRow->szColValue[0];
        delTemplate[3].szValue = aclRow->szColValue[1];
        delTemplate[4].szValue = aclRow->szColValue[2];
        delTemplate[5].szValue = aclRow->szColValue[3];
    }
    else if( httpVarObject != 0 && *httpVarObject->lpszValue != 0 )
    {
        //
        // Deleting by Object name
        //

        // Find rows
        database_bind_text( req->stmtGetACLId, 1, httpVarObject->lpszValue );
        aclRow = database_steps( req->stmtGetACLId );
        if( aclRow == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Object not found";
            log_printf( req, LOG_ERROR, "ACL::Delete: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Check if user commit changes
        struct NNCMS_VARIABLE *httpVarDelete = main_get_variable( req, "acl_delete" );
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
            database_bind_text( req->stmtDeleteObject, 1, httpVarObject->lpszValue );
            database_steps( req->stmtDeleteObject );
            log_printf( req, LOG_ACTION, "ACL::Delete: Deleted acl where object=\"%s\"", httpVarObject->lpszValue );

            // Redirect back
            redirect_to_referer( req );
            database_freeRows( aclRow );
            return;
        }

        delTemplate[2].szValue = "";
        delTemplate[3].szValue = httpVarObject->lpszValue;
        delTemplate[4].szValue = "";
        delTemplate[5].szValue = "";
    }
    else
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No id/object specified";
        log_printf( req, LOG_ERROR, "ACL::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Referer
    delTemplate[0].szValue = FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp );

    // Static homeURL
    delTemplate[1].szValue = homeURL;

    // Load editor
    template_iparse( req, TEMPLATE_ACL_DELETE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, delTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

    database_freeRows( aclRow );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

bool acl_check_perm( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszSubject, char *lpszPerm )
{
    // Default check result
    bool bResult = false;

    // Find rows associated with our object by 'name'
    database_bind_text( req->stmtGetObject, 1, lpszName );
    struct NNCMS_ROW *aclRow = database_steps( req->stmtGetObject );
    if( aclRow == 0 ) return bResult;

    // Find user and get its groups
    database_bind_text( req->stmtFindUser, 1, lpszSubject );
    struct NNCMS_ROW *userRow = database_steps( req->stmtFindUser );

    // Loop thru all rows
    struct NNCMS_ROW *curRow = aclRow;
    while( curRow != 0 )
    {
        // List thru acl subjects
        //for( char *saveptr1 = NULL, *str1 = curRow->szColValue[0]; ; str1 = NULL )
        //{
        //    char *token1 = strtok_r( str1, " ", &saveptr1 );
        //    if( token1 == NULL ) break;
        //
        //    // Compare acl subject with requested subject
        //    //if( strcmp( token1, lpszSubject ) == 0 ) goto check_perms;
        //
        //    // Complare acl subject with user's groups
        //    if( userRow == 0 ) continue;
        //   for( char *saveptr2 = NULL, *str2 = userRow->szColValue[0]; ; str2 = NULL )
        //    {
        //        char *token2 = strtok_r( str2, " ", &saveptr2 );
        //        if( token2 == NULL ) break;
        //        if( strcmp( token1, token2 ) == 0 ) goto check_perms;
        //    }
        //}

        // Compare acl subject with requested subject
        if( strcmp( curRow->szColValue[0], lpszSubject ) == 0 ) goto check_perms;

        // Complare acl subject with user's group
        if( strcmp( curRow->szColValue[0], userRow->szColValue[0] ) == 0 ) goto check_perms;

        // Select next row
        curRow = curRow->next;
    }
    goto check_perm_exit;

check_perms:
    // Check for full access
    if( curRow->szColValue[1][0] == '*' )
    {
        bResult = true;
        goto check_perm_exit;
    }

    // List thru acl perms
    for( char *saveptr1 = NULL, *str1 = curRow->szColValue[1]; ; str1 = NULL )
    {
        char *token1 = strtok_r( str1, " ", &saveptr1 );
        if( token1 == NULL ) break;

        // Compare acl perm with requested perm
        if( strcmp( token1, lpszPerm ) == 0 )
        {
            bResult = true;
            goto check_perm_exit;
        }
    }

check_perm_exit:
    database_freeRows( aclRow );
    database_freeRows( userRow );
    return bResult;
}

// #############################################################################

void acl_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Access Control List";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/preferences-system-authentication.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS aclTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ 0 },
            { /* szName */ "acl_id", /* szValue */ 0 },
            { /* szName */ "acl_object", /* szValue */ 0 },
            { /* szName */ "acl_subject", /* szValue */ 0 },
            { /* szName */ "acl_perm", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS objectTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ 0 },
            { /* szName */ "acl_object", /* szValue */ 0 },
            { /* szName */ "acl_entries", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS footTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ 0 },
            { /* szName */ "acl_object", /* szValue */ 0 },
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
    if( acl_check_perm( req, "acl", req->g_username, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "ACL::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get object name
    struct NNCMS_VARIABLE *httpVarObject = main_get_variable( req, "acl_object" );
    if( httpVarObject == 0 )
    {
        // Show list of objects
        struct NNCMS_ROW *aclRow = database_steps( req->stmtListACL );
        if( aclRow == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No ACL data";
            log_printf( req, LOG_CRITICAL, "ACL::View: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Load ACL header
        template_iparse( req, TEMPLATE_ACL_OBJECT_HEAD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
        smart_cat( &smartBuffer, req->lpszFrame );

        // homeURL for template
        objectTemplate[0].szValue = homeURL;

        // Loop thru all rows
        unsigned int nEntryCount = 1;
        struct NNCMS_ROW *curRow = aclRow;
        while( 1 )
        {
            // Count entries and skip dublicates
            if( curRow->next != 0 )
            {
                if( strcmp( curRow->szColValue[0],
                    curRow->next->szColValue[0] ) == 0 )
                {
                    curRow = curRow->next;
                    nEntryCount++;
                    continue;
                }
            }

            // Fill template values
            objectTemplate[1].szValue = curRow->szColValue[0];
            char szEntryCount[32]; snprintf( szEntryCount, sizeof(szEntryCount), "%u", nEntryCount );
            objectTemplate[2].szValue = szEntryCount;

            // Parse one row
            template_iparse( req, TEMPLATE_ACL_OBJECT_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, objectTemplate );
            smart_cat( &smartBuffer, req->lpszFrame );

            // Select next row
            nEntryCount = 1;
            curRow = curRow->next;
            if( curRow == 0 )
                break;
        }

        // Free memory from query result
        database_freeRows( aclRow );

        // Add footer
        footTemplate[0].szValue = homeURL;
        footTemplate[1].szValue = "";
        template_iparse( req, TEMPLATE_ACL_OBJECT_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, footTemplate );
    }
    else
    {
        // Filter object name from pure evil
        filter_table_replace( httpVarObject->lpszValue, (unsigned int) strlen( httpVarObject->lpszValue ), pathFilter );

        // Find rows associated with our object by 'name'
        database_bind_text( req->stmtFindACLByName, 1, httpVarObject->lpszValue );
        struct NNCMS_ROW *aclRow = database_steps( req->stmtFindACLByName );
        if( aclRow == 0 )
        {
            // If object not found, then ask user to add
            // entry to this object to make it exists
            redirectf( req, "%s/acl_add.fcgi?acl_object=%s", homeURL, httpVarObject->lpszValue );
            return;
        }

        // Load ACL header
        template_iparse( req, TEMPLATE_ACL_VIEW_HEAD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
        smart_cat( &smartBuffer, req->lpszFrame );

        // homeURL and Object name are same for every row
        aclTemplate[0].szValue = homeURL;
        aclTemplate[2].szValue = httpVarObject->lpszValue;

        // Loop thru all rows
        struct NNCMS_ROW *curRow = aclRow;
        while( 1 )
        {
            // Fill template values
            aclTemplate[1].szValue = curRow->szColValue[0];
            aclTemplate[3].szValue = curRow->szColValue[1];
            aclTemplate[4].szValue = curRow->szColValue[2];

            // Parse one row
            template_iparse( req, TEMPLATE_ACL_VIEW_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, aclTemplate );
            smart_cat( &smartBuffer, req->lpszFrame );

            // Select next row
            curRow = curRow->next;
            if( curRow == 0 )
                break;
        }

        // Free memory from query result
        database_freeRows( aclRow );

        // Add footer
        footTemplate[0].szValue = homeURL;
        footTemplate[1].szValue = httpVarObject->lpszValue;
        template_iparse( req, TEMPLATE_ACL_VIEW_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, footTemplate );
    }

    smart_cat( &smartBuffer, req->lpszFrame );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################
