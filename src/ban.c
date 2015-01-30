// #############################################################################
// Source file: ban.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "ban.h"

#include "database.h"
#include "template.h"
#include "user.h"
#include "acl.h"
#include "log.h"
#include "memory.h"
#include "i18n.h"
#include "filter.h"

// #############################################################################
// includes of system headers
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// #############################################################################
// global variables
//

struct NNCMS_BAN_FIELDS
{
    struct NNCMS_FIELD ban_id;
    struct NNCMS_FIELD ban_user_id;
    struct NNCMS_FIELD ban_timestamp;
    struct NNCMS_FIELD ban_ip;
    struct NNCMS_FIELD ban_mask;
    struct NNCMS_FIELD ban_reason;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD ban_add;
    struct NNCMS_FIELD ban_edit;
    struct NNCMS_FIELD none;
}
ban_fields =
{
    { .name = "ban_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "ban_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "ban_timestamp", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "ban_ip", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 50, .char_table = printable_validate },
    { .name = "ban_mask", .value = "255.255.255.255", .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 50, .char_table = printable_validate },
    { .name = "ban_reason", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "ban_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "ban_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// Ban list
struct NNCMS_BAN_INFO *banList = 0;

// #############################################################################
// function declarations

bool ban_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_BAN_FIELDS *fields );
char *ban_links( struct NNCMS_THREAD_INFO *req, char *ban_id );

// #############################################################################
// functions

bool ban_global_init( )
{
    main_local_init_add( &ban_local_init );
    main_local_destroy_add( &ban_local_destroy );

    main_page_add( "ban_add", &ban_add );
    main_page_add( "ban_list", &ban_list );
    main_page_add( "ban_view", &ban_view );
    main_page_add( "ban_edit", &ban_edit );
    main_page_add( "ban_delete", &ban_delete );

    return true;
}

// #############################################################################

bool ban_global_destroy( )
{
    return true;
}

// #############################################################################

bool ban_local_init( struct NNCMS_THREAD_INFO *req )
{
    //if( b_ban == true ) return false;
    //b_ban = true;

    // Prepared statements
    req->stmt_add_ban = database_prepare( req, "INSERT INTO bans VALUES(null, ?, ?, ?, ?, ?)" );
    req->stmt_list_bans = database_prepare( req, "SELECT id, user_id, timestamp, ip, mask, reason FROM bans ORDER BY timestamp ASC LIMIT ? OFFSET ?" );
    req->stmt_find_ban_by_id = database_prepare( req, "SELECT id, user_id, timestamp, ip, mask, reason FROM bans WHERE id=?" );
    req->stmt_edit_ban = database_prepare( req, "UPDATE bans SET ip=?, mask=?, reason=? WHERE id=?" );
    req->stmt_delete_ban = database_prepare( req, "DELETE FROM bans WHERE id=?" );

    // Load bans only once, not for every thread
    //ban_load( req );

    // Test
    //struct in_addr addr;
    //inet_aton( "194.168.1.55", &addr );
    //ban_check( req, &addr );

    return true;
}

// #############################################################################

bool ban_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_add_ban );
    database_finalize( req, req->stmt_list_bans );
    database_finalize( req, req->stmt_find_ban_by_id );
    database_finalize( req, req->stmt_edit_ban );
    database_finalize( req, req->stmt_delete_ban );

    ban_unload( req );

    return true;
}

// #############################################################################

