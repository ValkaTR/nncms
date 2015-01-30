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
#include "i18n.h"

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

struct NNCMS_CFG_FIELDS
{
    struct NNCMS_FIELD cfg_id;
    struct NNCMS_FIELD cfg_name;
    struct NNCMS_FIELD cfg_value;
    struct NNCMS_FIELD cfg_type;
    struct NNCMS_FIELD cfg_lpmem;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD cfg_edit;
    struct NNCMS_FIELD none;
}
cfg_fields =
{
    { .name = "cfg_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "cfg_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "cfg_value", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "cfg_type", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "cfg_lpmem", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "cfg_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// #############################################################################
// functions

// #############################################################################

bool cfg_global_init( )
{
    bool bCfgLoaded = 0;

    bCfgLoaded |= cfg_parse_file( "/etc/nncms.conf", g_options );
    bCfgLoaded |= cfg_parse_file( "nncms.conf", g_options );

    // Check homeURL
    size_t nHomeUrlLen = strlen( homeURL );
    nHomeUrlLen--;
    if( homeURL[nHomeUrlLen] == '/' )
        homeURL[nHomeUrlLen] = 0;

    main_page_add( "cfg_list", &cfg_list );
    main_page_add( "cfg_view", &cfg_view );
    main_page_add( "cfg_edit", &cfg_edit );

    return bCfgLoaded;
}

// #############################################################################

bool cfg_global_destroy( )
{
    return true;
}

// #############################################################################

void cfg_list( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "cfg_list_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "cfg", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }    

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "cfg_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "cfg_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "cfg_value_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "cfg_type_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "cfg_lpmem_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Loop thru all rows
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; i < NNCMS_CFG_OPTIONS_MAX; i++ )
    {
        // Skip empty config entries
        if( g_options[i].lpMem == 0 ) break;

        // Fill template values
        int cfg_id = i;
        char *cfg_name = g_options[i].name;
        void *cfg_lpmem = g_options[i].lpMem;

        // Soft type convert
        union NNCMS_VARIABLE_UNION cfg_value;
        enum NNCMS_VARIABLE_TYPE cfg_type;
        char *cfg_type_name;
        switch( g_options[i].nType )
        {
            case NNCMS_INTEGER:
            {
                cfg_value.integer = *(int *) g_options[i].lpMem;
                cfg_type = NNCMS_TYPE_INTEGER;
                cfg_type_name = "Integer";
                break;
            }
            case NNCMS_STRING:
            {
                cfg_value.string = (char *) g_options[i].lpMem;
                cfg_type = TEMPLATE_TYPE_UNSAFE_STRING;
                cfg_type_name = "String";
                break;
            }
            case NNCMS_BOOL:
            {
                cfg_value.boolean = *(bool *) g_options[i].lpMem;
                cfg_type = NNCMS_TYPE_BOOL;
                cfg_type_name = "Boolean";
                break;
            }
        }

        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "cfg_id", .value.integer = cfg_id, NNCMS_TYPE_INTEGER },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "cfg_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "cfg_edit", i18n_string_temp( req, "edit", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.integer = cfg_id, .type = NNCMS_TYPE_INTEGER, .options = NULL },
            { .value.string = cfg_name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = cfg_value, .type = cfg_type, .options = NULL },
            { .value.string = cfg_type_name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.pmem = cfg_lpmem, .type = NNCMS_TYPE_MEM, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = edit, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .row_count = gcells->len / (sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1),
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );

    // Generate links
    char *links = cfg_links( req, NULL );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-terminal.png", .type = NNCMS_TYPE_STRING },
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

bool cfg_parse_buffer( struct NNCMS_THREAD_INFO *req, char *buffer, struct NNCMS_VARIABLE *vars )
{
    // All Lua contexts are held in this structure. We work with it almost
    // all the time.
    lua_State *L = req->L; //luaL_newstate( );
    if( L == NULL )
    {
        log_print( 0, LOG_CRITICAL, "Cannot create state: not enough memory" );
        return false;
    }

    // Load Lua libraries
    //luaL_openlibs( L );

    // Load script in buffer
    int load_result = luaL_loadbuffer( L, buffer, strlen(buffer), "cfg_parse_buffer" );
    if( load_result != 0 )
    {
        // Get error string
        char *stack_msg = (char *) lua_tostring( L, -1 );
        lua_pop( L, 1 );
        log_printf( 0, LOG_ERROR, "Lua loading error: %s", stack_msg );

        return false;
    }

    // Call the script
    int pcall_result = lua_pcall( L, 0, 0, 0 );
    if( pcall_result != 0 )
    {
        // Get error string
        char *stack_msg = (char *) lua_tostring( L, -1 );
        lua_pop( L, 1 );
        log_printf( 0, LOG_ERROR, "Lua calling error: %s", stack_msg );

        return false;
    }
    
    // Check if all is ok
    int stack_top = lua_gettop( L );
    if( stack_top != 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Lua stack is not empty, position %i", stack_top );

        // Debug
        lua_stackdump( L );

        // Empty the stack
        lua_settop( L, 0 );
    }

    // Ok, loop thru all options getting data from lua
    for( unsigned int i = 0; vars != NULL && vars[i].type != NNCMS_TYPE_NONE; i = i + 1 )
    {
        // Pushes onto the stack the value
        lua_getglobal( L, vars[i].name );

        // If variable is not defined
        //if( lua_isnil( L, 1 ) )
        //{
        //    lua_pop( L, 1 );
        //    continue;
        //}

        // Option found, check it's type
        switch( vars[i].type )
        {
            case NNCMS_TYPE_INTEGER:
            {
                // Convert string to integer
                lua_Integer value = lua_tointeger( L, 1 );
                vars[i].value.integer = value;
                break;
            }
            case NNCMS_TYPE_PMEM:
            {
                size_t len;
                const char *value = lua_tolstring( L, 1, &len );
                * ((char **) vars[i].value.pmem) = memdup_temp( req, (void *) value, len );
                break;
            }
            case NNCMS_TYPE_STRING:
            {
                size_t len;
                const char *value = lua_tolstring( L, 1, &len );
                vars[i].value.string = memdup_temp( req, (void *) value, len );
                break;
            }
            case NNCMS_TYPE_BOOL:
            {
                int value = lua_toboolean( L, 1 );
                vars[i].value.boolean = value;
                break;
            }
            default:
            {
                log_print( req, LOG_WARNING, "Unknow option type" );
            }
        }

        lua_pop( L, 1 );
    }

    // Cya, Lua
    //lua_close( L );
    
    return true;
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
    fclose( pFile );

    //
    // The whole file is loaded in the buffer.
    // Let's begin parsing
    //

    // All Lua contexts are held in this structure. We work with it almost
    // all the time.
    lua_State *L = luaL_newstate( );
    if( L == NULL )
    {
        log_print( 0, LOG_CRITICAL, "Cannot create state: not enough memory" );
        FREE( szTemp );
        return false;
    }

    // Load Lua libraries
    luaL_openlibs( L );

    // Call a script
    int load_result = luaL_loadbuffer( L, szTemp, lSize, "cfg_parse_file" );
    if( load_result == 0 )
    {
        lua_pcall( L, 0, LUA_MULTRET, 0 );
    }
    else
    {
        // Get error string
        char *stack_msg = (char *) lua_tostring( L, -1 );
        lua_pop( L, 1 );
        log_printf( 0, LOG_ERROR, "Lua error: %s", stack_msg );
        lua_close( L );
        
        FREE( szTemp );
        return false;
    }

    // Check if all is ok
    int stack_top = lua_gettop( L );
    if( stack_top != 0 )
    {
        log_printf( 0, LOG_EMERGENCY, "Lua stack is not empty, position %i", stack_top );

        // Debug
        lua_stackdump( L );

        // Empty the stack
        lua_settop( L, 0 );
    }

    // Ok, loop thru all options getting data from lua
    for( unsigned int i = 0; i < NNCMS_CFG_OPTIONS_MAX; i++ )
    {
        // Terminating option
        if( lpOptions[i].name[0] == 0 ) break;
        if( lpOptions[i].lpMem == 0 )
            goto error;

        // Pushes onto the stack the value
        lua_getglobal( L, lpOptions[i].name );

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
                const char *szVarValue = lua_tostring( L, 1 );
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
                log_print( 0, LOG_EMERGENCY, "Unknow option type" );
                goto error;
            }
        }

next:
        lua_pop( L, 1 );
    }

    // Cya, Lua
    lua_close( L );
    FREE( szTemp );
    return true;

error:
    // Error
    lua_close( L );
    FREE( szTemp );
    return false;
}

