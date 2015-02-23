// #############################################################################
// Header file: threadinfo.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <stdio.h>

#include <fcgiapp.h>
#include <pthread.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <sqlite3.h>

#include <glib.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __threadinfo_h__
#define __threadinfo_h__

// #############################################################################
// type and constant definitions
//

// WTF
// /usr/include/ImageMagick/magick/magick-config.h:29:3: Warnung: #warning "you should set MAGICKCORE_QUANTUM_DEPTH to sensible default set it to configure time default" [-Wcpp]
// /usr/include/ImageMagick/magick/magick-config.h:30:3: Warnung: #warning "this is an obsolete behavior please fix your makefile" [-Wcpp]
// /usr/include/ImageMagick/magick/magick-config.h:52:3: Warnung: #warning "you should set MAGICKCORE_HDRI_ENABLE to sensible default set it to configure time default" [-Wcpp]
// /usr/include/ImageMagick/magick/magick-config.h:53:3: Warnung: #warning "this is an obsolete behavior please fix yours makefile" [-Wcpp]
//#define MAGICKCORE_QUANTUM_DEPTH    16
//#define MAGICKCORE_HDRI_ENABLE      0

#define NNCMS_USERNAME_LEN      128
#define NNCMS_COLUMNS_MAX       16
#define NNCMS_VARIABLES_MAX     20
#define NNCMS_BLOCK_LEN_MAX     10 * 1024
#define NNCMS_HEADERS_LEN_MAX   1024*2
#define NNCMS_UPLOAD_LEN_MAX    10 * 1024 * 1024
#define NNCMS_VAR_NAME_LEN_MAX  48
#define NNCMS_BOUNDARY_LEN_MAX  128
#define NNCMS_PATH_LEN_MAX      255//FILENAME_MAX

// For file and post module
#define NNCMS_ICONS_MAX         1024
#define NNCMS_ACTIONS_LEN_MAX   2048

// Structure of one row
struct NNCMS_ROW
{
    char *name[NNCMS_COLUMNS_MAX];          // Contains a pointer to column name
    char *value[NNCMS_COLUMNS_MAX];         // Contains a pointer to column's value
    struct NNCMS_ROW *next;                 // Pointer to the next row
                                            // If zero, then this is last row
};

//
// Structure to hold query variables
//

enum NNCMS_VARIABLE_TYPE
{
    NNCMS_TYPE_NONE = 0,
    NNCMS_TYPE_STRING,
    NNCMS_TYPE_DUP_STRING,
    NNCMS_TYPE_INTEGER,
    NNCMS_TYPE_UNSIGNED_INTEGER,
    NNCMS_TYPE_BOOL,
    NNCMS_TYPE_STR_BOOL,
    NNCMS_TYPE_PMEM,
    NNCMS_TYPE_MEM,
    NNCMS_TYPE_TIMESTAMP,
    NNCMS_TYPE_STR_TIMESTAMP,
    TEMPLATE_TYPE_UNSAFE_STRING,    // String will be html escaped
    TEMPLATE_TYPE_LINKS,            // Pointer to array of NNCMS_LINK structures with null terminating
    TEMPLATE_TYPE_USER,             // user_id is converted into html
    TEMPLATE_TYPE_GROUP,
    TEMPLATE_TYPE_LANGUAGE,         // two letter to native name
    TEMPLATE_TYPE_ICON,             // makes <img src="var.value" title="var.name">
    TEMPLATE_TYPE_FIELD_CONFIG,
};

union NNCMS_VARIABLE_UNION
{
    void *pmem;                        // [Pointer] Pointer to variable value
    char *string;
    int integer;
    unsigned int unsigned_integer;
    bool boolean;
    time_t time;
};

struct NNCMS_VARIABLE
{
    char name[NNCMS_VAR_NAME_LEN_MAX];     // [String] Variable name
    union NNCMS_VARIABLE_UNION value;      // [Pointer] Pointer to variable value
    enum NNCMS_VARIABLE_TYPE type;         // [Number] ID number describing the value of the variable
};

//
// List of uploaded files
//

struct NNCMS_FILE
{
    char *var_name;
    char *file_name;
    char *data;
    size_t size;
};

enum NNCMS_HTTPD
{
    HTTPD_NONE = 0,
    HTTPD_UNKNOWN,
    HTTPD_LIGHTTPD,
    HTTPD_NGINX
};

// Thread info
struct NNCMS_THREAD_INFO
{
    int nThreadID;
    pthread_t pThread;

    // Temporary buffers
    GString *buffer;
    GString *frame;
    GString *temp;

    // Reserved for main_output
    // Do not use it as variable for main_output
    GString *output;

    // FastCGI Request data
    FCGX_Request *fcgi_request;
    int content_lenght; char *content_type; char *content_data;
    char *request_method; char *script_name; char *query_string; char *cookie;
    char *function_name;
    char *referer;

    GTree *get_tree;
    GTree *post_tree;
    GTree *cookie_tree;
    GArray *files;

    // HTTPd info
    char *server_software;
    enum NNCMS_HTTPD httpd_type;

    // Response data
    char response_code[NNCMS_HEADERS_LEN_MAX];
    char response_headers[NNCMS_HEADERS_LEN_MAX];
    char response_content_type[NNCMS_HEADERS_LEN_MAX];
    char headers_sent;

    // Login data
    bool session_checked;
    char user_id[32];
    char user_name[NNCMS_USERNAME_LEN];
    char *user_nick;
    char user_group[NNCMS_USERNAME_LEN];
    char *user_avatar;
    char *user_folder_id;
    char **group_id;
    char *session_id;
    char *user_address, *user_agent; // For ban and session check
    bool is_guest;
    char *language;

