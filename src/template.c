// #############################################################################
// Source file: template.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#define _GNU_SOURCE

#include "template.h"
#include "file.h"
#include "filter.h"
#include "main.h"
#include "log.h"
#include "smart.h"
#include "acl.h"
#include "user.h"
#include "post.h"
#include "i18n.h"
#include "threadinfo.h"
#include "database.h"

#include "strlcpy.h"
#include "strlcat.h"
#include "strdup.h"
#include "dirname_r.h"

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

// Path to tempates
char templateDir[NNCMS_PATH_LEN_MAX] = "/templates/";

// For fast access to templates
GHashTable *hashed_templates = NULL;

// No cache option
bool bTemplateCache = true;

// Default number of items per page
int default_pager_quantity = 25;

// #############################################################################
// functions

static int template_lua_parse( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Template name
    char *template_name = (char *) luaL_checkstring( L, 1 );
    if( template_name == NULL )
    {
        log_print( req, LOG_WARNING, "No template name" );
        lua_pushstring( L, "<p><font color=\"red\">No template name</font></p>" );
        return 1;
    }
    
    // Get string to be parsed
    char *template_data = (char *) luaL_checkstring( L, 2 );
    if( template_data == NULL )
    {
        log_print( req, LOG_WARNING, "No data" );
        lua_pushstring( L, "<p><font color=\"red\">No data</font></p>" );
        return 1;
    }

    // Specify values for template
    struct NNCMS_VARIABLE template_tags[] =
        {
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "user_id", .value.string = req->user_id, .type = NNCMS_TYPE_STRING },
            { .name = "user_name", .value.string = req->user_name, .type = NNCMS_TYPE_STRING },
            { .name = "user_avatar", .value.string = req->user_avatar, .type = NNCMS_TYPE_STRING },
            { .name = "user_folder_id", .value.string = req->user_folder_id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    GString *buf_out = g_string_sized_new( 16 );    
    template_sparse( req, template_name, template_data, buf_out, template_tags );

    // Push the text in the Lua stack
    lua_pushlstring( L, buf_out->str, buf_out->len );
    g_string_free( buf_out, true );

    return 1;  /* number of results */
}

// #############################################################################

static int template_lua_load( lua_State *L )
{
    //double d = lua_tonumber( L, 1 );  /* get argument */
    //lua_pushnumber( L, sin( d ) );  /* push result */

    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Check if filename specified
    char *lpszText = (char *) luaL_checkstring( L, 1 );
    if( lpszText == NULL )
    {
        log_print( req, LOG_WARNING, "Filename not specified" );
        lua_pushstring( L, "<p><font color=\"red\">Filename not specified</font></p>" );
        return 1;
    }

    // Make a full path name
    char szFileName[NNCMS_PATH_LEN_MAX];
    snprintf( szFileName, sizeof(szFileName), "%s%s", fileDir, lpszText );

    // Get file info
    struct NNCMS_FILE_INFO fileInfo;
    file_get_info( &fileInfo, lpszText );
    if( fileInfo.bExists == false )
    {
        log_printf( req, LOG_ERROR, "Stat failed on block '%s' file", szFileName );
        lua_pushfstring( L, "<p><font color=\"red\">Stat failed on block '%s' file</font></p>", szFileName );
        return 1; //char *lpszOutput = MALLOC( NNCMS_PATH_LEN_MAX + 64 ); snprintf( lpszOutput, NNCMS_PATH_LEN_MAX + 64, ); lua_pushstring( L, 1 );
    }

    // To show or not to show
    //if( file_check_perm( req, &fileInfo, "download" ) == false )
    if( 0 )
    {
        // Report or not to report, get default value
        bool bQuiet = bIncludeDefaultQuiet;

        //bQuiet = true;

        // Access denied
        if( bQuiet == false )
        {
            lua_pushfstring( L,  "<p><font color=\"red\">Access denied to '%s' file</font></p>", szFileName );
            return 1;
        }
        else
        {
            // Silently
            lua_pushstring( L,  "" );
            return 1;
        }
    }

    // Include a file
    size_t nSize = NNCMS_PAGE_SIZE_MAX;
    char *lpszOutput = main_get_file( szFileName, &nSize );

    // Push the file text in the Lua stack
    if( lpszOutput == 0 )
    {
        // Error
        lua_pushfstring( L,  "<p><font color=\"red\">Unable to open '%s' file</font></p>", szFileName );
    }
    else
    {
        lua_pushlstring( L, lpszOutput, nSize );
        FREE( lpszOutput );
    }

    return 1;  /* number of results */
}

// #############################################################################

static int template_lua_escape_html( lua_State *L )
{
    // Get req pointer
#ifdef DEBUG
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }
#else
    struct NNCMS_THREAD_INFO *req = 0;
#endif

    // Check if text specified
    char *lpszText = (char *) lua_tostring( L, 1 );
    if( lpszText == NULL )
    {
        //log_print( req, LOG_WARNING, "Text not specified" );
        //lua_pushstring( L, "<p><font color=\"red\">Text not specified</font></p>" );
        lua_pushstring( L, "" );
        return 1;
    }

    // Convert
    char *lpszOutput = template_escape_html( lpszText );

    // Return back to Lua
    lua_pushstring( L, lpszOutput );
    g_free( lpszOutput );

    // Number of results
    return 1;
}

