// #############################################################################
// Source file: i18n.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "i18n.h"

#include "main.h"
#include "database.h"
#include "log.h"
#include "template.h"
#include "user.h"
#include "acl.h"
#include "memory.h"

// #############################################################################
// includes of system headers
//

// #############################################################################
// global variables
//

// #############################################################################
// functions

bool i18n_global_init( )
{
    main_local_init_add( &i18n_local_init );
    main_local_destroy_add( &i18n_local_destroy );
    
    main_page_add( "i18n_view", &i18n_view );
    main_page_add( "i18n_edit", &i18n_edit );
    
    return true;
}

// #############################################################################

bool i18n_global_destroy( )
{
    return true;
}

// #############################################################################

bool i18n_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_add_locales_target = database_prepare( req, "INSERT INTO locales_target (id, lid, language, translation) VALUES(null, ?, ?, ?)" );
    req->stmt_edit_locales_target = database_prepare( req, "UPDATE locales_target SET translation=? WHERE id=?" );
    req->stmt_find_locales_target = database_prepare( req, "SELECT id, lid, language, translation FROM `locales_target` WHERE lid=? AND language=?" );
    req->stmt_find_locales_source = database_prepare( req, "SELECT id, location, textgroup, source FROM `locales_source` WHERE id=?" );
    req->stmt_count_locales_source = database_prepare( req, "SELECT COUNT(*) FROM locales_source" );
    req->stmt_list_locales_source = database_prepare( req, "SELECT id, location, textgroup, source FROM locales_source ORDER BY source LIMIT ? OFFSET ?" );
    req->stmt_list_language_enabled = database_prepare( req, "SELECT id, name, native, prefix, weight FROM languages WHERE enabled=1" );
    req->stmt_find_language_by_prefix = database_prepare( req, "SELECT id, name, native, prefix FROM languages WHERE prefix=?" );

    // Short versions for i18n_string
    req->stmt_add_source = database_prepare( req, "INSERT INTO `locales_source` (id, source) VALUES(null, ?)" );
    req->stmt_quick_find_source = database_prepare( req, "SELECT `id` FROM `locales_source` WHERE source=?" );
    req->stmt_quick_find_target = database_prepare( req, "SELECT `translation` FROM `locales_target` WHERE lid=? AND language=?" );

    return true;
}

// #############################################################################

bool i18n_local_destroy( struct NNCMS_THREAD_INFO *req )
{

    database_finalize( req, req->stmt_add_locales_target );
    database_finalize( req, req->stmt_edit_locales_target );
    database_finalize( req, req->stmt_find_locales_target );
    database_finalize( req, req->stmt_find_locales_source );
    database_finalize( req, req->stmt_count_locales_source );
    database_finalize( req, req->stmt_list_locales_source );
    database_finalize( req, req->stmt_list_language_enabled );
    database_finalize( req, req->stmt_find_language_by_prefix );
    
    database_finalize( req, req->stmt_add_source );
    database_finalize( req, req->stmt_quick_find_source );
    database_finalize( req, req->stmt_quick_find_target );

    return true;
}

// #############################################################################

// This function returns localized string.
// Tries to find string in locales_source, if not found then add it.
// If found then fetch id number and find localized string in locales_target.
// Returned string should be freed with g_free() after it was used.
char *i18n_string( struct NNCMS_THREAD_INFO *req, char *format, va_list args )
{
    GString *source = g_string_sized_new( 50 );
    
    // Parse variable number of arguments
    if( args )
        g_string_vprintf( source, format, args );
    else
        g_string_assign( source, format );
    
    // Find string in locales_source
    database_bind_text( req->stmt_quick_find_source, 1, source->str );
    struct NNCMS_ROW *source_row = database_steps( req, req->stmt_quick_find_source );
    if( source_row == 0 )
    {
        // Not found, then add it
        database_bind_text( req->stmt_add_source, 1, source->str );
        database_steps( req, req->stmt_add_source );
        
        return g_string_free( source, FALSE );
    }
    
    // Find localized string in locales_target
    database_bind_text( req->stmt_quick_find_target, 1, source_row->value[0] );
    database_bind_text( req->stmt_quick_find_target, 2, req->language );
    struct NNCMS_ROW *target_row = database_steps( req, req->stmt_quick_find_target );
    
    database_free_rows( source_row );
    
    if( target_row == 0 )
    {
        // No translation found
        return g_string_free( source, FALSE );
    }
    
    g_string_free( source, TRUE );
    char *translation = g_strdup( target_row->value[0] );
    database_free_rows( target_row );
    
    return translation;
}

