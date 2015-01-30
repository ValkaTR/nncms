// #############################################################################
// Source file: xxx.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "xxx.h"
#include "acl.h"
#include "template.h"
#include "user.h"
#include "log.h"
#include "i18n.h"
#include "memory.h"
#include "filter.h"

// #############################################################################
// includes of system headers
//

// #############################################################################
// global variables
//

struct NNCMS_XXX_FIELDS
{
    struct NNCMS_FIELD xxx_id;
    struct NNCMS_FIELD xxx_data;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD xxx_add;
    struct NNCMS_FIELD xxx_edit;
    struct NNCMS_FIELD none;
}
xxx_fields =
{
    { .name = "xxx_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "xxx_data", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "xxx_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "xxx_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// #############################################################################
// functions

bool xxx_global_init( )
{
    main_local_init_add( &xxx_local_init );
    main_local_destroy_add( &xxx_local_destroy );

    main_page_add( "xxx_list", &xxx_list );
    main_page_add( "xxx_add", &xxx_add );
    main_page_add( "xxx_edit", &xxx_edit );
    main_page_add( "xxx_view", &xxx_view );
    main_page_add( "xxx_delete", &xxx_delete );
    
    return true;
}

// #############################################################################

bool xxx_global_destroy( )
{
    return true;
}

// #############################################################################

bool xxx_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_list_xxx = database_prepare( req, "SELECT id, data FROM xxx LIMIT ? OFFSET ?" );
    req->stmt_count_xxx = database_prepare( req, "SELECT COUNT(*) FROM xxx" );
    req->stmt_find_xxx = database_prepare( req, "SELECT id, data FROM xxx WHERE id=?" );
    req->stmt_add_xxx = database_prepare( req, "INSERT INTO xxx (id, data) VALUES(null, ?)" );
    req->stmt_edit_xxx = database_prepare( req, "UPDATE xxx SET data=? WHERE id=?" );
    req->stmt_delete_xxx = database_prepare( req, "DELETE FROM xxx WHERE id=?" );

    return true;
}

// #############################################################################

bool xxx_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_list_xxx );
    database_finalize( req, req->stmt_count_xxx );
    database_finalize( req, req->stmt_find_xxx );
    database_finalize( req, req->stmt_add_xxx );
    database_finalize( req, req->stmt_edit_xxx );
    database_finalize( req, req->stmt_delete_xxx );

    return true;
}

// #############################################################################

