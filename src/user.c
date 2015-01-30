// #############################################################################
// Source file: user.c

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
#include "threadinfo.h"
#include "user.h"
#include "group.h"
#include "filter.h"
#include "database.h"
#include "template.h"
#include "log.h"
#include "acl.h"
#include "smart.h"
#include "memory.h"
#include "i18n.h"

#include "strlcpy.h"
#include "strlcat.h"

#define HAVE_IMAGEMAGICK
#ifdef HAVE_IMAGEMAGICK
#include <wand/MagickWand.h>
#endif // HAVE_IMAGEMAGICK

// #############################################################################
// global variables
//

struct NNCMS_USER_FIELDS
{
    struct NNCMS_FIELD user_id;
    struct NNCMS_FIELD user_name;
    struct NNCMS_FIELD user_nick;
    //struct NNCMS_FIELD user_group;
    struct NNCMS_FIELD user_pass;
    struct NNCMS_FIELD user_pass_retry;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD user_login;
    struct NNCMS_FIELD user_add;
    struct NNCMS_FIELD user_edit;
    struct NNCMS_FIELD none;
}
user_fields =
{
    { .name = "user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "user_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = alphanum_underscore_validate },
    { .name = "user_nick", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000 },
    //{ .name = "user_group", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "user_pass", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "user_pass_retry", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "user_login", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "user_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "user_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// How often check and remove old sessions
int tSessionExpires = 2 * 60;
int tCheckSessionInterval = 1 * 60;

static char session_charmap[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l',
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
    'z', 'x', 'c', 'v', 'b', 'n', 'm',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M'
};

char *guest_user_id;
char *guest_user_name;
char *guest_user_nick;
char *guest_user_group = "@guest";
char **guest_user_groups;

// Path to user directories
char userDir[NNCMS_PATH_LEN_MAX] = "./uf/";
char avatarDir[NNCMS_PATH_LEN_MAX] = "./a/";

// #############################################################################
// functions

bool user_global_init( )
{
    main_local_init_add( &user_local_init );
    main_local_destroy_add( &user_local_destroy );

    main_page_add( "user_register", &user_register );
    main_page_add( "user_login", &user_login );
    main_page_add( "user_logout", &user_logout );
    main_page_add( "user_sessions", &user_sessions );
    main_page_add( "user_list", &user_list );
    main_page_add( "user_view", &user_view );
    main_page_add( "user_avatar", &user_avatar );
    main_page_add( "user_edit", &user_edit );
    main_page_add( "user_delete", &user_delete );

    struct NNCMS_USER_ROW *user_row;
    user_row = (struct NNCMS_USER_ROW *) database_query( NULL, "SELECT `id`, `name`, `nick` FROM users WHERE name='guest'" );
    if( user_row == NULL ) return false;
    guest_user_id = user_row->id;
    guest_user_name = user_row->name;
    guest_user_nick = user_row->nick;
    garbage_add( global_garbage, user_row, MEMORY_GARBAGE_DB_FREE );

    struct NNCMS_USERGROUP_ROW *group_row;
    group_row = (struct NNCMS_USERGROUP_ROW *) database_query( NULL, "SELECT id, user_id, group_id FROM usergroups WHERE user_id=%q", guest_user_id );
    GArray *user_groups = g_array_new( true, false, sizeof(char *) );
    for( int i = 0; group_row != NULL && group_row[i].id != NULL; i = i + 1 )
    {
        g_array_append_vals( user_groups, &group_row[i].group_id, 1 );
    }
    guest_user_groups = (char **) g_array_free( user_groups, false );
    garbage_add( global_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    garbage_add( global_garbage, guest_user_groups, MEMORY_GARBAGE_GFREE );

    return true;
}

// #############################################################################

bool user_global_destroy( )
{
    return true;
}

// #############################################################################

bool user_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_find_user_by_name = database_prepare( req, "SELECT `id`, `name` FROM `users` WHERE `name`=?" );
    req->stmt_find_user_by_id = database_prepare( req, "SELECT `id`, `name`, `nick`, `group`, `hash`, `avatar`, `folder_id` FROM `users` WHERE `id`=?" );
    req->stmt_count_users = database_prepare( req, "SELECT COUNT(*) FROM `users`" );
    req->stmt_list_users = database_prepare( req, "SELECT `id`, `name`, `nick`, `group`, `avatar`, `folder_id` FROM `users` LIMIT ? OFFSET ?" );
    req->stmt_add_user = database_prepare( req, "INSERT INTO `users` VALUES(null, ?, ?, '@user', ?, ?, ?)" );
    req->stmt_login_user_by_name = database_prepare( req, "SELECT `id` FROM `users` WHERE `name`=? AND `hash`=? LIMIT 1" );
    req->stmt_login_user_by_id = database_prepare( req, "SELECT `name` FROM `users` WHERE `id`=? AND `hash`=? LIMIT 1" );
    req->stmt_find_session = database_prepare( req, "SELECT id, `user_id`, `user_name`, `user_agent`, `remote_addr`, timestamp FROM `sessions` WHERE `id`=?" );
    req->stmt_add_session = database_prepare( req, "INSERT INTO `sessions` VALUES(?, ?, ?, ?, ?, ?)" );
    req->stmt_delete_session = database_prepare( req, "DELETE FROM `sessions` WHERE `id`=?" );
    req->stmt_edit_session = database_prepare( req, "UPDATE sessions SET `user_id`=?, `user_name`=?, `user_agent`=?, `remote_addr`=?, `timestamp`=? WHERE id=?" );
    req->stmt_list_sessions = database_prepare( req, "SELECT `id`, `user_id`, `user_name`, `user_agent`, `remote_addr`, `timestamp` FROM `sessions` ORDER BY `timestamp` DESC LIMIT ? OFFSET ?" );
    req->stmt_count_sessions = database_prepare( req, "SELECT COUNT(*) FROM sessions" );
    req->stmt_clean_sessions = database_prepare( req, "DELETE FROM `sessions` WHERE `timestamp` < ?" );
    req->stmt_edit_user_hash = database_prepare( req, "UPDATE `users` SET `hash`=? WHERE `id`=?" );
    req->stmt_edit_user_nick = database_prepare( req, "UPDATE `users` SET `nick`=? WHERE `id`=?" );
    req->stmt_edit_user_group = database_prepare( req, "UPDATE `users` SET `group`=? WHERE `id`=?" );
    req->stmt_edit_user_avatar = database_prepare( req, "UPDATE `users` SET `avatar`=? WHERE `id`=?" );
    req->stmt_delete_user = database_prepare( req, "DELETE FROM `users` WHERE `id`=?" );

    user_reset_session( req );

    return true;
}

// #############################################################################

bool user_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_find_user_by_name );
    database_finalize( req, req->stmt_find_user_by_id );
    database_finalize( req, req->stmt_count_users );
    database_finalize( req, req->stmt_list_users );
    database_finalize( req, req->stmt_add_user );
    database_finalize( req, req->stmt_login_user_by_name );
    database_finalize( req, req->stmt_login_user_by_id );
    database_finalize( req, req->stmt_find_session );
    database_finalize( req, req->stmt_add_session );
    database_finalize( req, req->stmt_edit_session );
    database_finalize( req, req->stmt_delete_session );
    database_finalize( req, req->stmt_clean_sessions );
    database_finalize( req, req->stmt_count_sessions );
    database_finalize( req, req->stmt_list_sessions );
    database_finalize( req, req->stmt_edit_user_hash );
    database_finalize( req, req->stmt_edit_user_nick );
    database_finalize( req, req->stmt_edit_user_group );
    database_finalize( req, req->stmt_edit_user_avatar );
    database_finalize( req, req->stmt_delete_user );

    return true;
}

