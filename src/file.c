// #############################################################################
// Source file: file.c

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
#include "file.h"
#include "main.h"
#include "mime.h"
#include "user.h"
#include "group.h"
#include "filter.h"
#include "template.h"
#include "log.h"
#include "smart.h"
#include "cache.h"
#include "i18n.h"
#include "database.h"

#include "strlcpy.h"
#include "strlcat.h"
#include "strdup.h"

#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#include <glib.h>
#include <glib/gstdio.h>

#define HAVE_IMAGEMAGICK
#ifdef HAVE_IMAGEMAGICK
#include <wand/MagickWand.h>
#endif // HAVE_IMAGEMAGICK

// #############################################################################
// global variables
//

// Path where files are located
char fileDir[NNCMS_PATH_LEN_MAX] = "./files/";

// How much columns in a row in gallery mode
int nGalleryCols = 4;

// How many items on a single page by default
int nItemsPerPage = 60;

// File icons
struct NNCMS_FILE_ICON fileIcons[] =
{
    { /* szExtension */ "default",  /* szIcon */ "images/mimetypes/text-x-preview.png" },
    { /* szExtension */ "folder",   /* szIcon */ "images/places/folder.png" },

    { /* szExtension */ "txt",      /* szIcon */ "images/mimetypes/text-x-generic.png" },
    { /* szExtension */ "log",      /* szIcon */ "images/mimetypes/text-x-generic.png" },
    { /* szExtension */ "conf",     /* szIcon */ "images/mimetypes/text-x-generic.png" },

    { /* szExtension */ "html",     /* szIcon */ "images/mimetypes/text-html.png" },
    { /* szExtension */ "htm",      /* szIcon */ "images/mimetypes/text-html.png" },

    { /* szExtension */ "mp3",      /* szIcon */ "images/mimetypes/audio-x-mp3.png" },
    { /* szExtension */ "wav",      /* szIcon */ "images/mimetypes/audio-x-wav.png" },

    { /* szExtension */ "mpg",      /* szIcon */ "images/mimetypes/video-mpeg.png" },
    { /* szExtension */ "avi",      /* szIcon */ "images/mimetypes/video-x-msvideo.png" },

    { /* szExtension */ "gif",      /* szIcon */ "images/mimetypes/image-gif.png" },
    { /* szExtension */ "jpg",      /* szIcon */ "images/mimetypes/image-jpeg.png" },
    { /* szExtension */ "jpe",      /* szIcon */ "images/mimetypes/image-jpeg.png" },
    { /* szExtension */ "png",      /* szIcon */ "images/mimetypes/image-png.png" },
    { /* szExtension */ "svg",      /* szIcon */ "images/mimetypes/image-svg.png" },

    { /* szExtension */ "exe",      /* szIcon */ "images/mimetypes/application-x-executable.png" },
    { /* szExtension */ "com",      /* szIcon */ "images/mimetypes/application-x-executable.png" },
    { /* szExtension */ "bat",      /* szIcon */ "images/mimetypes/application-x-executable.png" },

    { /* szExtension */ "rar",      /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "zip",      /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "gz",       /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "bz2",      /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "lzma",     /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "tar",      /* szIcon */ "images/mimetypes/package-x-generic.png" },

    { /* szExtension */ "doc",      /* szIcon */ "images/mimetypes/x-office-document.png" },

    { /* szExtension */ 0,          /* szIcon */ 0 } // Terminating row
};

