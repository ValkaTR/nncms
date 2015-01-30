// #############################################################################
// Source file: post.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <time.h>

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "log.h"
#include "acl.h"
#include "user.h"
#include "group.h"
#include "template.h"
#include "database.h"
#include "post.h"
#include "filter.h"
#include "smart.h"
#include "file.h"
#include "i18n.h"

#include "strlcpy.h"
#include "strlcat.h"
#include "strdup.h"

#include "memory.h"

// #############################################################################
// global variables
//

// Maximum tree depth for "reply, modify, remove" actions
int nPostDepth = 2;

// #############################################################################
// Forms
//

struct NNCMS_SELECT_ITEM post_types[] =
{
    { .name = "msg", .desc = NULL, .selected = false },
    { .name = "topic", .desc = NULL, .selected = false },
    { .name = "subforum", .desc = NULL, .selected = false },
    { .name = "group", .desc = NULL, .selected = false },
    { .name = "forum", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_VARIABLE post_type_list[] =
{
    { .name = "none", .value.integer = NNCMS_POST_NONE, .type = NNCMS_TYPE_INTEGER },
    { .name = "msg", .value.integer = NNCMS_POST_MESSAGE, .type = NNCMS_TYPE_INTEGER },
    { .name = "topic", .value.integer = NNCMS_POST_TOPIC, .type = NNCMS_TYPE_INTEGER },
    { .name = "subforum", .value.integer = NNCMS_POST_SUBFORUM, .type = NNCMS_TYPE_INTEGER },
    { .name = "group", .value.integer = NNCMS_POST_GROUP, .type = NNCMS_TYPE_INTEGER },
    { .name = "forum", .value.integer = NNCMS_POST_FORUM, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

struct NNCMS_POST_ADD_FIELDS
{
    struct NNCMS_FIELD post_id;
    struct NNCMS_FIELD post_user_id;
    struct NNCMS_FIELD post_group_id;
    struct NNCMS_FIELD post_parent_post_id;
    struct NNCMS_FIELD post_subject;
    struct NNCMS_FIELD post_body;
    struct NNCMS_FIELD post_type;
    struct NNCMS_FIELD post_mode;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD post_add;
    struct NNCMS_FIELD none;
}
post_add_fields =
{
    { .name = "post_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = false, .text_name = NULL, .text_description = NULL },
    { .name = "post_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .char_table = numeric_validate },
    { .name = "post_group_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_GROUP, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .char_table = numeric_validate },
    { .name = "post_parent_post_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .char_table = numeric_validate },
    { .name = "post_subject", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1024 },
    { .name = "post_body", .value = NULL, .data = NULL, .label_type = FIELD_LABEL_HIDDEN, .type = NNCMS_FIELD_TEXTAREA, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1024*100 },
    { .name = "post_type", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = post_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_mode", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1024 },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "post_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_POST_EDIT_FIELDS
{
    struct NNCMS_FIELD post_id;
    struct NNCMS_FIELD post_user_id;
    struct NNCMS_FIELD post_group_id;
    struct NNCMS_FIELD post_parent_post_id;
    struct NNCMS_FIELD post_timestamp;
    struct NNCMS_FIELD post_subject;
    struct NNCMS_FIELD post_body;
    struct NNCMS_FIELD post_type;
    struct NNCMS_FIELD post_mode;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD post_edit;
    struct NNCMS_FIELD none;
}
post_edit_fields =
{
    { .name = "post_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_group_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_GROUP, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_parent_post_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_timestamp", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_subject", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_body", .value = NULL, .data = NULL, .label_type = FIELD_LABEL_HIDDEN, .type = NNCMS_FIELD_TEXTAREA, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_type", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = post_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_mode", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1024 },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "post_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_POST_VIEW_FIELDS
{
    struct NNCMS_FIELD post_id;
    struct NNCMS_FIELD post_user_id;
    struct NNCMS_FIELD post_group_id;
    struct NNCMS_FIELD post_parent_post_id;
    struct NNCMS_FIELD post_timestamp;
    struct NNCMS_FIELD post_subject;
    struct NNCMS_FIELD post_body;
    struct NNCMS_FIELD post_type;
    struct NNCMS_FIELD post_mode;
    struct NNCMS_FIELD none;
}
post_view_fields =
{
    { .name = "post_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_group_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_GROUP, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_parent_post_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_timestamp", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_subject", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_body", .value = NULL, .label_type = FIELD_LABEL_HIDDEN, .data = NULL, .type = NNCMS_FIELD_TEXTAREA, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_type", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = post_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "post_mode", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1024 },
    { .type = NNCMS_FIELD_NONE }
};


// #############################################################################
// functions

bool post_global_init( )
{
    main_local_init_add( &post_local_init );
    main_local_destroy_add( &post_local_destroy );

    main_page_add( "post_add", &post_add );
    main_page_add( "post_list", &post_list );
    main_page_add( "post_edit", &post_edit );
    main_page_add( "post_delete", &post_delete );
    main_page_add( "post_view", &post_view );

    return true;
}

// #############################################################################

bool post_global_destroy( )
{
    return true;
}

// #############################################################################

bool post_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_add_post = database_prepare( req, "INSERT INTO posts VALUES(null, ?, ?, ?, ?, ?, ?, ?, ?)" );
    req->stmt_find_post_parent_type = database_prepare( req, "SELECT id, user_id, timestamp, parent_post_id, subject, body, type, group_id, mode FROM posts WHERE parent_post_id=? AND type=?" );
    req->stmt_find_post = database_prepare( req, "SELECT id, user_id, timestamp, parent_post_id, subject, body, type, group_id, mode FROM posts WHERE id=? LIMIT 1" );
    req->stmt_power_edit_post = database_prepare( req, "UPDATE posts SET subject=?, body=?, parent_post_id=? WHERE id=?" );
    req->stmt_edit_post = database_prepare( req, "UPDATE posts SET user_id=?, timestamp=?, parent_post_id=?, subject=?, body=?, type=?, group_id=?, mode=? WHERE id=?" );
    req->stmt_delete_post = database_prepare( req, "DELETE FROM posts WHERE id=?" );
    req->stmt_get_parent_post = database_prepare( req, "SELECT parent_post_id FROM posts WHERE id=?" );
    req->stmt_root_posts = database_prepare( req, "SELECT parent_post_id FROM posts" );

    req->stmt_topic_posts = database_prepare( req, "SELECT id, user_id, timestamp, parent_post_id, subject, body, type, group_id, mode FROM posts WHERE parent_post_id=? ORDER BY timestamp DESC LIMIT ? OFFSET ?" );
    req->stmt_count_child_posts = database_prepare( req, "SELECT COUNT() FROM posts WHERE parent_post_id=?" );

    return true;
}

// #############################################################################

bool post_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_add_post );
    database_finalize( req, req->stmt_find_post_parent_type );
    database_finalize( req, req->stmt_find_post );
    database_finalize( req, req->stmt_power_edit_post );
    database_finalize( req, req->stmt_edit_post );
    database_finalize( req, req->stmt_delete_post );
    database_finalize( req, req->stmt_get_parent_post );
    database_finalize( req, req->stmt_root_posts );

    database_finalize( req, req->stmt_topic_posts );

    database_finalize( req, req->stmt_count_child_posts );

    return true;
}

// #############################################################################

void post_get_access( struct NNCMS_THREAD_INFO *req, char *user_id, char *post_id, bool *read_access_out, bool *write_access_out, bool *exec_access_out )
{
    bool read_access = false;
    bool write_access = false;
    bool exec_access = false;
    
    // Get group list of the user
    struct NNCMS_USERGROUP_ROW *group_row = NULL;
    GArray *user_groups = NULL;
    char **user_group_array = NULL;
    if( user_id != NULL )
    {
        database_bind_text( req->stmt_find_ug_by_user_id, 1, user_id );
        group_row = (struct NNCMS_USERGROUP_ROW *) database_steps( req, req->stmt_find_ug_by_user_id );
        if( group_row == NULL ) return;
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
        user_id = req->user_id;
        user_group_array = req->group_id;
    }

    struct NNCMS_POST_ROW *post_row = NULL;
    database_bind_text( req->stmt_find_post, 1, post_id );
    while( 1 )
    {
        // Find rows
        post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post );
        if( post_row == NULL )
        {
            goto post_no_access;
        }
        
        if( post_row->mode[0] != 0 )
            break;
        
        // Inherit from parent
        database_bind_int( req->stmt_find_post, 1, atoi( post_row->parent_post_id ) );
        database_free_rows( (struct NNCMS_ROW *) post_row );
    }
    
    // Convert string mode to integer
    unsigned int post_mode = (unsigned int) g_ascii_strtoll( post_row->mode, NULL, 16 );
    
    // owner  group  other
    //
    //  rwx    rwx    rwx
    // 0110   0100   0100
    //
    
    //  check   access  result
    //  0       0       1
    //  0       1       1
    //  1       0       0
    //  1       1       1
    
    read_access |= post_mode & NNCMS_IROTH;
    write_access |= post_mode & NNCMS_IWOTH;
    exec_access |= post_mode & NNCMS_IXOTH;
    
    for( int i = 0; user_group_array != NULL && user_group_array[i] != NULL; i = i + 1 )
    {
        // Check group access
        if( strcmp( post_row->group_id, user_group_array[i] ) == 0 )
        {
            read_access |= post_mode & NNCMS_IRGRP;
            write_access |= post_mode & NNCMS_IWGRP;
            exec_access |= post_mode & NNCMS_IXGRP;
        }
    }
    
    // Check if user is owner of this post
    if( strcmp( post_row->user_id, user_id ) == 0 )
    {
        read_access |= post_mode & NNCMS_IRUSR;
        write_access |= post_mode & NNCMS_IWUSR;
        exec_access |= post_mode & NNCMS_IXUSR;
    }

    // Not needed now
    database_free_rows( (struct NNCMS_ROW *) post_row );

post_no_access:
    if( group_row != NULL ) database_free_rows( (struct NNCMS_ROW *) group_row );
    if( user_groups != NULL ) g_array_free( user_groups, true );

    if( read_access_out )   *read_access_out = read_access;
    if( write_access_out )   *write_access_out = write_access;
    if( exec_access_out )   *exec_access_out = exec_access;
}

bool post_check_perm( struct NNCMS_THREAD_INFO *req, char *user_id, char *post_id, bool read_check, bool write_check, bool exec_check )
{
    bool read_access = false;
    bool write_access = false;
    bool exec_access = false;
    post_get_access( req, user_id, post_id, &read_access, &write_access, &exec_access );

    if( (read_check == true && read_access == false) ||
        (write_check == true && write_access == false) ||
        (exec_check == true && exec_access == false) )
    return false;

    return true;
}

// #############################################################################

struct NNCMS_FORM *post_add_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_POST_ROW *post_row )
{
    // Fields
    struct NNCMS_POST_ADD_FIELDS *fields = memdup( &post_add_fields, sizeof(post_add_fields) );
    fields->post_user_id.value = req->user_id;
    fields->post_parent_post_id.value = (post_row != NULL ? post_row->id : NULL);
    fields->post_subject.value = (post_row != NULL ? post_row->subject : NULL);
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    if( acl_check_perm( req, "post", NULL, "chown" ) == false )
    {
        fields->post_user_id.editable = false;
        fields->post_group_id.editable = false;
        fields->post_group_id.viewable = false;
    }
    
    if( acl_check_perm( req, "post", NULL, "chmod" ) == false )
    {
        fields->post_mode.editable = false;
        fields->post_mode.viewable = false;
    }

    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "post_id", .value.string = (post_row != NULL ? post_row->id : NULL), .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *action = main_temp_link( req, "post_add", NULL, vars );

    // Form
    struct NNCMS_FORM *form = MALLOC( sizeof(struct NNCMS_FORM) * 1 );
    *form =
    (struct NNCMS_FORM) {
        .name = "post_add", .action = action, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    garbage_add( req->loop_garbage, fields, MEMORY_GARBAGE_FREE );
    garbage_add( req->loop_garbage, form, MEMORY_GARBAGE_FREE );

    return form;
}


void post_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "post", NULL, "add" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }
    bool is_devel = acl_check_perm( req, "post", NULL, "devel" );

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "post_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( post_check_perm( req, NULL, httpVarId, false, false, true ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_post, 1, httpVarId );
    struct NNCMS_POST_ROW *post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post );
    if( post_row != NULL )
    {
        garbage_add( req->loop_garbage, post_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "post_parent_post_id", .value.string = (post_row != NULL ? post_row->parent_post_id : NULL), .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "post_add_header", vars );

    // Fields
    struct NNCMS_POST_ADD_FIELDS *fields = memdup( &post_add_fields, sizeof(post_add_fields) );
    garbage_add( req->loop_garbage, fields, MEMORY_GARBAGE_FREE );    
    fields->post_user_id.value = req->user_id;
    fields->post_parent_post_id.value = (post_row != NULL ? post_row->id : NULL);
    fields->post_subject.value = (post_row != NULL ? post_row->subject : NULL);
    fields->post_type.value = "forum";
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    if( acl_check_perm( req, "post", NULL, "chown" ) == false )
    {
        fields->post_user_id.editable = false;
        fields->post_group_id.editable = false;
        fields->post_group_id.viewable = false;
    }
    
    if( acl_check_perm( req, "post", NULL, "chmod" ) == false )
    {
        fields->post_mode.editable = false;
        fields->post_mode.viewable = false;
    }

    if( is_devel == false )
    {
        fields->post_parent_post_id.type = NNCMS_FIELD_HIDDEN;
        fields->post_type.editable = false;
        fields->post_type.viewable = false;
    }

    // Select valid post type
    enum NNCMS_POST_TYPE post_type = NNCMS_POST_NONE;
    if( post_row != NULL )
    {
        post_type = filter_str_to_int( post_row->type, post_type_list );
        switch( post_type )
        {
            case NNCMS_POST_FORUM: { fields->post_type.value = "group"; break; }
            case NNCMS_POST_GROUP: { fields->post_type.value = "subforum"; break; }
            case NNCMS_POST_SUBFORUM: { fields->post_type.value = "topic"; break; }
            case NNCMS_POST_TOPIC: { fields->post_type.value = "msg"; break; }
            default:
            case NNCMS_POST_MESSAGE: { fields->post_type.value = "msg"; break; }
        }
    }
    
    /*if( is_devel == false )
    {
        if( post_type != NNCMS_POST_SUBFORUM && post_type != NNCMS_POST_TOPIC )
        {
            main_vmessage( req, "not_allowed" );
            return;
        }
    }*/

    //
    // Check if user commit changes
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "post_add" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Retrieve GET data
        form_post_data( req, (struct NNCMS_FIELD *) fields );

        // Validate the data
        bool valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( valid == true )
        {
            // Query Database
            database_bind_text( req->stmt_add_post, 1, req->user_id );
            database_bind_int( req->stmt_add_post, 2, time( 0 ) );
            database_bind_text( req->stmt_add_post, 3, fields->post_parent_post_id.value );
            database_bind_text( req->stmt_add_post, 4, fields->post_subject.value );
            database_bind_text( req->stmt_add_post, 5, fields->post_body.value );
            database_bind_text( req->stmt_add_post, 6, fields->post_type.value );
            database_bind_text( req->stmt_add_post, 7, fields->post_group_id.value );
            database_bind_text( req->stmt_add_post, 8, fields->post_mode.value );
            database_steps( req, req->stmt_add_post );
            int post_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
                {
                    { .name = "post_id", .value.integer = post_id, .type = NNCMS_TYPE_INTEGER },
                    { .name = "post_parent_post_id", .value.string = fields->post_parent_post_id.value, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE } // Terminating row
                };
            log_vdisplayf( req, LOG_ACTION, "post_add_success", vars );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    //char *links = post_links( req, (post_row != NULL ? post_row->id : NULL), (post_row != NULL ? post_row->parent_post_id : NULL) );
    char *links = "";

    // Form
    struct NNCMS_FORM form =
    {
        .name = "post_add", .action = NULL, .method = "POST",
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
            { .name = "icon", .value.string = "images/actions/mail-message-new.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    {
        if( post_row != NULL )
        {
            // Page title
            struct NNCMS_VARIABLE vars[] =
                {
                    { .name = "post_id", .value.string = post_row->id, .type = NNCMS_TYPE_STRING },
                    { .name = "post_subject", .value.string = post_row->subject, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE } // Terminating row
                };
            char *header_str = i18n_string_temp( req, "post_view_header", vars );
            
            // Html output
            struct NNCMS_FORM *form = post_view_form( req, post_row );
            char *html = form_html( req, form );

            // Generate links
            char *links = post_links( req, post_row->id, post_row->parent_post_id );

            // Specify values for template
            struct NNCMS_VARIABLE frame_template[] =
                {
                    { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
                    { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
                    { .name = "icon", .value.string = "images/stock/net/stock_mail-open.png", .type = NNCMS_TYPE_STRING },
                    { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
                    { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE } // Terminating row
                };

            // Make a cute frame
            template_hparse( req, "frame.html", req->frame, frame_template );
        }
    }

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #############################################################################

void post_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "post", NULL, "edit" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "post_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( post_check_perm( req, NULL, httpVarId, true, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_post, 1, httpVarId );
    struct NNCMS_POST_ROW *post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post );
    if( post_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, post_row, MEMORY_GARBAGE_DB_FREE );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "post_id", .value.string = post_row->id, .type = NNCMS_TYPE_STRING },
            { .name = "post_subject", .value.string = post_row->subject, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "post_edit_header", vars );

    // Fields
    struct NNCMS_POST_EDIT_FIELDS *fields = memdup_temp( req, &post_edit_fields, sizeof(post_edit_fields) );
    fields->post_id.value = post_row->id;
    fields->post_user_id.value = post_row->user_id;
    fields->post_group_id.value = post_row->group_id;
    fields->post_parent_post_id.value = post_row->parent_post_id;
    fields->post_timestamp.value = post_row->timestamp;
    fields->post_subject.value = post_row->subject;
    fields->post_body.value = post_row->body;
    fields->post_type.value = post_row->type;
    fields->post_mode.value = post_row->mode;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    //
    // Check if user commit changes
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "post_edit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Retrieve GET data
        form_post_data( req, (struct NNCMS_FIELD *) fields );

        // Validate the data
        // todo

        // Query Database
        database_bind_text( req->stmt_edit_post, 1, fields->post_user_id.value );
        database_bind_text( req->stmt_edit_post, 2, fields->post_timestamp.value );
        database_bind_text( req->stmt_edit_post, 3, fields->post_parent_post_id.value );
        database_bind_text( req->stmt_edit_post, 4, fields->post_subject.value );
        database_bind_text( req->stmt_edit_post, 5, fields->post_body.value );
        database_bind_text( req->stmt_edit_post, 6, fields->post_type.value );
        database_bind_text( req->stmt_edit_post, 7, fields->post_group_id.value );
        database_bind_text( req->stmt_edit_post, 8, fields->post_mode.value );
        database_bind_text( req->stmt_edit_post, 9, httpVarId );
        database_steps( req, req->stmt_edit_post );

        log_vdisplayf( req, LOG_ACTION, "post_edit_success", vars );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Generate links
    char *links = post_links( req, post_row->id, post_row->parent_post_id );

    // Form
    struct NNCMS_FORM form =
    {
        .name = "post_edit", .action = NULL, .method = "POST",
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
            { .name = "icon", .value.string = "images/actions/gtk-edit.png", .type = NNCMS_TYPE_STRING },
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

void post_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to delete ACLs
    if( acl_check_perm( req, "post", NULL, "delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "post_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( post_check_perm( req, NULL, httpVarId, true, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_post, 1, httpVarId );
    struct NNCMS_POST_ROW *post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post );
    if( post_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, post_row, MEMORY_GARBAGE_DB_FREE );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "post_id", .value.string = post_row->id, .type = NNCMS_TYPE_STRING },
            { .name = "post_subject", .value.string = post_row->subject, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "post_delete_header", vars );

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
        database_bind_text( req->stmt_delete_post, 1, httpVarId );
        database_steps( req, req->stmt_delete_post );

        log_vdisplayf( req, LOG_ACTION, "post_delete_success", vars );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Generate links
    char *links = post_links( req, post_row->id, post_row->parent_post_id );

    // Form
    struct NNCMS_FORM *form = template_confirm( req, post_row->subject );

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

void post_list( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "post", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }
    bool is_devel = acl_check_perm( req, "post", NULL, "devel" );

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "post_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( post_check_perm( req, NULL, httpVarId, true, false, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    //
    // Find row by id
    //
    database_bind_text( req->stmt_find_post, 1, httpVarId );
    struct NNCMS_POST_ROW *post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post );
    if( post_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, post_row, MEMORY_GARBAGE_DB_FREE );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "post_id", .value.string = post_row->id, .type = NNCMS_TYPE_STRING },
            { .name = "post_subject", .value.string = post_row->subject, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "post_view_header", vars );

    // Hierarchy
    //
    //  forum
    //      group
    //          subforum
    //              topic
    //                  msg
    //                  msg
    //          subforum
    //              topic
    //          subforum
    //      group
    //          subforum
    //          subforum
    //          subforum
    

    enum NNCMS_POST_TYPE post_type = filter_str_to_int( post_row->type, post_type_list );
    if( post_type == NNCMS_POST_FORUM )
    {
        // Get list of groups associated to this forum
        database_bind_text( req->stmt_find_post_parent_type, 1, httpVarId );
        database_bind_text( req->stmt_find_post_parent_type, 2, "group" );
        struct NNCMS_POST_ROW *group_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post_parent_type );
        if( group_row != NULL )
        {
            garbage_add( req->loop_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
        }

        // Header cells
        struct NNCMS_TABLE_CELL header_cells[] =
        {
            { .value = i18n_string_temp( req, "post_subject_forum_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_subject_count_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_msg_count_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_msg_last_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        // Fetch table data
        GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
        garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
        for( unsigned int i = 0; group_row != NULL && group_row[i].id != NULL; i = i + 1 )
        {
            if( post_check_perm( req, NULL, group_row[i].id, true, false, false ) == false )
            {
                continue;
            }
            
            // Get list of subforums associated to group
            database_bind_text( req->stmt_find_post_parent_type, 1, group_row[i].id );
            database_bind_text( req->stmt_find_post_parent_type, 2, "subforum" );
            struct NNCMS_POST_ROW *subforum_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post_parent_type );
            if( subforum_row != NULL )
            {
                garbage_add( req->loop_garbage, subforum_row, MEMORY_GARBAGE_DB_FREE );
            }

            //
            // Data
            //
            struct NNCMS_TABLE_CELL cells[] =
            {
                { .value.string = group_row[i].subject, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .type = NNCMS_TYPE_NONE }
            };
            g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
            
            // Add subforums
            for( unsigned int j = 0; subforum_row != NULL && subforum_row[j].id != NULL; j = j + 1 )
            {
                if( post_check_perm( req, NULL, subforum_row[j].id, true, false, false ) == false )
                {
                    continue;
                }
                
                struct NNCMS_VARIABLE vars[] =
                {
                    { .name = "post_id", .value.string = subforum_row[j].id, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE }
                };
                char *list = main_temp_link( req, "post_list", subforum_row[j].subject, vars );

                // Number of subjects
                database_bind_text( req->stmt_count_child_posts, 1, subforum_row[j].id );
                struct NNCMS_ROW *subject_row_count = database_steps( req, req->stmt_count_child_posts );
                int subject_row_count_i = atoi( subject_row_count->value[0] );
                database_free_rows( subject_row_count );

                struct NNCMS_TABLE_CELL cells[] =
                {
                    { .value.string = list, .type = NNCMS_TYPE_STRING, .options = NULL },
                    { .value.integer = subject_row_count_i, .type = NNCMS_TYPE_INTEGER, .options = NULL },
                    { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
                    { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
                    { .type = NNCMS_TYPE_NONE }
                };
                g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
            }
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
            .row_count = -1,
            .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
            .pager_quantity = 0, .pages_displayed = 0,
            .headerz = header_cells,
            .cells = (struct NNCMS_TABLE_CELL *) gcells->data
        };

        // Generate links
        char *links = post_links( req, post_row->id, NULL );
        
        // Html output
        char *html_table = table_html( req, &table );

        // Specify values for template
        struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html_table, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/stock/net/stock_mail-open-multiple.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

        // Make a cute frame
        template_hparse( req, "frame.html", req->frame, frame_template );

        // Send generated html to client
        main_output( req, header_str, req->frame->str, 0 );
    }
    else if( post_type == NNCMS_POST_GROUP )
    {
        // Header cells
        struct NNCMS_TABLE_CELL header_cells[] =
        {
            { .value = i18n_string_temp( req, "post_subject_forum_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_subject_count_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_msg_count_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_msg_last_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        // Fetch table data
        GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
        garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );

        // Get list of subforums associated to group
        database_bind_text( req->stmt_find_post_parent_type, 1, post_row->id );
        database_bind_text( req->stmt_find_post_parent_type, 2, "subforum" );
        struct NNCMS_POST_ROW *subforum_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post_parent_type );
        if( subforum_row != NULL )
        {
            garbage_add( req->loop_garbage, subforum_row, MEMORY_GARBAGE_DB_FREE );
        }

        //
        // Data
        //
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = post_row->subject, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };
        g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
        
        // Add subforums
        for( unsigned int j = 0; subforum_row != NULL && subforum_row[j].id != NULL; j = j + 1 )
        {
            if( post_check_perm( req, NULL, subforum_row[j].id, true, false, false ) == false )
            {
                continue;
            }
            
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "post_id", .value.string = subforum_row[j].id, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            char *list = main_temp_link( req, "post_list", subforum_row[j].subject, vars );

            // Number of subjects
            database_bind_text( req->stmt_count_child_posts, 1, subforum_row[j].id );
            struct NNCMS_ROW *subject_row_count = database_steps( req, req->stmt_count_child_posts );
            int subject_row_count_i = atoi( subject_row_count->value[0] );
            database_free_rows( subject_row_count );

            struct NNCMS_TABLE_CELL cells[] =
            {
                { .value.string = list, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .value.integer = subject_row_count_i, .type = NNCMS_TYPE_INTEGER, .options = NULL },
                { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .value.string = NULL, .type = NNCMS_TYPE_STRING, .options = NULL },
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
            .row_count = -1,
            .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
            .pager_quantity = 0, .pages_displayed = 0,
            .headerz = header_cells,
            .cells = (struct NNCMS_TABLE_CELL *) gcells->data
        };

        // Generate links
        char *links = post_links( req, post_row->id, post_row->parent_post_id );
        
        // Html output
        char *html_table = table_html( req, &table );

        // Specify values for template
        struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html_table, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/stock/net/stock_mail-open-multiple.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

        // Make a cute frame
        template_hparse( req, "frame.html", req->frame, frame_template );

        // Send generated html to client
        main_output( req, header_str, req->frame->str, 0 );
    }
    else if( post_type == NNCMS_POST_SUBFORUM )
    {
        //
        // Find child rows by id
        //
        database_bind_text( req->stmt_count_child_posts, 1, httpVarId );
        struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_child_posts );
        garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
        char *http_start = main_variable_get( req, req->get_tree, "start" );
        database_bind_text( req->stmt_topic_posts, 1, httpVarId );
        database_bind_int( req->stmt_topic_posts, 2, default_pager_quantity );
        database_bind_text( req->stmt_topic_posts, 3, (http_start != NULL ? http_start : "0") );
        struct NNCMS_POST_ROW *child_post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_topic_posts );
        if( child_post_row != NULL )
        {
            garbage_add( req->loop_garbage, child_post_row, MEMORY_GARBAGE_DB_FREE );
        }

        // Header cells
        struct NNCMS_TABLE_CELL header_cells[] =
        {
            { .value = i18n_string_temp( req, "post_subject_topic_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_responds_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = i18n_string_temp( req, "post_created_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        // Fetch table data
        GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
        garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
        for( unsigned int i = 0; child_post_row != NULL && child_post_row[i].id; i = i + 1 )
        {
            if( post_check_perm( req, NULL, child_post_row[i].id, true, false, false ) == false )
            {
                continue;
            }
            
            // Actions
            char *link;
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "post_id", .value.string = child_post_row[i].id, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };

            char *view = main_temp_link( req, "post_list", child_post_row[i].subject, vars );

            //
            // Process n-child rows 
            //
            database_bind_text( req->stmt_count_child_posts, 1, child_post_row[i].id );
            struct NNCMS_ROW *child_row_count = database_steps( req, req->stmt_count_child_posts );
            int child_row_count_i = atoi( child_row_count->value[0] );
            database_free_rows( child_row_count );

            //
            // Data
            //
            struct NNCMS_TABLE_CELL cells[] =
            {
                { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
                { .value.integer = child_row_count_i, .type = NNCMS_TYPE_INTEGER, .options = NULL },
                { .value.string = child_post_row[i].timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP, .options = NULL },
                { .value.string = child_post_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
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

        // Generate links
        char *links = post_links( req, post_row->id, post_row->parent_post_id );
        
        // Html output
        char *html_table = table_html( req, &table );

        // Specify values for template
        struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html_table, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/stock/net/stock_mail-open-multiple.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

        // Make a cute frame
        template_hparse( req, "frame.html", req->frame, frame_template );

        // Send generated html to client
        main_output( req, header_str, req->frame->str, 0 );    
    }
    else // TOPIC, MESSAGE
    {
        // Generate links
        char *root_links = post_links( req, post_row->id, post_row->parent_post_id );

        database_bind_text( req->stmt_find_user_by_id, 1, post_row->user_id );
        struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );

        GString *html = g_string_sized_new( 100 );
        garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GSTRING_FREE );
        struct NNCMS_VARIABLE post_template[] =
        {
            { .name = "header", .value.string = post_row->subject, .type = NNCMS_TYPE_STRING },
            { .name = "body", .value.string = post_row->body, .type = NNCMS_TYPE_STRING },
            { .name = "user", .value.string = post_row->user_id, .type = TEMPLATE_TYPE_USER },
            { .name = "user_id", .value.string = user_row->id, .type = NNCMS_TYPE_STRING },
            { .name = "user_name", .value.string = user_row->name, .type = NNCMS_TYPE_STRING },
            { .name = "user_nick", .value.string = user_row->nick, .type = NNCMS_TYPE_STRING },
            { .name = "user_avatar", .value.string = user_row->avatar, .type = NNCMS_TYPE_STRING },
            { .name = "timestamp", .value.string = post_row->timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP },
            { .name = "links", .value.string = root_links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
        template_hparse( req, "post.html", html, post_template );

        database_free_rows( (struct NNCMS_ROW *) user_row );

        //
        // Find child rows by id
        //
        //database_bind_text( req->stmt_count_child_posts, 1, httpVarId );
        //struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_child_posts );
        //garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
        //char *http_start = main_variable_get( req, req->get_tree, "start" );
        //database_bind_text( req->stmt_topic_posts, 1, httpVarId );
        //database_bind_int( req->stmt_topic_posts, 2, default_pager_quantity );
        //database_bind_text( req->stmt_topic_posts, 3, (http_start != NULL ? http_start : "0") );

        // Recursive stack
        struct POST_STACK
        {
            int depth;
            struct NNCMS_POST_ROW *post_row;
        };
        GArray *gstack = g_array_new( true, false, sizeof(struct POST_STACK)  );
        g_array_append_vals( gstack, & (struct POST_STACK) { .depth = 0, .post_row = post_row }, 1 );

        // Stack will increase as new posts found
        for( unsigned int i = 0; i < gstack->len; i = i + 1 )
        {
            struct POST_STACK *current_stack = & ((struct POST_STACK *) (gstack->data)) [i];
            int depth = current_stack->depth;
            
            database_bind_text( req->stmt_topic_posts, 1, current_stack->post_row->id );
            database_bind_int( req->stmt_topic_posts, 2, -1 );
            database_bind_int( req->stmt_topic_posts, 3, 0 );
            struct NNCMS_POST_ROW *child_post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_topic_posts );
            if( child_post_row == NULL )
            {
                continue;
            }
            garbage_add( req->loop_garbage, child_post_row, MEMORY_GARBAGE_DB_FREE );

            for( unsigned int j = 0; child_post_row[j].id; j = j + 1 )
            {
                struct POST_STACK stack =
                {
                    .depth = depth + 1,
                    .post_row = &child_post_row[j]
                };
                g_array_insert_vals( gstack, i + 1, &stack, 1 );
            }
        }

        // Fetch table data
        for( unsigned int i = 1; i < gstack->len; i = i + 1 )
        {
            struct POST_STACK *current_stack = & ((struct POST_STACK *) (gstack->data)) [i];
            
            if( post_check_perm( req, NULL, current_stack->post_row->id, true, false, false ) == false )
            {
                continue;
            }
            
            // Generate links
            char *links = post_links( req, current_stack->post_row->id, NULL );
            
            database_bind_text( req->stmt_find_user_by_id, 1, current_stack->post_row->user_id );
            struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );
            
            struct NNCMS_VARIABLE post_template[] =
            {
                { .name = "header", .value.string = current_stack->post_row->subject, .type = NNCMS_TYPE_STRING },
                { .name = "depth", .value.integer = current_stack->depth, .type = NNCMS_TYPE_INTEGER },
                { .name = "body", .value.string = current_stack->post_row->body, .type = NNCMS_TYPE_STRING },
                { .name = "user", .value.string = user_row->id, .type = TEMPLATE_TYPE_USER },
                { .name = "user_id", .value.string = user_row->id, .type = NNCMS_TYPE_STRING },
                { .name = "user_name", .value.string = user_row->name, .type = NNCMS_TYPE_STRING },
                { .name = "user_nick", .value.string = user_row->nick, .type = NNCMS_TYPE_STRING },
                { .name = "user_avatar", .value.string = user_row->avatar, .type = NNCMS_TYPE_STRING },
                { .name = "timestamp", .value.string = current_stack->post_row->timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP },
                { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };

            template_hparse( req, "post.html", html, post_template );
            
            database_free_rows( (struct NNCMS_ROW *) user_row );
        }
        
        g_array_free( gstack, true );

        //
        // Add new message form
        //
        char *html_add_form = NULL;
        if( post_check_perm( req, NULL, post_row->id, true, false, true ) == true )
        {
            struct NNCMS_POST_ADD_FIELDS *fields = memdup_temp( req, &post_add_fields, sizeof(post_add_fields) );
            fields->post_id.viewable = false;
            fields->post_user_id.value = req->user_id;
            fields->post_parent_post_id.value = post_row->id;
            fields->post_subject.value = NULL;
            fields->referer.value = FCGX_GetParam( "REQUEST_URI", req->fcgi_request->envp );
            fields->fkey.value = req->session_id;

            if( acl_check_perm( req, "post", NULL, "chown" ) == false )
            {
                fields->post_user_id.editable = false;
                fields->post_group_id.editable = false;
                fields->post_group_id.viewable = false;
            }
            
            if( acl_check_perm( req, "post", NULL, "chmod" ) == false )
            {
                fields->post_mode.editable = false;
                fields->post_mode.viewable = false;
            }

            if( is_devel == false )
            {
                fields->post_parent_post_id.type = NNCMS_FIELD_HIDDEN;
                fields->post_type.editable = false;
                fields->post_type.viewable = false;
            }

            struct NNCMS_VARIABLE vars[] =
                {
                    { .name = "post_id", .value.string = post_row->id, .type = NNCMS_TYPE_STRING },
                    { .type = NNCMS_TYPE_NONE } // Terminating row
                };
            char *action = main_temp_link( req, "post_add", NULL, vars );

            // Form
            struct NNCMS_FORM form =
            {
                .name = "post_add", .action = action, .method = "POST",
                .title = NULL, .help = NULL,
                .header_html = NULL, .footer_html = NULL,
                .fields = (struct NNCMS_FIELD *) fields
            };
            html_add_form = form_html( req, &form );
        }

        g_string_append( html, html_add_form );

        // Generate links
        char *links = post_links( req, post_row->id, post_row->parent_post_id );

        char *pager = "";//pager_html( req, atoi( row_count->value[0] ) );

        // Specify values for template
        struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html->str, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/stock/net/stock_mail-open-multiple.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "pager", .value.string = pager, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

        // Make a cute frame
        template_hparse( req, "frame.html", req->frame, frame_template );

        // Send generated html to client
        main_output( req, header_str, req->frame->str, 0 );    
    }
}

// #############################################################################

struct NNCMS_FORM *post_view_form( struct NNCMS_THREAD_INFO *req, struct NNCMS_POST_ROW *post_row )
{
    bool is_devel = acl_check_perm( req, "post", NULL, "devel" );
    
    // Fields
    struct NNCMS_POST_VIEW_FIELDS *fields = memdup( &post_view_fields, sizeof(post_view_fields) );
    fields->post_id.value = post_row->id;
    fields->post_user_id.value = post_row->user_id;
    fields->post_group_id.value = post_row->group_id;
    fields->post_parent_post_id.value = post_row->parent_post_id;
    fields->post_timestamp.value = post_row->timestamp;
    fields->post_subject.value = post_row->subject;
    fields->post_body.value = post_row->body;
    fields->post_type.value = post_row->type;
    fields->post_mode.value = post_row->mode;

    if( acl_check_perm( req, "post", NULL, "chown" ) == false )
    {
        fields->post_user_id.editable = false;
        fields->post_group_id.editable = false;
        fields->post_group_id.viewable = false;
    }
    
    if( acl_check_perm( req, "post", NULL, "chmod" ) == false )
    {
        fields->post_mode.editable = false;
        fields->post_mode.viewable = false;
    }

    if( is_devel == false )
    {
        fields->post_id.viewable = false;
        fields->post_parent_post_id.type = NNCMS_FIELD_HIDDEN;
        fields->post_type.editable = false;
        fields->post_type.viewable = false;
    }

    // Form
    struct NNCMS_FORM *form = MALLOC( sizeof(struct NNCMS_FORM) * 1 );
    *form =
    (struct NNCMS_FORM) {
        .name = "post_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };
    
    garbage_add( req->loop_garbage, fields, MEMORY_GARBAGE_FREE );
    garbage_add( req->loop_garbage, form, MEMORY_GARBAGE_FREE );
    
    return form;
}

void post_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to view ACLs
    if( acl_check_perm( req, "post", NULL, "view" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "post_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_post, 1, httpVarId );
    struct NNCMS_POST_ROW *post_row = (struct NNCMS_POST_ROW *) database_steps( req, req->stmt_find_post );
    if( post_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, post_row, MEMORY_GARBAGE_DB_FREE );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "post_id", .value.string = post_row->id, .type = NNCMS_TYPE_STRING },
            { .name = "post_subject", .value.string = post_row->subject, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "post_view_header", vars );

    // Html output
    struct NNCMS_FORM *form = post_view_form( req, post_row );
    char *html = form_html( req, form );

    // Generate links
    char *links = post_links( req, post_row->id, post_row->parent_post_id );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/stock/net/stock_mail-open.png", .type = NNCMS_TYPE_STRING },
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

char *post_links( struct NNCMS_THREAD_INFO *req, char *post_id, char *post_parent_post_id )
{
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "post_id", .value.string = post_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_parent[] =
    {
        { .name = "post_id", .value.string = post_parent_post_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Get access
    bool read_access = false;
    bool write_access = false;
    bool exec_access = false;
    bool parent_read_access = false;
    bool parent_write_access = false;
    bool parent_exec_access = false;
    if( post_id )  post_get_access( req, NULL, post_id, &read_access, &write_access, &exec_access );
    if( post_parent_post_id )  post_get_access( req, NULL, post_parent_post_id, &parent_read_access, &parent_write_access, &parent_exec_access );

    // Create array for links
    struct NNCMS_LINK link =
    {
        .function = NULL,
        .title = NULL,
        .vars = NULL
    };

    GArray *links = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_LINK) );

    // Fill the link array with links

    if( read_access == true )
    {
        link.function = "post_list";
        link.title = i18n_string_temp( req, "list_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );
    }

    if( parent_read_access == true )
    {
        if( post_parent_post_id != NULL )
        {
            link.function = "post_list";
            link.title = i18n_string_temp( req, "parent_list_link", NULL );
            link.vars = vars_parent;
            g_array_append_vals( links, &link, 1 );
        }
    }

    if( exec_access == true )
    {
        link.function = "post_add";
        link.title = i18n_string_temp( req, "add_link", NULL );
        link.vars = vars;
        g_array_append_vals( links, &link, 1 );
    }

    if( post_id != NULL )
    {
        if( read_access == true )
        {
            link.function = "post_view";
            link.title = i18n_string_temp( req, "view_link", NULL );
            link.vars = vars;
            g_array_append_vals( links, &link, 1 );
        }

        if( write_access == true )
        {
            link.function = "post_edit";
            link.title = i18n_string_temp( req, "edit_link", NULL );
            link.vars = vars;
            g_array_append_vals( links, &link, 1 );

            link.function = "post_delete";
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