// #############################################################################

// Registration page
void user_register( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "user_register_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    //
    // Form
    //
    struct NNCMS_USER_FIELDS *fields = memdup_temp( req, &user_fields, sizeof(user_fields) );
    fields->user_id.viewable = false;
    //fields->user_group.viewable = false;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->user_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "user_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Check if submit button was pressed
    char *httpVarReg = main_variable_get( req, req->post_tree, "user_add" );
    if( httpVarReg != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }
        
        form_post_data( req, (struct NNCMS_FIELD *) fields );
        
        // Validate
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( field_valid == true )
        {
            //
            // Compare passwords
            //
            if( strcmp( fields->user_pass.value, fields->user_pass_retry.value ) != 0 )
            {
                // Passwords are different
                main_vmessage( req, "not_match" );
                return;
            }

            // Generate a hash value for password
            unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
            unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
            sha512( fields->user_pass.value, strlen( (char *) fields->user_pass.value ), digest );
            hash[2 * SHA512_DIGEST_SIZE] = '\0';
            for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
                sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

            //
            // Check if username exists in database
            //
            database_bind_text( req->stmt_find_user_by_name, 1, fields->user_name.value );
            struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_name );
            if( user_row != NULL )
            {
                // User with same user name was found
                main_vmessage( req, "already_exists" );
                return;
            }

            //
            // Add user in database
            //
            database_bind_text( req->stmt_add_user, 1, fields->user_name.value );
            database_bind_text( req->stmt_add_user, 2, fields->user_nick.value );
            database_bind_text( req->stmt_add_user, 3, hash );
            database_steps( req, req->stmt_add_user );
            unsigned int user_id = database_last_rowid( req );

            // Log action
            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "user_id", .value.unsigned_integer = user_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "user_name", .value.string = fields->user_name.value, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .name = "user_nick", .value.string = fields->user_nick.value, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            //log_vdisplayf( req, LOG_ACTION, "user_register_success", vars );
            main_vmessage( req, "user_register_success" );
            log_printf( req, LOG_ACTION, "User '%s' was registered (id = %u)", fields->user_name.value, user_id );
            
            redirect( req, "/" );
        }
    }

    // Generate links
    char *links = user_links( req, NULL );

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

