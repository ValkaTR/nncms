// #############################################################################
// Source file: cfg.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "cfg.h"
#include "main.h"
#include "template.h"
#include "user.h"
#include "acl.h"
#include "log.h"
#include "smart.h"

#include "strlcpy.h"
#include "strlcat.h"

#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32)
#else
#include <sys/file.h>
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

void cfg_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Configuration view";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/utilities-terminal.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS cfgTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "cfg_id", /* szValue */ 0 },
            { /* szName */ "cfg_name", /* szValue */ 0 },
            { /* szName */ "cfg_value", /* szValue */ 0 },
            { /* szName */ "cfg_type", /* szValue */ 0 },
            { /* szName */ "cfg_lpmem", /* szValue */ 0 },
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
    if( acl_check_perm( req, "cfg", req->g_username, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Config::View: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Header template
    template_iparse( req, TEMPLATE_CFG_VIEW_HEAD, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
    smart_cat( &smartBuffer, req->lpszFrame );

    // Loop thru all rows
    for( unsigned int i = 0; i < NNCMS_CFG_OPTIONS_MAX; i++ )
    {
        // Skip empty config entries
        if( g_options[i].lpMem == 0 ) break;

        // Fill template values
        char szId[64]; sprintf( szId, "%u", i );
        cfgTemplate[1].szValue = szId;
        cfgTemplate[2].szValue = g_options[i].szName;
        switch( g_options[i].nType )
        {
            case NNCMS_INTEGER:
            {
                char szCfgValue[64]; sprintf( szCfgValue, "%i", *(int *) g_options[i].lpMem );
                cfgTemplate[3].szValue = szCfgValue;
                cfgTemplate[4].szValue = "Integer";
                break;
            }
            case NNCMS_STRING:
            {
                cfgTemplate[3].szValue = g_options[i].lpMem;
                cfgTemplate[4].szValue = "String";
                break;
            }
            case NNCMS_BOOL:
            {
                if( *(bool *) g_options[i].lpMem == true )
                {
                    cfgTemplate[3].szValue = "True";
                }
                else if( *(bool *) g_options[i].lpMem == false )
                {
                    cfgTemplate[3].szValue = "False";
                }
                cfgTemplate[4].szValue = "Boolean";
                break;
            }
        }
        char szLpMem[64]; sprintf( szLpMem, "%08Xh", (unsigned int) g_options[i].lpMem );
        cfgTemplate[5].szValue = szLpMem;

        // Parse one row
        template_iparse( req, TEMPLATE_CFG_VIEW_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, cfgTemplate );
        smart_cat( &smartBuffer, req->lpszFrame );
    }

    // Footer template
    template_iparse( req, TEMPLATE_CFG_VIEW_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
    smart_cat( &smartBuffer, req->lpszFrame );

output:
    // Make a cute frame
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

bool cfg_parse_file( const char *lpszFileName, struct NNCMS_OPTION *lpOptions )
{
    if( lpOptions == 0 )
        return false;

    // Try to open file for reading
    FILE *pFile = fopen( lpszFileName, "rb" );
    if( pFile == 0 )
    {
        // Oops, no file
        return false;
    }

    // Obtain file size.
    fseek( pFile, 0, SEEK_END );
    unsigned int lSize = ftell( pFile );
    rewind( pFile );

    // Allocate memory to contain the whole file.
    char *szTemp = (char *) MALLOC( lSize );
    if( szTemp == 0 ) return false;

    // Copy the file into the buffer.
    fread( szTemp, 1, lSize, pFile );

    // Close the file associated with the specified pFile stream after flushing
    // all buffers associated with it.
    fclose (pFile);

    //
    // The whole file is loaded in the buffer.
    // Let's begin parsing
    //

    // All Lua contexts are held in this structure. We work with it almost
    // all the time.
    lua_State *L = luaL_newstate( );
    if( L == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Cfg::ParseFile: Cannot create state: not enough memory" );
        return false;
    }

    // Load Lua libraries
    luaL_openlibs( L );

    // Call a script
    int nLoadResult = luaL_loadbuffer( L, szTemp, lSize, "cfg" );
    if( nLoadResult == 0 )
    {
        lua_pcall( L, 0, LUA_MULTRET, 0 );
    }
    else
    {
        // Get error string
        char *lpStackMsg = (char *) lua_tostring( L, -1 );
        lua_pop( L, 1 );
        log_printf( 0, LOG_ERROR, "Cfg::ParseFile: Lua error: %s", lpStackMsg );
        lua_close( L );
        return false;
    }

    // Check if all is ok
    int luaStackTop = lua_gettop( L );
    if( luaStackTop != 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Cfg::ParseFile: Lua stack is not empty, position %i", luaStackTop );

        // Debug
        lua_stackdump( L );

        // Empty the stack
        lua_settop( L, 0 );
    }

    // Ok, loop thru all options getting data from lua
    for( unsigned int i = 0; i < NNCMS_CFG_OPTIONS_MAX; i++ )
    {
        // Terminating option
        if( lpOptions[i].szName[0] == 0 ) break;
        if( lpOptions[i].lpMem == 0 )
            goto error;

        // Pushes onto the stack the value
        lua_getfield( L, LUA_GLOBALSINDEX, lpOptions[i].szName );

        // If variable is not defined in configuration file
        if( lua_isnil( L, 1 ) )
            goto next;

        // Option found, check it's type
        switch( lpOptions[i].nType )
        {
            case NNCMS_INTEGER:
            {
                // Convert string to integer
                lua_Integer szVarValue = lua_tointeger( L, 1 );
                *(int *) lpOptions[i].lpMem = szVarValue;
                break;
            }
            case NNCMS_STRING:
            {
                char *szVarValue = lua_tostring( L, 1 );
                strcpy( (char *) lpOptions[i].lpMem, szVarValue );
                break;
            }
            case NNCMS_BOOL:
            {
                int szVarValue = lua_toboolean( L, 1 );
                *(bool *) lpOptions[i].lpMem = szVarValue;
                break;
            }
            default:
            {
                log_printf( 0, LOG_EMERGENCY, "Cfg::ParseFile: Unknow option type" );
                goto error;
            }
        }

next:
        lua_pop( L, 1 );
    }

    // Cya, Lua
    lua_close( L );
    return true;

error:
    // Error
    lua_close( L );
    return false;
}

// #############################################################################

// Edit cfg
void cfg_edit( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Edit config";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/apps/utilities-terminal.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS editTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "cfg_id", /* szValue */ 0 },
            { /* szName */ "cfg_name", /* szValue */ 0 },
            { /* szName */ "cfg_value", /* szValue */ 0 },
            { /* szName */ "cfg_type", /* szValue */ 0 },
            { /* szName */ "cfg_lpmem", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    editTemplate[3].szValue = req->g_sessionid;

    // First we check what permissions do user have
    if( acl_check_perm( req, "cfg", req->g_username, "edit" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "Config::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get id
    struct NNCMS_VARIABLE *httpVarId = main_get_variable( req, "cfg_id" );
    if( httpVarId == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No id specified";
        log_printf( req, LOG_NOTICE, "Cfg::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check boundary
    unsigned int nCfgId = atoi( httpVarId->lpszValue );
    if( nCfgId > NNCMS_CFG_OPTIONS_MAX )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Out of limits";
        log_printf( req, LOG_NOTICE, "Cfg::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Is id known?
    if( g_options[nCfgId].lpMem == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Unknown config id";
        log_printf( req, LOG_NOTICE, "Cfg::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "cfg_edit" );
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
        struct NNCMS_VARIABLE *httpVarValue = main_get_variable( req, "cfg_value" );
        if( httpVarValue == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_ERROR, "Cfg::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Apply changes
        switch( g_options[nCfgId].nType )
        {
            case NNCMS_INTEGER:
            {
                *(int *) g_options[nCfgId].lpMem = atoi( httpVarValue->lpszValue );
                break;
            }
            case NNCMS_STRING:
            {
                strlcpy( g_options[nCfgId].lpMem, httpVarValue->lpszValue, NNCMS_PATH_LEN_MAX );
                break;
            }
            case NNCMS_BOOL:
            {
                if( strcasecmp( httpVarValue->lpszValue, "1" ) == 0 ||
                    strcasecmp( httpVarValue->lpszValue, "true" ) == 0 )
                {
                    *(bool *) g_options[nCfgId].lpMem = true;
                }
                else if( strcasecmp( httpVarValue->lpszValue, "0" ) == 0 ||
                         strcasecmp( httpVarValue->lpszValue, "false" ) == 0 )
                {
                    *(bool *) g_options[nCfgId].lpMem = false;
                }
                break;
            }
        }

        // Log action
        log_printf( req, LOG_ACTION, "Cfg::Edit: Editted \"%s\" variable", g_options[nCfgId].szName);

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Set template data
    editTemplate[2].szValue = httpVarId->lpszValue;
    editTemplate[3].szValue = g_options[nCfgId].szName;
    switch( g_options[nCfgId].nType )
    {
        case NNCMS_INTEGER:
        {
            char szCfgValue[64]; sprintf( szCfgValue, "%i", *(int *) g_options[nCfgId].lpMem );
            editTemplate[4].szValue = szCfgValue;
            editTemplate[5].szValue = "Integer";
            break;
        }
        case NNCMS_STRING:
        {
            editTemplate[4].szValue = g_options[nCfgId].lpMem;
            editTemplate[5].szValue = "String";
            break;
        }
        case NNCMS_BOOL:
        {
            if( *(bool *) g_options[nCfgId].lpMem == true )
            {
                editTemplate[4].szValue = "True";

            }
            else if( *(bool *) g_options[nCfgId].lpMem == false )
            {
                editTemplate[4].szValue = "False";
            }
            editTemplate[5].szValue = "Boolean";
            break;
        }
    }
    char szLpMem[64]; sprintf( szLpMem, "%08Xh", (unsigned int) g_options[nCfgId].lpMem );
    editTemplate[6].szValue = szLpMem;

    // Load editor
    template_iparse( req, TEMPLATE_CFG_EDIT, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, editTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################