struct NNCMS_SELECT_ITEM file_types[] =
{
    { .name = "d", .desc = NULL, .selected = false },
    { .name = "f", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_FILE_FIELDS
{
    struct NNCMS_FIELD file_id;
    struct NNCMS_FIELD file_user_id;
    struct NNCMS_FIELD file_group;
    struct NNCMS_FIELD file_mode;
    struct NNCMS_FIELD file_parent_file_id;
    struct NNCMS_FIELD file_name;
    struct NNCMS_FIELD file_type;
    struct NNCMS_FIELD file_size;
    struct NNCMS_FIELD file_timestamp;
    struct NNCMS_FIELD file_title;
    struct NNCMS_FIELD file_description;
    struct NNCMS_FIELD file_content;
    struct NNCMS_FIELD file_upload;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD file_add;
    struct NNCMS_FIELD file_edit;
    struct NNCMS_FIELD none;
}
file_fields =
{
    { .name = "file_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "file_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "file_group", .value = NULL, .data = NULL, .type = NNCMS_FIELD_GROUP, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "file_mode", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "file_parent_file_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = numeric_validate },
    { .name = "file_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 255, .char_table = unix_file_validate },
    { .name = "file_type", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = file_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1, .char_table = alphanum_validate },
    { .name = "file_size", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "file_timestamp", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "file_title", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "file_description", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = NNCMS_UPLOAD_LEN_MAX },
    { .name = "file_content", .value = NULL, .data = & (struct NNCMS_TEXTAREA_OPTIONS) { .escape = true }, .type = NNCMS_FIELD_TEXTAREA, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = NNCMS_UPLOAD_LEN_MAX },
    { .name = "file_upload", .value = NULL, .data = NULL, .type = NNCMS_FIELD_UPLOAD, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "file_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "file_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// #############################################################################
// function declarations

char *file_links( struct NNCMS_THREAD_INFO *req, char *file_id, char *file_parent_file_id, char *file_type );
bool file_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_FILE_FIELDS *fields );

// #############################################################################
// functions

bool file_global_init( )
{
    main_local_init_add( &file_local_init );
    main_local_destroy_add( &file_local_destroy );

    main_page_add( "file_gallery", &file_gallery );
    main_page_add( "file_list", &file_list );
    main_page_add( "file_edit", &file_edit );
    main_page_add( "file_delete", &file_delete );
    main_page_add( "file_upload", &file_upload );
    main_page_add( "file_mkdir", &file_mkdir );
    main_page_add( "file_add", &file_add );
    main_page_add( "file_view", &file_view );

    // Initialize ImageMagick
#ifdef HAVE_IMAGEMAGICK
    // Initialize the MagickWand environment
    MagickWandGenesis();
#endif // HAVE_IMAGEMAGICK

    return true;
}

// #############################################################################

bool file_global_destroy( )
{
#ifdef HAVE_IMAGEMAGICK
        // Cleanup ImageMagick stuff
        MagickWandTerminus();
#endif

    return true;
}

// #############################################################################

bool file_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    req->stmt_list_folders = database_prepare( req, "SELECT id, user_id, parent_file_id, name, type, title, description, \"group\", mode FROM files WHERE type='d'" );
    req->stmt_list_files = database_prepare( req, "SELECT id, user_id, parent_file_id, name, type, title, description, \"group\", mode FROM files WHERE id=?" );
    req->stmt_count_file_childs = database_prepare( req, "SELECT COUNT(*) FROM files WHERE parent_file_id=?" );
    req->stmt_list_file_childs = database_prepare( req, "SELECT id, user_id, parent_file_id, name, type, title, description, \"group\", mode FROM files WHERE parent_file_id=? ORDER BY type ASC, name ASC" );
    req->stmt_find_file_by_name = database_prepare( req, "SELECT id, user_id, parent_file_id, name, type, title, description, \"group\", mode FROM files WHERE parent_file_id=? AND name=?" );
    req->stmt_find_file_by_id = database_prepare( req, "SELECT id, user_id, parent_file_id, name, type, title, description, \"group\", mode FROM files WHERE id=?" );
    req->stmt_add_file = database_prepare( req, "INSERT INTO files (id, user_id, parent_file_id, name, type, title, description, \"group\", mode) VALUES(null, ?, ?, ?, ?, ?, ?, ?, ?)" );
    req->stmt_edit_file = database_prepare( req, "UPDATE files SET user_id=?, parent_file_id=?, name=?, type=?, title=?, description=?, \"group\"=?, mode=? WHERE id=?" );
    req->stmt_delete_file = database_prepare( req, "DELETE FROM files WHERE id=?" );

    return true;
}

// #############################################################################

bool file_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_list_folders );
    database_finalize( req, req->stmt_list_files );
    database_finalize( req, req->stmt_count_file_childs );
    database_finalize( req, req->stmt_list_file_childs );
    database_finalize( req, req->stmt_find_file_by_name );
    database_finalize( req, req->stmt_find_file_by_id );
    database_finalize( req, req->stmt_add_file );
    database_finalize( req, req->stmt_edit_file );
    database_finalize( req, req->stmt_delete_file );

    return true;
}

// #############################################################################

void file_upload( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "file_add_header", NULL );

    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "add" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *file_id = main_variable_get( req, req->get_tree, "file_id" );
    if( file_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, file_id, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    //
    // Form
    //
    struct NNCMS_FILE_FIELDS *fields = memdup_temp( req, &file_fields, sizeof(file_fields) );
    fields->file_id.viewable = false;
    fields->file_user_id.value = req->user_id;
    fields->file_parent_file_id.value = (file_row != NULL ? file_row->id : NULL);
    fields->file_size.viewable = false;
    fields->file_timestamp.viewable = false;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->file_add.viewable = true;
    
    fields->file_name.viewable = false;
    fields->file_name.editable = false;
    fields->file_type.viewable = false;
    fields->file_content.viewable = false;
    fields->file_upload.viewable = true;

    if( acl_check_perm( req, "file", NULL, "chmod" ) == false )
    {
        fields->file_mode.editable = false;
    }
    
    if( acl_check_perm( req, "file", NULL, "chown" ) == false )
    {
        fields->file_user_id.editable = false;
        fields->file_group.editable = false;
    }

    // Form
    struct NNCMS_FORM form =
    {
        .name = "file_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *file_add = main_variable_get( req, req->post_tree, "file_add" );
    if( file_add != NULL )
    //if( fields->file_add.value != NULL )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Retrieve GET data
        form_post_data( req, (struct NNCMS_FIELD *) fields );

        // Validation
        bool file_valid = file_validate( req, fields );
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( file_valid == true && field_valid == true )
        {
            struct NNCMS_FILE *file = NULL;
            if( req->files != NULL && req->files->len != 0 )
            {
                // From uploaded file
                file = & ((struct NNCMS_FILE *) req->files->data) [0];
                if( file->var_name != NULL && file->file_name != NULL && file->data != NULL )
                {
                    // Convert id from DB to path in FS
                    GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );
                    g_string_prepend( file_path, fileDir );

                    // Create the file
                    g_string_append( file_path, file->file_name );
                    
                    bool result = main_store_file( file_path->str, file->data, &file->size );
                    if( result == false )
                    {
                        main_vmessage( req, "file_add_fail" );
                        log_printf( req, LOG_ERROR, "Failed to open file '%s' for writing the contents of uploaded file", file_path->str );
                        return;
                    }

                    //log_printf( req, LOG_DEBUG, "name: %s, file: %s, data: %s", file->var_name, file->file_name, file->data );
                }
                else
                {
                    main_vmessage( req, "file_add_fail" );
                    log_print( req, LOG_ERROR, "Received file with missing parameters" );
                    return;
                }
            }
            else
            {
                main_vmessage( req, "file_add_fail" );
                log_print( req, LOG_ERROR, "Form submitted without file" );
                return;
            }

            // Query Database
            database_bind_text( req->stmt_add_file, 1, fields->file_user_id.value );
            database_bind_text( req->stmt_add_file, 2, fields->file_parent_file_id.value );
            database_bind_text( req->stmt_add_file, 3, file->file_name );
            database_bind_text( req->stmt_add_file, 4, "f" );
            database_bind_text( req->stmt_add_file, 5, fields->file_title.value );
            database_bind_text( req->stmt_add_file, 6, fields->file_description.value );
            database_bind_text( req->stmt_add_file, 7, fields->file_group.value );
            database_bind_text( req->stmt_add_file, 8, fields->file_mode.value );
            database_steps( req, req->stmt_add_file );
            unsigned int file_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "file_id", .value.unsigned_integer = file_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "file_name", .value.string = file->file_name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "file_add_success", vars );
            log_printf( req, LOG_ACTION, "File '%s' was uploaded (id = %i)", file->file_name, file_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Get the list of folders
    fields->file_parent_file_id.data = file_folder_list( req, file_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/document-new.png", .type = NNCMS_TYPE_STRING },
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

void file_mkdir( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "file_add_header", NULL );

    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "add" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *file_id = main_variable_get( req, req->get_tree, "file_id" );
    if( file_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, file_id, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    //
    // Form
    //
    struct NNCMS_FILE_FIELDS *fields = memdup_temp( req, &file_fields, sizeof(file_fields) );
    fields->file_id.viewable = false;
    fields->file_user_id.value = req->user_id;
    fields->file_parent_file_id.value = (file_row != NULL ? file_row->id : NULL);
    fields->file_size.viewable = false;
    fields->file_timestamp.viewable = false;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->file_add.viewable = true;

    fields->file_name.viewable = true;
    fields->file_name.editable = true;
    fields->file_type.viewable = false;
    fields->file_content.viewable = false;
    fields->file_upload.viewable = false;

    if( acl_check_perm( req, "file", NULL, "chmod" ) == false )
    {
        fields->file_mode.editable = false;
    }
    
    if( acl_check_perm( req, "file", NULL, "chown" ) == false )
    {
        fields->file_user_id.editable = false;
        fields->file_group.editable = false;
    }

    // Form
    struct NNCMS_FORM form =
    {
        .name = "file_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *file_add = main_variable_get( req, req->post_tree, "file_add" );
    if( file_add != NULL )
    //if( fields->file_add.value != NULL )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Retrieve GET data
        form_post_data( req, (struct NNCMS_FIELD *) fields );

        // Validation
        bool file_valid = file_validate( req, fields );
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( file_valid == true && field_valid == true )
        {
            // Convert id from DB to path in FS
            GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );
            g_string_prepend( file_path, fileDir );
            g_string_append( file_path, fields->file_name.value );

            //          read   write execute
            // owner      x      x      x  
            // group      x             x  
            // other      x             x  
            int result = mkdir( file_path->str, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
            if( result != 0 )
            {
                // Error
                char szErrorBuf[100];
                strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                log_printf( req, LOG_ALERT, "Error creating \"%s\" directory (error: %s)", file_path->str, &szErrorBuf );

                // Cannot create directory
                main_vmessage( req, "file_add_mkdir_fail" );
                return;
            }

            // Query Database
            database_bind_text( req->stmt_add_file, 1, fields->file_user_id.value );
            database_bind_text( req->stmt_add_file, 2, fields->file_parent_file_id.value );
            database_bind_text( req->stmt_add_file, 3, fields->file_name.value );
            database_bind_text( req->stmt_add_file, 4, "d" );
            database_bind_text( req->stmt_add_file, 5, fields->file_title.value );
            database_bind_text( req->stmt_add_file, 6, fields->file_description.value );
            database_bind_text( req->stmt_add_file, 7, fields->file_group.value );
            database_bind_text( req->stmt_add_file, 8, fields->file_mode.value );
            database_steps( req, req->stmt_add_file );
            unsigned int file_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "file_id", .value.unsigned_integer = file_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "file_name", .value.string = fields->file_name.value, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "file_add_success", vars );
            log_printf( req, LOG_ACTION, "File '%s' was uploaded (id = %i)", fields->file_name.value, file_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Get the list of folders
    fields->file_parent_file_id.data = file_folder_list( req, file_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/document-new.png", .type = NNCMS_TYPE_STRING },
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

void file_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "file_add_header", NULL );

    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "add" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *file_id = main_variable_get( req, req->get_tree, "file_id" );
    if( file_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, file_id, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    //
    // Form
    //
    struct NNCMS_FILE_FIELDS *fields = memdup_temp( req, &file_fields, sizeof(file_fields) );
    fields->file_id.viewable = false;
    fields->file_user_id.value = req->user_id;
    fields->file_parent_file_id.value = (file_row != NULL ? file_row->id : NULL);
    fields->file_size.viewable = false;
    fields->file_timestamp.viewable = false;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->file_add.viewable = true;

    fields->file_name.viewable = true;
    fields->file_name.editable = true;
    fields->file_type.viewable = false;
    fields->file_content.viewable = true;
    fields->file_upload.viewable = false;

    if( acl_check_perm( req, "file", NULL, "chmod" ) == false )
    {
        fields->file_mode.editable = false;
    }
    
    if( acl_check_perm( req, "file", NULL, "chown" ) == false )
    {
        fields->file_user_id.editable = false;
        fields->file_group.editable = false;
    }

    // Form
    struct NNCMS_FORM form =
    {
        .name = "file_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *file_add = main_variable_get( req, req->post_tree, "file_add" );
    if( file_add != NULL )
    //if( fields->file_add.value != NULL )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        // Retrieve GET data
        form_post_data( req, (struct NNCMS_FIELD *) fields );

        // Validation
        bool file_valid = file_validate( req, fields );
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( file_valid == true && field_valid == true )
        {
            // Convert id from DB to path in FS
            GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );
            g_string_prepend( file_path, fileDir );

            // Create the file
            g_string_append( file_path, fields->file_name.value );

            if( fields->file_content.value != NULL && fields->file_content.editable == true )
            {
                bool result = main_store_file( file_path->str, fields->file_content.value, NULL );
                if( result == false )
                {
                    main_vmessage( req, "file_add_fail" );
                    log_printf( req, LOG_ERROR, "Failed to open file '%s' for writing the contents of textarea", file_path->str );
                    return;
                }
            }

            // Query Database
            database_bind_text( req->stmt_add_file, 1, fields->file_user_id.value );
            database_bind_text( req->stmt_add_file, 2, fields->file_parent_file_id.value );
            database_bind_text( req->stmt_add_file, 3, fields->file_name.value );
            database_bind_text( req->stmt_add_file, 4, "f" );
            database_bind_text( req->stmt_add_file, 5, fields->file_title.value );
            database_bind_text( req->stmt_add_file, 6, fields->file_description.value );
            database_bind_text( req->stmt_add_file, 7, fields->file_group.value );
            database_bind_text( req->stmt_add_file, 8, fields->file_mode.value );
            database_steps( req, req->stmt_add_file );
            unsigned int file_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "file_id", .value.unsigned_integer = file_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "file_name", .value.string = fields->file_name.value, .type = TEMPLATE_TYPE_UNSAFE_STRING },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "file_add_success", vars );
            log_printf( req, LOG_ACTION, "File '%s' was uploaded (id = %i)", fields->file_name.value, file_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Get the list of folders
    fields->file_parent_file_id.data = file_folder_list( req, file_row->id );

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/actions/document-new.png", .type = NNCMS_TYPE_STRING },
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

// Update folder contents FS -> DB
bool file_update_folder( struct NNCMS_THREAD_INFO *req, char *file_id )
{
    // Convert id from DB to path in FS
    GString *file_path = file_dbfs_path( req, atoi( file_id ) );
    g_string_prepend( file_path, fileDir );

    // Opens a directory for reading. The names of the files in the directory
    // can then be retrieved using g_dir_read_name(). The ordering is not
    // defined. 
    GError *err = NULL;
    GDir *dir = g_dir_open( file_path->str, 0, &err);
    if( err != NULL )
    {
        log_printf( req, LOG_ERROR, "Unable to open directory: %s", err->message );
        return false;
    }
    
    // Retrieve the name of another entry in the directory, or NULL. The order
    // of entries returned from this function is not defined, and may vary by
    // file system or other operating-system dependent factors.
    GString *full_path = g_string_sized_new( 10 );
    while( 1 )
    {
        char *file_name = (char *) g_dir_read_name( dir );
        if( file_name == NULL )
            break;
        
        // Make full path to the file
        g_string_assign( full_path, file_path->str );
        g_string_append( full_path, file_name );
        
        // Check if file is already in database
        database_bind_text( req->stmt_find_file_by_name, 1, file_id ); /* parent_file_id */
        database_bind_text( req->stmt_find_file_by_name, 2, file_name );
        struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_name );
        if( file_row != NULL )
        {
            database_free_rows( (struct NNCMS_ROW *) file_row );
            continue;
        }
        
        // Obtain information about the named file
        GStatBuf buf;
        int result = g_stat( full_path->str, &buf);
        if( result == -1 )
            continue;
        bool is_dir = S_ISDIR( buf.st_mode );
        bool is_file = S_ISREG( buf.st_mode );
        if( (is_file == false && is_dir == false) || (is_file == true && is_dir == true) )
            continue;
        
        // Add to database
        database_bind_text( req->stmt_add_file, 1, "2" ); /* user_id, Administrator */
        database_bind_text( req->stmt_add_file, 2, file_id ); /* parent_file_id */
        database_bind_text( req->stmt_add_file, 3, file_name );
        if( is_file == true )
            database_bind_text( req->stmt_add_file, 4, "f" );
        else if( is_dir == true )
            database_bind_text( req->stmt_add_file, 4, "d" );
        database_bind_text( req->stmt_add_file, 5, file_name ); /* title */
        database_bind_text( req->stmt_add_file, 6, "" ); /* description */
        database_bind_text( req->stmt_add_file, 7, "" ); /* group */
        database_bind_text( req->stmt_add_file, 8, "" ); /* mode */
        database_steps( req, req->stmt_add_file );
        unsigned int file_id = database_last_rowid( req );

        // Report
        /*struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_id", .value.unsigned_integer = file_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
            { .name = "file_name", .value.string = file_name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        log_vdisplayf( req, LOG_ACTION, "file_add_success", vars );*/
        log_printf( req, LOG_ACTION, "File '%s' was added by FTP administrator (id = %i)", file_name, file_id );
    }

    // Closes the directory and deallocates all related resources. 
    g_dir_close( dir );
    g_string_free( full_path, TRUE );
}

// #############################################################################

void file_gallery( struct NNCMS_THREAD_INFO *req )
{
    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "gallery" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get folder id
    char *file_id = main_variable_get( req, req->get_tree, "file_id" );
    if( file_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, file_id, true, false, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find rows
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    // Convert id from DB to path in FS
    GString *file_path = file_dbfs_path( req, atoi( file_id ) );

    file_update_folder( req, file_id );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_path", .value.string = file_path->str, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "file_gallery_header", vars );

    // Find files associated with selected file
    database_bind_text( req->stmt_count_file_childs, 1, file_id );
    struct NNCMS_ROW *file_count = database_steps( req, req->stmt_count_file_childs );
    if( file_count != 0 )
    {
        // Folder may be empty
        garbage_add( req->loop_garbage, file_count, MEMORY_GARBAGE_DB_FREE );
    }
    database_bind_text( req->stmt_list_file_childs, 1, file_id );
    struct NNCMS_FILE_ROW *child_file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_list_file_childs );
    if( child_file_row != 0 )
    {
        // Folder may be empty
        garbage_add( req->loop_garbage, child_file_row, MEMORY_GARBAGE_DB_FREE );
    }

    GString *html = g_string_sized_new( 100 );
    garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GSTRING_FREE );

    // Fetch table data
    GString *child_file_path = g_string_sized_new( NNCMS_PATH_LEN_MAX );
    garbage_add( req->loop_garbage, child_file_path, MEMORY_GARBAGE_GSTRING_FREE );
    for( unsigned int i = 0; child_file_row != NULL && child_file_row[i].id != NULL; i = i + 1 )
    {
        if( child_file_row[i].type[0] == 'f' )
        {
            if( is_admin == false && file_check_perm( req, NULL, child_file_row[i].id, true, false, false ) == false )
                continue;
        }
        else
        {
            if( is_admin == false && file_check_perm( req, NULL, child_file_row[i].id, false, false, true ) == false )
                continue;
        }

        //
        // Find real file in FS
        //

        // Catcenate path and file name
        g_string_assign( child_file_path, fileDir );
        g_string_append( child_file_path, file_path->str );
        g_string_append( child_file_path, child_file_row[i].name );
        struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, child_file_path->str );

        // This normally shouldn't happen
        if( file_info->is_file == true && child_file_row[i].type[0] == 'd' )
        {
            log_printf( req, LOG_ERROR, "Conflict. Database folder (id = %s) is actually a file.", child_file_row[i].id );
            file_info->icon = "images/status/dialog-error.png";
        }
        else if( file_info->is_dir == true && child_file_row[i].type[0] == 'f' )
        {
            log_printf( req, LOG_ERROR, "Conflict. Database file (id = %s) is actually a folder.", child_file_row[i].id );
            file_info->icon = "images/status/dialog-error.png";
        }

        // Make trimmed file_name
        GString *file_name_trim = g_string_sized_new( 30 );
        garbage_add( req->loop_garbage, file_name_trim, MEMORY_GARBAGE_GSTRING_FREE );
        size_t len = g_utf8_strlen( child_file_row[i].name, -1 );
        size_t limit = 40;
        if( len > limit )
        {
            size_t str1_len = g_utf8_offset_to_pointer( child_file_row[i].name, limit / 2 ) - child_file_row[i].name;
            size_t str2_len = g_utf8_offset_to_pointer( child_file_row[i].name, len - limit / 2 ) - child_file_row[i].name;
            g_string_append_len( file_name_trim, child_file_row[i].name, str1_len );
            g_string_append( file_name_trim, "<wbr>…<wbr>" );
            //g_string_append( file_name_trim, "…" );
            g_string_append( file_name_trim, & (child_file_row[i].name[str2_len]) );
        }
        else
        {
            g_string_assign( file_name_trim, child_file_row[i].name );
        }

        // Make thumbnails for images
        char *thumbnail = "";
        if( strncmp( file_info->mime_type, "image", 5 ) == 0 )
        {
            thumbnail = file_thumbnail( req, child_file_row[i].id, 200, 200 );
        }

        struct NNCMS_VARIABLE file_template[] =
        {
            { .name = "file_id", .value.string = child_file_row[i].id, .type = NNCMS_TYPE_STRING },
            { .name = "file_user_id", .value.string = child_file_row[i].user_id, .type = NNCMS_TYPE_STRING },
            { .name = "file_parent_file_id", .value.string = child_file_row[i].parent_file_id, .type = NNCMS_TYPE_STRING },
            { .name = "file_path", .value.string = file_path->str, .type = NNCMS_TYPE_STRING },
            { .name = "file_name", .value.string = child_file_row[i].name, .type = NNCMS_TYPE_STRING },
            { .name = "file_name_trimmed", .value.string = file_name_trim->str, .type = NNCMS_TYPE_STRING },
            { .name = "file_title", .value.string = child_file_row[i].title, .type = NNCMS_TYPE_STRING },
            { .name = "file_description", .value.string = child_file_row[i].description, .type = NNCMS_TYPE_STRING },
            { .name = "file_icon", .value.string = file_info->icon, .type = TEMPLATE_TYPE_ICON },
            { .name = "file_size", .value.string = file_info->size, .type = NNCMS_TYPE_STRING },
            { .name = "file_timestamp", .value.time = file_info->timestamp, .type = NNCMS_TYPE_TIMESTAMP },
            { .name = "file_type", .value.string = child_file_row[i].type, .type = NNCMS_TYPE_STRING },
            { .name = "file_thumbnail", .value.string = thumbnail, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

        template_hparse( req, "gallery_file.html", html, file_template );
    }

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html->str, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/categories/applications-internet.png", .type = NNCMS_TYPE_STRING },
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

void file_list( struct NNCMS_THREAD_INFO *req )
{
    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get folder id
    char *file_id = main_variable_get( req, req->get_tree, "file_id" );
    if( file_id == NULL )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find rows
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    if( is_admin == false && file_check_perm( req, NULL, file_id, true, false, ( file_row->type[0] == 'd' ) ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Convert id from DB to path in FS
    GString *file_path = file_dbfs_path( req, atoi( file_id ) );

    file_update_folder( req, file_id );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_path", .value.string = file_path->str, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "file_list_header", vars );

    // Find files associated with selected file
    database_bind_text( req->stmt_count_file_childs, 1, file_id );
    struct NNCMS_ROW *file_count = database_steps( req, req->stmt_count_file_childs );
    if( file_count != 0 )
    {
        // Folder may be empty
        garbage_add( req->loop_garbage, file_count, MEMORY_GARBAGE_DB_FREE );
    }
    database_bind_text( req->stmt_list_file_childs, 1, file_id );
    struct NNCMS_FILE_ROW *child_file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_list_file_childs );
    if( child_file_row != 0 )
    {
        // Folder may be empty
        garbage_add( req->loop_garbage, child_file_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        //{ .value = i18n_string_temp( req, "file_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        //{ .value = i18n_string_temp( req, "file_user_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = (struct NNCMS_VARIABLE []) { { .name = "width", .value.string = "20", .type = NNCMS_TYPE_STRING }, { .type = NNCMS_TYPE_NONE } } },
        { .value = i18n_string_temp( req, "file_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        //{ .value = i18n_string_temp( req, "file_type_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "file_size_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "file_timestamp_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        //{ .value = i18n_string_temp( req, "file_title_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        //{ .value = i18n_string_temp( req, "file_description_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );

    // Fetch table data
    GString *child_file_path = g_string_sized_new( NNCMS_PATH_LEN_MAX );
    garbage_add( req->loop_garbage, child_file_path, MEMORY_GARBAGE_GSTRING_FREE );
    for( unsigned int i = 0; child_file_row != NULL && child_file_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_id", .value.string = child_file_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        if( child_file_row[i].type[0] == 'f' )
        {
            if( is_admin == false && file_check_perm( req, NULL, child_file_row[i].id, true, false, false ) == false )
                continue;
        }
        else
        {
            if( is_admin == false && file_check_perm( req, NULL, child_file_row[i].id, false, false, true ) == false )
                continue;
        }
        

        char *list = main_temp_link( req, "file_list", i18n_string_temp( req, "list", NULL ), vars );
        char *view = main_temp_link( req, "file_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "file_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "file_delete", i18n_string_temp( req, "delete", NULL ), vars );

        //
        // Find real file in FS
        //

        // Catcenate path and file name
        g_string_assign( child_file_path, fileDir );
        g_string_append( child_file_path, file_path->str );
        g_string_append( child_file_path, child_file_row[i].name );
        struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, child_file_path->str );

        // This normally shouldn't happen
        if( file_info->is_file == true && child_file_row[i].type[0] == 'd' )
        {
            log_printf( req, LOG_ERROR, "Conflict. Database folder (id = %s) is actually a file.", child_file_row[i].id );
            file_info->icon = "images/status/dialog-error.png";
        }
        else if( file_info->is_dir == true && child_file_row[i].type[0] == 'f' )
        {
            log_printf( req, LOG_ERROR, "Conflict. Database file (id = %s) is actually a folder.", child_file_row[i].id );
            file_info->icon = "images/status/dialog-error.png";
        }

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            //{ .value.string = child_file_id, .type = NNCMS_TYPE_STRING, .options = NULL },
            //{ .value.string = child_file_user_id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = file_info->icon, .type = TEMPLATE_TYPE_ICON, .options = NULL },
            { .value.string = child_file_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            //{ .value.string = child_file_type, .type = NNCMS_TYPE_STRING, .options = NULL },
            //{ .value.string = file_icon, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = file_info->size, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.time = file_info->timestamp, .type = NNCMS_TYPE_TIMESTAMP, .options = NULL },
            //{ .value.string = child_file_title, .type = NNCMS_TYPE_STRING, .options = NULL },
            //{ .value.string = child_file_description, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = ( child_file_row[i].type[0] == 'd' ? list : view ), .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .row_count = ( file_count ? atoi(file_count->value[0]) : 0 ),
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Html output
    char *html = table_html( req, &table );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/categories/applications-internet.png", .type = NNCMS_TYPE_STRING },
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

void file_delete( struct NNCMS_THREAD_INFO *req )
{
    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "delete" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get log entry id
    char *httpVarId = main_variable_get( req, req->get_tree, "file_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, httpVarId, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row associated with our object by 'name'
    database_bind_text( req->stmt_find_file_by_id, 1, httpVarId );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    if( file_check_perm( req, NULL, file_row->parent_file_id, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page title
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "file_id", .value.string = file_row->id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "file_user_id", .value.string = file_row->user_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "file_parent_file_id", .value.string = file_row->parent_file_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "file_name", .value.string = file_row->name, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "file_type", .value.string = file_row->type, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "file_title", .value.string = file_row->title, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .name = "file_description", .value.string = file_row->description, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "file_delete_header", vars );

    //
    // Check if file commit changes
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "delete_submit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Convert id from DB to path in FS
        GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );
        g_string_prepend( file_path, fileDir );
        struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, file_path->str );
        
        // Delete file in FS
        if( file_info->exists == true )
        {
            bool result = main_remove( req, file_path->str );
            if( result == false )
            {
                main_vmessage( req, "file_delete_fail" );
                log_printf( req, LOG_ERROR, "Unable to delete file '%s'", file_path->str );
                return;
            }
        }

        if( file_row->type[0] == 'd' )
        {
            // Recursive delete in db for folders
            GPtrArray *gstack = g_ptr_array_new( );
            g_ptr_array_add( gstack, file_row );

            // Stack will increase as new directories found
            for( unsigned int i = 0; i < gstack->len; i = i + 1 )
            {
                struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) (gstack->pdata[i]);
                
                database_bind_text( req->stmt_list_file_childs, 1, file_row->id );
                struct NNCMS_FILE_ROW *child_file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_list_file_childs );
                if( child_file_row == NULL )
                    continue;
                garbage_add( req->loop_garbage, child_file_row, MEMORY_GARBAGE_DB_FREE );
                for( unsigned int j = 0; child_file_row[j].id; j = j + 1 )
                {
                    if( child_file_row[j].type[0] == 'f' )
                    {
                        // Delete files
                        database_bind_text( req->stmt_delete_file, 1, child_file_row[j].id );
                        database_steps( req, req->stmt_delete_file );
                        //log_printf( req, LOG_DEBUG, "del %s", child_file_row[j].id );
                    }
                    else if( child_file_row[j].type[0] == 'd' )
                    {
                        // Store directories for later deletion
                        //log_printf( req, LOG_DEBUG, "recurse %s", child_file_row[j].id );
                        g_ptr_array_add( gstack, &child_file_row[j] );
                    }
                }
            }
            
            // Traverse stack array backwards and remove directories
            for( unsigned int i = gstack->len; i != 0; i = i - 1 )
            {
                struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) (gstack->pdata[i - 1]);
                
                database_bind_text( req->stmt_delete_file, 1, file_row->id );
                database_steps( req, req->stmt_delete_file );
                //log_printf( req, LOG_DEBUG, "del %s", file_row->id );
            }

            g_ptr_array_free( gstack, TRUE );
        }
        else
        {
            // Query database
            database_bind_text( req->stmt_delete_file, 1, file_row->id );
            database_steps( req, req->stmt_delete_file );
        }

        // Log action
        log_vdisplayf( req, LOG_ACTION, "file_delete_success", vars );
        log_printf( req, LOG_ACTION, "File '%s' was deleted", file_row->name );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, file_row->id );

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

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

void file_view( struct NNCMS_THREAD_INFO *req )
{
    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "view" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "file_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, httpVarId, true, false, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_file_by_id, 1, httpVarId );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_name", .value.string = file_row->name, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "file_view_header", vars );

    // Convert id from DB to path in FS
    GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );
    g_string_prepend( file_path, fileDir );
    struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, file_path->str );

    // Convert integer to string
    char *file_timestamp = main_type( req, (union NNCMS_VARIABLE_UNION) { .integer = file_info->timestamp }, NNCMS_TYPE_INTEGER );
    garbage_add( req->loop_garbage, file_timestamp, MEMORY_GARBAGE_GFREE );

    //
    // Form
    //
    struct NNCMS_FILE_FIELDS *fields = memdup_temp( req, &file_fields, sizeof(file_fields) );
    field_set_editable( (struct NNCMS_FIELD *) fields, false );
    fields->file_id.value = file_row->id;
    fields->file_user_id.value = file_row->user_id;
    fields->file_parent_file_id.value = file_row->parent_file_id;
    fields->file_name.value = file_row->name;
    fields->file_type.editable = false;
    fields->file_type.value = file_row->type;
    fields->file_size.value = file_info->size;
    fields->file_timestamp.value = file_timestamp;
    fields->file_title.value = file_row->title;
    fields->file_description.value = file_row->description;
    fields->file_group.value = file_row->group;
    fields->file_mode.value = file_row->mode;
    fields->file_upload.viewable = false;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->file_edit.viewable = false;

    // Check if file can be displayed
    if( file_info->exists == true )
    {
        if( file_info->binary == false )
        {
            if( file_info->fstat.st_size > NNCMS_UPLOAD_LEN_MAX )
            {
                fields->file_content.value = "[too huge]";
                fields->file_content.editable = false;
            }
        }
        else
        {
            fields->file_content.value = "[binary]";
            fields->file_content.editable = false;
        }
    }
    else
    {
            fields->file_content.value = "[not found]";
            fields->file_content.editable = false;
    }

    // Form
    struct NNCMS_FORM form =
    {
        .name = "file_view", .action = NULL, .method = NULL,
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Try to open file and read the contents
    if( fields->file_content.value == NULL )
    {
        size_t file_size = NNCMS_UPLOAD_LEN_MAX;
        char *file_content = main_get_file( file_path->str, &file_size );
        if( file_content != NULL )
        {
            garbage_add( req->loop_garbage, file_content, MEMORY_GARBAGE_FREE );
            fields->file_content.value = file_content;
        }
    }

    // Html output
    char *html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = html, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/mimetypes/application-octet-stream.png", .type = NNCMS_TYPE_STRING },
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

void file_edit( struct NNCMS_THREAD_INFO *req )
{
    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Permission check
    if( is_admin == false && acl_check_perm( req, "file", NULL, "edit" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get Id
    char *file_id = main_variable_get( req, req->get_tree, "file_id" );
    if( file_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    if( is_admin == false && file_check_perm( req, NULL, file_id, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );

    if( file_check_perm( req, NULL, file_row->parent_file_id, false, true, false ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page title
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_name", .value.string = file_row->name, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "file_edit_header", vars );

    // Convert id from DB to path in FS
    GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );
    g_string_prepend( file_path, fileDir );
    struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, file_path->str );

    // Convert integer to string
    char *file_timestamp = main_type( req, (union NNCMS_VARIABLE_UNION) { .integer = file_info->timestamp }, NNCMS_TYPE_INTEGER );
    garbage_add( req->loop_garbage, file_timestamp, MEMORY_GARBAGE_GFREE );

    //
    // Form
    //
    struct NNCMS_FILE_FIELDS *fields = memdup_temp( req, &file_fields, sizeof(file_fields) );
    fields->file_id.value = file_row->id;
    fields->file_user_id.value = file_row->user_id;
    fields->file_parent_file_id.value = file_row->parent_file_id;
    fields->file_name.value = file_row->name;
    fields->file_type.editable = false;
    fields->file_type.value = file_row->type;
    fields->file_size.value = file_info->size;
    fields->file_timestamp.value = file_timestamp;
    fields->file_title.value = file_row->title;
    fields->file_description.value = file_row->description;
    fields->file_group.value = file_row->group;
    fields->file_mode.value = file_row->mode;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->file_edit.viewable = true;

    fields->file_name.viewable = true;
    fields->file_name.editable = true;
    fields->file_type.viewable = false;
    fields->file_content.viewable = true;
    fields->file_upload.viewable = false;

    if( acl_check_perm( req, "file", NULL, "chmod" ) == false )
    {
        fields->file_mode.editable = false;
    }
    
    if( acl_check_perm( req, "file", NULL, "chown" ) == false )
    {
        fields->file_user_id.editable = false;
        fields->file_group.editable = false;
    }

    // Check if file can be displayed
    if( file_info->exists == true )
    {
        if( file_info->binary == false )
        {
            if( file_info->fstat.st_size > NNCMS_UPLOAD_LEN_MAX )
            {
                fields->file_content.value = "[too huge]";
                fields->file_content.editable = false;
            }
        }
        else
        {
            fields->file_content.value = "[binary]";
            fields->file_content.editable = false;
        }
    }
    else
    {
            fields->file_content.value = "[not found]";
            fields->file_content.editable = false;
    }

    //
    // Check if user commit changes
    //
    char *file_edit = main_variable_get( req, req->post_tree, "file_edit" );
    if( file_edit != 0 )
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
        bool file_valid = file_validate( req, fields );
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) fields );
        if( file_valid == true && field_valid == true )
        {
            // Write data
            /*if( req->files != NULL && req->files->len != 0 )
            {
                // From uploaded file
                if( file_info->is_file == true )
                {
                    struct NNCMS_FILE *file = & ((struct NNCMS_FILE *) req->files->data) [0];
                    if( file->var_name != NULL && file->file_name != NULL && file->data != NULL )
                    {
                        bool result = main_store_file( file_path->str, file->data, &file->size );
                        if( result == false )
                        {
                            main_vmessage( req, "file_edit_fail" );
                            log_printf( req, LOG_ERROR, "Failed to open file '%s' for writing the contents of uploaded file", file_path->str );
                            return;
                        }

                        //log_printf( req, LOG_DEBUG, "name: %s, file: %s, data: %s", file->var_name, file->file_name, file->data );
                    }
                    else
                    {
                            main_vmessage( req, "file_edit_fail" );
                            log_print( req, LOG_ERROR, "Received file with missing parameters" );
                            return;
                    }
                }
                else
                {
                    // Cannot edit content of a directory
                    main_vmessage( req, "file_edit_fail" );
                    return;
                }
            }
            else */ if( fields->file_content.value != NULL && fields->file_content.editable == true )
            {
                // From text area
                if( file_info->is_file == true )
                {
                    bool result = main_store_file( file_path->str, fields->file_content.value, NULL );
                    if( result == false )
                    {
                        main_vmessage( req, "file_edit_fail" );
                        log_printf( req, LOG_ERROR, "Failed to open file '%s' for writing the contents of textarea", file_path->str );
                        return;
                    }
                }
                else
                {
                    // Cannot edit content of a directory
                    main_vmessage( req, "file_edit_fail" );
                    log_printf( req, LOG_ALERT, "Attempt to write file_contents to a directory '%s'", file_path->str );
                    return;
                }
            }
            
            // Rename/move file
            if( strcmp( file_row->name, fields->file_name.value ) != 0 ||
                strcmp( file_row->parent_file_id, fields->file_parent_file_id.value ) != 0 )
            {
                // Path to new file name
                GString *file_path_new = file_dbfs_path( req, atoi( fields->file_parent_file_id.value ) );
                g_string_prepend( file_path_new, fileDir );
                g_string_append( file_path_new, fields->file_name.value );
                
                int result = rename( file_path->str, file_path_new->str );
                if( result != 0 )
                {
                    // Error
                    char szErrorBuf[100];
                    strerror_r( errno, szErrorBuf, sizeof(szErrorBuf) );
                    log_printf( req, LOG_ALERT, "Error renaming '%s' file to '%s' (%s)", file_path->str, file_path_new->str, &szErrorBuf );

                    // Cannot edit content of a directory
                    main_vmessage( req, "file_rename_fail" );
                    return;
                }
                else
                {
                    log_printf( req, LOG_ACTION, "File '%s' was renamed to '%s'", file_path->str, file_path_new->str );
                }
            }

            // Query Database
            database_bind_text( req->stmt_edit_file, 1, fields->file_user_id.value );
            database_bind_text( req->stmt_edit_file, 2, fields->file_parent_file_id.value );
            database_bind_text( req->stmt_edit_file, 3, fields->file_name.value );
            database_bind_text( req->stmt_edit_file, 4, fields->file_type.value );
            database_bind_text( req->stmt_edit_file, 5, fields->file_title.value );
            database_bind_text( req->stmt_edit_file, 6, fields->file_description.value );
            database_bind_text( req->stmt_edit_file, 7, fields->file_group.value );
            database_bind_text( req->stmt_edit_file, 8, fields->file_mode.value );
            database_bind_text( req->stmt_edit_file, 9, file_id );
            database_steps( req, req->stmt_edit_file );

            log_vdisplayf( req, LOG_ACTION, "file_edit_success", vars );
            log_printf( req, LOG_ACTION, "File '%s' was editted", file_path->str );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = file_links( req, file_row->id, file_row->parent_file_id, file_row->type );

    // Get the list of folders
    fields->file_parent_file_id.data = file_folder_list( req, file_row->id );

    // Open file and read the contents
    if( fields->file_content.editable == true )
    {
        size_t file_size = NNCMS_UPLOAD_LEN_MAX;
        char *file_content = main_get_file( file_path->str, &file_size );
        if( file_content != NULL )
        {
            garbage_add( req->loop_garbage, file_content, MEMORY_GARBAGE_FREE );
            fields->file_content.value = file_content;
        }
    }

    // Form
    struct NNCMS_FORM form =
    {
        .name = "file_edit", .action = NULL, .method = "POST",
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
            { .name = "icon", .value.string = "images/stock/stock_edit.png", .type = NNCMS_TYPE_STRING },
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

void file_get_access( struct NNCMS_THREAD_INFO *req, char *user_id, char *file_id, bool *read_access_out, bool *write_access_out, bool *exec_access_out )
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

    struct NNCMS_FILE_ROW *file_row = NULL;
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    while( 1 )
    {
        // Find rows
        file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
        if( file_row == NULL )
        {
            goto file_no_access;
        }
        
        if( file_row->mode[0] != 0 )
            break;
        
        // Inherit from parent
        database_bind_int( req->stmt_find_file_by_id, 1, atoi( file_row->parent_file_id ) );
        database_free_rows( (struct NNCMS_ROW *) file_row );
    }
    
    // Convert string mode to integer
    unsigned int file_mode = (unsigned int) g_ascii_strtoll( file_row->mode, NULL, 16 );
    
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
    
    read_access |= file_mode & NNCMS_IROTH;
    write_access |= file_mode & NNCMS_IWOTH;
    exec_access |= file_mode & NNCMS_IXOTH;
    
    for( int i = 0; user_group_array != NULL && user_group_array[i] != NULL; i = i + 1 )
    {
        // Check group access
        if( strcmp( file_row->group, user_group_array[i] ) == 0 )
        {
            read_access |= file_mode & NNCMS_IRGRP;
            write_access |= file_mode & NNCMS_IWGRP;
            exec_access |= file_mode & NNCMS_IXGRP;
        }
    }
    
    // Check if user is owner of this file
    if( strcmp( file_row->user_id, user_id ) == 0 )
    {
        read_access |= file_mode & NNCMS_IRUSR;
        write_access |= file_mode & NNCMS_IWUSR;
        exec_access |= file_mode & NNCMS_IXUSR;
    }

    // Not needed now
    database_free_rows( (struct NNCMS_ROW *) file_row );

file_no_access:
    if( group_row != NULL ) database_free_rows( (struct NNCMS_ROW *) group_row );
    if( user_groups != NULL ) g_array_free( user_groups, true );

    if( read_access_out )   *read_access_out = read_access;
    if( write_access_out )   *write_access_out = write_access;
    if( exec_access_out )   *exec_access_out = exec_access;
}

bool file_check_perm( struct NNCMS_THREAD_INFO *req, char *user_id, char *file_id, bool read_check, bool write_check, bool exec_check )
{
    bool read_access = false;
    bool write_access = false;
    bool exec_access = false;
    file_get_access( req, user_id, file_id, &read_access, &write_access, &exec_access );

    if( (read_check == true && read_access == false) ||
        (write_check == true && write_access == false) ||
        (exec_check == true && exec_access == false) )
    return false;

    return true;
}

// #############################################################################

// This function will return string with path to icon for given filename.
char *file_get_icon( char *lpszFile )
{
    char *lpszExtension = get_extension( lpszFile );
    char *lpszIcon = fileIcons[0].szIcon;
    if( lpszExtension != 0 )
    {
        lpszExtension++;
        for( unsigned int i = 0; i < NNCMS_ICONS_MAX; i++ )
        {
            // Check for terminating row
            if( fileIcons[i].szExtension == 0 || fileIcons[i].szIcon == 0 )
                break;

            // Compare extension and current icon extension
            if( strcasecmp( lpszExtension, fileIcons[i].szExtension ) == 0 )
            {
                lpszIcon = fileIcons[i].szIcon;
                break;
            }
        }
    }
    return lpszIcon;
}

// #############################################################################

struct NNCMS_FILE_INFO_V2 *file_get_info_v2( struct NNCMS_THREAD_INFO *req, char *file_path )
{
    struct NNCMS_FILE_INFO_V2 *file = MALLOC( sizeof(struct NNCMS_FILE_INFO_V2) );
    garbage_add( req->loop_garbage, file, MEMORY_GARBAGE_FREE );

    // Obtain information about the file
    //
    // /srv/http/temp/002609.jpg
    // { st_dev = 2051, st_ino = 271349, st_nlink = 1,
    // st_mode = 33188, st_uid = 1000, st_gid = 1000, __pad0 = 0,
    // st_rdev = 0, st_size = 3912, st_blksize = 4096, st_blocks = 8, 
    // st_atim = {tv_sec = 1261601580, tv_nsec = 0},
    // st_mtim = {tv_sec = 1261601580, tv_nsec = 0},
    // st_ctim = {tv_sec = 1340917565, tv_nsec = 236977136}, __unused = {0, 0, 0}}
    //
    int result = lstat( file_path, &file->fstat );
    if( result == 0 )
    {
        file->is_dir = S_ISDIR( file->fstat.st_mode );
        file->is_file = S_ISREG( file->fstat.st_mode );

        if( file->is_dir == true )
        {
            // Not calculating size of directory
            // Could be cached...
            file->size = "&lt;DIR&gt;";
        }
        else if( file->is_file == true )
        {
            // Add prefix if number is large
            file->size = MALLOC( 32 );
            file_human_size( file->size, file->fstat.st_size );
            garbage_add( req->loop_garbage, file->size, MEMORY_GARBAGE_FREE );
        }

        file->timestamp = file->fstat.st_ctime;
        file->exists = true;
    }
    else
    {
        // This may happed if file was deleted on server
        // by FTP admin, but it's still in DB
        file->is_dir = false; file->is_file = false;
        file->size = "&lt;ERROR&gt;";
        file->timestamp = 0;
        file->exists = false;
    }

    // Get mimetype and icon
    if( file->is_dir == 1 )
    {
        file->icon = file_get_icon( ".folder" );
        file->mime_type = "folder";
    }
    else if( file->is_file == 1 )
    {
        file->icon = file_get_icon( file_path );
        file->mime_type = get_mime( file_path );
    }
    else
    {
        file->icon = "images/status/dialog-error.png";
        file->mime_type = "error";
    }

    if( strncmp( file->mime_type, "text/", 5 ) == 0 )
        file->binary = false;
    else
        file->binary = true;

    return file;
}


void file_get_info( struct NNCMS_FILE_INFO *fileInfo, char *lpszFile )
{
    char szTempPath[NNCMS_PATH_LEN_MAX];
    size_t nLen;

    // Allocate memory for FILE_INFO structure
    memset( fileInfo, 0, sizeof(struct NNCMS_FILE_INFO) );

    // Copy path to temp buffer and filter it from possible evil
    strlcpy( szTempPath, lpszFile, NNCMS_PATH_LEN_MAX );
    filter_path( szTempPath, 1, -1 );

    // szTempPath is now relative fullpath
    unsigned int nRelFullPathSize = strlen( szTempPath );
    strlcpy( fileInfo->lpszRelFullPath, szTempPath, NNCMS_PATH_LEN_MAX );

    // Create absolute full path
    nLen = strlcpy( fileInfo->lpszAbsFullPath, fileDir, NNCMS_PATH_LEN_MAX );
    strlcpy( &fileInfo->lpszAbsFullPath[nLen], szTempPath + sizeof(char), NNCMS_PATH_LEN_MAX - nLen );

    // Get relative path, we need to just remove filename from szTempPath
    char *lpszFileName = rindex( szTempPath, '/' );
    if( lpszFileName == 0 ) return;
    lpszFileName++; // Skip slash
    unsigned int nRelPathLen = lpszFileName - szTempPath;
    strlcpy( fileInfo->lpszRelPath, szTempPath, nRelPathLen + 1 );

    // Create absolute path
    nLen = strlcpy( fileInfo->lpszAbsPath, fileDir, NNCMS_PATH_LEN_MAX );
    strlcat( &fileInfo->lpszAbsPath[nLen], fileInfo->lpszRelPath + 1, NNCMS_PATH_LEN_MAX - nLen );

    // Get file name
    strlcpy( fileInfo->lpszFileName, szTempPath + nRelPathLen, NNCMS_PATH_LEN_MAX );

    // Get file extension
    char *lpszExtension = rindex( fileInfo->lpszFileName, '.' );
    if( lpszExtension != 0 )
    {
        // Skip dot and copy to extension var
        lpszExtension++;
        strlcpy( fileInfo->lpszExtension, lpszExtension, sizeof(fileInfo->lpszExtension) );
    }
    else
    {
        *fileInfo->lpszExtension = 0;
    }

    // Now, we must know, is it a directory or a file?
    if( szTempPath[nRelFullPathSize - sizeof(char)] == '/' )
    {
        fileInfo->bIsDir = true;
        fileInfo->bIsFile = false;
    }
    else
    {
        fileInfo->bIsDir = false;
        fileInfo->bIsFile = true;
    }

    // Get mimetype (simple)
    fileInfo->lpszMimeType = get_mime( fileInfo->lpszFileName );

    // Get file size
    if( lstat( fileInfo->lpszAbsFullPath, &fileInfo->fileStat ) == 0 )
    {
        // Check what is this
        if( S_ISREG( fileInfo->fileStat.st_mode ) == 1 )
        {
            fileInfo->bIsDir = false;
            fileInfo->bIsFile = true;
        }
        else if( S_ISDIR( fileInfo->fileStat.st_mode ) == 1 )
        {
            fileInfo->bIsDir = true;
            fileInfo->bIsFile = false;
        }

        // File/directory exists
        fileInfo->bExists = true;
    }
    fileInfo->nSize = fileInfo->fileStat.st_size;

    // Get icon (simple)
    if( fileInfo->bIsDir == true )
        fileInfo->lpszIcon = file_get_icon( ".folder" );
    else
        fileInfo->lpszIcon = file_get_icon( fileInfo->lpszFileName );

    // Get parent relative directory path
    strlcpy( fileInfo->lpszParentDir, szTempPath, nRelFullPathSize );
    char *lpszDirName = strrchr( fileInfo->lpszParentDir, '/' );
    if( lpszDirName != 0 )
        lpszDirName[1] = 0; // [1] - means that we leave '/' character, so
                            // parent directory would look like: "/temp/\0"

    // Test
    /*printf( "-----\nRelFullPath: %s\nAbsFullPath: %s\nRelPath: %s\n"
            "AbsPath: %s\nFileName: %s\nExtension: %s\nMimeType: %s\n"
            "IsDir: %s\nIsFile: %s\nIsExists: %s\nSize: %u\nIcon: %s\n"
            "ParentDir: %s\n-----\n",
        fileInfo->lpszRelFullPath, fileInfo->lpszAbsFullPath, fileInfo->lpszRelPath, fileInfo->lpszAbsPath,
        fileInfo->lpszFileName, fileInfo->lpszExtension, fileInfo->lpszMimeType, (fileInfo->bIsDir == true ? "Yes" : "No"),
        (fileInfo->bIsFile == true ? "Yes" : "No"), (fileInfo->bExists == true ? "Yes" : "No"),
        fileInfo->nSize, fileInfo->lpszIcon, fileInfo->lpszParentDir ); // */
}

// #############################################################################

static char *szSizePrefix[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "PiB" };
void file_human_size( char *szDest, unsigned int nSize )
{
    unsigned char i = 0;
    double nSizeTemp = nSize;

    while( nSizeTemp > 1024 )
    {
        nSizeTemp /= 1024;
        i++;
    }

    if( i == 0 ) sprintf( szDest, "%0.0f %s", nSizeTemp, szSizePrefix[i] );
    else sprintf( szDest, "%0.1f %s", nSizeTemp, szSizePrefix[i] );
}

// #############################################################################

char *file_links( struct NNCMS_THREAD_INFO *req, char *file_id, char *file_parent_file_id, char *file_type )
{
    struct NNCMS_VARIABLE vars_file[] =
    {
        { .name = "file_id", .value.string = file_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_parent[] =
    {
        { .name = "file_id", .value.string = file_parent_file_id, .type = NNCMS_TYPE_STRING },
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
    bool is_root = ( file_parent_file_id != NULL && strcmp( file_parent_file_id, "root" ) == 0 ? true : false );
    bool is_dir = ( file_type != NULL && file_type[0] == 'd' ? true : false );
    bool is_file = ( file_type != NULL && file_type[0] == 'f' ? true : false );

    bool read_access = false;
    bool write_access = false;
    bool exec_access = false;
    bool parent_read_access = false;
    bool parent_write_access = false;
    bool parent_exec_access = false;
    if( file_id )  file_get_access( req, NULL, file_id, &read_access, &write_access, &exec_access );
    if( file_parent_file_id )  file_get_access( req, NULL, file_parent_file_id, &parent_read_access, &parent_write_access, &parent_exec_access );

    if( read_access == true )
    {
        // File list link only for folders
        if( (file_type != NULL && is_dir == true) || is_root == true )
        {    
            link.function = "file_list";
            link.title = i18n_string_temp( req, "list_link", NULL );
            link.vars = vars_file;
            g_array_append_vals( links, &link, 1 );
               
            link.function = "file_gallery";
            link.title = i18n_string_temp( req, "gallery_link", NULL );
            link.vars = vars_file;
            g_array_append_vals( links, &link, 1 );
        }

        // When viewing file, the folder it belongs to is stored
        // in 'parent_file_id' variable
        if( file_type != NULL && is_file == true )
        {
            link.function = "file_list";
            link.title = i18n_string_temp( req, "list_link", NULL );
            link.vars = vars_parent;
            g_array_append_vals( links, &link, 1 );
            
            link.function = "file_gallery";
            link.title = i18n_string_temp( req, "gallery_link", NULL );
            link.vars = vars_parent;
            g_array_append_vals( links, &link, 1 );
        }
    }

    if( parent_read_access == true )
    {
        // Do not display 'parent' link for root folder
        // Display only for folders
        if( is_root == false && is_dir == true )
        {
            link.function = "file_list";
            link.title = i18n_string_temp( req, "parent_list_link", NULL );
            link.vars = vars_parent;
            g_array_append_vals( links, &link, 1 );
        }
    }

    if( write_access == true )
    {
        // Add files only into folders
        if( (file_type != NULL && is_dir == true) || is_root == true )
        {    
            link.function = "file_add";
            link.title = i18n_string_temp( req, "add_link", NULL );
            link.vars = vars_file;
            g_array_append_vals( links, &link, 1 );

            link.function = "file_upload";
            link.title = i18n_string_temp( req, "upload_link", NULL );
            link.vars = vars_file;
            g_array_append_vals( links, &link, 1 );

            link.function = "file_mkdir";
            link.title = i18n_string_temp( req, "mkdir_link", NULL );
            link.vars = vars_file;
            g_array_append_vals( links, &link, 1 );
        }
    }

    if( read_access == true )
    {
        link.function = "file_view";
        link.title = i18n_string_temp( req, "view_link", NULL );
        link.vars = vars_file;
        g_array_append_vals( links, &link, 1 );
    }

    if( parent_write_access == true && write_access == true )
    {
        link.function = "file_edit";
        link.title = i18n_string_temp( req, "edit_link", NULL );
        link.vars = vars_file;
        g_array_append_vals( links, &link, 1 );

        if( file_id != NULL && is_root == false )
        {
            link.function = "file_delete";
            link.title = i18n_string_temp( req, "delete_link", NULL );
            link.vars = vars_file;
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

GString *file_dbfs_path( struct NNCMS_THREAD_INFO *req, int file_id )
{
    GString *path = g_string_sized_new( NNCMS_PATH_LEN_MAX );
    garbage_add( req->loop_garbage, path, MEMORY_GARBAGE_GSTRING_FREE );
    
    // Convert id from DB to path in FS
    while( 1 )
    {
        database_bind_int( req->stmt_find_file_by_id, 1, file_id );
        struct NNCMS_ROW *file_row = database_steps( req, req->stmt_find_file_by_id );
        if( file_row != NULL )
        {
            if( file_row->value[4][0] == 'd' )
                g_string_prepend_c( path, '/' );
            g_string_prepend( path, file_row->value[3] );
            file_id = atoi( file_row->value[2] );
            database_free_rows( file_row );
        }
        else
        {
            break;
        }
    }
    
    //char *str_path = g_string_free( path, FALSE );
    //return str_path;
    return path;
}

// #############################################################################

struct NNCMS_SELECT_OPTIONS *file_folder_list( struct NNCMS_THREAD_INFO *req, char *file_id )
{
    bool is_admin = acl_check_perm( req, "file", NULL, "admin" );
    
    // Make a list of folders
    GArray *folders = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_SELECT_ITEM) );
    garbage_add( req->loop_garbage, folders, MEMORY_GARBAGE_GARRAY_FREE );

    // Root folder
    //struct NNCMS_SELECT_ITEM root = { .name = file_id, .desc = "/ - Root", .selected = false };
    //g_array_append_vals( folders, &root, 1 );

    struct NNCMS_FILE_ROW *folder_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_list_folders );
    garbage_add( req->loop_garbage, folder_row, MEMORY_GARBAGE_DB_FREE );
    if( folder_row != NULL )
    {
        for( unsigned int i = 0; folder_row[i].id; i = i + 1 )
        {
            if( is_admin == false && file_check_perm( req, NULL, folder_row[i].id, false, true, false ) == false )
            {
                continue;
            }

            // Option description
            GString *folder_path = file_dbfs_path( req, atoi( folder_row[i].id ) );
            //g_string_prepend_c( folder_path, '/' );
            g_string_append( folder_path, " - " );
            g_string_append( folder_path, folder_row[i].title );
            
            // Add option
            struct NNCMS_SELECT_ITEM folder =
            {
                .name = folder_row[i].id,
                .desc = folder_path->str,
                .selected = false
            };
            g_array_append_vals( folders, &folder, 1 );
        }
    }
    
    struct NNCMS_SELECT_OPTIONS options =
    {
        .link = NULL,
        .items = (struct NNCMS_SELECT_ITEM *) folders->data
    };
    
    return (struct NNCMS_SELECT_OPTIONS *) memdup_temp( req, &options, sizeof(options) );
}

// #############################################################################

bool file_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_FILE_FIELDS *fields )
{
    bool result = true;
    
    // Validate file name of uploaded file
    if( req->files != NULL && req->files->len != 0 )
    {
        struct NNCMS_FILE *file = & ((struct NNCMS_FILE *) req->files->data) [0];
        
        // Invalid file names (in Linux)
        //
        char *file_name = file->file_name;

        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_name", .value = file_name, .type = NNCMS_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        
        // File name ".." or "."
        if( file_name == NULL ||
            (file_name[0] == '.' && file_name[1] == 0) ||
            (file_name[0] == '.' && file_name[1] == '.' && file_name[2] == 0) )
        {
            log_vdisplayf( req, LOG_ERROR, "invalid_file_name", vars );
            result = false;
        }
    }
    
    // Validate file name
    if( fields->file_name.editable == true )
    {
        // Invalid file names (in Linux)
        //
        char *file_name = fields->file_name.value;

        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "file_name", .value = file_name, .type = NNCMS_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        // File name ".." or "."
        if( file_name == NULL ||
            (file_name[0] == '.' && file_name[1] == 0) ||
            (file_name[0] == '.' && file_name[1] == '.' && file_name[2] == 0) )
        {
            log_vdisplayf( req, LOG_ERROR, "invalid_file_name", vars );
            result = false;
        }

        // Characters between 0x000 and 0x01F inclusively
        // Characters '/'
        //
        // That is checked by field_validate
    }
    
    // Validate folder
    if( fields->file_parent_file_id.editable == true )
    {
        if( fields->file_parent_file_id.value == NULL )
        {
            log_vdisplayf( req, LOG_ERROR, "invalid_parent_file_id", NULL );
            result = false;
        }
        else
        {
            database_bind_text( req->stmt_find_file_by_id, 1, fields->file_parent_file_id.value );
            struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
            if( file_row == NULL )
            {
                log_vdisplayf( req, LOG_ERROR, "invalid_parent_file_id", NULL );
                result = false;
            }
            else
            {
                if( file_row->type[0] != 'd' )
                {
                    log_vdisplayf( req, LOG_ERROR, "invalid_parent_file_id", NULL );
                    result = false;
                }
                database_free_rows( (struct NNCMS_ROW *) file_row );
            }
        }
    }
    
    return result;
}

// #############################################################################

void file_force_cache( struct NNCMS_THREAD_INFO *req )
{
    char time_buf[NNCMS_TIME_STR_LEN_MAX];
    char expire_buf[9 + NNCMS_TIME_STR_LEN_MAX + 1];

    // Make browser cache the image:
    //    Last-Modified: Wed, 08 Sep 2010 14:59:39 GMT
    //    Expires: Thu, 31 Dec 2037 23:55:55 GMT
    //    Cache-Control: max-age=315360000
    main_header_add( req, "Cache-Control: max-age=315360000" );
    main_format_time_string( req, time_buf, time( 0 ) + 315360000 );
    snprintf( expire_buf, sizeof(expire_buf), "Expires: %s", time_buf );
    main_header_add( req, expire_buf );
}

// #############################################################################

char *file_thumbnail( struct NNCMS_THREAD_INFO *req, char *file_id, unsigned int width, unsigned int height )
{

#ifndef HAVE_IMAGEMAGICK
    return NULL;
#endif // !HAVE_IMAGEMAGICK
#ifdef HAVE_IMAGEMAGICK

    // Find row by id
    database_bind_text( req->stmt_find_file_by_id, 1, file_id );
    struct NNCMS_FILE_ROW *file_row = (struct NNCMS_FILE_ROW *) database_steps( req, req->stmt_find_file_by_id );
    if( file_row == NULL )
    {
        return NULL;
    }
    garbage_add( req->loop_garbage, file_row, MEMORY_GARBAGE_DB_FREE );
    
    // Convert id from DB to relative path in FS
    GString *file_path = file_dbfs_path( req, atoi( file_row->id ) );

    // Get thumbnail size
    unsigned long int new_width = width;
    unsigned long int new_height = height;

    // Limit
    if( new_width  > THUMBNAIL_WIDTH_MAX )  { new_width = THUMBNAIL_WIDTH_MAX; }
    else if( new_width  < THUMBNAIL_WIDTH_MIN )  { new_width = THUMBNAIL_WIDTH_MIN; }
    if( new_height > THUMBNAIL_HEIGTH_MAX ) { new_height = THUMBNAIL_HEIGTH_MAX; }
    else if( new_height < THUMBNAIL_HEIGTH_MIN ) { new_height = THUMBNAIL_HEIGTH_MIN; }

    // Make a path to thumbnail
    GString *thumbnail_file = g_string_new( file_path->str );
    garbage_add( req->loop_garbage, thumbnail_file, MEMORY_GARBAGE_GSTRING_FREE );

    // Insert thumbnail size between extension and file name
    GString *thumbnail_size = g_string_sized_new( 100 );
    g_string_printf( thumbnail_size, "_%ix%i", new_width, new_height );
    char *extension_addr = rindex( file_path->str, '.' );
    if( extension_addr != NULL )
    {
        size_t extension_pos = extension_addr - file_path->str;
        size_t extension_size = file_path->len - extension_pos;
        if( extension_size > 4 )
            goto append_size;
        g_string_insert( thumbnail_file, extension_pos, thumbnail_size->str );
    }
    else
    {
        append_size:
        g_string_append( thumbnail_file, thumbnail_size->str );
    }
    g_string_free( thumbnail_size, true );

    //
    // Find image thumbnail in cache
    //
    bool access_file = cache_access_file( req, thumbnail_file->str );
    if( access_file == true )
    {
        return thumbnail_file->str;
    }

    // Prepend root directory
    g_string_prepend( file_path, fileDir );
    struct NNCMS_FILE_INFO_V2 *file_info = file_get_info_v2( req, file_path->str );

    log_printf( req, LOG_DEBUG, "thumb: %s", thumbnail_file->str );
    log_printf( req, LOG_DEBUG, "file: %s", file_path->str );

    //
    // Initialize the image info structure and read an image.
    //
    MagickWand *magick_wand = NewMagickWand( );
    MagickBooleanType result = MagickReadImage( magick_wand, file_path->str );
    if( result == MagickFalse )
    {
        ExceptionType severity;
        char *description = MagickGetException( magick_wand, &severity );
        log_printf( req, LOG_ERROR, "Image not loaded: %s %s", GetMagickModule(), description );
        description = (char *) MagickRelinquishMemory( description );
        magick_wand = DestroyMagickWand( magick_wand );
        return NULL;
    }

    //
    // Calculate new width and height for thumblnail, keeping the
    // same aspect ratio.
    //
    double img_width = (double) MagickGetImageWidth( magick_wand );
    double img_height = (double) MagickGetImageHeight( magick_wand );
    double img_ratio = img_width / img_height;
    if( img_ratio > 1.0f )
        new_height = (unsigned long int) ((double) new_height / img_ratio);
    else
        new_width = (unsigned long int) ((double) new_width * img_ratio);

    //
    // Set format to JPG, if image is not PNG, but keep PNG for PNG, or
    // transparent background will look bad, when resized
    //
    //if( strcmp( fileInfo.lpszExtension, "png" ) != 0 )
    //{
    //    MagickSetFormat( magick_wand, "JPEG" );
    //}

    //
    //  Turn the images into a thumbnail sequence.
    //
    MagickResetIterator( magick_wand );
    MagickNextImage( magick_wand );
    MagickResizeImage( magick_wand, new_width, new_height, LanczosFilter, 1.0 );

    size_t lenght;
    unsigned char *blob = MagickGetImageBlob( magick_wand, &lenght );
    if( blob == NULL || lenght == 0 )
    {
        ExceptionType severity;
        char *description = MagickGetException( magick_wand, &severity );
        log_printf( req, LOG_ERROR, "Unable to convert image to blob: %s %s", GetMagickModule(), description );
        description = (char *) MagickRelinquishMemory( description );
        magick_wand = DestroyMagickWand( magick_wand );
        return NULL;
    }

    // Store in cache
    cache_store( req, thumbnail_file->str, blob, lenght );

    // Cleanup
    MagickRelinquishMemory( blob );
    magick_wand = DestroyMagickWand( magick_wand );
    
    return thumbnail_file->str;

#endif // HAVE_IMAGEMAGICK
}

// #############################################################################