void ban_load( struct NNCMS_THREAD_INFO *req )
{
    if( banList != 0 )
        return;
    
    //sleep(3);
    // Find rows
    struct NNCMS_ROW *ban_row = database_steps( req, req->stmt_list_bans );

    // No bans
    if( ban_row == 0 )
        return;

    // Loop thru all rows
    struct NNCMS_ROW *cur_row = ban_row;
    struct NNCMS_BAN_INFO *firstStruct = 0;
    while( cur_row )
    {
        // Prepare ban structure
        struct NNCMS_BAN_INFO *banStruct = MALLOC( sizeof(struct NNCMS_BAN_INFO) );
        banStruct->id = atoi( cur_row->value[0] );
        inet_aton( cur_row->value[3], &banStruct->ip );
        inet_aton( cur_row->value[4], &banStruct->mask );
        banStruct->next = 0;

        // Sorted insert
        if( firstStruct == 0 )
        {
            firstStruct = banStruct;
        }
        else
        {
            // Check if we just need to add at the beggining
            if( (unsigned int) firstStruct->ip.s_addr > (unsigned int) banStruct->ip.s_addr )
            {
                // We have to add to the beginning of the list
                banStruct->next = firstStruct;
                firstStruct = banStruct;
                goto next_row;
            }

            // Find a place somewhere in the middle or end
            struct NNCMS_BAN_INFO *curStruct = firstStruct;
            while( curStruct )
            {
                if( curStruct->next == 0 )
                {
                    // Position is at the end
                    curStruct->next = banStruct;
                    break;
                }

                // Position for structure
                if( (unsigned int) curStruct->next->ip.s_addr > (unsigned int) banStruct->ip.s_addr )
                {
                    // Add somewhere in the middle
                    banStruct->next = curStruct->next;
                    curStruct->next = banStruct;
                    break;
                }

                // Next
                curStruct = curStruct->next;
            }
        }

        // Test
        //struct NNCMS_BAN_INFO *curStruct;
next_row:
        //curStruct = firstStruct;
        //while( curStruct )
        //{
        //    printf( "%02u: %u\n", curStruct->id, curStruct->ip.s_addr );
        //
        //    // Next
        //    curStruct = curStruct->next;
        //}
        //printf("\n");

        // Select next row
        cur_row = cur_row->next;
    }

    // Free memory from query result
    database_free_rows( ban_row );

    // Finished, save the chain
    banList = firstStruct;
}

// #############################################################################

void ban_unload( struct NNCMS_THREAD_INFO *req )
{
    if( banList == 0 )
        return;
    
    // Remove bans from memory
    struct NNCMS_BAN_INFO *curStruct = banList;
    banList = 0;
    while( curStruct )
    {
        struct NNCMS_BAN_INFO *tempStruct = curStruct->next;
        FREE( curStruct );

        // Next
        curStruct = tempStruct;
    }
}

// #############################################################################

bool ban_check( struct NNCMS_THREAD_INFO *req, struct in_addr *addr )
{
    // Check if ip address is in table
    struct NNCMS_BAN_INFO *curStruct = banList;
    while( curStruct )
    {
        if( (addr->s_addr & curStruct->mask.s_addr) ==
            (curStruct->ip.s_addr & curStruct->mask.s_addr) )
        {
            log_printf( req, LOG_WARNING, "Ip %u.%u.%u.%u hits the ban #%u (%u.%u.%u.%u/%u.%u.%u.%u)",
                (addr->s_addr & (0x0FF << (8*0))) >> (8*0),
                (addr->s_addr & (0x0FF << (8*1))) >> (8*1),
                (addr->s_addr & (0x0FF << (8*2))) >> (8*2),
                (addr->s_addr & (0x0FF << (8*3))) >> (8*3),
                curStruct->id,
                (curStruct->ip.s_addr & (0x0FF << (8*0))) >> (8*0),
                (curStruct->ip.s_addr & (0x0FF << (8*1))) >> (8*1),
                (curStruct->ip.s_addr & (0x0FF << (8*2))) >> (8*2),
                (curStruct->ip.s_addr & (0x0FF << (8*3))) >> (8*3),
                (curStruct->mask.s_addr & (0x0FF << (8*0))) >> (8*0),
                (curStruct->mask.s_addr & (0x0FF << (8*1))) >> (8*1),
                (curStruct->mask.s_addr & (0x0FF << (8*2))) >> (8*2),
                (curStruct->mask.s_addr & (0x0FF << (8*3))) >> (8*3) );
            return true;
        }

        // Next
        curStruct = curStruct->next;
    }

    return false;
}

// #############################################################################

