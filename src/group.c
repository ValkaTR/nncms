// #############################################################################
// Source file: group.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "group.h"
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

struct NNCMS_GROUP_FIELDS
{
    struct NNCMS_FIELD group_id;
    struct NNCMS_FIELD group_name;
    struct NNCMS_FIELD group_description;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD group_add;
    struct NNCMS_FIELD group_edit;
    struct NNCMS_FIELD none;
}
group_fields =
{
    { .name = "group_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "group_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "group_description", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "group_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "group_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_USERGROUP_FIELDS
{
    struct NNCMS_FIELD ug_id;
    struct NNCMS_FIELD user_id;
    struct NNCMS_FIELD group_id;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD ug_add;
    struct NNCMS_FIELD ug_edit;
    struct NNCMS_FIELD none;
}
ug_fields =
{
    { .name = "ug_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "group_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_GROUP, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "ug_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "ug_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// Special groups
char *admin_group_id;
char *user_group_id;
char *guest_group_id;
char *nobody_group_id;
char *owner_group_id;

// #############################################################################
// functions

bool group_global_init( )
{
    main_local_init_add( &group_local_init );
    main_local_destroy_add( &group_local_destroy );

    main_page_add( "group_list", &group_list );
    main_page_add( "group_add", &group_add );
    main_page_add( "group_edit", &group_edit );
    main_page_add( "group_view", &group_view );
    main_page_add( "group_delete", &group_delete );
    main_page_add( "ug_list", &ug_list );
    main_page_add( "ug_add", &ug_add );
    main_page_add( "ug_edit", &ug_edit );
    main_page_add( "ug_view", &ug_view );
    main_page_add( "ug_delete", &ug_delete );

    struct NNCMS_GROUP_ROW *group_row;
    
    group_row = (struct NNCMS_GROUP_ROW *) database_query( NULL, "SELECT id, name, description FROM groups WHERE name='admin'" );
    if( group_row == NULL ) return false;
    admin_group_id = group_row->id;
    garbage_add( global_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    
    group_row = (struct NNCMS_GROUP_ROW *) database_query( NULL, "SELECT id, name, description FROM groups WHERE name='user'" );
    if( group_row == NULL ) return false;
    user_group_id = group_row->id;
    garbage_add( global_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    
    group_row = (struct NNCMS_GROUP_ROW *) database_query( NULL, "SELECT id, name, description FROM groups WHERE name='guest'" );
    if( group_row == NULL ) return false;
    guest_group_id = group_row->id;
    garbage_add( global_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    
    group_row = (struct NNCMS_GROUP_ROW *) database_query( NULL, "SELECT id, name, description FROM groups WHERE name='nobody'" );
    if( group_row == NULL ) return false;
    nobody_group_id = group_row->id;
    garbage_add( global_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    
    group_row = (struct NNCMS_GROUP_ROW *) database_query( NULL, "SELECT id, name, description FROM groups WHERE name='owner'" );
    if( group_row == NULL ) return false;
    owner_group_id = group_row->id;
    garbage_add( global_garbage, group_row, MEMORY_GARBAGE_DB_FREE );

    return true;
}

// #############################################################################

bool group_global_destroy( )
{
    return true;
}

// #############################################################################

bool group_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_list_group = database_prepare( req, "SELECT id, name, description FROM groups LIMIT ? OFFSET ?" );
    req->stmt_count_group = database_prepare( req, "SELECT COUNT(*) FROM groups" );
    req->stmt_find_group = database_prepare( req, "SELECT id, name, description FROM groups WHERE id=?" );
    req->stmt_add_group = database_prepare( req, "INSERT INTO groups (id, name, description) VALUES(null, ?, ?)" );
    req->stmt_edit_group = database_prepare( req, "UPDATE groups SET name=?, description=? WHERE id=?" );
    req->stmt_delete_group = database_prepare( req, "DELETE FROM groups WHERE id=?" );

    req->stmt_list_ug = database_prepare( req, "SELECT id, user_id, group_id FROM usergroups LIMIT ? OFFSET ?" );
    req->stmt_count_ug = database_prepare( req, "SELECT COUNT(*) FROM usergroups" );
    req->stmt_find_ug = database_prepare( req, "SELECT id, user_id, group_id FROM usergroups WHERE id=?" );
    req->stmt_find_ug_by_group_id = database_prepare( req, "SELECT id, user_id, group_id FROM usergroups WHERE group_id=?" );
    req->stmt_find_ug_by_user_id = database_prepare( req, "SELECT id, user_id, group_id FROM usergroups WHERE user_id=?" );
    req->stmt_add_ug = database_prepare( req, "INSERT INTO usergroups (id, user_id, group_id) VALUES(null, ?, ?)" );
    req->stmt_edit_ug = database_prepare( req, "UPDATE usergroups SET user_id=?, group_id=? WHERE id=?" );
    req->stmt_delete_ug = database_prepare( req, "DELETE FROM usergroups WHERE id=?" );

    return true;
}

// #############################################################################

bool group_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_list_group );
    database_finalize( req, req->stmt_count_group );
    database_finalize( req, req->stmt_find_group );
    database_finalize( req, req->stmt_add_group );
    database_finalize( req, req->stmt_edit_group );
    database_finalize( req, req->stmt_delete_group );

    return true;
}

// #############################################################################

char *ug_list_html( struct NNCMS_THREAD_INFO *req, char *user_id, char *group_id )
{
    // Find rows
    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "user_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = user_id, .filter_exposed = false, .operator_exposed = false, .required = false },
        { .field_name = "group_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = group_id, .filter_exposed = false, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .table = "usergroups",
        .filter = filter,
        .sort = NULL,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_USERGROUP_ROW *ug_row = (struct NNCMS_USERGROUP_ROW *) database_select( req, &query_select );
    if( ug_row != NULL )
    {
        garbage_add( req->loop_garbage, ug_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "ug_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ug_user_id", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ug_group_id", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; ug_row != NULL && ug_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "ug_id", .value.string = ug_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "ug_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "ug_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "ug_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = ug_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ug_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
            { .value.string = ug_row[i].group_id, .type = TEMPLATE_TYPE_GROUP, .options = NULL },
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
        .row_count = row_count,
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );
    
    return html;
}

// #############################################################################

void group_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "group", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "group_list_header", NULL );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_group );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_group, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_group, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_list_group );
    if( group_row != NULL )
    {
        garbage_add( req->loop_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "group_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "group_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "group_description_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; group_row != NULL && group_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "group_id", .value.string = group_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "group_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "group_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "group_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = group_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = group_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = group_row[i].description, .type = NNCMS_TYPE_STRING, .options = NULL },
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

    {
        // Html output
        char *html = table_html( req, &table );

        // Generate links
        char *links = group_links( req, NULL );

        // Specify values for template
        struct NNCMS_VARIABLE frame_template[] =
            {
                { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
                { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
                { .name = "icon", .value.string = "images/actions/user-group-properties.png", .type = NNCMS_TYPE_STRING },
                { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
                { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };

        // Make a cute frame
        template_hparse( req, "frame.html", req->frame, frame_template );
    }

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

void group_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "group", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "group_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "group_add_header", vars );

    //
    // Form
    //
    struct NNCMS_GROUP_FIELDS *fields = memdup_temp( req, &group_fields, sizeof(group_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->group_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "group_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *group_add = main_variable_get( req, req->post_tree, "group_add" );
    if( group_add != 0 )
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
            database_bind_text( req->stmt_add_group, 1, fields->group_name.value );
            database_bind_text( req->stmt_add_group, 2, fields->group_description.value );
            database_steps( req, req->stmt_add_group );
            unsigned int group_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "group_id", .value.unsigned_integer = group_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "group_add_success", vars );
            log_printf( req, LOG_ACTION, "Group was added (id = %u)", group_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = group_links( req, NULL );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/user-group-new.png", .type = NNCMS_TYPE_STRING },
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

void group_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "group", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *group_id = main_variable_get( req, req->get_tree, "group_id" );
    if( group_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_group, 1, group_id );
    struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_find_group );
    if( group_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, group_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "group_id", .value.string = group_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "group_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_GROUP_FIELDS *fields = memdup_temp( req, &group_fields, sizeof(group_fields) );
    fields->group_id.value = group_row->id;
    fields->group_name.value = group_row->name;
    fields->group_description.value = group_row->description;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->group_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "group_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *group_edit = main_variable_get( req, req->post_tree, "group_edit" );
    if( group_edit != 0 )
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
            database_bind_text( req->stmt_edit_group, 1, fields->group_name.value );
            database_bind_text( req->stmt_edit_group, 2, fields->group_description.value );
            database_bind_text( req->stmt_edit_group, 3, group_id );
            database_steps( req, req->stmt_edit_group );

            log_vdisplayf( req, LOG_ACTION, "group_edit_success", vars );
            log_printf( req, LOG_ACTION, "Group '%s' was editted", group_row->name );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = group_links( req, group_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/user-group-properties.png", .type = NNCMS_TYPE_STRING },
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

void group_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "group", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *group_id = main_variable_get( req, req->get_tree, "group_id" );
    if( group_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_group, 1, group_id );
    struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_find_group );
    if( group_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, group_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "group_id", .value.string = group_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "group_view_header", vars );

    //
    // Form
    //
    struct NNCMS_GROUP_FIELDS *fields = memdup_temp( req, &group_fields, sizeof(group_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->group_id.value = group_row->id;
    fields->group_name.value = group_row->name;
    fields->group_description.value = group_row->description;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "group_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = group_links( req, group_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/user-group-properties.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    {
        // Html output
        char *html = ug_list_html( req, NULL, group_id );

        // Generate links
        char *links = ug_links( req, NULL );

        // Page header
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "group_id", .value.string = group_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        char *header_str = i18n_string_temp( req, "ug_list_header", vars );

        // Specify values for template
        struct NNCMS_VARIABLE frame_template[] =
            {
                { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
                { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
                { .name = "icon", .value.string = "images/actions/user-group-properties.png", .type = NNCMS_TYPE_STRING },
                { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
                { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };

        // Make a cute frame
        template_hparse( req, "frame.html", req->frame, frame_template );
    }

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

void group_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "group", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *group_id = main_variable_get( req, req->get_tree, "group_id" );
    if( group_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "group_id", .value.string = group_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "group_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_group, 1, group_id );
    struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_find_group );
    if( group_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, group_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_group, 1, group_id );
        database_steps( req, req->stmt_delete_group );

        log_vdisplayf( req, LOG_ACTION, "group_delete_success", vars );
        log_printf( req, LOG_ACTION, "Group '%s' was deleted", group_row->name );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, group_id );

    // Generate links
    char *links = group_links( req, group_id );

    // Html output
    char *html = form_html( req, form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/user-group-delete.png", .type = NNCMS_TYPE_STRING },
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

void ug_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "group", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "ug_list_header", NULL );

    // Find rows
    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "user_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .field_name = "group_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .table = "usergroups",
        .filter = filter,
        .sort = NULL,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_USERGROUP_ROW *ug_row = (struct NNCMS_USERGROUP_ROW *) database_select( req, &query_select );
    if( ug_row != NULL )
    {
        garbage_add( req->loop_garbage, ug_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "ug_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ug_user_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ug_group_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; ug_row != NULL && ug_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "ug_id", .value.string = ug_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "ug_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "ug_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "ug_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = ug_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ug_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
            { .value.string = ug_row[i].group_id, .type = NNCMS_TYPE_STRING, .options = NULL },
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

    // Generate links
    char *links = ug_links( req, NULL );

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

void ug_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "group", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ug_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "ug_add_header", vars );

    //
    // Form
    //
    struct NNCMS_USERGROUP_FIELDS *fields = memdup_temp( req, &ug_fields, sizeof(ug_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->ug_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "ug_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *ug_add = main_variable_get( req, req->post_tree, "ug_add" );
    if( ug_add != 0 )
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
            database_bind_text( req->stmt_add_ug, 1, fields->user_id.value );
            database_bind_text( req->stmt_add_ug, 2, fields->group_id.value );
            database_steps( req, req->stmt_add_ug );
            unsigned int ug_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "ug_id", .value.unsigned_integer = ug_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "ug_add_success", vars );
            log_printf( req, LOG_ACTION, "Usergroup was added (id = %u)", ug_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = ug_links( req, NULL );

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

void ug_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "group", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *ug_id = main_variable_get( req, req->get_tree, "ug_id" );
    if( ug_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_ug, 1, ug_id );
    struct NNCMS_USERGROUP_ROW *ug_row = (struct NNCMS_USERGROUP_ROW *) database_steps( req, req->stmt_find_ug );
    if( ug_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ug_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ug_id", .value.string = ug_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "ug_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_USERGROUP_FIELDS *fields = memdup_temp( req, &ug_fields, sizeof(ug_fields) );
    fields->ug_id.value = ug_row->id;
    fields->user_id.value = ug_row->user_id;
    fields->group_id.value = ug_row->group_id;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->ug_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "ug_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *ug_edit = main_variable_get( req, req->post_tree, "ug_edit" );
    if( ug_edit != 0 )
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
            database_bind_text( req->stmt_edit_ug, 1, fields->user_id.value );
            database_bind_text( req->stmt_edit_ug, 2, fields->group_id.value );
            database_bind_text( req->stmt_edit_ug, 3, ug_id );
            database_steps( req, req->stmt_edit_ug );

            log_vdisplayf( req, LOG_ACTION, "ug_edit_success", vars );
            log_printf( req, LOG_ACTION, "Usergroup '%s' was editted", ug_row->id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = ug_links( req, ug_row->id );

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

void ug_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "group", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *ug_id = main_variable_get( req, req->get_tree, "ug_id" );
    if( ug_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_ug, 1, ug_id );
    struct NNCMS_USERGROUP_ROW *ug_row = (struct NNCMS_USERGROUP_ROW *) database_steps( req, req->stmt_find_ug );
    if( ug_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ug_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ug_id", .value.string = ug_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "ug_view_header", vars );

    //
    // Form
    //
    struct NNCMS_USERGROUP_FIELDS *fields = memdup_temp( req, &ug_fields, sizeof(ug_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->ug_id.value = ug_row->id;
    fields->user_id.value = ug_row->user_id;
    fields->group_id.value = ug_row->group_id;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "ug_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = ug_links( req, ug_row->id );

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

void ug_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "group", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *ug_id = main_variable_get( req, req->get_tree, "ug_id" );
    if( ug_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ug_id", .value.string = ug_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "ug_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_ug, 1, ug_id );
    struct NNCMS_USERGROUP_ROW *ug_row = (struct NNCMS_USERGROUP_ROW *) database_steps( req, req->stmt_find_ug );
    if( ug_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ug_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_ug, 1, ug_id );
        database_steps( req, req->stmt_delete_ug );

        log_vdisplayf( req, LOG_ACTION, "ug_delete_success", vars );
        log_printf( req, LOG_ACTION, "Usergroup '%s' was deleted", ug_row->id );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, ug_id );

    // Generate links
    char *links = ug_links( req, ug_id );

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

char *group_links( struct NNCMS_THREAD_INFO *req, char *group_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "group_id", .value.string = group_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "group_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    link.function = "group_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( group_id != NULL )
    {
        link.function = "group_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "group_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "group_delete";
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

char *ug_links( struct NNCMS_THREAD_INFO *req, char *ug_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ug_id", .value.string = ug_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "ug_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    link.function = "ug_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( ug_id != NULL )
    {
        link.function = "ug_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "ug_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "ug_delete";
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
