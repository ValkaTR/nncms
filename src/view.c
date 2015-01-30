// #############################################################################
// Source file: view.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include "config.h"

#define _GNU_SOURCE
#include <string.h>

#ifdef HAVE_IMAGEMAGICK
#include <wand/MagickWand.h>
#endif // HAVE_IMAGEMAGICK

//#include <mgl/mgl_c.h>

// #############################################################################
// includes of local headers
//

#include "acl.h"
#include "field.h"
#include "view.h"
#include "main.h"
#include "template.h"
#include "mime.h"
#include "file.h"
#include "log.h"
#include "cache.h"
#include "i18n.h"
#include "node.h"
#include "filter.h"
#include "user.h"

#include "strlcpy.h"
#include "strlcat.h"

#include "memory.h"

// #############################################################################
// global variables
//

struct NNCMS_SELECT_ITEM view_formats[] =
{
    { .name = "list", .desc = NULL, .selected = false },
    { .name = "grid", .desc = NULL, .selected = false },
    { .name = "table", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_VARIABLE view_format_list[] =
{
    { .name = "unknown", .value.integer = 0, .type = NNCMS_TYPE_INTEGER },
    { .name = "list", .value.integer = 1, .type = NNCMS_TYPE_INTEGER },
    { .name = "grid", .value.integer = 2, .type = NNCMS_TYPE_INTEGER },
    { .name = "table", .value.integer = 3, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

struct NNCMS_VIEW_FIELDS
{
    struct NNCMS_FIELD view_id;
    struct NNCMS_FIELD view_name;
    struct NNCMS_FIELD view_format;
    struct NNCMS_FIELD view_header;
    struct NNCMS_FIELD view_footer;
    struct NNCMS_FIELD view_pager_items;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD view_add;
    struct NNCMS_FIELD view_edit;
    struct NNCMS_FIELD none;
}
view_fields =
{
    { .name = "view_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "view_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "view_format", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = view_formats }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "view_header", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 10000 },
    { .name = "view_footer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 10000 },
    { .name = "view_pager_items", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = numeric_validate },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "view_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "view_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_FIELD_VIEW_FIELDS
{
    struct NNCMS_FIELD field_view_id;
    struct NNCMS_FIELD field_view_view_id;
    struct NNCMS_FIELD field_view_field_id;
    struct NNCMS_FIELD field_view_label;
    struct NNCMS_FIELD field_view_empty_hide;
    struct NNCMS_FIELD field_view_config;
    struct NNCMS_FIELD field_view_weight;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD field_view_add;
    struct NNCMS_FIELD field_view_edit;
    struct NNCMS_FIELD none;
}
field_view_fields =
{
    { .name = "field_view_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "field_view_view_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_VIEW_ID, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "field_view_field_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_FIELD_ID, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "field_view_label", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = label_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "field_view_empty_hide", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = printable_validate },
    { .name = "field_view_config", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = printable_validate },
    { .name = "field_view_weight", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = numeric_validate },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "field_view_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "field_view_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_VIEWS_FILTER_FIELDS
{
    struct NNCMS_FIELD view_filter_id;
    struct NNCMS_FIELD view_filter_view_id;
    struct NNCMS_FIELD view_filter_field_id;
    struct NNCMS_FIELD view_filter_operator;
    struct NNCMS_FIELD view_filter_data;
    struct NNCMS_FIELD view_filter_data_exposed;
    struct NNCMS_FIELD view_filter_filter_exposed;
    struct NNCMS_FIELD view_filter_operator_exposed;
    struct NNCMS_FIELD view_filter_required;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD view_filter_add;
    struct NNCMS_FIELD view_filter_edit;
    struct NNCMS_FIELD none;
}
view_filter_fields =
{
    { .name = "view_filter_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "view_filter_view_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_VIEW_ID, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "view_filter_field_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_FIELD_ID, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "view_filter_operator", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = filter_operators }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = printable_validate },
    { .name = "view_filter_data", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "view_filter_data_exposed", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "view_filter_filter_exposed", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "view_filter_operator_exposed", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "view_filter_required", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "view_filter_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "view_filter_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// #############################################################################
// function declarations

char *view_links( struct NNCMS_THREAD_INFO *req, char *view_id );
char *view_sort_links( struct NNCMS_THREAD_INFO *req, char *view_sort_id, char *view_sort_view_id );
char *view_filter_links( struct NNCMS_THREAD_INFO *req, char *view_filter_id, char *view_filter_view_id );
char *field_view_links( struct NNCMS_THREAD_INFO *req, char *field_view_id, char *field_view_view_id );

// #############################################################################
// functions

bool view_global_init( )
{

#ifdef HAVE_IMAGEMAGICK
    // Initialize the MagickWand environment
    MagickWandGenesis();
#endif // HAVE_IMAGEMAGICK

    main_local_init_add( &view_local_init );
    main_local_destroy_add( &view_local_destroy );

    main_page_add( "view_display", &view_display );

    main_page_add( "view_list", &view_list );
    main_page_add( "view_add", &view_add );
    main_page_add( "view_edit", &view_edit );
    main_page_add( "view_view", &view_view );
    main_page_add( "view_delete", &view_delete );

    main_page_add( "field_view_list", &field_view_list );
    main_page_add( "field_view_add", &field_view_add );
    main_page_add( "field_view_edit", &field_view_edit );
    main_page_add( "field_view_view", &field_view_view );
    main_page_add( "field_view_delete", &field_view_delete );

    main_page_add( "view_filter_list", &view_filter_list );
    main_page_add( "view_filter_add", &view_filter_add );
    main_page_add( "view_filter_edit", &view_filter_edit );
    main_page_add( "view_filter_view", &view_filter_view );
    main_page_add( "view_filter_delete", &view_filter_delete );

    return true;
}

// #############################################################################

bool view_global_destroy( )
{
#ifdef HAVE_IMAGEMAGICK
        // Cleanup ImageMagick stuff
        MagickWandTerminus();
#endif

    return true;
}

// #############################################################################

bool view_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_list_view = database_prepare( req, "SELECT id, name, format, header, footer, pager_items FROM views LIMIT ? OFFSET ?" );
    req->stmt_count_view = database_prepare( req, "SELECT COUNT(*) FROM views" );
    req->stmt_find_view = database_prepare( req, "SELECT id, name, format, header, footer, pager_items FROM views WHERE id=?" );
    req->stmt_add_view = database_prepare( req, "INSERT INTO views (id, name, format, header, footer, pager_items) VALUES(null, ?, ?, ?, ?, ?)" );
    req->stmt_edit_view = database_prepare( req, "UPDATE views SET name=?, format=?, header=?, footer=?, pager_items=? WHERE id=?" );
    req->stmt_delete_view = database_prepare( req, "DELETE FROM views WHERE id=?" );

    req->stmt_list_field_view = database_prepare( req, "SELECT id, view_id, field_id, label, empty_hide, config, weight FROM field_view ORDER BY weight LIMIT ? OFFSET ?" );
    req->stmt_count_field_view = database_prepare( req, "SELECT COUNT(*) FROM field_view" );
    req->stmt_matching_field_view = database_prepare( req, "SELECT id, view_id, field_id, label, empty_hide, config, weight FROM field_view WHERE view_id=? AND field_id=?" );
    req->stmt_field_view_by_view_id = database_prepare( req, "SELECT id, view_id, field_id, label, empty_hide, config, weight FROM field_view WHERE view_id=? ORDER BY weight" );
    req->stmt_find_field_view = database_prepare( req, "SELECT id, view_id, field_id, label, empty_hide, config, weight FROM field_view WHERE id=?" );
    req->stmt_add_field_view = database_prepare( req, "INSERT INTO field_view (id, view_id, field_id, label, empty_hide, config, weight) VALUES(null, ?, ?, ?, ?, ?, ?)" );
    req->stmt_edit_field_view = database_prepare( req, "UPDATE field_view SET view_id=?, field_id=?, label=?, empty_hide=?, config=?, weight=? WHERE id=?" );
    req->stmt_delete_field_view = database_prepare( req, "DELETE FROM field_view WHERE id=?" );

    req->stmt_list_view_sort = database_prepare( req, "SELECT id, view_id, field_id, direction FROM views_sort LIMIT ? OFFSET ?" );
    req->stmt_count_view_sort = database_prepare( req, "SELECT COUNT(*) FROM views_sort" );
    req->stmt_find_view_sort_by_view_id = database_prepare( req, "SELECT id, view_id, field_id, direction FROM views_sort WHERE view_id=?" );
    req->stmt_find_view_sort = database_prepare( req, "SELECT id, view_id, field_id, direction FROM views_sort WHERE id=?" );
    req->stmt_add_view_sort = database_prepare( req, "INSERT INTO views_sort (id, view_id, field_id, direction) VALUES(null, ?, ?, ?)" );
    req->stmt_edit_view_sort = database_prepare( req, "UPDATE views_sort SET view_id=?, field_id=?, direction=? WHERE id=?" );
    req->stmt_delete_view_sort = database_prepare( req, "DELETE FROM views_sort WHERE id=?" );

    req->stmt_list_view_filter = database_prepare( req, "SELECT id, view_id, field_id, operator, data, data_exposed, filter_exposed, operator_exposed, required FROM views_filter LIMIT ? OFFSET ?" );
    req->stmt_count_view_filter = database_prepare( req, "SELECT COUNT(*) FROM views_filter" );
    req->stmt_find_view_filter_by_view_id = database_prepare( req, "SELECT id, view_id, field_id, operator, data, data_exposed, filter_exposed, operator_exposed, required FROM views_filter WHERE view_id=?" );
    req->stmt_find_view_filter = database_prepare( req, "SELECT id, view_id, field_id, operator, data, data_exposed, filter_exposed, operator_exposed, required FROM views_filter WHERE id=?" );
    req->stmt_add_view_filter = database_prepare( req, "INSERT INTO views_filter (id, view_id, field_id, operator, data, data_exposed, filter_exposed, operator_exposed, required) VALUES(null, ?, ?, ?, ?, ?, ?, ?, ?)" );
    req->stmt_edit_view_filter = database_prepare( req, "UPDATE views_filter SET view_id=?, field_id=?, operator=?, data=?, data_exposed=?, filter_exposed=?, operator_exposed=?, required=? WHERE id=?" );
    req->stmt_delete_view_filter = database_prepare( req, "DELETE FROM views_filter WHERE id=?" );

    return true;
}

// #############################################################################

bool view_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_list_view );
    database_finalize( req, req->stmt_count_view );
    database_finalize( req, req->stmt_find_view );
    database_finalize( req, req->stmt_add_view );
    database_finalize( req, req->stmt_edit_view );
    database_finalize( req, req->stmt_delete_view );

    database_finalize( req, req->stmt_list_field_view );
    database_finalize( req, req->stmt_count_field_view );
    database_finalize( req, req->stmt_field_view_by_view_id );
    database_finalize( req, req->stmt_matching_field_view );
    database_finalize( req, req->stmt_find_field_view );
    database_finalize( req, req->stmt_add_field_view );
    database_finalize( req, req->stmt_edit_field_view );
    database_finalize( req, req->stmt_delete_field_view );

    database_finalize( req, req->stmt_list_view_sort );
    database_finalize( req, req->stmt_count_view_sort );
    database_finalize( req, req->stmt_find_view_sort_by_view_id );
    database_finalize( req, req->stmt_find_view_sort );
    database_finalize( req, req->stmt_add_view_sort );
    database_finalize( req, req->stmt_edit_view_sort );
    database_finalize( req, req->stmt_delete_view_sort );

    database_finalize( req, req->stmt_list_view_filter );
    database_finalize( req, req->stmt_count_view_filter );
    database_finalize( req, req->stmt_find_view_filter_by_view_id );
    database_finalize( req, req->stmt_find_view_filter );
    database_finalize( req, req->stmt_add_view_filter );
    database_finalize( req, req->stmt_edit_view_filter );
    database_finalize( req, req->stmt_delete_view_filter );

    return true;
}

// #############################################################################

void view_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "view", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "view_list_header", NULL );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_view );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_view, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_view, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_list_view );
    if( view_row != NULL )
    {
        garbage_add( req->loop_garbage, view_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "view_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_format_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; view_row != NULL && view_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "view_id", .value.string = view_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *fields = main_temp_link( req, "view_display", i18n_string_temp( req, "display", NULL ), vars );
        char *view = main_temp_link( req, "view_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "view_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "view_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = view_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view_row[i].format, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = fields, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );

    // Generate links
    char *links = view_links( req, NULL );

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

void view_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "view", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_add_header", vars );

    //
    // Form
    //
    struct NNCMS_VIEW_FIELDS *fields = memdup_temp( req, &view_fields, sizeof(view_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->view_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "view_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *view_add = main_variable_get( req, req->post_tree, "view_add" );
    if( view_add != 0 )
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
            database_bind_text( req->stmt_add_view, 1, fields->view_name.value );
            database_bind_text( req->stmt_add_view, 2, fields->view_format.value );
            database_bind_text( req->stmt_add_view, 3, fields->view_header.value );
            database_bind_text( req->stmt_add_view, 4, fields->view_footer.value );
            database_steps( req, req->stmt_add_view );
            unsigned int view_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "view_id", .value.unsigned_integer = view_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "view_add_success", vars );
            log_printf( req, LOG_ACTION, "View '%s' was added (id = %u)", fields->view_name.value, view_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = view_links( req, NULL );

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

void view_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "view", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *view_id = main_variable_get( req, req->get_tree, "view_id" );
    if( view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_view, 1, view_id );
    struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_find_view );
    if( view_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_id", .value.string = view_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_VIEW_FIELDS *fields = memdup_temp( req, &view_fields, sizeof(view_fields) );
    fields->view_id.value = view_row->id;
    fields->view_name.value = view_row->name;
    fields->view_format.value = view_row->format;
    fields->view_header.value = view_row->header;
    fields->view_footer.value = view_row->footer;
    fields->view_pager_items.value = view_row->pager_items;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->view_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "view_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *view_edit = main_variable_get( req, req->post_tree, "view_edit" );
    if( view_edit != 0 )
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
            database_bind_text( req->stmt_edit_view, 1, fields->view_name.value );
            database_bind_text( req->stmt_edit_view, 2, fields->view_format.value );
            database_bind_text( req->stmt_edit_view, 3, fields->view_header.value );
            database_bind_text( req->stmt_edit_view, 4, fields->view_footer.value );
            database_bind_text( req->stmt_edit_view, 5, fields->view_pager_items.value );
            database_bind_text( req->stmt_edit_view, 6, view_id );
            database_steps( req, req->stmt_edit_view );

            log_vdisplayf( req, LOG_ACTION, "view_edit_success", vars );
            log_printf( req, LOG_ACTION, "View '%s' was editted", view_row->name );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = view_links( req, view_row->id );

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

void view_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "view", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *view_id = main_variable_get( req, req->get_tree, "view_id" );
    if( view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_view, 1, view_id );
    struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_find_view );
    if( view_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_id", .value.string = view_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_view_header", vars );

    //
    // Form
    //
    struct NNCMS_VIEW_FIELDS *fields = memdup_temp( req, &view_fields, sizeof(view_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->view_id.value = view_row->id;
    fields->view_name.value = view_row->name;
    fields->view_format.value = view_row->format;
    fields->view_header.value = view_row->header;
    fields->view_footer.value = view_row->footer;
    fields->view_pager_items.value = view_row->pager_items;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "view_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = view_links( req, view_row->id );

    // Html output
    char *view_html = form_html( req, &form );

    //
    // Fields list
    //
    char *field_view_html;
    {

    // Find rows
    database_bind_text( req->stmt_field_view_by_view_id, 1, view_id );
    struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_steps( req, req->stmt_field_view_by_view_id );
    if( field_view_row != NULL )
    {
        garbage_add( req->loop_garbage, field_view_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "field_view_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_view_field_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_view_label_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_view_empty_hide_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_view_config_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; field_view_row != NULL && field_view_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_view_id", .value.string = field_view_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "field_view_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "field_view_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "field_view_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = field_view_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = field_view_row[i].field_id, .type = TEMPLATE_TYPE_FIELD_CONFIG, .options = NULL },
            { .value.string = field_view_row[i].label, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = field_view_row[i].empty_hide, .type = NNCMS_TYPE_STR_BOOL, .options = NULL },
            { .value.string = field_view_row[i].config, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = edit, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = delete, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
    }

    // Generate links
    char *links = field_view_links( req, NULL, view_id );

    // Create a table
    struct NNCMS_TABLE table =
    {
        .caption = i18n_string_temp( req, "field_view_list_header", NULL ),
        .header_html = links, .footer_html = NULL,
        .options = NULL,
        .cellpadding = NULL, .cellspacing = NULL,
        .border = NULL, .bgcolor = NULL,
        .width = NULL, .height = NULL,
        .row_count = -1,
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    field_view_html = table_html( req, &table );

    }

    //
    // Filter table
    //
    char *filter_html;
    {

    // Find rows
    database_bind_text( req->stmt_find_view_filter_by_view_id, 1, view_id );
    struct NNCMS_VIEWS_FILTER_ROW *view_filter_row = (struct NNCMS_VIEWS_FILTER_ROW *) database_steps( req, req->stmt_find_view_filter_by_view_id );
    if( view_filter_row != NULL )
    {
        garbage_add( req->loop_garbage, view_filter_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "view_filter_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_filter_field_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_filter_operator_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_filter_data_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; view_filter_row != NULL && view_filter_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "view_filter_id", .value.string = view_filter_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "view_filter_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "view_filter_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "view_filter_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = view_filter_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view_filter_row[i].field_id, .type = TEMPLATE_TYPE_FIELD_CONFIG, .options = NULL },
            { .value.string = view_filter_row[i].operator, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view_filter_row[i].data, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = edit, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = delete, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
    }

    // Generate links
    char *links = view_filter_links( req, NULL, view_id );

    // Create a table
    struct NNCMS_TABLE table =
    {
        .caption = i18n_string_temp( req, "view_filter_list_header", NULL ),
        .header_html = links, .footer_html = NULL,
        .options = NULL,
        .cellpadding = NULL, .cellspacing = NULL,
        .border = NULL, .bgcolor = NULL,
        .width = NULL, .height = NULL,
        .row_count = -1,
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    filter_html = table_html( req, &table );

    }
    
    GString *output = g_string_sized_new( 100 );
    garbage_add( req->loop_garbage, output, MEMORY_GARBAGE_GSTRING_FREE );
    g_string_append( output, view_html );
    g_string_append( output, field_view_html );
    g_string_append( output, filter_html );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = output->str, .type = NNCMS_TYPE_STRING },
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

void view_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "view", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *view_id = main_variable_get( req, req->get_tree, "view_id" );
    if( view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_id", .value.string = view_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_view, 1, view_id );
    struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_find_view );
    if( view_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_view, 1, view_id );
        database_steps( req, req->stmt_delete_view );

        log_vdisplayf( req, LOG_ACTION, "view_delete_success", vars );
        log_printf( req, LOG_ACTION, "View '%s' was deleted", view_row->name );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, view_id );

    // Generate links
    char *links = view_links( req, view_id );

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

char *view_links( struct NNCMS_THREAD_INFO *req, char *view_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_id", .value.string = view_id, .type = NNCMS_TYPE_STRING },
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

    if( acl_check_perm( req, "view", NULL, "list" ) == true )
    {
        link.function = "view_list";
        link.title = i18n_string_temp( req, "list_link", NULL );
        link.vars = NULL;
        g_array_append_vals( links, &link, 1 );
    }

    if( acl_check_perm( req, "view", NULL, "add" ) == true )
    {
        link.function = "view_add";
        link.title = i18n_string_temp( req, "add_link", NULL );
        link.vars = NULL;
        g_array_append_vals( links, &link, 1 );
    }
    
    if( view_id != NULL )
    {
        if( acl_check_perm( req, "view", NULL, "view" ) == true )
        {
            link.function = "view_view";
            link.title = i18n_string_temp( req, "view_link", NULL );
            link.vars = vars;
            g_array_append_vals( links, &link, 1 );
        }

        if( acl_check_perm( req, "view", NULL, "display" ) == true )
        {
            link.function = "view_display";
            link.title = i18n_string_temp( req, "display_link", NULL );
            link.vars = vars;
            g_array_append_vals( links, &link, 1 );
        }

        if( acl_check_perm( req, "view", NULL, "edit" ) == true )
        {
            link.function = "view_edit";
            link.title = i18n_string_temp( req, "edit_link", NULL );
            link.vars = vars;
            g_array_append_vals( links, &link, 1 );
        }

        if( acl_check_perm( req, "view", NULL, "delete" ) == true )
        {
            link.function = "view_delete";
            link.title = i18n_string_temp( req, "delete_link", NULL );
            link.vars = vars;
            g_array_append_vals( links, &link, 1 );
        }
    }

    // Convert arrays to HTML code
    char *html = template_links( req, (struct NNCMS_LINK *) links->data );
    garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GFREE );

    // Free array
    g_array_free( links, TRUE );
    
    return html;
}

// #############################################################################

void field_view_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "field_view", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "field_view_list_header", NULL );

    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "view_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };
    
    struct NNCMS_SORT sort[] =
    {
        { .field_name = "weight", .direction = NNCMS_SORT_ASCENDING },
        { .direction = NNCMS_SORT_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .table = "field_view",
        .filter = filter,
        .sort = sort,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_select( req, &query_select );
    if( field_view_row != NULL )
    {
        garbage_add( req->loop_garbage, field_view_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Filter form
    char *filter_form = filter_html( req, &query_select );

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "field_view_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_view_field_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_view_weight_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; field_view_row != NULL && field_view_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_view_id", .value.string = field_view_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "field_view_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "field_view_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "field_view_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = field_view_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = field_view_row[i].field_id, .type = TEMPLATE_TYPE_FIELD_CONFIG, .options = NULL },
            { .value.string = field_view_row[i].weight, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .header_html = filter_form, .footer_html = NULL,
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
    char *links = field_view_links( req, NULL, filter[0].field_value );

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

void field_view_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "field_view", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }
    
    char *view_id = main_variable_get( req, req->get_tree, "view_id" );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "field_view_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "field_view_add_header", vars );

    //
    // Form
    //
    struct NNCMS_FIELD_VIEW_FIELDS *fields = memdup_temp( req, &field_view_fields, sizeof(field_view_fields) );
    fields->field_view_id.viewable = false;
    fields->field_view_view_id.value = view_id;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->field_view_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "field_view_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *field_view_add = main_variable_get( req, req->post_tree, "field_view_add" );
    if( field_view_add != 0 )
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
            database_bind_text( req->stmt_add_field_view, 1, fields->field_view_view_id.value );
            database_bind_text( req->stmt_add_field_view, 2, fields->field_view_field_id.value );
            database_bind_text( req->stmt_add_field_view, 3, fields->field_view_label.value );
            database_bind_text( req->stmt_add_field_view, 4, fields->field_view_empty_hide.value );
            database_bind_text( req->stmt_add_field_view, 5, fields->field_view_config.value );
            database_bind_text( req->stmt_add_field_view, 6, fields->field_view_weight.value );
            database_steps( req, req->stmt_add_field_view );
            unsigned int field_view_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "field_view_id", .value.unsigned_integer = field_view_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "field_view_add_success", vars );
            log_printf( req, LOG_ACTION, "Field view was added (id = %u)", field_view_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = field_view_links( req, NULL, view_id );

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

void field_view_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "field_view", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *field_view_id = main_variable_get( req, req->get_tree, "field_view_id" );
    if( field_view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_field_view, 1, field_view_id );
    struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_steps( req, req->stmt_find_field_view );
    if( field_view_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, field_view_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "field_view_id", .value.string = field_view_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "field_view_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_FIELD_VIEW_FIELDS *fields = memdup_temp( req, &field_view_fields, sizeof(field_view_fields) );
    fields->field_view_id.value = field_view_row->id;
    fields->field_view_view_id.value = field_view_row->view_id;
    fields->field_view_field_id.value = field_view_row->field_id;
    fields->field_view_label.value = field_view_row->label;
    fields->field_view_empty_hide.value = field_view_row->empty_hide;
    fields->field_view_weight.value = field_view_row->weight;
    fields->field_view_config.value = field_view_row->config;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->field_view_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "field_view_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *field_view_edit = main_variable_get( req, req->post_tree, "field_view_edit" );
    if( field_view_edit != 0 )
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
            database_bind_text( req->stmt_edit_field_view, 1, fields->field_view_view_id.value );
            database_bind_text( req->stmt_edit_field_view, 2, fields->field_view_field_id.value );
            database_bind_text( req->stmt_edit_field_view, 3, fields->field_view_label.value );
            database_bind_text( req->stmt_edit_field_view, 4, fields->field_view_empty_hide.value );
            database_bind_text( req->stmt_edit_field_view, 5, fields->field_view_config.value );
            database_bind_text( req->stmt_edit_field_view, 6, fields->field_view_weight.value );
            database_bind_text( req->stmt_edit_field_view, 7, field_view_id );
            database_steps( req, req->stmt_edit_field_view );

            log_vdisplayf( req, LOG_ACTION, "field_view_edit_success", vars );
            log_print( req, LOG_ACTION, "Field view was editted" );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = field_view_links( req, field_view_row->id, field_view_row->view_id );

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

void field_view_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "field_view", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *field_view_id = main_variable_get( req, req->get_tree, "field_view_id" );
    if( field_view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_field_view, 1, field_view_id );
    struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_steps( req, req->stmt_find_field_view );
    if( field_view_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, field_view_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "field_view_id", .value.string = field_view_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "field_view_view_header", vars );

    //
    // Form
    //
    struct NNCMS_FIELD_VIEW_FIELDS *fields = memdup_temp( req, &field_view_fields, sizeof(field_view_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->field_view_id.value = field_view_row->id;
    fields->field_view_view_id.value = field_view_row->view_id;
    fields->field_view_field_id.value = field_view_row->field_id;
    fields->field_view_label.value = field_view_row->label;
    fields->field_view_empty_hide.value = field_view_row->empty_hide;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "field_view_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = field_view_links( req, field_view_row->id, field_view_row->view_id );

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

void field_view_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "field_view", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *field_view_id = main_variable_get( req, req->get_tree, "field_view_id" );
    if( field_view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "field_view_id", .value.string = field_view_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "field_view_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_field_view, 1, field_view_id );
    struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_steps( req, req->stmt_find_field_view );
    if( field_view_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, field_view_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_field_view, 1, field_view_id );
        database_steps( req, req->stmt_delete_field_view );

        log_vdisplayf( req, LOG_ACTION, "field_view_delete_success", vars );
        log_print( req, LOG_ACTION, "Field view was deleted" );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, field_view_id );

    // Generate links
    char *links = field_view_links( req, field_view_id, field_view_row->view_id );

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

char *field_view_links( struct NNCMS_THREAD_INFO *req, char *field_view_id, char *field_view_view_id )
{
    struct NNCMS_VARIABLE vars_field_view_id[] =
    {
        { .name = "field_view_id", .value.string = field_view_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_field_view_view_id[] =
    {
        { .name = "view_id", .value.string = field_view_view_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "field_view_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = vars_field_view_view_id;
    g_array_append_vals( links, &link, 1 );

    link.function = "field_view_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = vars_field_view_view_id;
    g_array_append_vals( links, &link, 1 );

    if( field_view_id != NULL )
    {
        link.function = "field_view_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars_field_view_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "field_view_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars_field_view_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "field_view_delete";
        link.title = i18n_string_temp( req, "delete_link", NULL );
        link.vars = vars_field_view_id;
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

char *view_sort_links( struct NNCMS_THREAD_INFO *req, char *view_sort_id, char *view_sort_view_id )
{
    struct NNCMS_VARIABLE vars_view_sort_id[] =
    {
        { .name = "sort_id", .value.string = view_sort_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_view_sort_view_id[] =
    {
        { .name = "view_id", .value.string = view_sort_view_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "sort_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = vars_view_sort_view_id;
    g_array_append_vals( links, &link, 1 );

    link.function = "sort_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = vars_view_sort_view_id;
    g_array_append_vals( links, &link, 1 );

    if( view_sort_id != NULL )
    {
        link.function = "sort_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars_view_sort_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "sort_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars_view_sort_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "sort_delete";
        link.title = i18n_string_temp( req, "delete_link", NULL );
        link.vars = vars_view_sort_id;
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

void view_filter_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "view_filter", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "view_filter_list_header", NULL );

    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "view_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };


    struct NNCMS_QUERY query_select =
    {
        .table = "views_filter",
        .filter = filter,
        .sort = NULL,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_VIEWS_FILTER_ROW *view_filter_row = (struct NNCMS_VIEWS_FILTER_ROW *) database_select( req, &query_select );
    if( view_filter_row != NULL )
    {
        garbage_add( req->loop_garbage, view_filter_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Filter form
    char *filter_form = filter_html( req, &query_select );

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "view_filter_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_filter_field_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_filter_operator_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "view_filter_data_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; view_filter_row != NULL && view_filter_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "view_filter_id", .value.string = view_filter_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "view_filter_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "view_filter_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "view_filter_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = view_filter_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view_filter_row[i].field_id, .type = TEMPLATE_TYPE_FIELD_CONFIG, .options = NULL },
            { .value.string = view_filter_row[i].operator, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view_filter_row[i].data, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .header_html = filter_form, .footer_html = NULL,
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
    char *links = view_filter_links( req, NULL, filter[0].field_value );

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

void view_filter_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "view_filter", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }
    
    char *view_id = main_variable_get( req, req->get_tree, "view_id" );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_filter_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_filter_add_header", vars );

    //
    // Form
    //
    struct NNCMS_VIEWS_FILTER_FIELDS *fields = memdup_temp( req, &view_filter_fields, sizeof(view_filter_fields) );
    fields->view_filter_id.viewable = false;
    fields->view_filter_view_id.value = view_id;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->view_filter_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "view_filter_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *view_filter_add = main_variable_get( req, req->post_tree, "view_filter_add" );
    if( view_filter_add != 0 )
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
            database_bind_text( req->stmt_add_view_filter, 1, fields->view_filter_view_id.value );
            database_bind_text( req->stmt_add_view_filter, 2, fields->view_filter_field_id.value );
            database_bind_text( req->stmt_add_view_filter, 3, fields->view_filter_operator.value );
            database_bind_text( req->stmt_add_view_filter, 4, fields->view_filter_data.value );
            database_bind_text( req->stmt_add_view_filter, 5, fields->view_filter_data_exposed.value );
            database_bind_text( req->stmt_add_view_filter, 6, fields->view_filter_filter_exposed.value );
            database_bind_text( req->stmt_add_view_filter, 7, fields->view_filter_operator_exposed.value );
            database_bind_text( req->stmt_add_view_filter, 8, fields->view_filter_required.value );
            database_steps( req, req->stmt_add_view_filter );
            unsigned int view_filter_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "view_filter_id", .value.unsigned_integer = view_filter_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "view_filter_add_success", vars );
            log_printf( req, LOG_ACTION, "View filter was added (id = %u)", view_filter_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = view_filter_links( req, NULL, view_id );

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

void view_filter_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "view_filter", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *view_filter_id = main_variable_get( req, req->get_tree, "view_filter_id" );
    if( view_filter_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_view_filter, 1, view_filter_id );
    struct NNCMS_VIEWS_FILTER_ROW *view_filter_row = (struct NNCMS_VIEWS_FILTER_ROW *) database_steps( req, req->stmt_find_view_filter );
    if( view_filter_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_filter_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_filter_id", .value.string = view_filter_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_filter_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_VIEWS_FILTER_FIELDS *fields = memdup_temp( req, &view_filter_fields, sizeof(view_filter_fields) );
    fields->view_filter_id.value = view_filter_row->id;
    fields->view_filter_view_id.value = view_filter_row->view_id;
    fields->view_filter_field_id.value = view_filter_row->field_id;
    fields->view_filter_operator.value = view_filter_row->operator;
    fields->view_filter_data.value = view_filter_row->data;
    fields->view_filter_data_exposed.value = view_filter_row->data_exposed;
    fields->view_filter_filter_exposed.value = view_filter_row->filter_exposed;
    fields->view_filter_operator_exposed.value = view_filter_row->operator_exposed;
    fields->view_filter_required.value = view_filter_row->required;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->view_filter_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "view_filter_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *view_filter_edit = main_variable_get( req, req->post_tree, "view_filter_edit" );
    if( view_filter_edit != 0 )
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
            database_bind_text( req->stmt_edit_view_filter, 1, fields->view_filter_view_id.value );
            database_bind_text( req->stmt_edit_view_filter, 2, fields->view_filter_field_id.value );
            database_bind_text( req->stmt_edit_view_filter, 3, fields->view_filter_operator.value );
            database_bind_text( req->stmt_edit_view_filter, 4, fields->view_filter_data.value );
            database_bind_text( req->stmt_edit_view_filter, 5, fields->view_filter_data_exposed.value );
            database_bind_text( req->stmt_edit_view_filter, 6, fields->view_filter_filter_exposed.value );
            database_bind_text( req->stmt_edit_view_filter, 7, fields->view_filter_operator_exposed.value );
            database_bind_text( req->stmt_edit_view_filter, 8, fields->view_filter_required.value );
            database_bind_text( req->stmt_edit_view_filter, 9, view_filter_id );
            database_steps( req, req->stmt_edit_view_filter );

            log_vdisplayf( req, LOG_ACTION, "view_filter_edit_success", vars );
            log_print( req, LOG_ACTION, "View filter was editted" );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = view_filter_links( req, view_filter_row->id, view_filter_row->view_id );

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

void view_filter_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "view_filter", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *view_filter_id = main_variable_get( req, req->get_tree, "view_filter_id" );
    if( view_filter_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_view_filter, 1, view_filter_id );
    struct NNCMS_VIEWS_FILTER_ROW *view_filter_row = (struct NNCMS_VIEWS_FILTER_ROW *) database_steps( req, req->stmt_find_view_filter );
    if( view_filter_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_filter_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_filter_id", .value.string = view_filter_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_filter_view_header", vars );

    //
    // Form
    //
    struct NNCMS_VIEWS_FILTER_FIELDS *fields = memdup_temp( req, &view_filter_fields, sizeof(view_filter_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->view_filter_id.value = view_filter_row->id;
    fields->view_filter_view_id.value = view_filter_row->view_id;
    fields->view_filter_field_id.value = view_filter_row->field_id;
    fields->view_filter_operator.value = view_filter_row->operator;
    fields->view_filter_data.value = view_filter_row->data;
    fields->view_filter_data_exposed.value = view_filter_row->data_exposed;
    fields->view_filter_filter_exposed.value = view_filter_row->filter_exposed;
    fields->view_filter_operator_exposed.value = view_filter_row->operator_exposed;
    fields->view_filter_required.value = view_filter_row->required;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "view_filter_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = view_filter_links( req, view_filter_row->id, view_filter_row->view_id );

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

void view_filter_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "view_filter", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *view_filter_id = main_variable_get( req, req->get_tree, "view_filter_id" );
    if( view_filter_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_filter_id", .value.string = view_filter_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_filter_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_view_filter, 1, view_filter_id );
    struct NNCMS_VIEWS_FILTER_ROW *view_filter_row = (struct NNCMS_VIEWS_FILTER_ROW *) database_steps( req, req->stmt_find_view_filter );
    if( view_filter_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_filter_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_view_filter, 1, view_filter_id );
        database_steps( req, req->stmt_delete_view_filter );

        log_vdisplayf( req, LOG_ACTION, "view_filter_delete_success", vars );
        log_print( req, LOG_ACTION, "View filter was deleted" );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, view_filter_id );

    // Generate links
    char *links = view_filter_links( req, view_filter_id, view_filter_row->view_id );

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

char *view_filter_links( struct NNCMS_THREAD_INFO *req, char *view_filter_id, char *view_filter_view_id )
{
    struct NNCMS_VARIABLE vars_view_filter_id[] =
    {
        { .name = "view_filter_id", .value.string = view_filter_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_view_filter_view_id[] =
    {
        { .name = "view_id", .value.string = view_filter_view_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "view_filter_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = vars_view_filter_view_id;
    g_array_append_vals( links, &link, 1 );

    link.function = "view_filter_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = vars_view_filter_view_id;
    g_array_append_vals( links, &link, 1 );

    if( view_filter_id != NULL )
    {
        link.function = "view_filter_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars_view_filter_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "view_filter_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars_view_filter_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "view_filter_delete";
        link.title = i18n_string_temp( req, "delete_link", NULL );
        link.vars = vars_view_filter_id;
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


void view_display( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "view", NULL, "display" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get id
    char *view_id = main_variable_get( req, req->get_tree, "view_id" );
    if( view_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_view, 1, view_id );
    struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_find_view );
    if( view_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, view_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "view_id", .value.string = view_row->id, .type = NNCMS_TYPE_STRING },
        { .name = "view_name", .value.string = view_row->name, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "view_display_header", vars );

    // Fields associated with this view
    database_bind_text( req->stmt_field_view_by_view_id, 1, view_id );
    struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_steps( req, req->stmt_field_view_by_view_id );
    if( field_view_row != NULL )
    {
        garbage_add( req->loop_garbage, field_view_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Get filters for the view
    database_bind_text( req->stmt_find_view_filter_by_view_id, 1, view_id );
    struct NNCMS_VIEWS_FILTER_ROW *view_filter_row = (struct NNCMS_VIEWS_FILTER_ROW *) database_steps( req, req->stmt_find_view_filter_by_view_id );
    if( view_filter_row != NULL )
    {
        garbage_add( req->loop_garbage, view_filter_row, MEMORY_GARBAGE_DB_FREE );
    }

    //
    // List node types according to filters
    //
    struct NNCMS_FILTER filter_node_type[] =
    {
        { .field_name = "node_type_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
        { .field_name = "node_type_name", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };
    
    // Fill filter with data from views_filter: node_type_id, node_type_name
    for( unsigned int i = 0; view_filter_row != NULL && view_filter_row[i].id != NULL; i = i + 1 )
    {
        int field_id = atoi( view_filter_row[i].field_id );
        int j;
        switch( field_id )
        {
            case FIELD_NODE_TYPE_ID: { j = 0; break; }
            case FIELD_NODE_TYPE_NAME: { j = 1; break; }
            default: { continue; }
        }

        filter_node_type[j].operator = filter_str_to_int( view_filter_row[i].operator, filter_operator_list );
        filter_node_type[j].field_value = (view_filter_row[i].data[0] != 0 ? view_filter_row[i].data : NULL );

        filter_node_type[j].filter_exposed = view_filter_row[i].filter_exposed;
        filter_node_type[j].operator_exposed = view_filter_row[i].operator_exposed;
        filter_node_type[j].required = view_filter_row[i].required;
    }

    // Load filter values from '$_GET' array
    struct NNCMS_QUERY query_select_node_type =
    {
        .table = "node_type",
        .filter = filter_node_type,
        .sort = NULL,
        .offset = 0, .limit = -1
    };
    filter_query_prepare( req, &query_select_node_type );
    
    filter_node_type[0].field_name = "id";
    filter_node_type[1].field_name = "name";
    
    
    // Get node types
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_select( req, &query_select_node_type );
    if( node_type_row != NULL )
    {
        garbage_add( req->loop_garbage, node_type_row, MEMORY_GARBAGE_DB_FREE );
    }
    
    struct FIELD_TABLE
    {
        struct NNCMS_FIELD_CONFIG_ROW *field_config_row;
        struct NNCMS_FIELD_DATA_ROW *field_data_row;
    };
    
    struct NODE_TABLE
    {
        struct NNCMS_NODE_ROW *node_row;
        struct NNCMS_NODE_REV_ROW *node_rev_row;
        struct FIELD_TABLE *field_table;
    };
    
    struct NODE_TYPE_TABLE
    {
        struct NNCMS_NODE_TYPE_ROW *node_type_row;
        struct NNCMS_FIELD_CONFIG_ROW *field_config_row;
        struct NODE_TABLE *node_table;
    };
    
    // Structure:
    //
    // NODE_TYPE_TABLE
    //  |
    //  +-- node_table, node_type_row, field_config_row
    //  |    |
    //  |    +-- field_table, node_rev_row
    //  |    |    |
    //  |    |    +-- field_data_row
    //  |    |    \-- field_data_row
    //  |    \-- field_table, node_rev_row
    //  |         |
    //  |         +-- field_data_row
    //  |         \-- field_data_row
    //  \-- node_table, node_type_row, field_config_row
    //       |
    //       +-- field_table, node_rev_row
    //       |    |
    //       |    +-- field_data_row
    //       |    \-- field_data_row
    //       \-- field_table, node_rev_row
    //            |
    //            +-- field_data_row
    //            \-- field_data_row

    int node_count = 0;

    GArray *node_type_table = g_array_new( TRUE, FALSE, sizeof(struct NODE_TYPE_TABLE) );
    garbage_add( req->loop_garbage, node_type_table, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; node_type_row != NULL && node_type_row[i].id != NULL; i = i + 1 )
    {
        //
        // Get field config
        //
        struct NNCMS_FILTER filter_field_config[] =
        {
            { .field_name = "node_type_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = node_type_row[i].id, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .operator = NNCMS_OPERATOR_NONE }
        };
        
        // Fill filter with data from views_filter: field_name, field_type
        //...

        // Load filter values from '$_GET' array
        struct NNCMS_QUERY query_select_field_config =
        {
            .table = "field_config",
            .filter = filter_field_config,
            .sort = NULL,
            .offset = 0, .limit = -1
        };
        //filter_query_prepare( req, &query_select_field_config );
        
        // Get node types
        struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_select( req, &query_select_field_config );
        if( field_config_row != NULL )
        {
            garbage_add( req->loop_garbage, field_config_row, MEMORY_GARBAGE_DB_FREE );
        }

        //
        // List nodes
        //
        struct NNCMS_FILTER filter_node[] =
        {
            { .field_name = "id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .field_name = "node_type_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = node_type_row[i].id, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .field_name = "user_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .field_name = "language", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .field_name = "published", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .field_name = "promoted", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .field_name = "sticky", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
            { .operator = NNCMS_OPERATOR_NONE }
        };
        
        struct NNCMS_SORT sort_node[] =
        {
            { .field_name = "id", .direction = NNCMS_SORT_DESCENDING },
            { .direction = NNCMS_SORT_NONE }
        };
        
        // Fill filter with data from views_filter: node_id, user_id, language, published, promoted, sticky
        for( unsigned int j = 0; view_filter_row != NULL && view_filter_row[j].id != NULL; j = j + 1 )
        {
            int field_id = atoi( view_filter_row[j].field_id );
            int k;
            switch( field_id )
            {
                case FIELD_NODE_ID: { k = 0; break; }
                //case FIELD_NODE_TYPE_ID: { k = 1; break; }
                case FIELD_NODE_USER_ID: { k = 2; break; }
                case FIELD_NODE_LANGUAGE: { k = 3; break; }
                case FIELD_NODE_PUBLISHED: { k = 4; break; }
                case FIELD_NODE_PROMOTED: { k = 5; break; }
                case FIELD_NODE_STICKY: { k = 6; break; }
                default: { continue; }
            }

            filter_node[k].operator = filter_str_to_int( view_filter_row[j].operator, filter_operator_list );
            filter_node[k].field_value = (view_filter_row[j].data[0] != 0 ? view_filter_row[j].data : NULL );
            filter_node[k].filter_exposed = view_filter_row[j].filter_exposed;
            filter_node[k].operator_exposed = view_filter_row[j].operator_exposed;
            filter_node[k].required = view_filter_row[j].required;
        }

        // Load filter values from '$_GET' array
        struct NNCMS_QUERY query_select_node =
        {
            .table = "node",
            .filter = filter_node,
            .sort = sort_node,
            .offset = 0, .limit = -1
        };
        filter_query_prepare( req, &query_select_node );
        
        // Get node types
        struct NNCMS_NODE_ROW *node_row = (struct NNCMS_NODE_ROW *) database_select( req, &query_select_node );
        if( node_row != NULL )
        {
            garbage_add( req->loop_garbage, node_row, MEMORY_GARBAGE_DB_FREE );
        }

        GArray *node_table = g_array_new( TRUE, FALSE, sizeof(struct NODE_TABLE) );
        garbage_add( req->loop_garbage, node_table, MEMORY_GARBAGE_GARRAY_FREE );
        for( unsigned int j = 0; node_row != NULL && node_row[j].id != NULL; j = j + 1 )
        {
            //
            // Get node revision
            //
            struct NNCMS_FILTER filter_node_rev[] =
            {
                { .field_name = "id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = node_row[j].node_rev_id, .filter_exposed = false, .operator_exposed = false, .required = false },
                { .field_name = "title", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
                { .field_name = "timestamp", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = false, .operator_exposed = false, .required = false },
                { .operator = NNCMS_OPERATOR_NONE }
            };
            
            // Fill filter with data from views_filter: title, timestamp
            for( unsigned int k = 0; view_filter_row != NULL && view_filter_row[k].id != NULL; k = k + 1 )
            {
                int field_id = atoi( view_filter_row[k].field_id );
                int l;
                switch( field_id )
                {
                    case FIELD_NODE_REV_TITLE: { l = 1; break; }
                    case FIELD_NODE_REV_TIMESTAMP: { l = 2; break; }
                    default: { continue; }
                }

                filter_node_rev[l].operator = filter_str_to_int( view_filter_row[k].operator, filter_operator_list );
                filter_node_rev[l].field_value = view_filter_row[k].data;
            }

            // Load filter values from '$_GET' array
            struct NNCMS_QUERY query_select_node_rev =
            {
                .table = "node_rev",
                .filter = filter_node_rev,
                .sort = NULL,
                .offset = 0, .limit = -1
            };
            //filter_query_prepare( req, &query_select_node_rev );
            
            // Get node types
            struct NNCMS_NODE_REV_ROW *node_rev_row = (struct NNCMS_NODE_REV_ROW *) database_select( req, &query_select_node_rev );
            if( node_rev_row != NULL )
            {
                garbage_add( req->loop_garbage, node_rev_row, MEMORY_GARBAGE_DB_FREE );
            }

            // Get data for each field config
            GArray *field_table = g_array_new( TRUE, FALSE, sizeof(struct FIELD_TABLE) );
            garbage_add( req->loop_garbage, field_table, MEMORY_GARBAGE_GARRAY_FREE );
            for( unsigned int k = 0; field_config_row != NULL && field_config_row[k].id != NULL; k = k + 1 )
            {
                //
                // Get field data
                //
                struct NNCMS_FILTER filter_field_data[] =
                {
                    { .field_name = "field_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = field_config_row[k].id, .filter_exposed = false, .operator_exposed = false, .required = false },
                    { .field_name = "node_rev_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = node_rev_row->id, .filter_exposed = false, .operator_exposed = false, .required = false },
                    { .operator = NNCMS_OPERATOR_NONE }
                };
                
                // Fill filter with data from views_filter: field_data
                //...

                // Load filter values from '$_GET' array
                struct NNCMS_QUERY query_select_field_data =
                {
                    .table = "field_data",
                    .filter = filter_field_data,
                    .sort = NULL,
                    .offset = 0, .limit = -1
                };
                //filter_query_prepare( req, &query_select_field_data );
                
                // Get node types
                struct NNCMS_FIELD_DATA_ROW *field_data_row = (struct NNCMS_FIELD_DATA_ROW *) database_select( req, &query_select_field_data );
                if( field_data_row != NULL )
                {
                    garbage_add( req->loop_garbage, field_data_row, MEMORY_GARBAGE_DB_FREE );
                }

                // Skip nodes according to field_data filters
                for( unsigned int l = 0; view_filter_row != NULL && view_filter_row[l].id != NULL; l = l + 1 )
                {
                    if( strcmp( field_config_row[k].id, view_filter_row[l].field_id ) == 0 )
                    {
                        // There is a filter for this field
                        char *filter_field_name = view_filter_row[l].data;
                        if( filter_field_name[0] == 0 )
                        {
                            filter_field_name = main_variable_get( req, req->get_tree, field_config_row[k].name );
                            if( filter_field_name == 0 )
                                continue;
                        }
                        
                        // Convert operator
                        enum NNCMS_OPERATOR filter_operator = filter_str_to_int( view_filter_row[l].operator, filter_operator_list );
                        char *filter_operator_sql = filter_int_to_str( filter_operator, filter_operator_sql_list );
                        
                        bool filter_result = false;
                        for( unsigned int m = 0; field_data_row != NULL && field_data_row[m].id != NULL; m = m + 1 )
                        {
                            struct NNCMS_ROW *result = database_query( req, "SELECT '%q' %s '%q'", field_data_row[m].data, filter_operator_sql, filter_field_name );
                            if( result != NULL )
                            {
                                if( result->value[0][0] == '1' )
                                {
                                    filter_result = true;
                                }
                                database_free_rows( result );
                            }
                        }
                        
                        if( filter_result == false )
                            goto skip_node;
                    }
                }

                struct FIELD_TABLE field_table_temp =
                {
                    .field_config_row = &field_config_row[k],
                    .field_data_row = field_data_row
                };
                g_array_append_vals( field_table, &field_table_temp, 1 );
            }

            /*for( unsigned int k = 0; field_data_row != NULL && field_data_row[k].id != NULL; k = k + 1 )
            {
                // Find field config for data
                struct NNCMS_FIELD_CONFIG_ROW *field_config_row_temp = NULL;
                for( unsigned int l = 0; field_config_row != NULL && field_config_row[l].id != NULL; l = l + 1 )
                {
                    if( strcmp( field_config_row[l].id, field_data_row[k].field_id ) == 0 )
                    {
                        field_config_row_temp = &field_config_row[l];
                        break;
                    }
                }
                
                if( field_config_row_temp == NULL )
                {
                    log_printf( req, LOG_WARNING, "No field config for field data (id = %s, field_id = %s)", field_data_row[k].id, field_data_row[k].field_id );
                }
                
                struct FIELD_TABLE field_table_temp =
                {
                    .field_config_row = field_config_row_temp,
                    .field_data_row = &field_data_row[k]
                };
                g_array_append_vals( field_table, &field_table_temp, 1 );
            }*/
            
            struct NODE_TABLE node_table_temp =
            {
                .node_row = &node_row[j],
                .node_rev_row = node_rev_row,
                .field_table = (struct FIELD_TABLE *) field_table->data
            };
            g_array_append_vals( node_table, &node_table_temp, 1 );
            
            node_count += 1;
skip_node:
            __asm( "nop" );
        }

        // Add node type to table
        struct NODE_TYPE_TABLE node_type_table_temp =
        {
            .field_config_row = field_config_row,
            .node_type_row = &node_type_row[i],
            .node_table = (struct NODE_TABLE *) node_table->data
        };
        g_array_append_vals( node_type_table, &node_type_table_temp, 1 );
    }
    
    // Sort
    //...
    
    // Debug
    /*GString *temp = g_string_sized_new( 100 );
    garbage_add( req->loop_garbage, temp, MEMORY_GARBAGE_GSTRING_FREE );
    for( unsigned int i = 0; i < node_type_table->len; i = i + 1 )
    {
        struct NODE_TYPE_TABLE *node_type = & ((struct NODE_TYPE_TABLE *) node_type_table->data) [i];
        g_string_append_printf( temp, "[node_type] id: %s, name: %s<br>\n", node_type->node_type_row->id, node_type->node_type_row->name );
        for( unsigned int j = 0; node_type->field_config_row[j].id != NULL; j = j + 1 )
        {
            g_string_append_printf( temp, "&nbsp;&nbsp;&nbsp;&nbsp;[field_config] id: %s, name: %s<br>\n", node_type->field_config_row[j].id, node_type->field_config_row[j].name );
        }
        g_string_append( temp, "<br>\n" );
        
        for( unsigned int j = 0; node_type->node_table[j].node_row != NULL; j = j + 1 )
        {
            struct NODE_TABLE *node = & (node_type->node_table[j]);
            g_string_append_printf( temp, "&nbsp;&nbsp;&nbsp;&nbsp;[node] id: %s, title: %s<br>\n", node->node_row->id, node->node_rev_row->title );
            for( unsigned int k = 0; node->field_table[k].field_data_row != NULL; k = k + 1 )
            {
                for( unsigned int l = 0; node->field_table[k].field_data_row[l].data != NULL; l = l + 1 )
                g_string_append_printf( temp, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[field_data] id: %s, field_name: %s, data: %s<br>\n", node->field_table[k].field_data_row[l].id, node->field_table[k].field_config_row->name, node->field_table[k].field_data_row[l].data );
            }
            g_string_append( temp, "<br>\n" );
        }
        g_string_append( temp, "<br>\n" );
    }*/

    // Output buffer
    GString *output_html = g_string_sized_new( 100 );
    garbage_add( req->loop_garbage, output_html, MEMORY_GARBAGE_GSTRING_FREE );
    //g_string_append( output_html, temp->str );
    
    // Field array for 'list' display
    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );

    // Skip nodes
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    int start = 0; int skipped = 0; int node_rendered = 0;
    if( http_start != 0 )
    {
        start = atoi( http_start );
    }

    // Loop every node
    for( unsigned int i = 0; i < node_type_table->len; i = i + 1 )
    {
        struct NODE_TYPE_TABLE *node_type = & ((struct NODE_TYPE_TABLE *) node_type_table->data) [i];
        for( unsigned int j = 0; node_type->node_table[j].node_row != NULL; j = j + 1 )
        {
            if( !(start <= skipped) )
            {
                skipped += 1;
                continue;
            }
            
            struct NODE_TABLE *node = & (node_type->node_table[j]);
            
            // Reset field
            g_array_set_size( gfields, 0 );
            
            // Loop through every field_view 
            for( unsigned int k = 0; field_view_row != NULL && field_view_row[k].id != NULL; k = k + 1 )
            {
                // Find field in node type
                struct NNCMS_FIELD_CONFIG_ROW *field_config = NULL;
                for( unsigned int l = 0; node_type->field_config_row[l].id != NULL; l = l + 1 )
                {
                    struct NNCMS_FIELD_CONFIG_ROW *field_config = &node_type->field_config_row[l];
                    if( strcmp( node_type->field_config_row[l].id, field_view_row[k].field_id ) == 0 )
                    {
                        field_config = &node_type->field_config_row[l];
                    }
                }
                
                if( field_config == NULL )
                {
                    // Look in database
                    database_bind_text( req->stmt_find_field_config, 1, field_view_row[k].field_id );
                    field_config = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_find_field_config );
                    if( field_config != NULL )
                    {
                        garbage_add( req->loop_garbage, field_config, MEMORY_GARBAGE_DB_FREE );
                    }
                    else
                    {
                        log_printf( req, LOG_ERROR, "Field config not found (field_view_id = %s, field_id = %s)", field_view_row[k].id, field_view_row[k].field_id );
                        continue;
                    }
                }
                
                // Fill field structure
                struct NNCMS_FIELD field;
                field_load_config( req, field_config, &field );
                field.id = field_view_row[k].field_id;
                field.editable = false;
                field.label_type = filter_str_to_int( field_view_row[k].label, label_type_list );
                field.empty_hide = filter_str_to_int( field_view_row[k].empty_hide, boolean_list );
                field.value = NULL;
                field.multivalue = NULL;
                field.config = field_view_row[k].config;
                
                GString *gconfig = g_string_sized_new( 10 );
                if( field_config->config ) g_string_append( gconfig, field_config->config );
                if( field_view_row[k].config ) g_string_append( gconfig, field_view_row[k].config );
                field.data = field_parse_config( req, field.type, gconfig->str );
                g_string_free( gconfig, TRUE );

                // Get data for the field
                int field_id = atoi( field_view_row[k].field_id );
                switch( field_id )
                {
                    case FIELD_NODE_TYPE_NAME:
                    {
                        field.name = "node_type_name";
                        field.value = node_type->node_type_row->name;
                        field.multivalue = NULL;
                        break;
                    }
                    
                    case FIELD_NODE_LANGUAGE:
                    {
                        field.name = "node_language";
                        field.value = node->node_row->language;
                        field.multivalue = NULL;
                        break;
                    }
                    
                    default:
                    {
                        // Data from field_data table
                        for( unsigned int m = 0; node->field_table[m].field_data_row != NULL; m = m + 1 )
                        {
                            if( strcmp( node->field_table[m].field_data_row->field_id, field_config->id ) == 0 )
                            {
                                field_load_data( req, node->field_table[m].field_data_row, &field );
                                break;
                            }
                        }
                        break;
                    }
                }
                
                // Add to array
                g_array_append_vals( gfields, &field, 1 );
            }
            
            // Render the node
            struct NNCMS_FORM form =
            {
                .name = "node_view", .action = NULL, .method = NULL,
                .title = NULL, .help = NULL,
                .header_html = NULL, .footer_html = NULL,
                .fields = (struct NNCMS_FIELD *) gfields->data
            };

            // Html output
            char *view_html = form_html( req, &form );

            // Specify values for template
            struct NNCMS_VARIABLE node_template[] =
                {
                    { .name = "header", .value.string = node->node_rev_row->title, .type = NNCMS_TYPE_STRING },
                    { .name = "content", .value.string = view_html, .type = NNCMS_TYPE_STRING },
                    { .name = "authoring", .value.string = node_type->node_type_row->authoring, .type = NNCMS_TYPE_STRING },
                    { .name = "node_id", .value.string = node->node_row->id, .type = NNCMS_TYPE_STRING },
                    { .name = "user_id", .value.string = node->node_row->user_id, .type = TEMPLATE_TYPE_USER },
                    { .name = "timestamp", .value.string = node->node_rev_row->timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP },
                    { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE } // Terminating row
                };
            template_hparse( req, "node.html", output_html, node_template );
            
            node_rendered += 1;
            if( node_rendered >= default_pager_quantity )
            {
                goto double_break;
            }
        }
    }
double_break:

    {
    char *html = output_html->str;
    char *links = view_links( req, view_id );
    char *pager = pager_html( req, node_count );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/categories/applications-internet.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "pager", .value.string = pager, .type = NNCMS_TYPE_STRING },
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

void view_prepare_fields( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *fields, char *view_id )
{
    for( unsigned int i = 0; fields[i].type == NNCMS_FIELD_NONE; i = i + 1 )
    {
        // Find field_view corresponding to field_config
        database_bind_text( req->stmt_matching_field_view, 1, view_id );
        database_bind_text( req->stmt_matching_field_view, 2, fields[i].id );
        struct NNCMS_FIELD_VIEW_ROW *field_view_row = (struct NNCMS_FIELD_VIEW_ROW *) database_steps( req, req->stmt_matching_field_view );
        if( field_view_row != NULL )
        {
            // Apply view settings
            fields[i].label_type = filter_str_to_int( field_view_row->label, label_type_list );
            fields[i].empty_hide = filter_str_to_int( field_view_row->empty_hide, boolean_list );
            fields[i].config = field_view_row->config;
            
            database_free_rows( (struct NNCMS_ROW *) field_view_row );
        }
    }
}

// #############################################################################