void ban_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "ban_add_header", NULL );

    // Check user permission to edit Bans
    if( acl_check_perm( req, "ban", NULL, "add" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    //
    // Form
    //
    struct NNCMS_BAN_FIELDS *fields = memdup_temp( req, &ban_fields, sizeof(ban_fields) );
    fields->ban_id.viewable = false;
    fields->ban_timestamp.viewable = false;
    fields->ban_user_id.value = req->user_id;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->ban_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "ban_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *ban_add = main_variable_get( req, req->post_tree, "ban_add" );
    if( ban_add != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Get POST data
        form_post_data( req, (struct NNCMS_FIELD *) fields );
        
        // Validate
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        bool ban_valid = ban_validate( req, fields );
        if( field_valid == true && ban_valid == true )
        {
            // Query Database
            database_bind_text( req->stmt_add_ban, 1, req->user_id );
            database_bind_int( req->stmt_add_ban, 2, time( 0 ) );
            database_bind_text( req->stmt_add_ban, 3, fields->ban_ip.value );
            database_bind_text( req->stmt_add_ban, 4, fields->ban_mask.value );
            database_bind_text( req->stmt_add_ban, 5, fields->ban_reason.value );
            database_steps( req, req->stmt_add_ban );            
            unsigned int ban_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "ban_id", .value.unsigned_integer = ban_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE }
            };
            log_printf( req, LOG_ACTION, "Ban '%s' was added (id = %i)", fields->ban_ip.value, ban_id );
            log_vdisplayf( req, LOG_ACTION, "ban_add_success", vars );

            // Recache bans
            ban_load( req );
            ban_unload( req );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = ban_links( req, NULL );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/status/security-medium.png", .type = NNCMS_TYPE_STRING },
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

void ban_list( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "ban", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find rows
    struct NNCMS_ROW *row_count = database_query( req, "SELECT COUNT(*) FROM bans" );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_bans, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_bans, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_BAN_ROW *ban_row = (struct NNCMS_BAN_ROW *) database_steps( req, req->stmt_list_bans );
    if( ban_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ban_row, MEMORY_GARBAGE_DB_FREE );

    // Page title
    char *header_str = i18n_string_temp( req, "ban_list_header", NULL );

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "ban_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ban_ip_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ban_mask_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ban_reason_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ban_time_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "ban_user_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; ban_row != NULL && ban_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "ban_id", .value.string = ban_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "ban_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "ban_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "ban_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = ban_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ban_row[i].ip, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ban_row[i].mask, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ban_row[i].reason, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ban_row[i].timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP, .options = NULL },
            { .value.string = ban_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
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
        .row_count = atoi( row_count->value[0] ),//gcells->len,
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Generate links
    char *links = ban_links( req, NULL );
    
    // Html output
    char *html = table_html( req, &table );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/status/security-medium.png", .type = NNCMS_TYPE_STRING },
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

void ban_view( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "ban_view_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "ban", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get ban id
    char *ban_id = main_variable_get( req, req->get_tree, "ban_id" );
    if( ban_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }
    
    // Find row associated with our object by 'id'
    database_bind_text( req->stmt_find_ban_by_id, 1, ban_id );
    struct NNCMS_BAN_ROW *ban_row = (struct NNCMS_BAN_ROW *) database_steps( req, req->stmt_find_ban_by_id );
    if( ban_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ban_row, MEMORY_GARBAGE_DB_FREE );

    //
    // Form
    //
    struct NNCMS_BAN_FIELDS *fields = memdup_temp( req, &ban_fields, sizeof(ban_fields) );
    fields->ban_id.value = ban_row->id;
    fields->ban_user_id.value = ban_row->user_id;
    fields->ban_timestamp.value = ban_row->timestamp;
    fields->ban_ip.value = ban_row->ip;
    fields->ban_mask.value = ban_row->mask;
    fields->ban_reason.value = ban_row->reason;
    fields->referer.viewable = false;
    fields->fkey.viewable = false;

    struct NNCMS_FORM form =
    {
        .name = "ban_view", .action = NULL, .method = NULL,
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = ban_links( req, ban_id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/status/security-medium.png", .type = NNCMS_TYPE_STRING },
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

void ban_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "ban", NULL, "edit" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get ban id
    char *ban_id = main_variable_get( req, req->get_tree, "ban_id" );
    if( ban_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'id'
    database_bind_text( req->stmt_find_ban_by_id, 1, ban_id );
    struct NNCMS_BAN_ROW *ban_row = (struct NNCMS_BAN_ROW *) database_steps( req, req->stmt_find_ban_by_id );
    if( ban_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ban_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ban_id", .value.string = ban_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "ban_edit_header", NULL );

    //
    // Form
    //
    struct NNCMS_BAN_FIELDS *fields = memdup_temp( req, &ban_fields, sizeof(ban_fields) );
    fields->ban_id.value = ban_row->id;
    fields->ban_user_id.value = ban_row->user_id;
    fields->ban_timestamp.value = ban_row->timestamp;
    fields->ban_ip.value = ban_row->ip;
    fields->ban_mask.value = ban_row->mask;
    fields->ban_reason.value = ban_row->reason;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->ban_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "ban_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *ban_edit = main_variable_get( req, req->post_tree, "ban_edit" );
    if( ban_edit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Get POST data
        form_post_data( req, (struct NNCMS_FIELD *) fields );
        
        // Validate
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        bool ban_valid = ban_validate( req, fields );
        if( field_valid == true && ban_valid == true )
        {
            // Query Database
            database_bind_text( req->stmt_edit_ban, 1, fields->ban_ip.value );
            database_bind_text( req->stmt_edit_ban, 2, fields->ban_mask.value );
            database_bind_text( req->stmt_edit_ban, 3, fields->ban_reason.value );
            database_bind_text( req->stmt_edit_ban, 4, ban_id );
            database_steps( req, req->stmt_edit_ban );

            log_vdisplayf( req, LOG_ACTION, "ban_edit_success", vars );
            log_printf( req, LOG_ACTION, "Ban '%s' was edited", ban_row->ip );
            
            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = ban_links( req, ban_id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/status/security-medium.png", .type = NNCMS_TYPE_STRING },
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

void ban_delete( struct NNCMS_THREAD_INFO *req )
{

    // Check user permission to edit ACLs
    if( acl_check_perm( req, "ban", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get ban id
    char *ban_id = main_variable_get( req, req->get_tree, "ban_id" );
    if( ban_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ban_id", .value.string = ban_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "ban_delete_header", vars );

    // Find selected ban id
    database_bind_text( req->stmt_find_ban_by_id, 1, ban_id );
    struct NNCMS_BAN_ROW *ban_row = (struct NNCMS_BAN_ROW *) database_steps( req, req->stmt_find_ban_by_id );
    if( ban_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, ban_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_ban, 1, ban_id );
        database_steps( req, req->stmt_delete_ban );

        log_printf( req, LOG_ACTION, "Ban '%s' was deleted", ban_row->ip );
        log_vdisplayf( req, LOG_ACTION, "ban_delete_success", vars );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, ban_id );

    // Generate links
    char *links = ban_links( req, ban_id );

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

char *ban_links( struct NNCMS_THREAD_INFO *req, char *ban_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "ban_id", .value.string = ban_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "ban_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    link.function = "ban_add";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( ban_id != NULL )
    {
        link.function = "ban_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "ban_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );

        link.function = "ban_delete";
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

bool ban_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_BAN_FIELDS *fields )
{
    bool result = true;

    // Check if address written correctly
    struct in_addr addr;
    if( inet_aton( fields->ban_ip.value, &addr ) == 0 )
    {
        result = false; 
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_name", .value.string = fields->ban_ip.name, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        log_vdisplayf( req, LOG_ERROR, "ban_validate_invalid_ip", vars );
    }

    if( inet_aton( fields->ban_mask.value, &addr ) == 0 )
    {
        result = false; 
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_name", .value.string = fields->ban_mask.name, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        log_vdisplayf( req, LOG_ERROR, "ban_validate_invalid_ip", vars );
    }
    
    return result;
}
