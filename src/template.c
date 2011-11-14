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
#include "tree.h"
#include "acl.h"
#include "user.h"
#include "post.h"
#include "threadinfo.h"

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

// Template module status
static bool bTemplateInit = false;

// Path to tempates
char templateDir[NNCMS_PATH_LEN_MAX] = "/templates/";

// For fast access to templates
struct TEMPLATE_FILE
{
    char *szFileName;   // Filename of template
    char *lpMem;        // Contents of template
}
templates[] =
{
    { /* szFileName */ "acl_add.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_delete.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_edit.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_object_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_object_head.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_object_row.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_view_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_view_head.html", /* lpMem */ 0 },
    { /* szFileName */ "acl_view_row.html", /* lpMem */ 0 },
    { /* szFileName */ "ban_add.html", /* lpMem */ 0 },
    { /* szFileName */ "ban_view.html", /* lpMem */ 0 },
    { /* szFileName */ "ban_view_row.html", /* lpMem */ 0 },
    { /* szFileName */ "ban_view_structure.html", /* lpMem */ 0 },
    { /* szFileName */ "ban_edit.html", /* lpMem */ 0 },
    { /* szFileName */ "ban_delete.html", /* lpMem */ 0 },
    { /* szFileName */ "block.html", /* lpMem */ 0 },
    { /* szFileName */ "cache_admin.html", /* lpMem */ 0 },
    { /* szFileName */ "cfg_edit.html", /* lpMem */ 0 },
    { /* szFileName */ "cfg_view_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "cfg_view_head.html", /* lpMem */ 0 },
    { /* szFileName */ "cfg_view_row.html", /* lpMem */ 0 },
    { /* szFileName */ "file_add.html", /* lpMem */ 0 },
    { /* szFileName */ "file_browser_dir.html", /* lpMem */ 0 },
    { /* szFileName */ "file_browser_file.html", /* lpMem */ 0 },
    { /* szFileName */ "file_browser_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "file_browser_head.html", /* lpMem */ 0 },
    { /* szFileName */ "file_delete.html", /* lpMem */ 0 },
    { /* szFileName */ "file_edit.html", /* lpMem */ 0 },
    { /* szFileName */ "file_mkdir.html", /* lpMem */ 0 },
    { /* szFileName */ "file_rename.html", /* lpMem */ 0 },
    { /* szFileName */ "file_upload.html", /* lpMem */ 0 },
    { /* szFileName */ "form.html", /* lpMem */ 0 },
    { /* szFileName */ "frame.html", /* lpMem */ 0 },
    { /* szFileName */ "gallery_dir.html", /* lpMem */ 0 },
    { /* szFileName */ "gallery_empty.html", /* lpMem */ 0 },
    { /* szFileName */ "gallery_file.html", /* lpMem */ 0 },
    { /* szFileName */ "gallery_image.html", /* lpMem */ 0 },
    { /* szFileName */ "gallery_row.html", /* lpMem */ 0 },
    { /* szFileName */ "gallery_structure.html", /* lpMem */ 0 },
    { /* szFileName */ "log_clear.html", /* lpMem */ 0 },
    { /* szFileName */ "log_view_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "log_view_head.html", /* lpMem */ 0 },
    { /* szFileName */ "log_view.html", /* lpMem */ 0 },
    { /* szFileName */ "log_view_row.html", /* lpMem */ 0 },
    { /* szFileName */ "page_current.html", /* lpMem */ 0 },
    { /* szFileName */ "page_number.html", /* lpMem */ 0 },
    { /* szFileName */ "post_add.html", /* lpMem */ 0 },
    { /* szFileName */ "post_board_row.html", /* lpMem */ 0 },
    { /* szFileName */ "post_board_structure.html", /* lpMem */ 0 },
    { /* szFileName */ "post_delete.html", /* lpMem */ 0 },
    { /* szFileName */ "post_edit.html", /* lpMem */ 0 },
    { /* szFileName */ "post_modify.html", /* lpMem */ 0 },
    { /* szFileName */ "post_news_row.html", /* lpMem */ 0 },
    { /* szFileName */ "post_news_structure.html", /* lpMem */ 0 },
    { /* szFileName */ "post_parent.html", /* lpMem */ 0 },
    { /* szFileName */ "post_remove.html", /* lpMem */ 0 },
    { /* szFileName */ "post_reply.html", /* lpMem */ 0 },
    { /* szFileName */ "post_root.html", /* lpMem */ 0 },
    { /* szFileName */ "post_rss_entry.xml", /* lpMem */ 0 },
    { /* szFileName */ "post_rss_structure.xml", /* lpMem */ 0 },
    { /* szFileName */ "post_view.html", /* lpMem */ 0 },
    { /* szFileName */ "post_topics_row.html", /* lpMem */ 0 },
    { /* szFileName */ "post_topics_structure.html", /* lpMem */ 0 },
    { /* szFileName */ "redirect.html", /* lpMem */ 0 },
    { /* szFileName */ "structure.html", /* lpMem */ 0 },
    { /* szFileName */ "user_block.html", /* lpMem */ 0 },
    { /* szFileName */ "user_delete.html", /* lpMem */ 0 },
    { /* szFileName */ "user_edit.html", /* lpMem */ 0 },
    { /* szFileName */ "user_login.html", /* lpMem */ 0 },
    { /* szFileName */ "user_profile.html", /* lpMem */ 0 },
    { /* szFileName */ "user_register.html", /* lpMem */ 0 },
    { /* szFileName */ "user_sessions_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "user_sessions_head.html", /* lpMem */ 0 },
    { /* szFileName */ "user_sessions_row.html", /* lpMem */ 0 },
    { /* szFileName */ "user_view_foot.html", /* lpMem */ 0 },
    { /* szFileName */ "user_view_head.html", /* lpMem */ 0 },
    { /* szFileName */ "user_view.html", /* lpMem */ 0 },
    { /* szFileName */ "user_view_row.html", /* lpMem */ 0 },
    { /* szFileName */ "view_file.html", /* lpMem */ 0 },
    { /* szFileName */ "view_gallery_cell.html", /* lpMem */ 0 },
    { /* szFileName */ "view_gallery_row_begin.html", /* lpMem */ 0 },
    { /* szFileName */ "view_gallery_row_end.html", /* lpMem */ 0 },
    { /* szFileName */ "view_gallery_table_begin.html", /* lpMem */ 0 },
    { /* szFileName */ "view_gallery_table_end.html", /* lpMem */ 0 },
    { /* szFileName */ "view_image.html", /* lpMem */ 0 }
};

// No cache option
bool bTemplateCache = true;

// #############################################################################
// functions

static int template_load( lua_State *L )
{
    //double d = lua_tonumber( L, 1 );  /* get argument */
    //lua_pushnumber( L, sin( d ) );  /* push result */

    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Template::Include: No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Check if filename specified
    char *lpszText = (char *) luaL_checkstring( L, 1 );
    if( lpszText == NULL )
    {
        log_printf( req, LOG_WARNING, "Template::Include: Filename not specified" );
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
        log_printf( req, LOG_ERROR, "Template::ProcessTag: Stat failed on block '%s' file", szFileName );
        lua_pushfstring( L, "<p><font color=\"red\">Stat failed on block '%s' file</font></p>", szFileName );
        return 1; //char *lpszOutput = MALLOC( NNCMS_PATH_LEN_MAX + 64 ); snprintf( lpszOutput, NNCMS_PATH_LEN_MAX + 64, ); lua_pushstring( L, 1 );
    }

    // To show or not to show
    if( file_check_perm( req, &fileInfo, "download" ) == false )
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
        log_printf( 0, LOG_CRITICAL, "Template::Escape: No userdata" );
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
        //log_printf( req, LOG_WARNING, "Template::Escape: Text not specified" );
        //lua_pushstring( L, "<p><font color=\"red\">Text not specified</font></p>" );
        lua_pushstring( L, "" );
        return 1;
    }

    // Convert
    char *lpszOutput = template_escape_html( lpszText );

    // Return back to Lua
    lua_pushstring( L, lpszOutput );
    FREE( lpszOutput );

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
static int template_acl( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Template::Acl: No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Retrieve parameters
    char *lpszObject = (char *) lua_tostring( L, 1 );
    char *lpszPerm = (char *) lua_tostring( L, 2 );
    char *lpszSubject = (char *) lua_tostring( L, 3 ); // Optional
    if( lpszObject == NULL || lpszPerm == NULL )
    {
        log_printf( req, LOG_WARNING, "Template::Acl: Not enought parameters specified" );
        lua_pushboolean( L, 0 );
        return 1;
    }

    // Current user as subject by default
    if( lpszSubject == 0 )
        lpszSubject = req->g_username;

    // Check ACL
    bool bResult = acl_check_perm( req, lpszObject, lpszSubject, lpszPerm );

    // Return back to Lua
    lua_pushboolean( L, (int) bResult );

    // Number of results
    return 1;
}
// #############################################################################

static int template_post_acl( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Template::PostAcl: No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Retrieve parameters
    char *lpszPost = (char *) lua_tostring( L, 1 );
    char *lpszPerm = (char *) lua_tostring( L, 2 );
    lua_Integer nDepth = lua_tointeger( L, 3 );
    if( lpszPost == NULL || lpszPerm == NULL )
    {
        log_printf( req, LOG_WARNING, "Template::PostAcl: Not enought parameters specified" );
        lua_pushboolean( L, 0 );
        return 1;
    }

    if( nDepth == 0 )
        nDepth = POST_CHECK_PERM_DEPTH;

    // Check ACL
    bool bResult = post_tree_check_perm( req, lpszPost, lpszPerm, nDepth );

    // Return back to Lua
    lua_pushboolean( L, (int) bResult );

    // Number of results
    return 1;
}

// #############################################################################

// {file_acl file="$file_path" permission="rename"}
static int template_file_acl( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Template::FileAcl: No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Retrieve parameters
    char *lpszFile = (char *) lua_tostring( L, 1 );
    char *lpszPerm = (char *) lua_tostring( L, 2 );
    char *lpszSubject = (char *) lua_tostring( L, 3 ); // Optional
    if( lpszFile == NULL || lpszPerm == NULL )
    {
        log_printf( req, LOG_WARNING, "Template::FileAcl: Not enought parameters specified" );
        lua_pushboolean( L, 0 );
        return 1;
    }

    // Current user as subject by default
    if( lpszSubject == 0 )
        lpszSubject = req->g_username;

    // Get file info
    struct NNCMS_FILE_INFO fileInfo;
    file_get_info( &fileInfo, lpszFile );
    if( fileInfo.bExists == false )
    {
        char *lpszFileName = (fileInfo.bExists == false ? lpszFile : fileInfo.lpszAbsFullPath );
        log_printf( req, LOG_ERROR, "Template::FileAcl: Stat failed on block '%s' file", lpszFileName );
        lua_pushboolean( L, 0 );
        return 1;
    }

     // Check ACL
    bool bResult = file_check_perm( req, &fileInfo, lpszPerm );

    // Return back to Lua
    lua_pushboolean( L, (int) bResult );

    // Number of results
    return 1;
}
// #############################################################################

static int template_login( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Template::FileAcl: No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Header
    char *szBlockTitle = "Login";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS loginTags[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "REQUEST_URI", req->fcgi_request->envp ) },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Generate the block
    template_iparse( req, TEMPLATE_USER_LOGIN, req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX, loginTags );

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS blockTags[] =
        {
            { /* szName */ "header", /* szValue */ szBlockTitle },
            { /* szName */ "content",  /* szValue */ req->lpszBlockTemp },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Generate the block
    size_t nLen = template_iparse( req, TEMPLATE_BLOCK, req->lpszBlockContent, NNCMS_BLOCK_LEN_MAX, blockTags );

    // Return back to Lua
    lua_pushlstring( L, req->lpszBlockContent, nLen );

    // Number of results
    return 1;
}

// #############################################################################

static int template_profile( lua_State *L )
{
    // Get req pointer
    lua_getglobal( L, "req" );
    struct NNCMS_THREAD_INFO *req = lua_touserdata( L, lua_gettop( L ) );
    lua_pop( L, 1 );
    if( req == NULL )
    {
        log_printf( 0, LOG_CRITICAL, "Template::FileAcl: No userdata" );
        lua_pushstring( L, "<p><font color=\"red\">No userdata</font></p>" );
        return 1;
    }

    // Header
    char *szBlockTitle = "Profile";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS userTags[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "user_name", /* szValue */ req->g_username },
            { /* szName */ "user_id",  /* szValue */ req->g_userid },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Generate the block
    template_iparse( req, TEMPLATE_USER_BLOCK, req->lpszBlockTemp, NNCMS_BLOCK_LEN_MAX, userTags );

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS blockTags[] =
        {
            { /* szName */ "header", /* szValue */ szBlockTitle },
            { /* szName */ "content",  /* szValue */ req->lpszBlockTemp },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Generate the block
    size_t nLen = template_iparse( req, TEMPLATE_BLOCK, req->lpszBlockContent, NNCMS_BLOCK_LEN_MAX, blockTags );

    // Return back to Lua
    lua_pushlstring( L, req->lpszBlockContent, nLen );

    // Number of results
    return 1;
}

// #############################################################################

bool template_init( struct NNCMS_THREAD_INFO *req )
{
    // Initialize cache if needed
    if( bTemplateInit == false )
    {
        bTemplateInit = true;

        // All Lua contexts are held in this structure. We work with it almost
        // all the time.
        req->L = luaL_newstate( );

        // Load Lua libraries
        luaL_openlibs( req->L );

        // Variables
        lua_pushlightuserdata( req->L, req ); lua_setglobal( req->L, "req" );

        // Functions
        lua_pushcfunction( req->L, template_load ); lua_setglobal( req->L, "load" );
        lua_pushcfunction( req->L, template_lua_escape_html ); lua_setglobal( req->L, "escape_html" );
        lua_pushcfunction( req->L, template_lua_escape_uri ); lua_setglobal( req->L, "escape_uri" );
        lua_pushcfunction( req->L, template_acl ); lua_setglobal( req->L, "acl" );
        lua_pushcfunction( req->L, template_post_acl ); lua_setglobal( req->L, "post_acl" );
        lua_pushcfunction( req->L, template_file_acl ); lua_setglobal( req->L, "file_acl" );
        lua_pushcfunction( req->L, template_login ); lua_setglobal( req->L, "login" );
        lua_pushcfunction( req->L, template_profile ); lua_setglobal( req->L, "profile" );

        return true;
    }

    return false;
}

// #############################################################################

bool template_deinit( struct NNCMS_THREAD_INFO *req )
{
    if( bTemplateInit == true )
    {
        bTemplateInit = false;

        // Cya, Lua
        lua_close( req->L );

        // Free loaded templates
        for( int i = 0; i < sizeof(templates)/sizeof(struct TEMPLATE_FILE); i++ )
        {
            if( templates[i].lpMem != 0 )
                FREE( templates[i].lpMem );
        }

        return true;
    }

    return false;
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

size_t template_parse( struct NNCMS_THREAD_INFO *req,
    const char *szFileName, char *szBuffer, size_t nLen,
    struct NNCMS_TEMPLATE_TAGS *templateTags )
{
    // Create full path to template file
    char szFullPath[NNCMS_PATH_LEN_MAX];
    strlcpy( szFullPath, templateDir, NNCMS_PATH_LEN_MAX );
    strlcat( szFullPath, szFileName, NNCMS_PATH_LEN_MAX );

    // Open template
    size_t nSize = nLen;
    char *pTemplate = main_get_file( szFullPath, &nSize );
    if( pTemplate != NULL )
    {
        // Parse
        size_t nOutputSize = template_sparse( req, (char *) szFileName, pTemplate, szBuffer, nLen, templateTags );
        FREE( pTemplate );
        return nOutputSize;
    }
    else
    {
        // Nothing
        *szBuffer = 0;
        return 0;
    }
}

// #############################################################################

size_t template_iparse( struct NNCMS_THREAD_INFO *req,
    enum TEMPLATE_INDEX tempNum, char *szBuffer, size_t nLen,
    struct NNCMS_TEMPLATE_TAGS *templateTags )
{
    struct TEMPLATE_FILE *templ = &templates[tempNum];

    // Check if loaded
    if( templ->lpMem == 0 )
    {
        // Create full path to template file
        char szFullPath[NNCMS_PATH_LEN_MAX];
        strlcpy( szFullPath, templateDir, NNCMS_PATH_LEN_MAX );
        strlcat( szFullPath, templ->szFileName, NNCMS_PATH_LEN_MAX );

        // Open template
        size_t nSize = nLen;
        templ->lpMem = main_get_file( szFullPath, &nSize );
        if( templ->lpMem == NULL )
        {
            // Nothing
            *szBuffer = 0;
            return 0;
        }
    }

    // The parsing
    size_t nOutputSize = template_sparse( req, templ->szFileName,templ->lpMem, szBuffer, nLen, templateTags );

    // No cache option
    if( bTemplateCache == false )
    {
        FREE( templ->lpMem );
        templ->lpMem = NULL;
    }

    return nOutputSize;
}

// #############################################################################

// This fuction parses template file and template structure
// it replaces tags in structure with its data in template file
size_t template_sparse( struct NNCMS_THREAD_INFO *req,
    char *lpName, char *pTemplate, char *szBuffer, size_t nLen,
    struct NNCMS_TEMPLATE_TAGS *templateTags )
{
    // Copy file to szBuffer if no tags specified
    if( templateTags == 0 )
        return strlcpy( szBuffer, pTemplate, nLen );

    // Indices for offsets, byte counting and loops
    unsigned int nOffset = 0;       // Output offset
    unsigned int nParseOffset = 0;  // Offset on template

    // Lua status
    bool bIsVarLoaded = false;      // Load var only once, and only if lua
                                    // tag was found
    //
    // NOW, when whole dumb file is in memory, start replacing tags
    // that are in pTemplate with. Tags names and their values are stored in
    // NNCMS_TEMPLATE_TAGS structure. Result will be saved in szBuffer. If szBuffer
    // is zero, then it will return a pointer to memory with result, this
    // buffer should be freed if not used.
    //
    for( nParseOffset = 0; pTemplate[nParseOffset] != 0; nParseOffset++ )
    {
        // Avoid accidents
        if( pTemplate[nParseOffset] == 0 ) break;

        // Simple byte copy
        if( pTemplate[nParseOffset] != '<' || pTemplate[nParseOffset + 1] != '?' )
        {
false_alarm:
            // Copy one byte if it is not a opening bracket and we have a buffer
            if( szBuffer != 0 )
            {
                szBuffer[nOffset] = pTemplate[nParseOffset];
            }

            // Increase output byte counter
            nOffset = nOffset + 1;
            if( nOffset + 1 > nLen ) goto overflow;
        }

        // Direct data output
        else if( pTemplate[nParseOffset + 2] == '=' && pTemplate[nParseOffset + 3] == ' ' )
        {
            // Remove closing bracket
            unsigned int nCloseBracketPos = (unsigned int) (strstr( pTemplate + nParseOffset, "?>" ) - pTemplate + 1);
            if( nCloseBracketPos - nParseOffset > NNCMS_TAG_NAME_MAX_SIZE ||
                nCloseBracketPos == 0 )
            {
                // If tag name in template is too big, then its not a tag,
                // maybe this is CSS or something else
                goto false_alarm;
            }
            if( pTemplate[nCloseBracketPos - 2] != ' ' ) goto false_alarm;
            pTemplate[nCloseBracketPos - 2] = 0;
            nParseOffset = nParseOffset + 4;

            // Compare it with every tag given in templateTags
            for( unsigned int nCurrentTagIndex = 0; templateTags[nCurrentTagIndex].szName[0] != 0; nCurrentTagIndex++ )
            {
                if( strncmp( templateTags[nCurrentTagIndex].szName, &pTemplate[nParseOffset], NNCMS_TAG_NAME_MAX_SIZE ) == 0 )
                {
                    // Tag found, check if it has any value
                    if( templateTags[nCurrentTagIndex].szValue == 0 ) break;

                    // Overflow check
                    size_t nTagValueLen = strlen( templateTags[nCurrentTagIndex].szValue );
                    if( nOffset + nTagValueLen + 1 > nLen ) goto overflow;

                    // Copy data
                    memcpy( &szBuffer[nOffset], templateTags[nCurrentTagIndex].szValue, nTagValueLen );
                    nOffset += nTagValueLen;

                    // Exit tag search loop
                    break;
                }
            }

            // Move pointer to closing bracket
            pTemplate[nCloseBracketPos - 2] = ' ';
            nParseOffset = nCloseBracketPos;
        }

        // Processing tag
        else if( pTemplate[nParseOffset + 2] == 'l' && pTemplate[nParseOffset + 3] == 'u' && pTemplate[nParseOffset + 4] == 'a' )
        {
            // Remove closing bracket
            unsigned int nCloseBracketPos = (unsigned int) (strstr( pTemplate + nParseOffset, "?>" ) - pTemplate + 1);
            if( nCloseBracketPos == 0 )
            {
                // Seems like tag was started, but unexpectedly ends
                goto false_alarm;
            }
            pTemplate[nCloseBracketPos - 1] = 0;
            nParseOffset = nParseOffset + 5;

            // Number of bytes inside tag
            unsigned int nDataLen = (nCloseBracketPos - 1) - nParseOffset;

            //
            // pTemplate[nParseOffset] now has the data and
            // in nDataLen number of bytes
            //
            // Lua
            //

            // Do something
            if( nOffset + nDataLen + 1 > nLen ) goto overflow;
            if( szBuffer != 0 )
            {
                //for( ; pTemplate[nParseOffset] != 0; nOffset++, nParseOffset++ )
                //{
                //    // Copy one byte if it is not a opening bracket
                //    szBuffer[nOffset] = pTemplate[nParseOffset] | 1;
                //}

                // Load variables once
                if( bIsVarLoaded == false )
                {
                    // Create a table
                    lua_newtable( req->L );

                    // Load template variables in Lua
                    for( unsigned int nCurrentTagIndex = 0; templateTags[nCurrentTagIndex].szName[0] != 0; nCurrentTagIndex++ )
                    {
                        // Check if tag has any value
                        if( templateTags[nCurrentTagIndex].szValue == 0 ) continue;

                        /* Table is at the top */
                        lua_pushstring( req->L, templateTags[nCurrentTagIndex].szName );
                        lua_pushstring( req->L, templateTags[nCurrentTagIndex].szValue );
                        lua_settable( req->L, -3 );
                    }

                    // Check session
                    user_check_session( req );

                    // Some more global variables
                    lua_pushstring( req->L, "guest" );
                    lua_pushboolean( req->L, (int) req->g_isGuest );
                    lua_settable( req->L, -3 );
                    lua_pushstring( req->L, "session_user_id" );
                    lua_pushstring( req->L, req->g_userid );
                    lua_settable( req->L, -3 );
                    lua_pushstring( req->L, "session_user_name" );
                    lua_pushstring( req->L, req->g_username );
                    lua_settable( req->L, -3 );

                    // Table name
                    lua_setglobal( req->L, "var" );

                    // Data loaded
                    bIsVarLoaded = true;
                }

                // Debug info
#ifdef DEBUG
                char szDebug[256];
                snprintf( szDebug, sizeof(szDebug), "%s[%u]", lpName, nParseOffset );
#else
                char *szDebug = "template";
#endif

                // Call a script
                int nLoadResult = luaL_loadbuffer( req->L, &pTemplate[nParseOffset], nDataLen, szDebug );
                int nCallResult;
                if( nLoadResult == 0 ) nCallResult = lua_pcall( req->L, 0, LUA_MULTRET, 0 );

                // Get error string
                char *lpStackMsg = (char *) lua_tostring( req->L, -1 );
                if( nLoadResult )
                    log_printf( req, LOG_ERROR, "Template::Parse: Lua error: %s", lpStackMsg );

                if( lpStackMsg || nCallResult )
                {
                    // Overflow check
                    size_t nTempLen = strlen( lpStackMsg );
                    if( nOffset + nTempLen + 1 > nLen ) goto overflow;

                    // Copy data
                    memcpy( &szBuffer[nOffset], lpStackMsg, nTempLen );
                    nOffset += nTempLen;
                    szBuffer[nOffset] = 0;

                    lua_pop( req->L, 1 );  /* pop error message from the stack */
                }

                // Check if all is ok
                int luaStackTop = lua_gettop(req->L );
                if( luaStackTop != 0 )
                {
                    log_printf( req, LOG_EMERGENCY, "Template::Parse: Lua stack is not empty, position %i", luaStackTop );

                    // Debug
                    lua_stackdump( req->L );

                    // Empty the stack
                    lua_settop( req->L, 0 );
                }
            }
            else
            {
                // If we have no buffer, then just count needed bytes
                nOffset += nDataLen;
                nParseOffset += nDataLen;
            }

            // Move pointer to closing bracket
            pTemplate[nCloseBracketPos - 1] = '?';
            nParseOffset = nCloseBracketPos;
        }
        else
        {
            // Silently ignore
            goto false_alarm;
        }
    }

overflow:
    // If we are not counting number of bytes then add terminating
    // character to the end of buffer
    if( szBuffer != 0 )
    {
        // 0 terminating character
        szBuffer[nOffset] = 0;
    }

    // Debug
    //printf( TERM_FG_RED "\ntemplate_parse:" TERM_RESET " \n%s\n", szBuffer );

    return nOffset + 1;
}

// #############################################################################

char *template_escape_html( char *lpszSource )
{
    // Count new buffer lenght
    size_t nSize = 1;
    for( size_t i = 0; i < NNCMS_PAGE_SIZE_MAX; i++ )
    {
        if( lpszSource[i] == '&' ) { nSize += 5; }
        else if( lpszSource[i] == '<' ) { nSize += 4; }
        else if( lpszSource[i] == '>' ) { nSize += 4; }
        else if( lpszSource[i] == '"' ) { nSize += 6; }
        else if( lpszSource[i] == '\'' ) { nSize += 6; }
        else if( lpszSource[i] == 0 ) { break; }
        else { nSize += 1; }
    }

    // Create smartbuffer and copy
    struct NNCMS_BUFFER smartBuffer =
        {
            /* lpBuffer */ 0,
            /* nSize */ nSize,
            /* nPos */ 0
        };
    smartBuffer.lpBuffer = MALLOC( smartBuffer.nSize );
    *smartBuffer.lpBuffer = 0;
    for( size_t i = 0; i < nSize; i++ )
    {
        if( lpszSource[i] == '&' ) { smart_cat( &smartBuffer, "&amp;" ); }
        else if( lpszSource[i] == '<' ) { smart_cat( &smartBuffer, "&lt;" ); }
        else if( lpszSource[i] == '>' ) { smart_cat( &smartBuffer, "&gt;" ); }
        else if( lpszSource[i] == '"' ) { smart_cat( &smartBuffer, "&quot;" ); }
        else if( lpszSource[i] == '\'' ) { smart_cat( &smartBuffer, "&apos;" ); }
        else if( lpszSource[i] == 0 )
        {
            break;
        }
        else
        {
            // Copy character by default
            smartBuffer.lpBuffer[smartBuffer.nPos] = lpszSource[i];
            smartBuffer.nPos++;
            smartBuffer.lpBuffer[smartBuffer.nPos] = 0;
        }
    }

    return smartBuffer.lpBuffer;
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