// This function adds string to garbage collector automatically
char *i18n_string_temp( struct NNCMS_THREAD_INFO *req, char *i18n_name, struct NNCMS_VARIABLE *vars )
{
    //va_list args;
    //va_start( args, format );
    //char *str = i18n_string( req, format, args );
    //va_end( args );

    // Get string from the database
    char *str = i18n_string( req, i18n_name, NULL );

    if( vars != 0 )
    {
        // Prepare output buffer
        GString *gtext = g_string_sized_new( 50 );

        // Parse the string
        template_sparse( req, i18n_name, str, gtext, vars );
        
        // String from database is not needed anymore
        g_free( str );

        //char *text = g_string_free( gtext, FALSE );
        garbage_add( req->loop_garbage, gtext, MEMORY_GARBAGE_GSTRING_FREE );
        return gtext->str;
    }
    else
    {
        garbage_add( req->loop_garbage, str, MEMORY_GARBAGE_GFREE );
        return str;
    }
}

// #############################################################################

void i18n_detect_language( struct NNCMS_THREAD_INFO *req )
{
    // User interface text language detection
    // If a translation of user interface text is available in the detected language, it will be displayed.
    
    // Default - Use the default site language
    req->language = "en";
    
    // URL - Determine the language from the URL (Path prefix or domain).	
    char *url_lang = main_variable_get( req, req->get_tree, "language" );
    if( url_lang != 0 ) req->language = url_lang;

    // Session - Determine the language from a request/session parameter.	
    
    // User - Follow the user's language preference.	
    
    // Browser - Determine the language from the browser's language settings.
    
}

// #############################################################################

