// #############################################################################
// Source file: database.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "database.h"
#include "log.h"
#include "template.h"
#include "threadinfo.h"
#include "user.h"
#include "acl.h"
#include "filter.h"
#include "i18n.h"

#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <glib.h>

// #############################################################################
// global variables
//

// SQLite3 sturcture
sqlite3 *db = 0;

// Path to database
char szDatabasePath[NNCMS_PATH_LEN_MAX] = "./nncms.db";

char *zEmpty = "";

struct NNCMS_ROW empty_row;

struct NNCMS_VARIABLE filter_operator_list[] =
{
    { .name = "none", .value.integer = NNCMS_OPERATOR_NONE, .type = NNCMS_TYPE_INTEGER },
    { .name = "equal", .value.integer = NNCMS_OPERATOR_EQUAL, .type = NNCMS_TYPE_INTEGER },
    { .name = "notequal", .value.integer = NNCMS_OPERATOR_NOTEQUAL, .type = NNCMS_TYPE_INTEGER },
    { .name = "like", .value.integer = NNCMS_OPERATOR_LIKE, .type = NNCMS_TYPE_INTEGER },
    { .name = "glob", .value.integer = NNCMS_OPERATOR_GLOB, .type = NNCMS_TYPE_INTEGER },
    { .name = "less", .value.integer = NNCMS_OPERATOR_LESS, .type = NNCMS_TYPE_INTEGER },
    { .name = "less_equal", .value.integer = NNCMS_OPERATOR_LESS_EQUAL, .type = NNCMS_TYPE_INTEGER },
    { .name = "greater", .value.integer = NNCMS_OPERATOR_GREATER, .type = NNCMS_TYPE_INTEGER },
    { .name = "greater_equal", .value.integer = NNCMS_OPERATOR_GREATER_EQUAL, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

struct NNCMS_VARIABLE filter_operator_sql_list[] =
{
    { .name = "none", .value.integer = NNCMS_OPERATOR_NONE, .type = NNCMS_TYPE_INTEGER },
    { .name = "==", .value.integer = NNCMS_OPERATOR_EQUAL, .type = NNCMS_TYPE_INTEGER },
    { .name = "!=", .value.integer = NNCMS_OPERATOR_NOTEQUAL, .type = NNCMS_TYPE_INTEGER },
    { .name = "LIKE", .value.integer = NNCMS_OPERATOR_LIKE, .type = NNCMS_TYPE_INTEGER },
    { .name = "GLOB", .value.integer = NNCMS_OPERATOR_GLOB, .type = NNCMS_TYPE_INTEGER },
    { .name = "<", .value.integer = NNCMS_OPERATOR_LESS, .type = NNCMS_TYPE_INTEGER },
    { .name = "<=", .value.integer = NNCMS_OPERATOR_LESS_EQUAL, .type = NNCMS_TYPE_INTEGER },
    { .name = ">", .value.integer = NNCMS_OPERATOR_GREATER, .type = NNCMS_TYPE_INTEGER },
    { .name = ">=", .value.integer = NNCMS_OPERATOR_GREATER_EQUAL, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

// #############################################################################
// functions

// Init module
bool database_global_init( )
{
    //
    // Open database
    //
    if( sqlite3_open( szDatabasePath, &db ) )
    {
        log_printf( 0, LOG_CRITICAL, "Can't open database: %s", sqlite3_errmsg( db ) );
        sqlite3_close( db );
        db = 0;
        return false;
    }

    main_local_init_add( &database_local_init );
    main_local_destroy_add( &database_local_destroy );

    main_page_add( "db_tables", &database_tables );
    main_page_add( "db_rows", &database_rows );
    main_page_add( "db_edit", &database_edit );
    main_page_add( "db_add", &database_add );
    main_page_add( "db_delete", &database_delete );
    main_page_add( "db_view", &database_view );

    memset( &empty_row, 0, sizeof(empty_row) );

    return true;
}

// #############################################################################

// DeInit module
bool database_global_destroy( )
{
    //
    // After we finish, we should close database
    //

    sqlite3_close( db );
    db = 0;

    return true;
}

// #############################################################################

bool database_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    //near "?": syntax error
    //req->stmt_find_by_id = database_prepare( req, "SELECT * FROM ? WHERE id=?" );

    return true;
}

// #############################################################################

bool database_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    //database_finalize( req->stmt_find_by_id );

    return true;
}

// #############################################################################

void database_tables( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "db_tables_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "database", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get table list
    struct NNCMS_ROW *row_count = database_query( req, "SELECT COUNT(*) FROM sqlite_master WHERE type='table'" );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    struct NNCMS_MASTER_ROW *table_list = (struct NNCMS_MASTER_ROW *) database_query( req, "SELECT type, name, tbl_name, rootpage, sql FROM sqlite_master WHERE type='table' LIMIT %i OFFSET %i", default_pager_quantity, atoi(http_start != NULL ? http_start : "0") );
    garbage_add( req->loop_garbage, table_list, MEMORY_GARBAGE_DB_FREE );

    // Generate links
    char *links = database_links( req, NULL, NULL );

    //
    // Display list of tables
    //

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "table_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; table_list != NULL && table_list[i].name != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "table_name", .value.string = table_list[i].name, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *rows = main_temp_link( req, "db_rows", i18n_string_temp( req, "rows", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = table_list[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = rows, .type = NNCMS_TYPE_STRING, .options = NULL },
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

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/places/server-database.png", .type = NNCMS_TYPE_STRING },
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

void database_rows( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "db_rows_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "database", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    char *http_table_name = main_variable_get( req, req->get_tree, "table_name" );
    if( http_table_name == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    //
    // Display selected table
    //

    // Generate links
    char *links = database_links( req, NULL, http_table_name );

    //
    // Fetch column names
    //
    
    struct NNCMS_TABLE_INFO_ROW *table_columns = (struct NNCMS_TABLE_INFO_ROW *) database_query( req, "PRAGMA table_info(%q);", http_table_name );
    if( table_columns == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, table_columns, MEMORY_GARBAGE_DB_FREE );

    struct NNCMS_ROW *row_count = database_query( req, "SELECT COUNT(*) FROM %q", http_table_name );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    struct NNCMS_ROW *table_rows = (struct NNCMS_ROW *) database_query( req, "SELECT * FROM %q LIMIT %i OFFSET %i", http_table_name, default_pager_quantity, atoi(http_start != NULL ? http_start : "0") );
    if( table_rows != 0 )
    {
        garbage_add( req->loop_garbage, table_rows, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    GArray *gheader_cells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gheader_cells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; table_columns != NULL && table_columns[i].name != NULL; i = i + 1 )
    {
        struct NNCMS_TABLE_CELL header_cells[] =
        {
            { .value = table_columns[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };
        g_array_append_vals( gheader_cells, &header_cells, sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
    }
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };
    g_array_append_vals( gheader_cells, &header_cells, sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; table_rows != NULL && table_rows[i].value[0] != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "table_name", .value.string = http_table_name, .type = NNCMS_TYPE_STRING },
            { .name = "row_id", .value.string = table_rows[i].value[0], .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        // Loop thru columns
        for( unsigned int j = 0; table_rows[i].value[j] != NULL; j++ )
        {
            struct NNCMS_TABLE_CELL cells[] =
            {
                { .value.string = table_rows[i].value[j], .type = NNCMS_TYPE_STRING, .options = NULL },
                { .type = NNCMS_TYPE_NONE }
            };
            g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
        }

        char *view = main_temp_link( req, "db_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "db_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "db_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
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
        .column_count = gheader_cells->len,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = (struct NNCMS_TABLE_CELL *) gheader_cells->data,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Generate HTML table
    char *html = table_html( req, &table );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/places/server-database.png", .type = NNCMS_TYPE_STRING },
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

void database_add( struct NNCMS_THREAD_INFO *req )
{

    // First we check what permissions do user have
    if( acl_check_perm( req, "database", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    char *http_table_name = main_variable_get( req, req->get_tree, "table_name" );
    if( http_table_name == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Fetch column names
    struct NNCMS_TABLE_INFO_ROW *table_columns = (struct NNCMS_TABLE_INFO_ROW *) database_query( req, "PRAGMA table_info(%q);", http_table_name );
    if( table_columns == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, table_columns, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "table_name", .value.string = http_table_name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "db_add_header", vars );
        
    char *http_add = main_variable_get( req, req->post_tree, "db_add" );
    if( http_add != NULL )
    {
        //
        // Add row
        //

        // Dynamic sql statement
        GString *dynamic_sql = g_string_sized_new( 50 );
        garbage_add( req->loop_garbage, dynamic_sql, MEMORY_GARBAGE_GSTRING_FREE );
        g_string_append_printf( dynamic_sql, "INSERT INTO %s VALUES(", http_table_name );
        for( unsigned int i = 0; table_columns[i].name != NULL; i++ )
        {
            if( strcmp( table_columns[i].name, "id" ) == 0 )
                g_string_append( dynamic_sql, "null" );
            else
                g_string_append( dynamic_sql, "?" );

            if( table_columns[i + 1].name != NULL )
                g_string_append( dynamic_sql, ", " );
        }
        g_string_append( dynamic_sql, ")" );

        // Prepare the statement
        sqlite3_stmt *stmt_dynamic_sql = database_prepare( req, dynamic_sql->str );
        if( stmt_dynamic_sql != NULL )
        {
            // Bind values
            unsigned int j = 1;
            for( unsigned int i = 0; table_columns[i].cid != NULL; i = i + 1 )
            {
                // Get cell value from form
                char *http_temp = main_variable_get( req, req->post_tree, table_columns[i].name );

                if( strcmp( table_columns[i].name, "id" ) == 0 )
                    continue;

                if( http_temp == NULL )
                    http_temp = "";

                database_bind_text( stmt_dynamic_sql, j, http_temp );
                
                j = j + 1;
            }
            
            // Execute the sql statement
            database_steps( req, stmt_dynamic_sql );
            
            unsigned int row_id = database_last_rowid( req );
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "row_id", .value.unsigned_integer = row_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "table_name", .value.string = http_table_name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "db_row_add_success", vars );
            log_printf( req, LOG_ACTION, "Inserted row (id = %i) to table '%s'", row_id, http_table_name );
            
            database_finalize( req, stmt_dynamic_sql );
            
            redirect_to_referer( req );
            return;
        }
        else
        {
            log_vdisplayf( req, LOG_ERROR, "db_row_add_fail", vars );
            log_printf( req, LOG_ERROR, "Unable to prepare statement for table '%s'", http_table_name );
        }
    }

    // Generate links
    char *links = database_links( req, NULL, http_table_name );

    //
    // Form
    //
    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; table_columns[i].name != NULL; i++ )
    {
        if( strcmp( table_columns[i].name, "id" ) == 0 )
        {
            continue;
        }

        struct NNCMS_FIELD field =
        {
            .name = table_columns[i].name,
            .value = NULL,
            .data = NULL, .type = NNCMS_FIELD_EDITBOX,
            .values_count = 1,
            .editable = true, .viewable = true,
            .text_name = NULL, .text_description = NULL
        };
        
        g_array_append_vals( gfields, &field, 1 );
    }

    struct NNCMS_FIELD fields[] =
    {
        { .name = "referer", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = req->session_id, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "db_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };

    g_array_append_vals( gfields, &fields, 3 );

    struct NNCMS_FORM form =
    {
        .name = "db_add", .action = main_temp_link( req, "db_add", NULL, vars ), .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) gfields->data
    };

    // Generate HTML table
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/places/server-database.png", .type = NNCMS_TYPE_STRING },
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

void database_delete( struct NNCMS_THREAD_INFO *req )
{

    // First we check what permissions do user have
    if( acl_check_perm( req, "database", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    char *http_table_name = main_variable_get( req, req->get_tree, "table_name" );
    char *http_id = main_variable_get( req, req->get_tree, "row_id" );
    if( http_table_name == 0 || http_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "row_id", .value.string = http_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "table_name", .value.string = http_table_name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "db_delete_header", vars );

    char *http_delete = main_variable_get( req, req->post_tree, "delete_submit" );
    if( http_delete != 0 )
    {
        //
        // Actual editing
        //

        // Delete row
        database_query( req, "delete from %q where id='%i'", http_table_name, atoi(http_id) );

        log_vdisplayf( req, LOG_ACTION, "db_row_delete_success", vars );
        log_printf( req, LOG_ACTION, "Deleted row (id = %s) from table '%s'", http_id, http_table_name );

        redirect_to_referer( req );
        return;
    }

    //
    // Delete item form
    //

    // Generate links
    char *links = database_links( req, http_id, http_table_name );

    struct NNCMS_FORM *form = template_confirm( req, http_id );
    //form->action = main_temp_link( req, "db_edit", NULL, vars );
    form->title = http_table_name;

    // Generate HTML table
    char *html = form_html( req, form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/places/server-database.png", .type = NNCMS_TYPE_STRING },
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

void database_edit( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "database", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    char *http_table_name = main_variable_get( req, req->get_tree, "table_name" );
    char *http_id = main_variable_get( req, req->get_tree, "row_id" );
    if( http_table_name == 0 || http_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Fetch column names
    struct NNCMS_ROW *row = database_query( req, "SELECT * FROM %q WHERE id=%i;", http_table_name, atoi(http_id) );
    if( row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "table_name", .value.string = http_table_name, .type = NNCMS_TYPE_STRING },
        { .name = "row_id", .value.string = http_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "db_edit_header", NULL );

    char *http_edit = main_variable_get( req, req->post_tree, "db_edit" );
    if( http_edit != 0 )
    {
        //
        // Actual editing
        //

        // Query column names
        struct NNCMS_TABLE_INFO_ROW *table_columns = (struct NNCMS_TABLE_INFO_ROW *) database_query( req, "PRAGMA table_info(%q);", http_table_name );
        if( table_columns == 0 )
        {
            main_vmessage( req, "not_found" );
            return;
        }
        garbage_add( req->loop_garbage, table_columns, MEMORY_GARBAGE_DB_FREE );
        
        // Update every column
        for( unsigned int i = 0; table_columns[i].cid != NULL; i = i + 1 )
        {
            // Get cell value from form
            char *http_temp = main_variable_get( req, req->post_tree, table_columns[i].name );
            
            if( http_temp != 0 && strcmp( table_columns[i].name, "id" ) != 0 )
            {
                database_query( req, "UPDATE %q SET %q='%q' WHERE id='%s'",
                    http_table_name, table_columns[i].name, http_temp, http_id );
            }
        }

        log_vdisplayf( req, LOG_ACTION, "db_row_edit_success", vars );
        log_printf( req, LOG_ACTION, "Updated row (id = %s) in table '%s'", http_id, http_table_name );

        redirect_to_referer( req );
        return;
    }

    //
    // Edit item form
    //

    // Generate links
    char *links = database_links( req, http_id, http_table_name );

    //
    // Form
    //
    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; row->name[i]; i++ )
    {
        struct NNCMS_FIELD field =
        {
            .name = row->name[i],
            .value = row->value[i],
            .data = NULL, .type = NNCMS_FIELD_EDITBOX,
            .values_count = 1,
            .editable = true, .viewable = true,
            .text_name = NULL, .text_description = NULL
        };

        if( strcmp( row->name[i], "id" ) == 0 )
        {
            field.editable = false;
        }
        
        g_array_append_vals( gfields, &field, 1 );
    }

    struct NNCMS_FIELD fields[] =
    {
        { .name = "referer", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = req->session_id, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "db_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };

    g_array_append_vals( gfields, &fields, 3 );

    struct NNCMS_FORM form =
    {
        .name = "db_edit", .action = main_temp_link( req, "db_edit", NULL, vars ), .method = "POST",
        .title = http_table_name, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) gfields->data
    };

    // Generate HTML table
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/places/server-database.png", .type = NNCMS_TYPE_STRING },
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

void database_view( struct NNCMS_THREAD_INFO *req )
{

    // First we check what permissions do user have
    if( acl_check_perm( req, "database", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    char *http_table_name = main_variable_get( req, req->get_tree, "table_name" );
    char *http_id = main_variable_get( req, req->get_tree, "row_id" );
    if( http_table_name == 0 || http_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "table_name", .value.string = http_table_name, .type = NNCMS_TYPE_STRING },
        { .name = "row_id", .value.string = http_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "db_view_header", NULL );

    // Fetch column names
    struct NNCMS_ROW *row = database_query( req, "select * from %q where id=%i;", http_table_name, atoi(http_id) );
    if( row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, row, MEMORY_GARBAGE_DB_FREE );

    // Generate links
    char *links = database_links( req, http_id, http_table_name );

    //
    // Edit item form
    //
    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; row->name[i]; i++ )
    {
        if( strcmp( row->name[i], "id" ) == 0 )
        {
            struct NNCMS_FIELD field =
            {
                .name = "row_id",
                .value = row->value[i],
                .data = NULL, .type = NNCMS_FIELD_TEXT,
                .values_count = 1,
                .editable = false, .viewable = true,
                .text_name = NULL, .text_description = NULL
            };
            
            g_array_append_vals( gfields, &field, 1 );
            
            continue;
        }

        struct NNCMS_FIELD field =
        {
            .name = row->name[i],
            .value = row->value[i],
            .data = NULL, .type = NNCMS_FIELD_EDITBOX,
            .values_count = 1,
            .editable = false, .viewable = true,
            .text_name = NULL, .text_description = NULL
        };
        
        g_array_append_vals( gfields, &field, 1 );
    }

    struct NNCMS_FORM form =
    {
        .name = "db_edit", .action = main_temp_link( req, "db_edit", NULL, vars ), .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) gfields->data
    };

    // Generate HTML table
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/places/server-database.png", .type = NNCMS_TYPE_STRING },
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

sqlite3_stmt *database_prepare( struct NNCMS_THREAD_INFO *req, const char *zSql )
{
    const char *zLeftOver;      /* Tail of unprocessed SQL */
    sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
    int nResult = sqlite3_prepare_v2(
            db,             /* Database handle */
            zSql,           /* SQL statement, UTF-8 encoded */
            -1,             /* Maximum length of zSql in bytes. */
            &pStmt,         /* OUT: Statement handle */
            &zLeftOver      /* OUT: Pointer to unused portion of zSql */
        );

    if( nResult != SQLITE_OK )
    {
        log_printf( req, LOG_ERROR, "SQLite Error: %s (zSql = %s)", sqlite3_errmsg( db ), zSql );
        return NULL;
    }
    return pStmt;
}

// #############################################################################

int database_finalize( struct NNCMS_THREAD_INFO *req, sqlite3_stmt *pStmt )
{
    return sqlite3_finalize( pStmt );
}

// #############################################################################

/*
gdb programm `ps -C nncms -o pid=`
break database_steps
continue

*/
struct NNCMS_ROW *database_steps( struct NNCMS_THREAD_INFO *req, sqlite3_stmt *stmt )
{
    // Don't do a thing if DB is not connected
    if( db == NULL ) return NULL;

    // The sqlite3_reset() function is called to reset a prepared statement
    // object back to its initial state, ready to be re-executed. Any SQL
    // statement variables that had values bound to them using the
    // sqlite3_bind_*() API retain their values. Use sqlite3_clear_bindings()
    // to reset the bindings.
    //sqlite3_reset( stmt );

    // Temporary node for first row
    GArray *rows = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_ROW) );

    // The loop
    while( 1 )
    {
        int result = sqlite3_step( stmt );
        if( result == SQLITE_ROW )
        {
            // If the SQL statement being executed returns any data, then
            // SQLITE_ROW is returned each time a new row of data is ready for
            // processing by the caller. The values may be accessed using the
            // column access functions.

            struct NNCMS_ROW cur_row;
            //memset( &cur_row, 0, sizeof(struct NNCMS_ROW) );
            cur_row.name[0] = NULL;
            cur_row.value[0] = NULL;
            cur_row.next = NULL;

            // Number of columns in the result set returned by the prepared statement.
            int column_count = sqlite3_column_count( stmt );

            // Loop
            for( int column = 0; column < column_count; column++ )
            {
                // Zero next column
                cur_row.name[column + 1] = NULL;
                cur_row.value[column + 1] = NULL;

                // Store column name
                // This is only needed for database interface
                char *name = (char *) sqlite3_column_name( stmt, column );
                cur_row.name[column] = g_strdup( name );
                
                // Make memory for column value
                int bytes = sqlite3_column_bytes( stmt, column );
                if( bytes == 0 )
                {
                    cur_row.value[column] = zEmpty;
                    continue;
                }
                cur_row.value[column] = MALLOC( bytes + 1 );

                // Copy column
                char *text = (char *) sqlite3_column_text( stmt, column );
                memcpy( cur_row.value[column], text, bytes );
                cur_row.value[column][bytes] = 0; // Zero terminators at the end of the string
            }

            // Backward compability
            // Doesn't work, because resizing shifts pointers
            //if( rows->len > 0 )
            //{
            //    ((struct NNCMS_ROW *) rows->data) [rows->len - 1].next = & ((struct NNCMS_ROW *) rows->data) [rows->len];
            //}

            // Append row to array
            g_array_append_vals( rows, &cur_row, 1 );
            
            // Important about pointers from sqlite3_column_text
            //
            // The pointers returned are valid until a type conversion occurs as described above,
            // or until sqlite3_step() or sqlite3_reset() or sqlite3_finalize() is called. The
            // memory space used to hold strings and BLOBs is freed automatically. Do not pass
            // the pointers returned sqlite3_column_blob(), sqlite3_column_text(), etc. into
            // sqlite3_free().
        }
        else if( result == SQLITE_BUSY )
        {
            // SQLITE_BUSY means that the database engine was unable to acquire
            // the database locks it needs to do its job.
            continue;
        }
        else if( result == SQLITE_DONE )
        {
            // SQLITE_DONE means that the statement has finished executing
            // successfully. sqlite3_step() should not be called again on this
            // virtual machine without first calling sqlite3_reset() to reset
            // the virtual machine back to its initial state.
            sqlite3_reset( stmt );
            
            if( rows->len == 0 )
            {
                g_array_free( rows, TRUE );
                return NULL;
            }
            else
            {
                struct NNCMS_ROW *p_rows = (struct NNCMS_ROW *) g_array_free( rows, FALSE );
                
                // Backward compability
                for( unsigned int i = 1; p_rows[i].value[0]; i = i + 1 )
                    p_rows[i - 1].next = &p_rows[i];
                
                return p_rows;
            }
        }
        else //if( result == SQLITE_ERROR )
        {
            // SQLITE_ERROR means that a run-time error (such as a constraint
            // violation) has occurred. sqlite3_step() should not be called
            // again on the VM.
            sqlite3_reset( stmt );

            log_printf( req, LOG_ERROR, "SQLite Error: %s", sqlite3_errmsg( db ) );

            g_array_free( rows, TRUE );

            return &empty_row;
        }
    }
}

// #############################################################################

bool database_bind_text( sqlite3_stmt *pStmt, int iParam, char *lpText )
{
    // Don't do a thing if DB is not connected
    if( db == 0 ) return false;

    int nResult;
    if( lpText != 0 )
    {
        size_t nLen = strlen( lpText );
        nResult = sqlite3_bind_text( pStmt, iParam, lpText, nLen, SQLITE_STATIC );
    }
    else
    {
        nResult = sqlite3_bind_text( pStmt, iParam, "", -1, SQLITE_STATIC );
    }
    if( nResult != SQLITE_OK )
    {
        log_printf( 0, LOG_DEBUG, "%s", sqlite3_errmsg( db ) );
        return false;
    }
    return true;

}

// #############################################################################

bool database_bind_int( sqlite3_stmt *pStmt, int iParam, int iNum )
{
    // Don't do a thing if DB is not connected
    if( db == 0 ) return false;

    int nResult = sqlite3_bind_int( pStmt, iParam, iNum );
    if( nResult != SQLITE_OK )
    {
        log_printf( 0, LOG_DEBUG, "%s", sqlite3_errmsg( db ) );
        return false;
    }
    return true;
}

// #############################################################################

struct NNCMS_ROW *database_vquery( struct NNCMS_THREAD_INFO *req, char *query_format, va_list vArgs )
{
    // This should hold sqlite error message
    char *zErrMsg = NULL;

    if( db == NULL ) return 0;

    // Format query string
    //char szQuery[NNCMS_QUERY_LEN_MAX];
    //vsnprintf( szQuery, NNCMS_QUERY_LEN_MAX, query_format, vArgs );
    char *lpszSQL = sqlite3_vmprintf( query_format, vArgs );

    // Debug
    //log_printf( req, LOG_DEBUG, "Query: %s\n", lpszSQL );
    //return 0;

    // Exec with sqlite3_prepare ... sqlite3_finalize
    sqlite3_stmt *stmt_dynamic_sql = database_prepare( req, lpszSQL );
    if( stmt_dynamic_sql == NULL )
        return NULL;

    struct NNCMS_ROW *rows = database_steps( req, stmt_dynamic_sql );
    
    database_finalize( req, stmt_dynamic_sql );
    
    return rows;

    // Exec with sqlite3_exec
/*
    int nResult = sqlite3_exec( db, lpszSQL, database_callback, req, &zErrMsg );
    sqlite3_free( lpszSQL );

    if( nResult != SQLITE_OK )
    {
        // Error. No data.
        log_printf( req, LOG_ALERT, "SQL Error: %s", zErrMsg );
        sqlite3_free( zErrMsg );
        return 0;
    }

    if( req != 0 )
    {
        struct NNCMS_ROW *tempRow = req->firstRow;
        req->firstRow = 0;
        return tempRow;
    }
    return 0;
*/
}

// #############################################################################

// Example:
//
/*

    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = "1" },
        { .field_name = "type", .operator = NNCMS_OPERATOR_EQUAL, .field_value = "f" },
        { .operator = NNCMS_OPERATOR_NONE }
    };

    struct NNCMS_SORT sort[] =
    {
        { .field_name = "type", .direction = NNCMS_SORT_ASCENDING },
        { .field_name = "name", .direction = NNCMS_SORT_ASCENDING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .op = NNCMS_QUERY_SELECT,
        .table = "files",
        .filter = filter,
        .sort = sort,
        .offset = 50,
        .limit = 25
    };

    database_select( req, &query_select );

    // SELECT * FROM files WHERE id=? AND type=? ORDER BY type ASC, name ASC LIMIT 25 OFFSET 50;
*/

struct NNCMS_ROW *database_select( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query )
{
    // Dynamic sql statement <-- I don't like this
    GString *dynamic_sql = g_string_sized_new( 50 );
    sqlite3_stmt *stmt_dynamic_sql = NULL;

    g_string_append( dynamic_sql, "SELECT *" );
    g_string_append_printf( dynamic_sql, " FROM %s", query->table );

    // Filter
    if( query->filter != NULL )
    {
        // Variable name = ?
        bool first = true;
        for( unsigned int i = 0; query->filter[i].operator != NNCMS_OPERATOR_NONE; i++ )
        {
            if( query->filter[i].field_value == NULL )
                continue;
            
            if( first == true )
            {
                g_string_append( dynamic_sql, " WHERE " );
            }
            
            // Comma separated result column
            if( first == false )
            {
                g_string_append( dynamic_sql, " AND " );
            }
            g_string_append( dynamic_sql, query->filter[i].field_name );
            switch( query->filter[i].operator )
            {
                default:
                case NNCMS_OPERATOR_EQUAL: { g_string_append( dynamic_sql, "=?" ); break; }
                case NNCMS_OPERATOR_NOTEQUAL: { g_string_append( dynamic_sql, "!=?" ); break; }
                case NNCMS_OPERATOR_LIKE: { g_string_append( dynamic_sql, " LIKE ?" ); break; }
                case NNCMS_OPERATOR_GLOB: { g_string_append( dynamic_sql, " GLOB ?" ); break; }
                case NNCMS_OPERATOR_LESS: { g_string_append( dynamic_sql, " < ?" ); break; }
                case NNCMS_OPERATOR_LESS_EQUAL: { g_string_append( dynamic_sql, " <= ?" ); break; }
                case NNCMS_OPERATOR_GREATER: { g_string_append( dynamic_sql, " > ?" ); break; }
                case NNCMS_OPERATOR_GREATER_EQUAL: { g_string_append( dynamic_sql, " >= ?" ); break; }
            }
            first = false;
        }
    }

    // Sort
    if( query->sort != NULL )
    {
        g_string_append( dynamic_sql, " ORDER BY " );
        
        // Variable name = ?
        for( unsigned int i = 0; query->sort[i].direction != NNCMS_SORT_NONE; i++ )
        {
            // Comma separated result column
            if( i != 0 )
            {
                g_string_append( dynamic_sql, ", " );
            }
            g_string_append( dynamic_sql, query->sort[i].field_name );
            g_string_append( dynamic_sql, " " );
            
            if( query->sort[i].direction == NNCMS_SORT_ASCENDING )
                g_string_append( dynamic_sql, "ASC" );
            else if( query->sort[i].direction == NNCMS_SORT_DESCENDING )
                g_string_append( dynamic_sql, "DESC" );
        }
    }

    // Limit
    if( query->limit != 0 )
        g_string_append_printf( dynamic_sql, " LIMIT %i", query->limit );

    // Offset
    if( query->limit != 0 )
        g_string_append_printf( dynamic_sql, " OFFSET %i", query->offset );

    //g_string_append( dynamic_sql, ";" );
    
    // Prepare the statement
    //log_printf( req, LOG_DEBUG, "Dynamic SQL = %s", dynamic_sql->str );
    stmt_dynamic_sql = database_prepare( req, dynamic_sql->str );

    // Dynamic statement is not required now
    g_string_free( dynamic_sql, TRUE );
    
    // Statement valid?
    if( stmt_dynamic_sql == NULL )
        return NULL;

    // Bind filter values
    if( query->filter != NULL )
    {
        bool first = true;
        unsigned int j = 1;
        for( unsigned int i = 0; query->filter[i].operator != NNCMS_OPERATOR_NONE; i = i + 1 )
        {
            if( query->filter[i].field_value == NULL )
                continue;
            
            //log_printf( req, LOG_DEBUG, "Bind text = %s (%i)", bind_text, i + 1 );
            bool result = database_bind_text( stmt_dynamic_sql, j, query->filter[i].field_value );
            j = j + 1;
            if( result == false )
            {
                database_finalize( req, stmt_dynamic_sql );
                return NULL;
            }
        }
    }

    // SQL expression is built, execute it
    struct NNCMS_ROW *rows = database_steps( req, stmt_dynamic_sql );
    database_finalize( req, stmt_dynamic_sql );

    // Done
    return rows;
}

int database_count( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query )
{
    // Dynamic sql statement <-- I don't like this
    GString *dynamic_sql = g_string_sized_new( 50 );
    sqlite3_stmt *stmt_dynamic_sql = NULL;

    g_string_append( dynamic_sql, "SELECT COUNT(*)" );
    g_string_append_printf( dynamic_sql, " FROM %s", query->table );

    // Filter
    if( query->filter != NULL )
    {
        // Variable name = ?
        bool first = true;
        for( unsigned int i = 0; query->filter[i].operator != NNCMS_OPERATOR_NONE; i++ )
        {
            if( query->filter[i].field_value == NULL )
                continue;

            if( first == true )
            {
                g_string_append( dynamic_sql, " WHERE " );
            }

            // Comma separated result column
            if( first == false )
            {
                g_string_append( dynamic_sql, " AND " );
            }
            g_string_append( dynamic_sql, query->filter[i].field_name );
            switch( query->filter[i].operator )
            {
                default:
                case NNCMS_OPERATOR_EQUAL: { g_string_append( dynamic_sql, "=?" ); break; }
                case NNCMS_OPERATOR_NOTEQUAL: { g_string_append( dynamic_sql, "!=?" ); break; }
                case NNCMS_OPERATOR_LIKE: { g_string_append( dynamic_sql, " LIKE ?" ); break; }
                case NNCMS_OPERATOR_GLOB: { g_string_append( dynamic_sql, " GLOB ?" ); break; }
                case NNCMS_OPERATOR_LESS: { g_string_append( dynamic_sql, " < ?" ); break; }
                case NNCMS_OPERATOR_LESS_EQUAL: { g_string_append( dynamic_sql, " <= ?" ); break; }
                case NNCMS_OPERATOR_GREATER: { g_string_append( dynamic_sql, " > ?" ); break; }
                case NNCMS_OPERATOR_GREATER_EQUAL: { g_string_append( dynamic_sql, " >= ?" ); break; }
            }
            first = false;
        }
    }

    //g_string_append( dynamic_sql, ";" );
    
    // Prepare the statement
    //log_printf( req, LOG_DEBUG, "Dynamic SQL = %s", dynamic_sql->str );
    stmt_dynamic_sql = database_prepare( req, dynamic_sql->str );

    // Dynamic statement is not required now
    g_string_free( dynamic_sql, TRUE );
    
    // Statement valid?
    if( stmt_dynamic_sql == NULL )
        return 0;

    // Bind variable values
    if( query->filter != NULL )
    {
        unsigned int j = 1;
        for( unsigned int i = 0; query->filter[i].operator != NNCMS_OPERATOR_NONE; i = i + 1 )
        {
            if( query->filter[i].field_value == NULL )
                continue;

            //log_printf( req, LOG_DEBUG, "Bind text = %s (%i)", bind_text, i + 1 );
            bool result = database_bind_text( stmt_dynamic_sql, j, query->filter[i].field_value );
            if( result == false )
            {
                database_finalize( req, stmt_dynamic_sql );
                return 0;
            }
            j = j + 1;
        }
    }

    // SQL expression is built, execute it
    struct NNCMS_ROW *rows = database_steps( req, stmt_dynamic_sql );
    database_finalize( req, stmt_dynamic_sql );

    int count = atoi( rows->value[0] );

    database_free_rows( rows );

    // Done
    return count;
}

// #############################################################################

struct NNCMS_ROW *database_query( struct NNCMS_THREAD_INFO *req, char *query_format, ... )
{
    va_list vArgs;
    va_start( vArgs, query_format );
    struct NNCMS_ROW *lpResult = database_vquery( req, query_format, vArgs );
    va_end( vArgs );

    return lpResult;
}

// #############################################################################

// Custom row fetcher
int database_callback( void *NotUsed, int argc, char **argv, char **azColName )
{
    // Global per thread structure
    struct NNCMS_THREAD_INFO *req = NotUsed;

    // If req == 0, then we are probably outside thread
    if( req == 0 )
        return 0;

    // Default indice
    int i;

    // Temporary node for current row, prepare it
    struct NNCMS_ROW *currentRow = g_malloc( sizeof(struct NNCMS_ROW) );
    memset( currentRow, 0, sizeof(struct NNCMS_ROW) );

    // When I was writing this code I was listening
    // to Armin Van Buuren - A State Of Trace, edit at
    // your risk (it may harm your brain)

    // Loop thru all columns (limited to NNCMS_COLUMNS_MAX)
    for( i = 0; i < (argc >= NNCMS_COLUMNS_MAX - 1 ? NNCMS_COLUMNS_MAX - 1 : argc); i++ )
    {
        // Fill currentRow with column name
        currentRow->name[i] = MALLOC( strlen(azColName[i]) + 1 );
        strcpy( currentRow->name[i], azColName[i] );

        // Fill currentRow with data
        currentRow->value[i] = MALLOC( strlen(argv[i] != 0 ? argv[i] : "0") + 1 );
        strcpy( currentRow->value[i], argv[i] != 0 ? argv[i] : "0" );

        // Debug:
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "0");
    }

    if( req->firstRow )
        // Next of last row should know about current row
        req->lastRow->next = currentRow;
    else
        // Save the first row, it will be used by default, so we wont
        // traverse backwards
        req->firstRow = currentRow;

    // Now current row becomes last row
    req->lastRow = currentRow;

    // Debug:
    //printf("===\n");
    return 0;
}

// #############################################################################

// Debug and template row traverse function
void database_traverse( struct NNCMS_ROW *row )
{
    int i = 0;

    // Check if row exists
    if( row != 0 )
    {
        // Loop thru columns
        for( i = 0; i < NNCMS_COLUMNS_MAX - 1; i++ )
        {
            // If data is empty, then skip this column and quit loop
            if( row->value[i] == 0 )
            {
                break;
            }
            else
            {
                // Otherwise print its data in stdout
                printf( "%s = %s\n", row->name[i], row->value[i] );
            }
        }

        // Simple divider, so we can extinguish different rows
        printf( "---\n" );

        // This function traverse forward. We can easilly change row->next to
        // row->prev and function will traverse backward.
        database_traverse( row->next );
    }
}

// #############################################################################

// Clean up after SQL query
void database_cleanup( struct NNCMS_THREAD_INFO *req )
{
    // Clean up
    database_free_rows( req->firstRow );
    req->firstRow = 0;
}

// #############################################################################

// Free memory from allocated memory for rows
void database_free_rows( struct NNCMS_ROW *row )
{
    // This function traverse forward and frees memory alocated data
    // until last row.
    if( row == NULL )
        return;

    // Check if row exists
    for( unsigned int i = 0; row[i].value[0]; i = i + 1 )
    {
        // Loop thru columns
        for( unsigned int j = 0; j < NNCMS_COLUMNS_MAX - 1; j++ )
        {
            // Check if variable is used and free memory
            if( row[i].value[j] != 0 && row[i].value[j] != zEmpty )
            {
                FREE( row[i].value[j] );
            }
            else if( row[i].value[j] == 0 )
            {
                break;
            }

            if( row[i].name[j] != 0 )
            {
                g_free( row[i].name[j] );
            }
        }
    }
    
    // Free the row itself
    g_free( row );
}

// #############################################################################

unsigned long int database_last_rowid( struct NNCMS_THREAD_INFO *req )
{
    unsigned long int retval = 0;
    
    if( db != 0 )
        retval = sqlite3_last_insert_rowid( db );
    
    return retval;
}

// #############################################################################

char *database_links( struct NNCMS_THREAD_INFO *req, char *row_id, char *table_name )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "table_name", .value.string = table_name, .type = NNCMS_TYPE_STRING },
        { .name = "row_id", .value.string = row_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "db_tables";
    link.title = i18n_string_temp( req, "tables_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( table_name != NULL )
    {
        link.function = "db_rows";
        link.title = i18n_string_temp( req, "list_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );
        
        link.function = "db_add";
        link.title = i18n_string_temp( req, "add_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );
    }

    if( table_name != NULL && row_id != NULL )
    {
        link.function = "db_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "db_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "db_delete";
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