// #############################################################################

static int template_lua_i18n_string( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Check if text specified
    char *source = (char *) lua_tostring( L, 1 );
    if( source == NULL )
    {
        //log_print( req, LOG_WARNING, "Text not specified" );
        //lua_pushstring( L, "<p><font color=\"red\">Text not specified</font></p>" );
        //lua_pushstring( L, "" );
        return 0;
    }
    
    char *localized = i18n_string( req, source, NULL );

    // Return back to Lua
    lua_pushstring( L, localized );

    g_free( localized );

    // Number of results
    return 1;
}

// #############################################################################

static int template_lua_print( lua_State *L )
{
    // Get req pointer
#ifdef DEBUG
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }
#else
    struct NNCMS_THREAD_INFO *req = 0;
#endif

    // Load buffer
    //lua_getfield( L, LUA_GLOBALSINDEX, "buffer" );
    lua_getglobal( L, "buffer" );
    GString *buffer = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( buffer == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No buffer" );
        //lua_pushstring( L, "<p><font color=\"red\">No buffer</font></p>" );
        return 0;
    }

    // Check if text specified
    char *lpszText = (char *) lua_tostring( L, 1 );
    if( lpszText == NULL )
    {
        //log_print( req, LOG_WARNING, "Text not specified" );
        //lua_pushstring( L, "<p><font color=\"red\">Text not specified</font></p>" );
        //lua_pushstring( L, "" );
        return 0;
    }

    // Append text to buffer
    g_string_append( buffer, lpszText );

    // Number of results
    return 0;
}

// #############################################################################

static int template_lua_escape_js( lua_State *L )
{
    // Check if text specified
    char *text = (char *) lua_tostring( L, 1 );
    if( text == NULL )
    {
        lua_pushstring( L, "" );
        return 1;
    }

    // Convert
    GString *escaped = g_string_sized_new( 16 );
    for( size_t i = 0; text[i] != 0; i++ )
    {
        if( text[i] == '"' ) { g_string_append( escaped, "\\\"" ); }
        else if( text[i] == '\'' ) { g_string_append( escaped, "\\'" ); }
        else if( text[i] == '\r' ) { g_string_append( escaped, "\\r" ); }
        else if( text[i] == '\n' ) { g_string_append( escaped, "\\n" ); }
        else if( text[i] == '\\' ) { g_string_append( escaped, "\\\\" ); }
        else if( text[i] == '\t' ) { g_string_append( escaped, "\\t" ); }
        else if( text[i] == 0 )
        {
            break;
        }
        else
        {
            // Copy character by default
            g_string_append_c_inline( escaped, text[i] );
        }
    }

    // Return back to Lua
    lua_pushstring( L, escaped->str );
    
    g_string_free( escaped, TRUE );

    // Number of results
    return 1;
}

// #############################################################################

static int template_lua_escape_uri( lua_State *L )
{
    // Check if text specified
    char *lpszText = (char *) lua_tostring( L, 1 );
    if( lpszText == NULL )
    {
        lua_pushstring( L, "" );
        return 1;
    }

    // Convert
    char *lpszOutput = template_escape_uri( lpszText );

    // Return back to Lua
    lua_pushstring( L, lpszOutput );
    FREE( lpszOutput );

    // Number of results
    return 1;
}

// #############################################################################