void user_login( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "user", NULL, "login" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "user_id", .value.string = NULL, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "user_login_header", vars );

    //
    // Form
    //
    struct NNCMS_USER_FIELDS *fields = memdup_temp( req, &user_fields, sizeof(user_fields) );
    fields->user_id.viewable = false;
    fields->user_nick.viewable = false;
    //fields->user_group.viewable = false;
    fields->user_pass_retry.viewable = false;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->user_login.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "user_login", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *user_login = main_variable_get( req, req->post_tree, "user_login" );
    if( user_login != NULL )
    {
        // Anti CSRF / XSRF vulnerabilities
        //if( user_xsrf( req ) == false )
        //{
        //    main_vmessage( req, "xsrf_fail" );
        //    return;
        //}

        // Get POST data
        form_post_data( req, (struct NNCMS_FIELD *) fields );
        
        // Validate
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( field_valid == true )
        {
            // Generate a hash value for password
            unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
            unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
            sha512( fields->user_pass.value, strlen( (char *) fields->user_pass.value ), digest );
            hash[2 * SHA512_DIGEST_SIZE] = '\0';
            for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
            {
                sprintf( (char *) hash + (2 * i), "%02x", digest[i] );
            }

            //
            // Find user in database
            //
            database_bind_text( req->stmt_login_user_by_name, 1, fields->user_name.value );
            database_bind_text( req->stmt_login_user_by_name, 2, hash );
            struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_login_user_by_name );
            if( user_row == NULL )
            {
                // Wrong password or user name
                log_vdisplayf( req, LOG_ERROR, "user_login_fail", vars );
            }
            else
            {
                //
                // Login approved
                //
                
                // Try to modify current session
                if( req->session_id != NULL )
                {
                    //
                    // Fill selected session id with some data
                    //
                    database_bind_text( req->stmt_edit_session, 1, user_row->id );
                    database_bind_text( req->stmt_edit_session, 2, fields->user_name.value );
                    database_bind_text( req->stmt_edit_session, 3, req->user_agent );
                    database_bind_text( req->stmt_edit_session, 4, req->user_address );
                    database_bind_int( req->stmt_edit_session, 5, time( 0 ) );
                    database_bind_text( req->stmt_edit_session, 6, req->session_id );
                    database_steps( req, req->stmt_edit_session );
                }
                else
                {
                    char *session_id = user_generate_session( req );
                    if( session_id == NULL )
                    {
                        main_vmessage( req, "fatal_error" );
                        return;
                    }
                    garbage_add( req->loop_garbage, session_id, MEMORY_GARBAGE_GFREE );

                    //
                    // Fill selected session id with some data
                    //
                    database_bind_text( req->stmt_add_session, 1, session_id );
                    database_bind_text( req->stmt_add_session, 2, user_row->id );
                    database_bind_text( req->stmt_add_session, 3, fields->user_name.value );
                    database_bind_text( req->stmt_add_session, 4, req->user_agent );
                    database_bind_text( req->stmt_add_session, 5, req->user_address );
                    database_bind_int( req->stmt_add_session, 6, time( 0 ) );
                    database_steps( req, req->stmt_add_session );
                    
                    // Set cookie and update session
                    main_set_cookie( req, "session_id", session_id );
                    req->session_id = session_id;
                }

                // User row is not needed anymore
                database_free_rows( (struct NNCMS_ROW *) user_row );

                log_vdisplayf( req, LOG_ACTION, "user_login_success", vars );

                // Redirect back
                redirect_to_referer( req );
                return;
            }
        }
    }

    // Generate links
    char *links = user_links( req, NULL );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/status/dialog-password.png", .type = NNCMS_TYPE_STRING },
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

