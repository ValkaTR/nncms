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
#include "group.h"
#include "template.h"
#include "log.h"
#include "smart.h"
#include "memory.h"
#include "i18n.h"

#include "gettok.h"

// #############################################################################
// global variables
//

struct NNCMS_ACL_FIELDS
{
    struct NNCMS_FIELD acl_id;
    struct NNCMS_FIELD acl_name;
    struct NNCMS_FIELD acl_subject;
    struct NNCMS_FIELD acl_perm;
    struct NNCMS_FIELD acl_grant;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD acl_add;
    struct NNCMS_FIELD acl_edit;
    struct NNCMS_FIELD none;
}
acl_fields =
{
    { .name = "acl_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "acl_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "acl_subject", .value = NULL, .data = NULL, .type = NNCMS_FIELD_GROUP, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "acl_perm", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "acl_grant", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "acl_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "acl_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// Prepare the fields for the form
struct NNCMS_SELECT_ITEM acl_grant_options[] =
{
    { .name = "0", .desc = "Not allowed", .selected = false },
    { .name = "1", .desc = "Allowed", .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

// #############################################################################
// function declarations

char *acl_links( struct NNCMS_THREAD_INFO *req, char *acl_id );

// #############################################################################
// functions

bool acl_global_init( )
{
    main_local_init_add( &acl_local_init );
    main_local_destroy_add( &acl_local_destroy );

    main_page_add( "acl_add", &acl_add );
    main_page_add( "acl_edit", &acl_edit );
    main_page_add( "acl_delete", &acl_delete );
    main_page_add( "acl_list", &acl_list );
    main_page_add( "acl_view", &acl_view );

    return true;
}

// #############################################################################

bool acl_global_destroy( )
{
    return true;
}

// #############################################################################

bool acl_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_add_acl = database_prepare( req, "INSERT INTO acl VALUES(null, ?, ?, ?, ?)" );
    req->stmt_find_acl_by_id = database_prepare( req, "SELECT id, name, subject, perm, grant FROM acl WHERE id=?" );
    req->stmt_find_acl_by_name = database_prepare( req, "SELECT id, name, subject, perm, grant FROM acl WHERE name=?" );
    req->stmt_count_acl = database_prepare( req, "SELECT COUNT(*) FROM acl" );
    req->stmt_list_acl = database_prepare( req, "SELECT id, name, subject, perm, grant FROM acl ORDER BY name ASC, subject ASC LIMIT ? OFFSET ?" );
    req->stmt_edit_acl = database_prepare( req, "UPDATE acl SET name=?, subject=?, perm=?, grant=? WHERE id=?" );
    req->stmt_delete_acl = database_prepare( req, "DELETE FROM acl WHERE id=?" );
    req->stmt_delete_object = database_prepare( req, "DELETE FROM acl WHERE name=?" );
    req->stmtGetACLId = database_prepare( req, "SELECT id, name, subject, perm, grant FROM acl WHERE name=?" );
    req->stmt_get_acl_name = database_prepare( req, "SELECT id, subject, perm, grant FROM acl WHERE name=?" );
    req->stmt_find_user = database_prepare( req, "SELECT id, name, nick, `group`, hash FROM `users` WHERE `name`=?" );
    req->stmt_check_acl_perm = database_prepare( req, "SELECT id, name, subject, perm, grant FROM acl WHERE name=? AND subject=? AND perm=?" );

    return true;
}

// #############################################################################

bool acl_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_add_acl );
    database_finalize( req, req->stmt_find_acl_by_id );
    database_finalize( req, req->stmt_find_acl_by_name );
    database_finalize( req, req->stmt_count_acl );
    database_finalize( req, req->stmt_edit_acl );
    database_finalize( req, req->stmt_delete_acl );
    database_finalize( req, req->stmt_delete_object );
    database_finalize( req, req->stmtGetACLId );
    database_finalize( req, req->stmt_get_acl_name );
    database_finalize( req, req->stmt_list_acl );
    database_finalize( req, req->stmt_find_user );
    database_finalize( req, req->stmt_check_acl_perm );

    return true;
}

// #############################################################################

bool acl_test_perm( struct NNCMS_THREAD_INFO *req, char *name, char *subject, char *permission )
{
    // Find row
    database_bind_text( req->stmt_check_acl_perm, 1, name );
    database_bind_text( req->stmt_check_acl_perm, 2, subject );
    database_bind_text( req->stmt_check_acl_perm, 3, permission );
    struct NNCMS_ACL_ROW *acl_row = (struct NNCMS_ACL_ROW *) database_steps( req, req->stmt_check_acl_perm );
    if( acl_row != NULL )
    {
        char grant = acl_row->grant[0];
        database_free_rows( (struct NNCMS_ROW *) acl_row );

        // ACL found, return it's value
        if( grant == '1' )
            return true;
        else
            return false;
    }

    // Add empty permission
    database_bind_text( req->stmt_add_acl, 1, name );
    database_bind_text( req->stmt_add_acl, 2, subject );
    database_bind_text( req->stmt_add_acl, 3, permission );
    database_bind_text( req->stmt_add_acl, 4, "0" );
    database_steps( req, req->stmt_add_acl );

    return false;
}

bool acl_check_perm( struct NNCMS_THREAD_INFO *req, char *name, char *subject, char *permission )
{
    // Get group list of the user
    struct NNCMS_USERGROUP_ROW *group_row = NULL;
    GArray *user_groups = NULL;
    char **user_group_array = NULL;
    if( subject != NULL )
    {
        database_bind_text( req->stmt_find_ug_by_user_id, 1, subject );
        group_row = (struct NNCMS_USERGROUP_ROW *) database_steps( req, req->stmt_find_ug_by_user_id );
        if( group_row == NULL ) return false;
        user_groups = g_array_new( true, false, sizeof(char *) );
        for( int i = 0; group_row != NULL && group_row[i].id != NULL; i = i + 1 )
        {
            g_array_append_vals( user_groups, &group_row[i].group_id, 1 );
        }
        user_group_array = (char **) user_groups->data;
    }
    else
    {
        // Current user
        user_group_array = req->group_id;
    }

    for( int i = 0; user_group_array != NULL && user_group_array[i] != NULL; i = i + 1 )
    {
        bool result = acl_test_perm( req, name, user_group_array[i], permission );

        if( group_row != NULL ) database_free_rows( (struct NNCMS_ROW *) group_row );
        if( user_groups != NULL ) g_array_free( user_groups, true );

        if( result == true ) return true;
    }

    return false;
}

// #############################################################################

void acl_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "acl_add_header", NULL );

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "acl", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    //
    // Form
    //
    struct NNCMS_OPTION *options = memdup_temp( req, &acl_grant_options, sizeof(acl_grant_options) );
    struct NNCMS_ACL_FIELDS *fields = memdup_temp( req, &acl_fields, sizeof(acl_fields) );
    fields->acl_grant.data = options;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->acl_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "acl_add", .action = NULL, .method = "POST",
        .title = NULL, .help = i18n_string_temp( req, "acl_add_help", NULL ),
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *acl_add = main_variable_get( req, req->post_tree, "acl_add" );
    if( acl_add != 0 )
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
            database_bind_text( req->stmt_add_acl, 1, fields->acl_name.value );
            database_bind_text( req->stmt_add_acl, 2, fields->acl_subject.value );
            database_bind_text( req->stmt_add_acl, 3, fields->acl_perm.value );
            database_bind_text( req->stmt_add_acl, 4, fields->acl_grant.value );
            database_steps( req, req->stmt_add_acl );
            unsigned int acl_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "acl_id", .value.unsigned_integer = acl_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "acl_add_success", vars );
            log_printf( req, LOG_ACTION, "ACL was added (id = %i)", acl_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = acl_links( req, NULL );

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

void acl_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "acl", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *acl_id = main_variable_get( req, req->get_tree, "acl_id" );
    if( acl_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find rows associated with our object by 'id'
    database_bind_text( req->stmt_find_acl_by_id, 1, acl_id );
    struct NNCMS_ACL_ROW *acl_row = (struct NNCMS_ACL_ROW *) database_steps( req, req->stmt_find_acl_by_id );
    if( acl_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, acl_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "acl_id", .value.string = acl_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "acl_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_OPTION *options = memdup_temp( req, &acl_grant_options, sizeof(acl_grant_options) );
    struct NNCMS_ACL_FIELDS *fields = memdup_temp( req, &acl_fields, sizeof(acl_fields) );
    fields->acl_id.value = acl_row->id;
    fields->acl_name.value = acl_row->name;
    fields->acl_subject.value = acl_row->subject;
    fields->acl_perm.value = acl_row->perm;
    fields->acl_grant.value = acl_row->grant;
    fields->acl_grant.data = options;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->acl_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "acl_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *acl_edit = main_variable_get( req, req->post_tree, "acl_edit" );
    if( acl_edit != 0 )
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
            database_bind_text( req->stmt_edit_acl, 1, fields->acl_name.value );
            database_bind_text( req->stmt_edit_acl, 2, fields->acl_subject.value );
            database_bind_text( req->stmt_edit_acl, 3, fields->acl_perm.value );
            database_bind_text( req->stmt_edit_acl, 4, fields->acl_grant.value );
            database_bind_text( req->stmt_edit_acl, 5, acl_id );
            database_steps( req, req->stmt_edit_acl );

            log_printf( req, LOG_ACTION, "ACL was edited (id = '%s')", acl_id );
            log_vdisplayf( req, LOG_ACTION, "acl_edit_success", vars );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = acl_links( req, acl_row->id );

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

void acl_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "acl", NULL, "delete" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get id or object
    char *acl_id = main_variable_get( req, req->get_tree, "acl_id" );
    if( acl_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find rows associated with our object by 'id'
    database_bind_text( req->stmt_find_acl_by_id, 1, acl_id );
    struct NNCMS_ACL_ROW *acl_row = (struct NNCMS_ACL_ROW *) database_steps( req, req->stmt_find_acl_by_id );
    if( acl_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, acl_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "acl_id", .value.string = acl_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "acl_delete_header", vars );

    // Check if user commit changes
    char *delete_submit = main_variable_get( req, req->post_tree, "delete_submit" );
    if( delete_submit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Query Database
        database_bind_text( req->stmt_delete_acl, 1, acl_id );
        database_steps( req, req->stmt_delete_acl );

        log_printf( req, LOG_ACTION, "ACL was deleted (id = '%s')", acl_id );
        log_vdisplayf( req, LOG_ACTION, "acl_delete_success", vars );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Generate links
    char *links = acl_links( req, acl_row->id );

    struct NNCMS_FORM *form = template_confirm( req, acl_row->id );

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

void acl_list( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "acl_list_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "acl", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Generate links
    char *links = acl_links( req, NULL );

    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "name", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .field_name = "subject", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };

    struct NNCMS_SORT sort[] =
    {
        { .field_name = "name", .direction = NNCMS_SORT_ASCENDING },
        { .field_name = "subject", .direction = NNCMS_SORT_ASCENDING },
        { .direction = NNCMS_SORT_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .table = "acl",
        .filter = filter,
        .sort = sort,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_ACL_ROW *acl_row = (struct NNCMS_ACL_ROW *) database_select( req, &query_select );
    if( acl_row != NULL )
    {
        garbage_add( req->loop_garbage, acl_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "acl_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "acl_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "acl_subject_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "acl_perm_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "acl_grant_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; acl_row != NULL && acl_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "acl_id", .value.string = acl_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "acl_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "acl_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "acl_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = acl_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = acl_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = acl_row[i].subject, .type = TEMPLATE_TYPE_GROUP, .options = NULL },
            { .value.string = acl_row[i].perm, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = acl_row[i].grant, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        //.column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        //.pager_quantity = 0, .pages_displayed = 0,
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

void acl_view( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "acl", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "acl_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find rows associated with our object by 'id'
    database_bind_text( req->stmt_find_acl_by_id, 1, httpVarId );
    struct NNCMS_ACL_ROW *acl_row = (struct NNCMS_ACL_ROW *) database_steps( req, req->stmt_find_acl_by_id );
    if( acl_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, acl_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "acl_id", .value.string = httpVarId, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "acl_view_header", vars );

    // Generate links
    char *links = acl_links( req, acl_row->id );

    //
    // Form
    //
    struct NNCMS_OPTION *options = memdup_temp( req, &acl_grant_options, sizeof(acl_grant_options) );
    struct NNCMS_ACL_FIELDS *fields = memdup_temp( req, &acl_fields, sizeof(acl_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->acl_id.value = acl_row->id;
    fields->acl_name.value = acl_row->name;
    fields->acl_subject.value = acl_row->subject;
    fields->acl_perm.value = acl_row->perm;
    fields->acl_grant.value = acl_row->grant;
    fields->acl_grant.data = options;
    fields->referer.viewable = false;
    fields->fkey.viewable = false;

    struct NNCMS_FORM form =
    {
        .name = "acl_view", .action = NULL, .method = "POST",
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

char *acl_links( struct NNCMS_THREAD_INFO *req, char *acl_id )
{
    struct NNCMS_VARIABLE vars_id[] =
    {
        { .name = "acl_id", .value.string = acl_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "acl_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = vars_id;
    g_array_append_vals( links, &link, 1 );

    link.function = "acl_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = vars_id;
    g_array_append_vals( links, &link, 1 );

    if( acl_id != NULL )
    {
        link.function = "acl_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "acl_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "acl_delete";
        link.title = i18n_string_temp( req, "delete_link", NULL );
        link.vars = vars_id;
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