// {acl object="acl" permission="edit"}
static int template_lua_acl( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Retrieve parameters
    char *name = (char *) lua_tostring( L, 1 );
    char *subject = (char *) lua_tostring( L, 2 ); // Optional
    char *permission = (char *) lua_tostring( L, 3 );
    if( permission == NULL )
    {
        log_print( req, LOG_WARNING, "Not enought parameters specified" );
        lua_pushboolean( L, 0 );
        return 1;
    }

    // Current user as subject by default
    //if( subject == 0 )
    //    subject = req->user_name;

    // Check ACL
    bool bResult = acl_check_perm( req, name, subject, permission );

    // Return back to Lua
    lua_pushboolean( L, (int) bResult );

    // Number of results
    return 1;
}
// #############################################################################

static int template_lua_post_acl( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Retrieve parameters
    char *lpszPost = (char *) lua_tostring( L, 1 );
    char *permission = (char *) lua_tostring( L, 2 );
    lua_Integer nDepth = lua_tointeger( L, 3 );
    if( lpszPost == NULL || permission == NULL )
    {
        log_print( req, LOG_WARNING, "Not enought parameters specified" );
        lua_pushboolean( L, 0 );
        return 1;
    }

    //if( nDepth == 0 )
    //    nDepth = POST_CHECK_PERM_DEPTH;

    // Check ACL
    bool bResult = false;//post_tree_check_perm( req, lpszPost, permission, nDepth );

    // Return back to Lua
    lua_pushboolean( L, (int) bResult );

    // Number of results
    return 1;
}

// #############################################################################

// {file_acl file="$file_path" permission="rename"}
static int template_lua_file_acl( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Retrieve parameters
    char *lpszFile = (char *) lua_tostring( L, 1 );
    char *permission = (char *) lua_tostring( L, 2 );
    char *subject = (char *) lua_tostring( L, 3 ); // Optional
    if( lpszFile == NULL || permission == NULL )
    {
        log_print( req, LOG_WARNING, "Not enought parameters specified" );
        lua_pushboolean( L, 0 );
        return 1;
    }

    // Current user as subject by default
    if( subject == 0 )
        subject = req->user_name;

    // Get file info
    struct NNCMS_FILE_INFO fileInfo;
    file_get_info( &fileInfo, lpszFile );
    if( fileInfo.bExists == false )
    {
        char *lpszFileName = (fileInfo.bExists == false ? lpszFile : fileInfo.lpszAbsFullPath );
        log_printf( req, LOG_ERROR, "Stat failed on block '%s' file", lpszFileName );
        lua_pushboolean( L, 0 );
        return 1;
    }

     // Check ACL
    bool bResult = true;//file_check_perm( req, &fileInfo, permission );

    // Return back to Lua
    lua_pushboolean( L, (int) bResult );

    // Number of results
    return 1;
}
// #############################################################################