// #############################################################################

// Edit cfg
void cfg_edit( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "cfg", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get id
    char *httpVarId = main_variable_get( req, req->get_tree, "cfg_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Check boundary
    unsigned int nCfgId = atoi( httpVarId );
    if( nCfgId >= NNCMS_CFG_OPTIONS_MAX )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Is id known?
    if( g_options[nCfgId].lpMem == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "cfg_name", .value.string = g_options[nCfgId].name, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "cfg_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_CFG_FIELDS *fields = memdup_temp( req, &cfg_fields, sizeof(cfg_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->cfg_edit.viewable = true;

    //
    // Check if user commit changes
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "cfg_edit" );
    if( httpVarEdit != 0 )
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
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( field_valid == true )
        {
            // Apply changes
            switch( g_options[nCfgId].nType )
            {
                case NNCMS_INTEGER:
                {
                    *(int *) g_options[nCfgId].lpMem = atoi( fields->cfg_value.value );
                    break;
                }
                case NNCMS_STRING:
                {
                    strlcpy( g_options[nCfgId].lpMem, fields->cfg_value.value, NNCMS_PATH_LEN_MAX );
                    break;
                }
                case NNCMS_BOOL:
                {
                    if( strcasecmp( fields->cfg_value.value, "1" ) == 0 ||
                        strcasecmp( fields->cfg_value.value, "true" ) == 0 )
                    {
                        *(bool *) g_options[nCfgId].lpMem = true;
                    }
                    else if( strcasecmp( fields->cfg_value.value, "0" ) == 0 ||
                             strcasecmp( fields->cfg_value.value, "false" ) == 0 )
                    {
                        *(bool *) g_options[nCfgId].lpMem = false;
                    }
                    break;
                }
            }

            // Log action
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "cfg_name", .value.string = g_options[nCfgId].name, .type = NNCMS_TYPE_STRING },
                { .name = "cfg_value", .value.string = fields->cfg_value.value, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "cfg_edit_success", vars );
            log_printf( req, LOG_ACTION, "Config parameter '%s' set to '%s'", g_options[nCfgId].name, fields->cfg_value.value );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Fill template values
    char *cfg_id = httpVarId;
    char *cfg_name = g_options[nCfgId].name;
    void *cfg_lpmem = g_options[nCfgId].lpMem;

    // Soft type convert
    union NNCMS_VARIABLE_UNION cfg_value;
    enum NNCMS_VARIABLE_TYPE cfg_type;
    char *cfg_type_text;
    switch( g_options[nCfgId].nType )
    {
        case NNCMS_INTEGER:
        {
            cfg_value.integer = *(int *) g_options[nCfgId].lpMem;
            cfg_type = NNCMS_TYPE_INTEGER;
            cfg_type_text = "Integer";
            break;
        }
        case NNCMS_STRING:
        {
            cfg_value.string = (char *) g_options[nCfgId].lpMem;
            cfg_type = TEMPLATE_TYPE_UNSAFE_STRING;
            cfg_type_text = "String";
            break;
        }
        case NNCMS_BOOL:
        {
            cfg_value.boolean = *(bool *) g_options[nCfgId].lpMem;
            cfg_type = NNCMS_TYPE_BOOL;
            cfg_type_text = "Boolean";
            break;
        }
    }
    char *cfg_value_text = main_type( req, cfg_value, cfg_type );
    garbage_add( req->loop_garbage, cfg_value_text, MEMORY_GARBAGE_GFREE );
    char *cfg_lpmem_text = main_type( req, (union NNCMS_VARIABLE_UNION) { .pmem = cfg_lpmem }, NNCMS_TYPE_MEM );
    garbage_add( req->loop_garbage, cfg_lpmem_text, MEMORY_GARBAGE_GFREE );

    // Generate links
    char *links = cfg_links( req, cfg_id );

    //
    // Fill form with data
    fields->cfg_id.value = cfg_id;
    fields->cfg_name.value = cfg_name;
    fields->cfg_value.value = cfg_value_text;
    fields->cfg_type.value = cfg_type_text;
    fields->cfg_lpmem.value = cfg_lpmem_text;

    struct NNCMS_FORM form =
    {
        .name = "cfg_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-terminal.png", .type = NNCMS_TYPE_STRING },
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

void cfg_view( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "cfg", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get id
    char *httpVarId = main_variable_get( req, req->get_tree, "cfg_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Check boundary
    unsigned int nCfgId = atoi( httpVarId );
    if( nCfgId >= NNCMS_CFG_OPTIONS_MAX )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Is id known?
    if( g_options[nCfgId].lpMem == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "cfg_name", .value.string = g_options[nCfgId].name, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "cfg_view_header", NULL );

    //
    // Fill template values
    //
    char *cfg_id = httpVarId;
    char *cfg_name = g_options[nCfgId].name;
    void *cfg_lpmem = g_options[nCfgId].lpMem;

    // Soft type convert
    union NNCMS_VARIABLE_UNION cfg_value;
    enum NNCMS_VARIABLE_TYPE cfg_type;
    char *cfg_type_text;
    switch( g_options[nCfgId].nType )
    {
        case NNCMS_INTEGER:
        {
            cfg_value.integer = *(int *) g_options[nCfgId].lpMem;
            cfg_type = NNCMS_TYPE_INTEGER;
            cfg_type_text = "Integer";
            break;
        }
        case NNCMS_STRING:
        {
            cfg_value.string = (char *) g_options[nCfgId].lpMem;
            cfg_type = TEMPLATE_TYPE_UNSAFE_STRING;
            cfg_type_text = "String";
            break;
        }
        case NNCMS_BOOL:
        {
            cfg_value.boolean = *(bool *) g_options[nCfgId].lpMem;
            cfg_type = NNCMS_TYPE_BOOL;
            cfg_type_text = "Boolean";
            break;
        }
    }
    char *cfg_value_text = main_type( req, cfg_value, cfg_type );
    garbage_add( req->loop_garbage, cfg_value_text, MEMORY_GARBAGE_GFREE );
    char *cfg_lpmem_text = main_type( req, (union NNCMS_VARIABLE_UNION) { .pmem = cfg_lpmem }, NNCMS_TYPE_MEM );
    garbage_add( req->loop_garbage, cfg_lpmem_text, MEMORY_GARBAGE_GFREE );

    // Generate links
    char *links = cfg_links( req, cfg_id );

    //
    // Form
    //
    struct NNCMS_CFG_FIELDS *fields = memdup_temp( req, &cfg_fields, sizeof(cfg_fields) );
    fields->cfg_id.value = cfg_id;
    fields->cfg_name.value = cfg_name;
    fields->cfg_value.value = cfg_value_text;
    fields->cfg_type.value = cfg_type_text;
    fields->cfg_lpmem.value = cfg_lpmem_text;
    fields->cfg_value.editable = false;
    fields->referer.viewable = false;
    fields->fkey.viewable = false;
    fields->cfg_edit.viewable = false;

    struct NNCMS_FORM form =
    {
        .name = "cfg_view", .action = NULL, .method = NULL,
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/utilities-terminal.png", .type = NNCMS_TYPE_STRING },
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

char *cfg_links( struct NNCMS_THREAD_INFO *req, char *cfg_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "cfg_id", .value.string = cfg_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "cfg_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    //link.function = "cfg_add";
    //link.title = i18n_string_temp( req, "add_link", NULL );
    //link.vars = vars_object;
    //g_array_append_vals( links, &link, 1 );

    if( cfg_id != NULL )
    {
        link.function = "cfg_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "cfg_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        //link.function = "cfg_delete";
        //link.title = i18n_string_temp( req, "delete_link", NULL );
        //link.vars = vars_id;
        //g_array_append_vals( links, &link, 1 );
    }

    // Convert arrays to HTML code
    char *html = template_links( req, (struct NNCMS_LINK *) links->data );
    garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GFREE );

    // Free array
    g_array_free( links, TRUE );
    
    return html;
}

// #############################################################################