char *user_generate_session( struct NNCMS_THREAD_INFO *req )
{
    GString *session_id = g_string_sized_new( NNCMS_SESSION_ID_LEN + 1 );

user_regenerate_session_id:
    {
        // Generate and check session id
        int random_value = 0;
        int iteration = 0;
        for( unsigned int i = 0; i < NNCMS_SESSION_ID_LEN; i++ )
        {
            char c = session_charmap[random_value % sizeof(session_charmap)];
            
            g_string_append_c( session_id, c );
            
            if( random_value <= sizeof(session_charmap) )
                random_value = rand( );
            else
                random_value = random_value / sizeof(session_charmap);
        }

        // Check if session number is already in use.
        // There's (sessions_in_use / (sizeof(session_charmap)^NNCMS_SESSION_ID_LEN))
        // chance that randomly generated session will be the same one as already
        // in use. (About [3,74 * 10^(-228)]%, that is very low chance, but we can not say
        // that there is no chance at all)
        database_bind_text( req->stmt_find_session, 1, session_id->str );
        struct NNCMS_ROW *session_row = database_steps( req, req->stmt_find_session );
        if( session_row != 0 )
        {
            database_free_rows( session_row );
            iteration += 1;
            if( iteration > 1 )
            {
                log_printf( req, LOG_CRITICAL, "Too much iterations while regenerating session id (iteration = %i)", iteration );
                return NULL;
            }
            else
            {
                log_printf( req, LOG_WARNING, "Regenerating session id (iteration = %i)", iteration );
                goto user_regenerate_session_id;
            }
        }
        
        // Session generated and it doesnt exist in database
        char *session_id_str = g_string_free( session_id, FALSE );
        return session_id_str;
    }
}

// #############################################################################

// This function verifies data in cookies with database
// and fills some global variables with neccessary login information
void user_check_session( struct NNCMS_THREAD_INFO *req )
{
    if( req->session_checked == true ) return;

    // Before anything - reset the global data to guest account, so
    // if something fails down there, then the guest account will be used
    // instead of last logged in person
    user_reset_session( req );
    req->session_checked = true;

    //
    // Get session_id from cookies
    //
    char *session_id = main_variable_get( req, req->cookie_tree, "session_id" );
    if( session_id == NULL ) return;
    filter_truncate_string( session_id, 256 );
    filter_table_replace( session_id, (unsigned int) strlen( session_id ), usernameFilter_cp1251 );

    // Find session in db
    database_bind_text( req->stmt_find_session, 1, session_id );
    struct NNCMS_SESSION_ROW *session_row = (struct NNCMS_SESSION_ROW *) database_steps( req, req->stmt_find_session );
    if( session_row == NULL )
    {
        // No sesssion found, assume guest account
        // Generate session id for guest
        char *session_id = user_generate_session( req );
        if( session_id == NULL )
        {
            return;
        }
        garbage_add( req->loop_garbage, session_id, MEMORY_GARBAGE_GFREE );
        
        // Store in database
        database_bind_text( req->stmt_add_session, 1, session_id );
        database_bind_text( req->stmt_add_session, 2, guest_user_id );
        database_bind_text( req->stmt_add_session, 3, guest_user_name );
        database_bind_text( req->stmt_add_session, 4, req->user_agent );
        database_bind_text( req->stmt_add_session, 5, req->user_address );
        database_bind_int( req->stmt_add_session, 6, time( 0 ) );
        database_steps( req, req->stmt_add_session );

        // Set Cookie and Redirect user to homepage
        main_set_cookie( req, "session_id", session_id );

        req->session_id = session_id;

        return;
    }

    // Check for cookie stealing
    if( strcmp( req->user_agent, session_row->user_agent ) != 0 ||
        strcmp( req->user_address, session_row->remote_addr ) != 0 )
    {
        log_print( req, LOG_WARNING, "Cookie stealing detected" );
        return;
    }

    // Find user
    database_bind_text( req->stmt_find_user_by_id, 1, session_row->user_id );
    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );
    if( user_row == 0 )
    {
        log_printf( req, LOG_WARNING, "Session (id = %s): user_id = %s points to unexisting user", session_row->id, session_row->user_id );
        return;
    }

    // Session found, copy data from it and free row data
    if( strcmp( user_row->id, guest_user_id ) != 0 )
    {
        req->is_guest = false;
    }
    req->session_id = session_id;
    strlcpy( req->user_id, user_row->id, sizeof(req->user_id) );
    strlcpy( req->user_name, user_row->name, sizeof(req->user_name) );
    strlcpy( req->user_group, user_row->group, sizeof(req->user_group) );
    req->user_nick = user_row->nick;
    req->user_avatar = user_row->avatar;
    req->user_folder_id = user_row->folder_id;
    
    // Load user groups
    database_bind_text( req->stmt_find_ug_by_user_id, 1, user_row->id );
    struct NNCMS_USERGROUP_ROW *group_row = (struct NNCMS_USERGROUP_ROW *) database_steps( req, req->stmt_find_ug_by_user_id );
    GArray *user_groups = g_array_new( true, false, sizeof(char *) );
    for( int i = 0; group_row != NULL && group_row[i].id != NULL; i = i + 1 )
    {
        g_array_append_vals( user_groups, &group_row[i].group_id, 1 );
    }
    req->group_id = (char **) g_array_free( user_groups, false );
    garbage_add( req->loop_garbage, group_row, MEMORY_GARBAGE_DB_FREE );
    garbage_add( req->loop_garbage, req->group_id, MEMORY_GARBAGE_GFREE );

    garbage_add( req->loop_garbage, user_row, MEMORY_GARBAGE_DB_FREE );
    //database_free_rows( (struct NNCMS_ROW *) user_row );
    database_free_rows( (struct NNCMS_ROW *) session_row );
}