void i18n_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "i18n_view_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "i18n", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get list of languages
    struct NNCMS_LANGUAGE_ROW *lang_row = (struct NNCMS_LANGUAGE_ROW *) database_steps( req, req->stmt_list_language_enabled );
    if( lang_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, lang_row, MEMORY_GARBAGE_DB_FREE );

    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "source", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };
    
    struct NNCMS_SORT sort[] =
    {
        { .field_name = "source", .direction = NNCMS_SORT_ASCENDING },
        { .direction = NNCMS_SORT_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .table = "locales_source",
        .filter = filter,
        .sort = sort,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_LOCALES_SOURCE_ROW *i18n_row = (struct NNCMS_LOCALES_SOURCE_ROW *) database_select( req, &query_select );
    if( i18n_row != NULL )
    {
        garbage_add( req->loop_garbage, i18n_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "i18n_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "i18n_source_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; i18n_row != NULL && i18n_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "i18n_id", .value.string = i18n_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        //char *view = main_temp_link( req, "i18n_view", i18n_string_temp( req, "view", NULL ), vars );
        //char *edit = main_temp_link( req, "i18n_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "i18n_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Iterate through enabled languages
        GString *lang_links = g_string_sized_new( 100 );
        garbage_add( req->loop_garbage, lang_links, MEMORY_GARBAGE_GSTRING_FREE );
        for( unsigned int j = 0; lang_row != NULL && lang_row[j].id != NULL; j = j + 1 )
        {
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "i18n_id", .value.string = i18n_row[i].id, .type = NNCMS_TYPE_STRING },
                { .name = "i18n_lang", .value.string = lang_row[j].prefix, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            
            // Find localized string in locales_target
            bool translated = false;
            database_bind_text( req->stmt_quick_find_target, 1, i18n_row[i].id );
            database_bind_text( req->stmt_quick_find_target, 2, lang_row[j].prefix );
            struct NNCMS_LOCALES_TARGET_ROW *target_row = (struct NNCMS_LOCALES_TARGET_ROW *) database_steps( req, req->stmt_quick_find_target );
            if( target_row != NULL )
            {
                translated = true;
                database_free_rows( (struct NNCMS_ROW *) target_row );
            }
            
            // Append a link to the list
            char *link = main_temp_link( req, "i18n_edit", lang_row[j].prefix, vars );
            if( translated == false )
            {
                g_string_append( lang_links, "<s>" );
                g_string_append( lang_links, link );
                g_string_append( lang_links, "</s>" );
            }
            else
            {
                g_string_append( lang_links, link );
            }
            
            // Delimiter at the end is not required
            if( lang_row[j + 1].id != NULL )
                g_string_append( lang_links, " " );
        }

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = i18n_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = i18n_row[i].source, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = lang_links->str, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = delete, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
    }

    // Create a table
    struct NNCMS_TABLE table =
    {
        .caption = NULL,
        .header_html = filter_html( req, &query_select ), .footer_html = NULL,
        .options = NULL,
        .cellpadding = NULL, .cellspacing = NULL,
        .border = NULL, .bgcolor = NULL,
        .width = NULL, .height = NULL,
        .row_count = row_count,
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/preferences-desktop-locale.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

void i18n_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "i18n", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "i18n_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }
    
    // Get desired language
    char *httpVarLang = main_variable_get( req, req->post_tree, "i18n_lang" );
    if( httpVarLang == 0 )
    {
        // Look for language in GET tree
        httpVarLang = main_variable_get( req, req->get_tree, "i18n_lang" );

        if( httpVarLang == 0 )
        {
            main_vmessage( req, "no_data" );
            return;
        }
    }
    
    // Check if such language exists
    database_bind_text( req->stmt_find_language_by_prefix, 1, httpVarLang );
    struct NNCMS_LANGUAGE_ROW *lang_row = (struct NNCMS_LANGUAGE_ROW *) database_steps( req, req->stmt_find_language_by_prefix );
    if( lang_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, lang_row, MEMORY_GARBAGE_DB_FREE );

    // Get the 'source' string from id
    database_bind_text( req->stmt_find_locales_source, 1, httpVarId );
    struct NNCMS_LOCALES_SOURCE_ROW *source_row = (struct NNCMS_LOCALES_SOURCE_ROW *) database_steps( req, req->stmt_find_locales_source );
    if( source_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, source_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "i18n_id", .value.string = source_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "i18n_source", .value.string = source_row->source, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "i18n_edit_header", vars );

    // Try to find string in 'target' table, it may or may not exist.
    // If it exists, then the value will be passed to editbox and when
    // submit button is pressed the row will be editted.
    // If not, then editbox will be empty and when submit button is
    // pressed the row will be added.
    database_bind_text( req->stmt_find_locales_target, 1, source_row->id );
    database_bind_text( req->stmt_find_locales_target, 2, lang_row->prefix );
    struct NNCMS_LOCALES_TARGET_ROW *target_row = (struct NNCMS_LOCALES_TARGET_ROW *) database_steps( req, req->stmt_find_locales_target );
    if( target_row != NULL )
    {
        garbage_add( req->loop_garbage, target_row, MEMORY_GARBAGE_DB_FREE );
    }

    //
    // Check if user commit changes
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "i18n_edit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Get POST data
        char *httpVarTranslation = main_variable_get( req, req->post_tree, "i18n_translation" );
        if( httpVarTranslation == 0 )
        {
            main_vmessage( req, "no_data" );
            return;
        }

        // Query Database
        if( target_row == NULL )
        {
            // Row doesn't exist in 'target' table, it needed to be added
            database_bind_text( req->stmt_add_locales_target, 1, source_row->id );
            database_bind_text( req->stmt_add_locales_target, 2, lang_row->prefix );
            database_bind_text( req->stmt_add_locales_target, 3, httpVarTranslation );
            database_steps( req, req->stmt_add_locales_target );
            unsigned int last_rowid = database_last_rowid( req );
            
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "i18n_id", .value.unsigned_integer = last_rowid, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "i18n_lid", .value.string = source_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "i18n_edit_success", vars );
            log_printf( req, LOG_ACTION, "String '%s:%s' was added", lang_row->prefix, source_row->source );
        }
        else
        {
            // Row already exists in the tables, it needed to be editted
            database_bind_text( req->stmt_edit_locales_target, 1, httpVarTranslation );
            database_bind_text( req->stmt_edit_locales_target, 2, target_row->id );
            database_steps( req, req->stmt_edit_locales_target );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "i18n_id", .value.string = source_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .name = "i18n_lid", .value.string = target_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "i18n_edit_success", vars );
            log_printf( req, LOG_ACTION, "String '%s:%s' was editted", lang_row->prefix, source_row->source );
        }

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    //
    // Form
    //

    struct NNCMS_FIELD fields[] =
    {
        { .name = "i18n_lid", .value = source_row->id, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "i18n_source", .value = source_row->source, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "i18n_lang", .value = lang_row->prefix, .data = NULL, .type = NNCMS_FIELD_LANGUAGE, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "i18n_translation", .value = (target_row != NULL ? target_row->translation : ""), .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "referer", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = req->session_id, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "i18n_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };

    struct NNCMS_FORM form =
    {
        .name = "i18n_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = fields
    };

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
    {
        { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
        { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
        { .name = "icon", .value.string = "images/apps/preferences-desktop-locale.png", .type = NNCMS_TYPE_STRING },
        { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE } // Terminating row
    };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################