    // Prepared statements for acl
    sqlite3_stmt *stmt_add_acl, *stmt_find_acl_by_id, *stmt_find_acl_by_name, *stmt_count_acl,
        *stmt_edit_acl, *stmt_delete_acl, *stmt_delete_object, *stmtGetACLId,
        *stmt_get_acl_name, *stmt_list_acl, *stmt_find_user, *stmt_check_acl_perm;

    // Prepared statements for log
    sqlite3_stmt *stmt_list_logs, *stmt_view_log, *stmt_add_log_req,
        *stmt_clear_log, *stmt_count_logs;

    // Prepared statements for user
    sqlite3_stmt *stmt_find_user_by_name, *stmt_find_user_by_id, *stmt_add_user,
        *stmt_login_user_by_name, *stmt_login_user_by_id, *stmt_find_session,
        *stmt_count_sessions, *stmt_add_session, *stmt_delete_session,
        *stmt_edit_session, *stmt_clean_sessions, *stmt_list_sessions,
        *stmt_list_users, *stmt_edit_user_hash, *stmt_edit_user_nick,
        *stmt_edit_user_group, *stmt_delete_user, *stmt_count_users,
        *stmt_edit_user_avatar;

    // Prepared statements for post
    sqlite3_stmt *stmt_add_post, *stmt_find_post, *stmt_power_edit_post, *stmt_edit_post,
        *stmt_delete_post, *stmt_get_parent_post, *stmt_root_posts;
    sqlite3_stmt *stmt_topic_posts, *stmt_find_post_parent_type;
    sqlite3_stmt *stmt_count_child_posts;

    // Prepared statements for ban
    sqlite3_stmt *stmt_add_ban, *stmt_list_bans, *stmt_find_ban_by_id, *stmt_edit_ban,
        *stmt_delete_ban;

    // Prepared statements for i18n
    sqlite3_stmt *stmt_add_source, *stmt_quick_find_source, *stmt_quick_find_target,
        *stmt_find_language_by_prefix, *stmt_count_locales_source,
        *stmt_list_locales_source, *stmt_list_language_enabled,
        *stmt_find_locales_source, *stmt_find_locales_target,
        *stmt_add_locales_target, *stmt_edit_locales_target;

    // Prepared statements for file
    sqlite3_stmt *stmt_find_file_by_id, *stmt_count_file_childs, *stmt_list_files,
        *stmt_list_file_childs, *stmt_add_file, *stmt_edit_file, *stmt_delete_file,
        *stmt_list_folders, *stmt_find_file_by_name;

    // Prepared statements for node
    sqlite3_stmt *stmt_delete_node, *stmt_delete_node_rev, *stmt_list_node_rev,
        *stmt_find_node_rev, *stmt_delete_field_data_by_node_rev_id,
        *stmt_list_field_config, *stmt_add_node, *stmt_add_node_rev,
        *stmt_add_field_data, *stmt_update_node_node_rev_id,
        *stmt_find_field_config, *stmt_update_field_config,
        *stmt_add_field_config, *stmt_delete_field_config,
        *stmt_delete_field_data_by_field_id, *stmt_count_field_config,
        *stmt_list_node_type, *stmt_count_node_type, *stmt_find_node_type,
        *stmt_add_node_type, *stmt_edit_node_type, *stmt_delete_node_type,
        *stmt_list_node, *stmt_count_node_rev, *stmt_count_node,
        *stmt_find_node, *stmt_find_field_data, *stmt_list_all_field_config, *stmt_edit_node;

    // Prepared statements for database
    sqlite3_stmt *stmt_find_by_id;

    // Prepared statements for view
    sqlite3_stmt *stmt_list_view, *stmt_count_view, *stmt_find_view,
        *stmt_add_view, *stmt_edit_view, *stmt_delete_view;
    sqlite3_stmt *stmt_list_field_view, *stmt_count_field_view, *stmt_matching_field_view, *stmt_find_field_view,
        *stmt_add_field_view, *stmt_edit_field_view, *stmt_delete_field_view, *stmt_field_view_by_view_id;
    sqlite3_stmt *stmt_list_view_filter, *stmt_count_view_filter, *stmt_find_view_filter,
        *stmt_add_view_filter, *stmt_edit_view_filter, *stmt_delete_view_filter, *stmt_find_view_filter_by_view_id;
    sqlite3_stmt *stmt_list_view_sort, *stmt_count_view_sort, *stmt_find_view_sort,
        *stmt_add_view_sort, *stmt_edit_view_sort, *stmt_delete_view_sort, *stmt_find_view_sort_by_view_id;

    // Prepared statements for group
    sqlite3_stmt *stmt_list_group, *stmt_count_group, *stmt_find_group,
        *stmt_add_group, *stmt_edit_group, *stmt_delete_group;
    sqlite3_stmt *stmt_list_ug, *stmt_count_ug, *stmt_find_ug,
        *stmt_add_ug, *stmt_edit_ug, *stmt_delete_ug,
        *stmt_find_ug_by_group_id, *stmt_find_ug_by_user_id;

    // Prepared statements for xxx
    sqlite3_stmt *stmt_list_xxx, *stmt_count_xxx, *stmt_find_xxx,
        *stmt_add_xxx, *stmt_edit_xxx, *stmt_delete_xxx;

    // Lua VM
    lua_State *L;

    // This collects pointers which are freed later
    GArray *loop_garbage;
    GArray *thread_garbage;

    // Remove me
    char *lpszBuffer;
    char *lpszFrame;
    char *lpszTemp;
    char *lpszOutput;
    char *lpszBlockContent;
    char *lpszBlockTemp;
    struct NNCMS_ROW *firstRow;
    struct NNCMS_ROW *lastRow;
};

// #############################################################################
// function declarations
//

// #############################################################################

#endif // __threadinfo_h__

// #############################################################################