// ############################################################################

bool user_xsrf( struct NNCMS_THREAD_INFO *req )
{
    //
    // Get session_id from cookies
    //
    char *session_id = main_variable_get( req, req->cookie_tree, "session_id" );
    if( session_id == 0 ) goto xsrf_attack;
    filter_truncate_string( session_id, 256 );
    filter_table_replace( session_id, (unsigned int) strlen( session_id ), usernameFilter_cp1251 );

    // Get POST data
    char *httpVarFKey = main_variable_get( req, req->post_tree, "fkey" );
    if( httpVarFKey == 0 ) goto xsrf_attack;
    filter_truncate_string( httpVarFKey, 256 );
    filter_table_replace( httpVarFKey, (unsigned int) strlen( httpVarFKey ), queryFilter );

    // Compare
    if( strcmp( session_id, httpVarFKey ) == 0 )
    {
        return true;
    }

xsrf_attack:
    log_print( req, LOG_ALERT, "XSRF attack detected" );

    return false;
}

// ############################################################################

void user_logout( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "user", NULL, "logout" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Reset cookie
    //main_set_cookie( req, "session_id", "0" );

    //
    // Get session_id from cookies
    //
    if( req->session_id != 0 && *(req->session_id) != 0 )
    {
        // Remove unused session from db
        //database_bind_text( req->stmt_delete_session, 1, req->session_id );
        //database_steps( req, req->stmt_delete_session );

        // Modify session to guest
        database_bind_text( req->stmt_edit_session, 1, "1" );
        database_bind_text( req->stmt_edit_session, 2, "guest" );
        database_bind_text( req->stmt_edit_session, 3, req->user_agent );
        database_bind_text( req->stmt_edit_session, 4, req->user_address );
        database_bind_int( req->stmt_edit_session, 5, time( 0 ) );
        database_bind_text( req->stmt_edit_session, 6, req->session_id );
        database_steps( req, req->stmt_edit_session );

        // Log action
        log_printf( req, LOG_ACTION, "User \"%s\" logged out", req->user_name );

        // Reset session to display next message in logged out state
        user_reset_session( req );

        // Display message
        main_vmessage( req, "user_logout_success" );
    }
    else
    {
        // Log action
        //log_printf( req, LOG_WARNING, "Session for user \"%s\" not found to log out", req->user_name );

        main_vmessage( req, "user_logout_fail" );
    }
}

// ############################################################################

time_t session_timer = 0;

// Reset session to initial status
void user_reset_session( struct NNCMS_THREAD_INFO *req )
{
    // Start a timer to check sessions
    if( session_timer == 0 )
        session_timer = time( 0 ) + 10;

    // Check timer time out
    time_t current_time = time( 0 );
    if( current_time >= session_timer )
    {
        // Update timer
        session_timer = current_time + tCheckSessionInterval;

        // Remove old sessions
        database_bind_int( req->stmt_clean_sessions, 1, current_time - tSessionExpires );
        database_steps( req, req->stmt_clean_sessions );
    }

    // Reset current session to "guest" account
    //memset( req->user_id, 0, sizeof(req->user_id) );
    //memset( req->user_name, 0, sizeof(req->user_name) );
    strcpy( req->user_id, guest_user_id );
    strcpy( req->user_name, guest_user_name );
    strcpy( req->user_group, guest_user_group );
    req->user_nick = guest_user_nick;
    req->user_avatar = NULL;
    req->user_folder_id = NULL;
    req->group_id = guest_user_groups;
    req->is_guest = true;
    req->session_id = NULL;
    req->session_checked = false;
}

// ############################################################################