static int template_lua_login( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    //lua_pushlstring( L, "piu", 3 ); return 1;

    // Header
    char *szBlockTitle = "Login";

    // Specify values for template
    struct NNCMS_VARIABLE loginTags[] =
        {
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "referer", .value.string = FCGX_GetParam( "REQUEST_URI", req->fcgi_request->envp ) },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Generate the block
    //template_iparse( req, TEMPLATE_USER_LOGIN, req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX, loginTags );
    GString *buf1 = g_string_sized_new( 128 );
    GString *buf2 = g_string_sized_new( 128 );
    template_hparse( req, "user_login.html", buf1, loginTags );

    // Specify values for template
    struct NNCMS_VARIABLE blockTags[] =
        {
            { .name = "header", .value.string = szBlockTitle, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = buf1->str, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Generate the block
    //size_t nLen = template_iparse( req, TEMPLATE_BLOCK, req->lpszBlockContent, NNCMS_BLOCK_LEN_MAX, blockTags );
    template_hparse( req, "block.html", buf2, blockTags );

    // Return back to Lua
    //lua_pushlstring( L, req->lpszBlockContent, nLen );
    lua_pushlstring( L, buf2->str, buf2->len );

    g_string_free( buf1, TRUE );
    g_string_free( buf2, TRUE );

    // Number of results
    return 1;
}

// #############################################################################

static int template_lua_profile( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_print( 0, LOG_CRITICAL, "No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Header
    char *szBlockTitle = "Profile";

    // Specify values for template
    struct NNCMS_VARIABLE userTags[] =
        {
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "user_name", .value.string = req->user_name, .type = NNCMS_TYPE_STRING },
            { .name = "user_nick", .value.string = req->user_nick, .type = NNCMS_TYPE_STRING },
            { .name = "user_id", .value.string = req->user_id, .type = NNCMS_TYPE_STRING },
            { .name = "user_avatar", .value.string = req->user_avatar, .type = NNCMS_TYPE_STRING },
            { .name = "user_folder_id", .value.string = req->user_folder_id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Generate the block
    //template_iparse( req, TEMPLATE_USER_BLOCK, req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX, userTags );
    GString *buf1 = g_string_sized_new( 128 );
    GString *buf2 = g_string_sized_new( 128 );
    template_hparse( req, "user_block.html", buf1, userTags );

    // Specify values for template
    struct NNCMS_VARIABLE blockTags[] =
        {
            { .name = "header", .value.string = szBlockTitle, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = buf1->str, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Generate the block
    //size_t nLen = template_iparse( req, TEMPLATE_BLOCK, req->lpszBlockContent, NNCMS_BLOCK_LEN_MAX, blockTags );
    template_hparse( req, "block.html", buf2, blockTags );

    // Return back to Lua
    //lua_pushlstring( L, req->lpszBlockContent, nLen );
    lua_pushlstring( L, buf2->str, buf2->len );

    g_string_free( buf1, TRUE );
    g_string_free( buf2, TRUE );

    // Number of results
    return 1;
}

// #############################################################################

bool template_global_init( )
{
    main_local_init_add( &template_local_init );
    main_local_destroy_add( &template_local_destroy );
    
    hashed_templates = g_hash_table_new_full( g_str_hash, g_str_equal, NULL, (GDestroyNotify) g_free );
    
    return true;
}

// #############################################################################

bool template_global_destroy( )
{
    g_hash_table_unref( hashed_templates );
    
    return true;
}

// #############################################################################

bool template_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Trying to avoid allocating large chunks of memory every request
    #define ALLOC_CHUNK(chunk_name,chunk_size) \
        chunk_name = MALLOC( chunk_size ); \
        if( chunk_name == 0 ) \
        { \
            log_printf( req, LOG_ALERT, "Unable to allocate memory for buffer " #chunk_name ", required %u bytes, thread №%i dropped!", chunk_size, req->nThreadID ); \
            return false; \
        }

    ALLOC_CHUNK( req->lpszBuffer, NNCMS_PAGE_SIZE_MAX + 1 )
    ALLOC_CHUNK( req->lpszOutput, NNCMS_PAGE_SIZE_MAX + 1 )
    ALLOC_CHUNK( req->lpszTemp, NNCMS_PAGE_SIZE_MAX + 1 )
    ALLOC_CHUNK( req->lpszFrame, NNCMS_PAGE_SIZE_MAX + 1 )
    ALLOC_CHUNK( req->lpszBlockContent, NNCMS_BLOCK_LEN_MAX + 1 )
    ALLOC_CHUNK( req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX + 1 )
    #undef ALLOC_CHUNK

    req->buffer = g_string_sized_new( 1000 );
    req->frame = g_string_sized_new( 1000 );
    req->temp = g_string_sized_new( 1000 );
    req->output = g_string_sized_new( 1000 );
    
    // All Lua contexts are held in this structure. We work with it almost
    // all the time.
    req->L = luaL_newstate( );

    // Load Lua libraries
    luaL_openlibs( req->L );

    // Variables
    lua_pushlightuserdata( req->L, req ); lua_setglobal( req->L, "req" );

    // Functions
    lua_pushcfunction( req->L, template_lua_load ); lua_setglobal( req->L, "load" );
    lua_pushcfunction( req->L, template_lua_parse ); lua_setglobal( req->L, "parse" );
    lua_pushcfunction( req->L, template_lua_print ); lua_setglobal( req->L, "print" );
    lua_pushcfunction( req->L, template_lua_escape_html ); lua_setglobal( req->L, "escape_html" );
    lua_pushcfunction( req->L, template_lua_escape_uri ); lua_setglobal( req->L, "escape_uri" );
    lua_pushcfunction( req->L, template_lua_escape_js ); lua_setglobal( req->L, "escape_js" );
    lua_pushcfunction( req->L, template_lua_acl ); lua_setglobal( req->L, "acl" );
    lua_pushcfunction( req->L, template_lua_post_acl ); lua_setglobal( req->L, "post_acl" );
    lua_pushcfunction( req->L, template_lua_file_acl ); lua_setglobal( req->L, "file_acl" );
    lua_pushcfunction( req->L, template_lua_login ); lua_setglobal( req->L, "login" );
    lua_pushcfunction( req->L, template_lua_profile ); lua_setglobal( req->L, "profile" );
    lua_pushcfunction( req->L, template_lua_i18n_string ); lua_setglobal( req->L, "i18n_string" );
    
    return true;
}

// #############################################################################

bool template_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free temporary buffers
    SAFE_FREE( req->lpszBuffer );
    SAFE_FREE( req->lpszOutput );
    SAFE_FREE( req->lpszTemp );
    SAFE_FREE( req->lpszFrame );
    SAFE_FREE( req->lpszBlockContent );
    SAFE_FREE( req->lpszBlockTemp );
    
    g_string_free( req->buffer, TRUE );
    g_string_free( req->frame, TRUE );
    g_string_free( req->temp, TRUE );
    g_string_free( req->output, TRUE );

    // Cya, Lua
    lua_close( req->L );
}

// #############################################################################

void lua_stackdump( lua_State *L )
{
    int i;
    int top = lua_gettop( L );
    printf( "== Lua stack dump ==\n" ); /* start the listing */
    for( i = 1; i <= top; i++ )
    {  /* repeat for each level */
        int t = lua_type( L, i );
        printf( "  * " );  /* put a separator */
        switch( t )
        {
            case LUA_TSTRING:  /* strings */
                printf( "%i: " "`%s'" "\n", i, lua_tostring( L, i ) );
                break;

            case LUA_TBOOLEAN:  /* booleans */
                if( lua_toboolean( L, i ) )
                    printf( "%i: true\n", i );
                else
                    printf( "%i: false\n", i );
                break;

            case LUA_TNUMBER:  /* numbers */
                printf( "%i: " "%g" "\n", i, lua_tonumber( L, i ) );
                break;

            default:  /* other values */
                printf( "%i: " "%s" "\n", i, lua_typename( L, t ) );
                break;
        }
    }
    printf( "== End of lua stack dump ==\n\n" );  /* end the listing */
}

// #############################################################################

// This fuction parses template file and template structure
// it replaces tags from structure with its data in template file
// Glib version with templates stored in hash table
size_t template_hparse( struct NNCMS_THREAD_INFO *req, char *template_name, GString *buffer, struct NNCMS_VARIABLE *template_tags )
{
    // Initialize hash table if required
    if( hashed_templates == NULL )
    {
        return 0;
    }
    
    // Find template in hash table
    char *template_data = g_hash_table_lookup( hashed_templates, template_name );
    if( template_data == NULL )
    {
        //
        // Load template if not found and add to hash table
        //
        
        // Create full path to template file
        char template_full_path[NNCMS_PATH_LEN_MAX];
        strlcpy( template_full_path, templateDir, NNCMS_PATH_LEN_MAX );
        strlcat( template_full_path, template_name, NNCMS_PATH_LEN_MAX );

        // Open template
        FILE *template_file = fopen( template_full_path, "rb" );
        if( template_file == 0 )
        {
            // File not found
            log_printf( 0, LOG_ERROR, "Could not open \"%s\" file", template_full_path );
            return 0;
        }

        // Get file size
        fseek( template_file, 0, SEEK_END );
        size_t template_size = ftell( template_file );
        rewind( template_file );

        // Allocate memory
        template_data = g_malloc( template_size + 1 );
        if( template_data == 0 )
        {
            // Out of memory
            log_printf( 0, LOG_ALERT, "Could not allocate %u bytes of memory for \"%s\" file", template_size, template_full_path );
            return 0;
        }

        // Read and close file
        size_t template_actuall_size = fread( template_data, 1, template_size, template_file );
        fclose( template_file );

        // Null terminating character
        template_data[template_actuall_size] = 0;

        if( bTemplateCache != false )
            g_hash_table_insert( hashed_templates, template_name, template_data );
    }
    

    size_t output_size = template_sparse( req, template_name, template_data, buffer, template_tags );

    // Debug
    //printf( TERM_FG_RED "\ntemplate_parse:" TERM_RESET " \n%s\n", szBuffer );
    
    if( bTemplateCache == false )
        g_free( template_data );
        
    return output_size;
}

// #############################################################################

char *template_links( struct NNCMS_THREAD_INFO *req, struct NNCMS_LINK *links )
{
    GString *html = g_string_sized_new( 100 );
    
    // Header of the links
    g_string_append( html, "<ul class=\"inline_links\">\n" );
    
    // Iterate though the links
    for( int i = 0; links[i].function; i++ )
    {
        // #define g_array_index(a,t,i)      (((t*) (void *) (a)->data) [(i)])
        // Undocumented feature
        //struct NNCMS_LINK *link = &((struct NNCMS_LINK *) links->data) [i];
        
        // Documented way
        //struct NNCMS_LINK *link = g_array_index( links, struct NNCMS_LINK, i );
        
        struct NNCMS_LINK *link = &links[i];
        
        // Generate link
        char *html_link = main_link( req, link->function, link->title, link->vars );
        
        // Append to HTML buffer
        g_string_append_printf( html, "    <li>%s</li>\n", html_link );
        
        // Free memory
        g_free( html_link );
    }

    // Footer of the links
    g_string_append( html, "</ul>\n" );

    char *phtml = g_string_free( html, FALSE );
    
    return phtml;
}

// #############################################################################

struct NNCMS_FORM *template_confirm( struct NNCMS_THREAD_INFO *req, char *name )
{
    // Prepare output buffer
    GString *gtext = g_string_sized_new( 50 );
    
    // Get string from the database
    char *template_data = i18n_string( req, "confirm_msg", NULL );
    
    // Parse the string
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "name", .value.string = name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    template_sparse( req, "confirm_msg", template_data, gtext, vars );
    
    // String from database is not needed anymore
    g_free( template_data );

    // Get real pointer and store it in garbage collector
    char *help_text = g_string_free( gtext, FALSE );
    garbage_add( req->loop_garbage, help_text, MEMORY_GARBAGE_GFREE );

    struct NNCMS_FIELD fields[] =
    {
        { .name = "referer", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = req->session_id, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "delete_submit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .name = "cancel_link", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_LINK, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };
    struct NNCMS_FIELD *pfields = MALLOC( sizeof(fields) );
    memcpy( pfields, &fields, sizeof(fields) );

    struct NNCMS_FORM form =
    {
        .name = "delete_confirm", .action = NULL, .method = "POST",
        .title = NULL, .help = help_text,
        .header_html = NULL, .footer_html = NULL,
        .fields = pfields
    };
    struct NNCMS_FORM *pform = MALLOC( sizeof(form) );
    memcpy( pform, &form, sizeof(form) );
    
    garbage_add( req->loop_garbage, pfields, MEMORY_GARBAGE_FREE );
    garbage_add( req->loop_garbage, pform, MEMORY_GARBAGE_FREE );
    
    return pform;
}

// #############################################################################

// This fuction parses template file and template structure
// it replaces tags in structure with its data in template file
size_t template_sparse( struct NNCMS_THREAD_INFO *req,
    char *template_name, char *template_data, GString *buffer,
    struct NNCMS_VARIABLE *template_tags )
{
    // Lua status
    bool var_loaded = false;      // Load var only once, and only if lua tag was found

    //
    // Start replacing tags
    // that are in template_data with tags names and their values which are stored in
    // NNCMS_TEMPLATE_TAGS structure.
    //
    for( unsigned int parse_offset = 0; template_data[parse_offset] != 0; parse_offset++ )
    {
        // Null terminating character
        if( template_data[parse_offset] == 0 ) break;

        // Simple byte copy
        if( template_data[parse_offset] != '<' || template_data[parse_offset + 1] != '?' )
        {
false_alarm:
            // Copy one byte if it is not a opening bracket
            g_string_append_c( buffer, template_data[parse_offset] );
        }

        // Direct data output
        else if( template_data[parse_offset + 2] == '=' && template_data[parse_offset + 3] == ' ' )
        {
            // Remove closing bracket
            unsigned int close_bracket_position = (unsigned int) (strstr( template_data + parse_offset, "?>" ) - template_data + 1);
            if( close_bracket_position - parse_offset > NNCMS_TAG_NAME_MAX_SIZE ||
                close_bracket_position == 0 )
            {
                // If tag name in template is too big, then its not a tag,
                // maybe this is CSS or something else
                goto false_alarm;
            }
            if( template_data[close_bracket_position - 2] != ' ' ) goto false_alarm;
            template_data[close_bracket_position - 2] = 0;
            parse_offset = parse_offset + 4;

            // Compare it with every tag given in template_tags
            for( unsigned int tag_index = 0; template_tags[tag_index].name[0] != 0; tag_index++ )
            {
                if( strncmp( template_tags[tag_index].name, &template_data[parse_offset], NNCMS_TAG_NAME_MAX_SIZE ) == 0 )
                {
                    // Tag found, check if it has any value
                    //if( template_tags[tag_index].value.string == 0 ) break;

                    // Parse value according to type
                    char *pbuffer = main_type( req, template_tags[tag_index].value, template_tags[tag_index].type );
                    g_string_append( buffer, pbuffer );
                    g_free( pbuffer );

                    // Exit tag search loop
                    break;
                }
            }

            // Move pointer to closing bracket
            template_data[close_bracket_position - 2] = ' ';
            parse_offset = close_bracket_position;
        }

        // Processing tag
        else if( template_data[parse_offset + 2] == 'l' && template_data[parse_offset + 3] == 'u' && template_data[parse_offset + 4] == 'a' )
        {
            // Remove closing bracket
            unsigned int close_bracket_position = (unsigned int) (strstr( template_data + parse_offset, "?>" ) - template_data + 1);
            if( close_bracket_position == 0 )
            {
                // Seems like tag was started, but end is not found
                goto false_alarm;
            }
            template_data[close_bracket_position - 1] = 0;
            parse_offset = parse_offset + 5;

            // Number of bytes inside tag
            unsigned int data_len = (close_bracket_position - 1) - parse_offset;

            //
            // template_data[parse_offset] now has the data and
            // in data_len number of bytes
            //
            // Lua
            //

            // Load variables once
            if( var_loaded == false )
            {
                // Create a table
                lua_newtable( req->L );

                // Load template variables in Lua
                for( unsigned int tag_index = 0; template_tags[tag_index].name[0] != 0; tag_index++ )
                {
                    // Check if tag has any value
                    //if( template_tags[tag_index].value.string == 0 ) continue;

                    // Table is at the top
                    lua_pushstring( req->L, template_tags[tag_index].name );
                    
                    // Parse value according to type
                    switch( template_tags[tag_index].type )
                    {
                        case NNCMS_TYPE_UNSIGNED_INTEGER:
                        {
                            unsigned int val = template_tags[tag_index].value.unsigned_integer;
                            lua_pushunsigned( req->L, val );
                            break;
                        }

                        case NNCMS_TYPE_INTEGER:
                        {
                            int val = template_tags[tag_index].value.integer;
                            lua_pushinteger( req->L, val );
                            break;
                        }

                        case NNCMS_TYPE_BOOL:
                        {
                            bool val = template_tags[tag_index].value.boolean;
                            lua_pushboolean( req->L, val );
                            break;
                        }

                        case TEMPLATE_TYPE_LINKS:
                        {
                            struct NNCMS_LINK *pval = (struct NNCMS_LINK *) template_tags[tag_index].value.string;
                            char *tmp = template_links( req, pval );
                            lua_pushstring( req->L, tmp );
                            g_free( tmp );
                            break;
                        }

                        case TEMPLATE_TYPE_UNSAFE_STRING:
                        {
                            char *pstring = (char *) template_tags[tag_index].value.string;
                            char *escaped = template_escape_html( pstring );
                            lua_pushstring( req->L, escaped );
                            g_free( escaped );
                            break;
                        }
                        
                        default:
                        {
                            //char *pstring = (char *) template_tags[tag_index].value.string;
                            char *pstring = main_type( req, template_tags[tag_index].value, template_tags[tag_index].type );
                            lua_pushstring( req->L, pstring );
                            g_free( pstring );
                            break;
                        }
                    }
                    
                    lua_settable( req->L, -3 );
                }

                // Some more global variables
                lua_pushstring( req->L, "guest" );
                lua_pushboolean( req->L, (int) req->is_guest );
                lua_settable( req->L, -3 );
                lua_pushstring( req->L, "session_user_id" );
                lua_pushstring( req->L, req->user_id );
                lua_settable( req->L, -3 );
                lua_pushstring( req->L, "session_user_name" );
                lua_pushstring( req->L, req->user_name );
                lua_settable( req->L, -3 );

                // Table name
                lua_setglobal( req->L, "var" );

                // Data loaded
                var_loaded = true;
            }

            // Save old buffer
            lua_getglobal( req->L, "buffer" );
            GString *old_buffer = lua_touserdata( req->L, lua_gettop( req->L ) );
            lua_pop( req->L, 1 );

            // Buffer for print function
            lua_pushlightuserdata( req->L, buffer );
            lua_setglobal( req->L, "buffer" );

            // Make chunk name
            char chunk_name[256];
            snprintf( chunk_name, sizeof(chunk_name), "%s[%u]", template_name, parse_offset );

            // Call the script
            int load_result = luaL_loadbuffer( req->L, &template_data[parse_offset], data_len, chunk_name );
            if( load_result == 0 )
            {
                // Script successfully loaded, call it
                int call_result = lua_pcall( req->L, 0, LUA_MULTRET, 0 );
                if( call_result != 0 )
                {
                    // Error in calling script
                    char *stack_msg = (char *) lua_tostring( req->L, -1 );
                    lua_pop( req->L, 1 );
                    log_printf( req, LOG_ERROR, "lua_pcall error: %s, chunk_name = %s", stack_msg, chunk_name );
                }
            }
            else
            {
                // Error loading script
                char *stack_msg = (char *) lua_tostring( req->L, -1 );
                lua_pop( req->L, 1 );
                log_printf( req, LOG_ERROR, "luaL_loadbuffer error: %s, chunk_name = %s", stack_msg, chunk_name );
            }

            // Check if all is ok
            int stack_top = lua_gettop(req->L );
            if( stack_top != 0 )
            {
                log_printf( req, LOG_WARNING, "Lua stack is not empty, stack_top = %i, chunk_name = %s", stack_top, chunk_name );

                // Debug
                lua_stackdump( req->L );

                // Empty the stack
                lua_settop( req->L, 0 );
            }

            // Restore old buffer
            lua_pushlightuserdata( req->L, old_buffer );
            lua_setglobal( req->L, "buffer" );

            // Move pointer to closing bracket
            template_data[close_bracket_position - 1] = '?';
            parse_offset = close_bracket_position;
        }
        else
        {
            // Silently ignore
            goto false_alarm;
        }
    }

    return buffer->len;
}

// #############################################################################

char *template_escape_html( char *src )
{
    GString *escaped = g_string_sized_new( 16 );

    for( size_t i = 0; src[i] != 0; i++ )
    {
        if( src[i] == '&' ) { g_string_append( escaped, "&amp;" ); }
        else if( src[i] == '<' ) { g_string_append( escaped, "&lt;" ); }
        else if( src[i] == '>' ) { g_string_append( escaped, "&gt;" ); }
        else if( src[i] == '"' ) { g_string_append( escaped, "&quot;" ); }
        else if( src[i] == '\'' ) { g_string_append( escaped, "&apos;" ); }
        else if( src[i] == 0 )
        {
            break;
        }
        else
        {
            // Copy character by default
            g_string_append_c_inline( escaped, src[i] );
        }
    }

    // Huge text test
    /*size_t test_size = 1000000;
    char *str_escaped = MALLOC( test_size );
    for( size_t i = 0; i < test_size - 10; i += 9 )
        memcpy( &str_escaped[i], "blah <br>", 9 );
    str_escaped[test_size - 10] = 0;*/

    char *str_escaped = g_string_free( escaped, FALSE );
    return str_escaped;
}
// #############################################################################

char *template_escape_uri( char *lpszSource )
{
    // Count new buffer lenght
    size_t nSize = 1;
    for( size_t i = 0; i < NNCMS_PAGE_SIZE_MAX; i++ )
    {
        unsigned char b = (unsigned char) lpszSource[i];
        if( lpszSource[i] == 0 )
        {
            break;
        }
        else if( uriFilter[b] == 0 )
        {
            nSize += 3;
        }
        else
        {
            nSize += 1;
        }
    }

    // Create smartbuffer
    struct NNCMS_BUFFER smartBuffer =
        {
            /* lpBuffer */ 0,
            /* nSize */ nSize,
            /* nPos */ 0
        };
    smartBuffer.lpBuffer = MALLOC( smartBuffer.nSize );
    *smartBuffer.lpBuffer = 0;

    // Replace and copy
    char szEscaped[4] = "%20";
    static const char nybble_chars[] = "0123456789ABCDEF";
    for( size_t i = 0; i < nSize; i++ )
    {
        unsigned char b = (unsigned char) lpszSource[i];
        if( lpszSource[i] == 0 )
        {
            break;
        }
        else if( uriFilter[b] == 0 )
        {
            szEscaped[1] = nybble_chars[(b >> 4) & 0x0F];
            szEscaped[2] = nybble_chars[b & 0x0F];

            smart_cat( &smartBuffer, szEscaped );
        }
        else
        {
            // Copy character by default
            smartBuffer.lpBuffer[smartBuffer.nPos] = b;
            smartBuffer.nPos++;
            smartBuffer.lpBuffer[smartBuffer.nPos] = 0;
        }
    }

    return smartBuffer.lpBuffer;
}

// #############################################################################