void xxx_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "xxx", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "xxx_list_header", NULL );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_xxx );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_xxx, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_xxx, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_XXX_ROW *xxx_row = (struct NNCMS_XXX_ROW *) database_steps( req, req->stmt_list_xxx );
    if( xxx_row != NULL )
    {
        garbage_add( req->loop_garbage, xxx_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "xxx_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "xxx_data_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; xxx_row != NULL && xxx_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "xxx_id", .value.string = xxx_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "xxx_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "xxx_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "xxx_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = xxx_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = xxx_row[i].data, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = edit, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = delete, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .row_count = atoi( row_count->value[0] ),
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );

    // Generate links
    char *links = xxx_links( req, NULL );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/preferences-system-authentication.png", .type = NNCMS_TYPE_STRING },
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

void xxx_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "xxx", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "xxx_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "xxx_add_header", vars );

    //
    // Form
    //
    struct NNCMS_XXX_FIELDS *fields = memdup_temp( req, &xxx_fields, sizeof(xxx_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->xxx_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "xxx_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *xxx_add = main_variable_get( req, req->post_tree, "xxx_add" );
    if( xxx_add != 0 )
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
            // Query Database
            database_bind_text( req->stmt_add_xxx, 1, fields->xxx_data.value );
            database_steps( req, req->stmt_add_xxx );
            unsigned int xxx_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "xxx_id", .value.unsigned_integer = xxx_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "xxx_add_success", vars );
            log_printf( req, LOG_ACTION, "Xxx was added (id = %u)", xxx_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = xxx_links( req, NULL );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/preferences-system-authentication.png", .type = NNCMS_TYPE_STRING },
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

void xxx_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "xxx", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *xxx_id = main_variable_get( req, req->get_tree, "xxx_id" );
    if( xxx_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_xxx, 1, xxx_id );
    struct NNCMS_XXX_ROW *xxx_row = (struct NNCMS_XXX_ROW *) database_steps( req, req->stmt_find_xxx );
    if( xxx_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, xxx_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "xxx_id", .value.string = xxx_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "xxx_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_XXX_FIELDS *fields = memdup_temp( req, &xxx_fields, sizeof(xxx_fields) );
    fields->xxx_id.value = xxx_row->id;
    fields->xxx_data.value = xxx_row->data;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->xxx_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "xxx_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *xxx_edit = main_variable_get( req, req->post_tree, "xxx_edit" );
    if( xxx_edit != 0 )
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
            // Query Database
            database_bind_text( req->stmt_edit_xxx, 1, fields->xxx_data.value );
            database_bind_text( req->stmt_edit_xxx, 2, xxx_id );
            database_steps( req, req->stmt_edit_xxx );

            log_vdisplayf( req, LOG_ACTION, "xxx_edit_success", vars );
            log_printf( req, LOG_ACTION, "Xxx '%s' was editted", xxx_row->data );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = xxx_links( req, xxx_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/preferences-system-authentication.png", .type = NNCMS_TYPE_STRING },
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

void xxx_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "xxx", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *xxx_id = main_variable_get( req, req->get_tree, "xxx_id" );
    if( xxx_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_xxx, 1, xxx_id );
    struct NNCMS_XXX_ROW *xxx_row = (struct NNCMS_XXX_ROW *) database_steps( req, req->stmt_find_xxx );
    if( xxx_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, xxx_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "xxx_id", .value.string = xxx_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "xxx_view_header", vars );

    //
    // Form
    //
    struct NNCMS_XXX_FIELDS *fields = memdup_temp( req, &xxx_fields, sizeof(xxx_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->xxx_id.value = xxx_row->id;
    fields->xxx_data.value = xxx_row->data;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "xxx_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = xxx_links( req, xxx_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/preferences-system-authentication.png", .type = NNCMS_TYPE_STRING },
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

void xxx_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "xxx", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *xxx_id = main_variable_get( req, req->get_tree, "xxx_id" );
    if( xxx_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "xxx_id", .value.string = xxx_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "xxx_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_xxx, 1, xxx_id );
    struct NNCMS_XXX_ROW *xxx_row = (struct NNCMS_XXX_ROW *) database_steps( req, req->stmt_find_xxx );
    if( xxx_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, xxx_row, MEMORY_GARBAGE_DB_FREE );

    // Did user pressed button?
    char *delete_submit = main_variable_get( req, req->post_tree, "delete_submit" );
    if( delete_submit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Query Database
        database_bind_text( req->stmt_delete_xxx, 1, xxx_id );
        database_steps( req, req->stmt_delete_xxx );

        log_vdisplayf( req, LOG_ACTION, "xxx_delete_success", vars );
        log_printf( req, LOG_ACTION, "Xxx '%s' was deleted", xxx_row->data );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, xxx_id );

    // Generate links
    char *links = xxx_links( req, xxx_id );

    // Html output
    char *html = form_html( req, form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/edit-delete.png", .type = NNCMS_TYPE_STRING },
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

char *xxx_links( struct NNCMS_THREAD_INFO *req, char *xxx_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "xxx_id", .value.string = xxx_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "xxx_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    link.function = "xxx_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( xxx_id != NULL )
    {
        link.function = "xxx_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "xxx_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "xxx_delete";
        link.title = i18n_string_temp( req, "delete_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );
    }

    // Convert arrays to HTML code
    char *html = template_links( req, (struct NNCMS_LINK *) links->data );
    garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GFREE );

    // Free array
    g_array_free( links, TRUE );
    
    return html;
}

// #############################################################################