void user_sessions( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "user_sessions_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", NULL, "view_sessions" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get session list
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_sessions );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_sessions, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_sessions, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_SESSION_ROW *session_row = (struct NNCMS_SESSION_ROW *) database_steps( req, req->stmt_list_sessions );
    if( session_row == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }
    garbage_add( req->loop_garbage, session_row, MEMORY_GARBAGE_DB_FREE );

    // Header cells
    struct NNCMS_VARIABLE cell_options[] =
    {
        { .name = "width", .value.string = "25%", .type = NNCMS_TYPE_STRING },
        { .name = "style", .value.string = "word-wrap: break-word;", .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "session_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = cell_options },
        { .value = i18n_string_temp( req, "user_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "user_agent_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "user_remote_addr_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "session_timestamp_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; session_row != NULL && session_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "session_id", .value.string = session_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *delete = main_temp_link( req, "session_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = session_row[i].id, .type = NNCMS_TYPE_STRING, .options = cell_options },
            { .value.string = session_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
            { .value.string = session_row[i].user_agent, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = session_row[i].remote_addr, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = session_row[i].timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP, .options = NULL },
            { .value.string = delete, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .type = NNCMS_TYPE_NONE }
        };

        g_array_append_vals( gcells, &cells, sizeof(cells) / sizeof(struct NNCMS_TABLE_CELL) - 1 );
    }

    // Create a table
    struct NNCMS_VARIABLE table_options[] =
    {
        { .name = "style", .value.string = "table-layout: fixed;", .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    struct NNCMS_TABLE table =
    {
        .caption = NULL,
        .header_html = NULL, .footer_html = NULL,
        .options = table_options,
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
            { .name = "icon", .value.string = "images/stock/text/stock_list_enum.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// ############################################################################

void user_list( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "user_list_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "user", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // List all users
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_users );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_users, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_users, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_list_users );
    if( user_row != NULL )
    {
        garbage_add( req->loop_garbage, user_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Generate links
    char *links = user_links( req, NULL );

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "user_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "user_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "user_nick_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "user_group_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; user_row != NULL && user_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "user_id", .value.string = user_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "user_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "user_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "user_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = user_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = user_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = user_row[i].nick, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = user_row[i].group, .type = NNCMS_TYPE_STRING, .options = NULL },
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

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/apps/system-users.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// ############################################################################

void user_view( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "user", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get log entry id
    char *user_id = main_variable_get( req, req->get_tree, "user_id" );
    if( user_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'id'
    database_bind_text( req->stmt_find_user_by_id, 1, user_id );
    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );
    if( user_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, user_row, MEMORY_GARBAGE_DB_FREE );

    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "user_id", .value.string = user_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_name", .value.string = user_row->name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_nick", .value.string = user_row->nick, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_group", .value.string = user_row->group, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Page header
    char *header_str = i18n_string_temp( req, "user_view_header", vars );

    // Generate links
    char *links = user_links( req, user_id );

    //
    // Form
    //

    struct NNCMS_FIELD fields[] =
    {
        { .name = "user_id", .value = user_id, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "user_name", .value = user_row->name, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "user_nick", .value = user_row->nick, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        //{ .name = "user_group", .value = user_row->group, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .type = NNCMS_FIELD_NONE }
    };

    struct NNCMS_FORM form =
    {
        .name = "user_view", .action = NULL, .method = "POST",
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
            { .name = "icon", .value.string = "images/actions/gtk-edit.png", .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    {
        // Html output
        char *html = ug_list_html( req, user_id, NULL );

        // Generate links
        char *links = ug_links( req, NULL );

        // Page header
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "user_id", .value.string = user_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
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

// ############################################################################

void user_avatar( struct NNCMS_THREAD_INFO *req )
{
    // Try to get log entry id
    char *user_id = main_variable_get( req, req->get_tree, "user_id" );
    if( user_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Permission check
    bool is_owner = (strcmp( user_id, req->user_id ) == 0);
    if( acl_check_perm( req, "user", NULL, "avatar" ) == false )
    {
        if( req->is_guest == true || is_owner == false || acl_test_perm( req, "user", owner_group_id, "avatar" ) == false )
        {
            main_vmessage( req, "not_allowed" );
            return;
        }
    }

    // Find row associated with our object by 'name'
    database_bind_text( req->stmt_find_user_by_id, 1, user_id );
    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );
    if( user_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, user_row, MEMORY_GARBAGE_DB_FREE );

    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "user_id", .value.string = user_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_name", .value.string = user_row->name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_nick", .value.string = user_row->nick, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_group", .value.string = user_row->group, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Page header
    char *header_str = i18n_string_temp( req, "user_avatar_header", vars );

    struct NNCMS_USER_AVATAR_FIELDS
    {
        struct NNCMS_FIELD user_avatar;
        struct NNCMS_FIELD file_avatar;
        struct NNCMS_FIELD referer;
        struct NNCMS_FIELD fkey;
        struct NNCMS_FIELD user_edit;
        struct NNCMS_FIELD none;
    }
    fields =
    {
        { .name = "user_avatar", .value = user_row->avatar, .data = NULL, .type = NNCMS_FIELD_PICTURE, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "file_avatar", .value = NULL, .data = NULL, .type = NNCMS_FIELD_UPLOAD, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "user_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };
    fields.referer.value = req->referer;
    fields.fkey.value = req->session_id;
    fields.user_edit.viewable = true;

    // Form
    struct NNCMS_FORM form =
    {
        .name = "user_avatar", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) &fields
    };

    //
    // Check if user commit changes
    //
    char *user_edit = main_variable_get( req, req->post_tree, "user_edit" );
    if( user_edit != NULL )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Retrieve GET data
        form_post_data( req, (struct NNCMS_FIELD *) &fields );

        // Validation
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) &fields );
        if( field_valid == false )
        {
            main_vmessage( req, "no_data" );
            return;
        }

        if( req->files == NULL )
        {
            main_message( req, "No file uploaded" );
            return;
        }

        if( req->files->len == 0 )
        {
            main_message( req, "No file uploaded" );
            return;
        }
        
        struct NNCMS_FILE *file = & ((struct NNCMS_FILE *) req->files->data) [0];
        if( file->var_name == NULL || file->file_name == NULL || file->data == NULL )
        {
            main_message( req, "No file uploaded" );
            return;
        }
        if( file->size >= 1 * 1024 * 1024 )
        {
            main_message( req, "File is too big" );
            return;
        }

        // Convert id from DB to path in FS
        GString *file_path = g_string_new( avatarDir );
        garbage_add( req->loop_garbage, file_path, MEMORY_GARBAGE_GSTRING_FREE );
        g_string_append( file_path, user_row->name );
        g_string_append( file_path, ".png" );

        /*char *extension_addr = rindex( file_path->str, '.' );
        if( extension_addr != NULL )
        {
            size_t extension_pos = extension_addr - file_path->str;
            size_t extension_size = file_path->len - extension_pos;
            if( extension_size > 4 )
            {
                main_message( req, "Extension is too big" );
                return;
            }

            if( strcmp( extension_addr + 1, "png" ) != 0 )
            {
                main_message( req, "Invalid extension" );
                return;
            }
        }
        else
        {
            main_message( req, "Extension not found" );
            return;
        }*/
        
        //
        // Initialize the image info structure and read an image.
        //
        MagickWand *magick_wand = NewMagickWand( );
        MagickBooleanType magick_result = MagickReadImageBlob( magick_wand, file->data, file->size );
        if( magick_result == MagickFalse )
        {
            ExceptionType severity;
            char *description = MagickGetException( magick_wand, &severity );
            log_printf( req, LOG_ERROR, "Image not loaded: %s %s", GetMagickModule(), description );
            description = (char *) MagickRelinquishMemory( description );
            magick_wand = DestroyMagickWand( magick_wand );
            
            main_message( req, description );
            
            return;
        }

        //
        // Calculate new width and height for thumblnail, keeping the
        // same aspect ratio.
        //
        double img_width = (double) MagickGetImageWidth( magick_wand );
        double img_height = (double) MagickGetImageHeight( magick_wand );
        unsigned long int new_width = 100;
        unsigned long int new_height = 100;
        if( img_width > new_width || img_height > new_height )
        {
            double img_ratio = img_width / img_height;
            if( img_ratio > 1.0f )
                new_height = (unsigned long int) ((double) new_height / img_ratio);
            else
                new_width = (unsigned long int) ((double) new_width * img_ratio);

            //
            //  Turn the images into a thumbnail sequence.
            //
            MagickResetIterator( magick_wand );
            MagickNextImage( magick_wand );
            MagickResizeImage( magick_wand, new_width, new_height, LanczosFilter, 1.0 );
        }

        // Convert to PNG
        MagickSetImageFormat( magick_wand, "PNG" );

        size_t lenght;
        unsigned char *blob = MagickGetImageBlob( magick_wand, &lenght );
        if( blob == NULL || lenght == 0 )
        {
            ExceptionType severity;
            char *description = MagickGetException( magick_wand, &severity );
            log_printf( req, LOG_ERROR, "Unable to convert image to blob: %s %s", GetMagickModule(), description );
            description = (char *) MagickRelinquishMemory( description );
            magick_wand = DestroyMagickWand( magick_wand );
            
            main_message( req, description );
            
            return;
        }

        bool result = main_store_file( file_path->str, blob, &lenght );
        if( result == false )
        {
            main_vmessage( req, "file_add_fail" );
            log_printf( req, LOG_ERROR, "Failed to open file '%s' for writing the contents of uploaded file", file_path->str );
            return;
        }

        GString *http_path = g_string_sized_new( 20 );
        garbage_add( req->loop_garbage, http_path, MEMORY_GARBAGE_GSTRING_FREE );
        g_string_append( http_path, "a/" );
        g_string_append( http_path, user_row->name );
        g_string_append( http_path, ".png" );

        //log_printf( req, LOG_DEBUG, "name: %s, file: %s, data: %s", file->var_name, file->file_name, file->data );
        database_bind_text( req->stmt_edit_user_avatar, 1, http_path->str );
        database_bind_text( req->stmt_edit_user_avatar, 2, user_row->id );
        database_steps( req, req->stmt_edit_user_avatar );

        // Log action
        log_vdisplayf( req, LOG_ACTION, "user_edit_success", vars );
        log_printf( req, LOG_ACTION, "User '%s' was editted", user_row->name );

        // Cleanup
        MagickRelinquishMemory( blob );
        magick_wand = DestroyMagickWand( magick_wand );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Generate links
    char *links = user_links( req, user_id );

    // Html output
    char *html = form_html( req, &form );
    //template_hparse( req, "user_avatar.html", req->temp, vars );

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

// ############################################################################

void user_edit( struct NNCMS_THREAD_INFO *req )
{
    // Try to get log entry id
    char *user_id = main_variable_get( req, req->get_tree, "user_id" );
    if( user_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'name'
    database_bind_text( req->stmt_find_user_by_id, 1, user_id );
    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );
    if( user_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, user_row, MEMORY_GARBAGE_DB_FREE );
    
    // Permission check
    bool is_owner = (strcmp( user_id, req->user_id ) == 0);
    if( acl_check_perm( req, "user", NULL, "edit" ) == false )
    {
        if( req->is_guest == true || is_owner == false || acl_test_perm( req, "user", owner_group_id, "edit" ) == false )
        {
            main_vmessage( req, "not_allowed" );
            return;
        }
    }

    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "user_id", .value.string = user_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_name", .value.string = user_row->name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_nick", .value.string = user_row->nick, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_group", .value.string = user_row->group, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Page header
    char *header_str = i18n_string_temp( req, "user_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_USER_FIELDS *fields = memdup_temp( req, &user_fields, sizeof(user_fields) );
    fields->user_id.value = user_row->id;
    fields->user_name.value = user_row->name;
    fields->user_nick.value = user_row->nick;
    //fields->user_group.value = user_row->group;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->user_edit.viewable = true;

    // Owners can't edit their names, only nick
    if( is_owner == true )
    {
        fields->user_name.editable = false;
    }

    struct NNCMS_FORM form =
    {
        .name = "user_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *user_edit = main_variable_get( req, req->post_tree, "user_edit" );
    if( user_edit != 0 )
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
            // Passwords
            if( fields->user_pass.value[0] != 0 && fields->user_pass_retry.value[0] != 0 )
            {
                //
                // Compare passwords
                //
                if( strcmp( fields->user_pass.value, fields->user_pass_retry.value ) != 0 )
                {
                    // Passwords are different
                    main_vmessage( req, "not_match" );
                    return;
                }

                // Generate a hash value for password
                unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
                unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
                sha512( fields->user_pass.value, strlen( (char *) fields->user_pass.value ), digest );
                hash[2 * SHA512_DIGEST_SIZE] = '\0';
                for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
                    sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

                // Query database
                database_bind_text( req->stmt_edit_user_hash, 1, hash );
                database_bind_text( req->stmt_edit_user_hash, 2, user_id );
                database_steps( req, req->stmt_edit_user_hash );

                // Log action
                log_vdisplayf( req, LOG_ACTION, "user_edit_pass_success", vars );
            }

            // Query database
            database_bind_text( req->stmt_edit_user_nick, 1, fields->user_nick.value );
            database_bind_text( req->stmt_edit_user_nick, 2, user_id );
            database_steps( req, req->stmt_edit_user_nick );
            //database_bind_text( req->stmt_edit_user_group, 1, fields->user_group.value );
            //database_bind_text( req->stmt_edit_user_group, 2, user_id );
            //database_steps( req, req->stmt_edit_user_group );

            // Log action
            log_vdisplayf( req, LOG_ACTION, "user_edit_success", vars );
            log_printf( req, LOG_ACTION, "User '%s' was editted", user_row->name );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = user_links( req, user_id );

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

// ############################################################################

void user_delete( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "user", NULL, "delete" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get log entry id
    char *user_id = main_variable_get( req, req->get_tree, "user_id" );
    if( user_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'name'
    database_bind_text( req->stmt_find_user_by_id, 1, user_id );
    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );
    if( user_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, user_row, MEMORY_GARBAGE_DB_FREE );

    // Fill template values
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "user_id", .value.string = user_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_name", .value.string = user_row->name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_nick", .value.string = user_row->nick, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "user_group", .value.string = user_row->group, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    // Page header
    char *header_str = i18n_string_temp( req, "user_delete_header", vars );

    //
    // Check if user commit changes
    //
    char *delete_submit = main_variable_get( req, req->post_tree, "delete_submit" );
    if( delete_submit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Query database
        database_bind_text( req->stmt_delete_user, 1, user_id );
        database_steps( req, req->stmt_delete_user );

        // Log action
        log_vdisplayf( req, LOG_ACTION, "user_delete_success", vars );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, user_id );

    // Generate links
    char *links = user_links( req, user_id );

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

char *user_links( struct NNCMS_THREAD_INFO *req, char *user_id )
{
    struct NNCMS_VARIABLE vars_id[] =
    {
        { .name = "user_id", .value.string = user_id, .type = NNCMS_TYPE_STRING },
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

    link.function = "user_list";
    link.title = i18n_string_temp( req, "list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    link.function = "user_register";
    link.title = i18n_string_temp( req, "add_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( user_id != NULL )
    {
        link.function = "user_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "user_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars_id;
        g_array_append_vals( links, &link, 1 );

        link.function = "user_delete";
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
