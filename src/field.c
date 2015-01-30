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

#include "database.h"
#include "user.h"
#include "group.h"
#include "log.h"
#include "i18n.h"
#include "memory.h"
#include "filter.h"
#include "view.h"
#include "node.h"

#include "strlcpy.h"

// #############################################################################
// includes of system headers
//

#include <glib.h>

// #############################################################################
// global variables
//

struct NNCMS_SELECT_ITEM char_tables[] =
{
    { .name = "validate_none", .desc = NULL, .selected = false },
    { .name = "unix_file", .desc = NULL, .selected = false },
    { .name = "printable", .desc = NULL, .selected = false },
    { .name = "alphanum", .desc = NULL, .selected = false },
    { .name = "numeric", .desc = NULL, .selected = false },
    { .name = "alphanum_underscore", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_VARIABLE char_table_list[] =
{
    { .name = "validate_none", .value.string = NULL, .type = NNCMS_TYPE_STRING },
    { .name = "unix_file", .value.string = unix_file_validate, .type = NNCMS_TYPE_STRING },
    { .name = "printable", .value.string = printable_validate, .type = NNCMS_TYPE_STRING },
    { .name = "alphanum", .value.string = alphanum_validate, .type = NNCMS_TYPE_STRING },
    { .name = "numeric", .value.string = numeric_validate, .type = NNCMS_TYPE_STRING },
    { .name = "alphanum_underscore", .value.string = alphanum_underscore_validate, .type = NNCMS_TYPE_STRING },
    { .type = NNCMS_TYPE_NONE }
};

struct NNCMS_SELECT_ITEM label_types[] =
{
    { .name = "above", .desc = NULL, .selected = false },
    { .name = "inline", .desc = NULL, .selected = false },
    { .name = "hidden", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_VARIABLE label_type_list[] =
{
    { .name = "unknown", .value.integer = FIELD_LABEL_UNKNOWN, .type = NNCMS_TYPE_INTEGER },
    { .name = "above", .value.integer = FIELD_LABEL_ABOVE, .type = NNCMS_TYPE_INTEGER },
    { .name = "inline", .value.integer = FIELD_LABEL_INLINE, .type = NNCMS_TYPE_INTEGER },
    { .name = "hidden", .value.integer = FIELD_LABEL_HIDDEN, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

struct NNCMS_SELECT_ITEM booleans[] =
{
    { .name = "0", .desc = NULL, .selected = false },
    { .name = "1", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_VARIABLE boolean_list[] =
{
    { .name = "0", .value.integer = 0, .type = NNCMS_TYPE_INTEGER },
    { .name = "on", .value.integer = 1, .type = NNCMS_TYPE_INTEGER },
    { .name = "1", .value.integer = 1, .type = NNCMS_TYPE_INTEGER },
    { .name = "true", .value.integer = 1, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

// #############################################################################
// functions

struct NNCMS_FIELD *field_find( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field, char *field_name )
{
    for( unsigned int i = 0; field[i].type != NNCMS_FIELD_NONE; i++ )
    {
        if( strcmp( field[i].name, field_name ) == 0 )
            return &field[i];
    }
    
    return NULL;
}

// #############################################################################

char *field_get_value( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field, char *field_name )
{
    struct NNCMS_FIELD *result = field_find( req, field, field_name );
    if( result != NULL )
        return result->value;
    else
        return NULL;
}

// #############################################################################

void *field_parse_config( struct NNCMS_THREAD_INFO *req, enum NNCMS_FIELD_TYPE field_type, char *config )
{
    if( config == NULL || config[0] == 0 )
        return NULL;

    lua_State *L = req->L;

    // Load script in buffer
    int load_result = luaL_loadbuffer( L, config, strlen(config), "field_parse_config" );
    if( load_result != 0 )
    {
        // Get error string
        char *stack_msg = (char *) lua_tostring( L, -1 );
        lua_pop( L, 1 );
        log_printf( req, LOG_ERROR, "Lua loading error: %s", stack_msg );
        return NULL;
    }

    // Call the script
    int pcall_result = lua_pcall( L, 0, 0, 0 );
    if( pcall_result != 0 )
    {
        // Get error string
        char *stack_msg = (char *) lua_tostring( L, -1 );
        lua_pop( L, 1 );
        log_printf( req, LOG_ERROR, "Lua calling error: %s", stack_msg );
        return NULL;
    }
    
    // Check if all is ok
    int stack_top = lua_gettop( L );
    if( stack_top != 0 )
    {
        log_printf( req, LOG_EMERGENCY, "Lua stack is not empty, position %i, script = %s", stack_top, config );

        // Debug
        lua_stackdump( L );

        // Empty the stack
        lua_settop( L, 0 );
    }

    switch( field_type )
    {
        case NNCMS_FIELD_SELECT:
        {
            // Get options from config string
            GArray *goptions = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_SELECT_ITEM) );
            garbage_add( req->loop_garbage, goptions, MEMORY_GARBAGE_GARRAY_FREE );
            struct NNCMS_SELECT_ITEM option =
            {
                .name = "",
                .desc = i18n_string_temp( req, "option_select_a_value", NULL ),
                .selected = false
            };
            g_array_append_vals( goptions, &option, 1 );

            // table is in the stack at index 1
            lua_getglobal( L, "options" );

            lua_pushnil( L );  // first key
            while( lua_next( L, 1 ) != 0 )
            {
                // uses 'key' (at index -2) and 'value' (at index -1)
                char *value = lua_tostring( L, -1 );
               
                struct NNCMS_SELECT_ITEM option =
                {
                    .name = value,
                    .desc = value,
                    .selected = false
                };
                g_array_append_vals( goptions, &option, 1 );

                // removes 'value'; keeps 'key' for next iteration
                lua_pop( L, 1 );
            }
             
            lua_pop( L, 1 );

            struct NNCMS_SELECT_OPTIONS options =
            {
                .link = NULL,
                .items = (struct NNCMS_SELECT_ITEM *) goptions->data
            };

            size_t len = 0;
            lua_getglobal( L, "link" );
            const char *str = lua_tolstring( L, 1, &len );
            if( str )
                options.link = memdup_temp( req, (char *) str, len + 1 );
            lua_pop( L, 1 );

            return (struct NNCMS_SELECT_OPTIONS *) memdup_temp( req, &options, sizeof(options) );
            
            break;
        }
        
        case NNCMS_FIELD_TEXTAREA:
        {
            struct NNCMS_TEXTAREA_OPTIONS textarea_options =
            {
                .link = NULL,
                .escape = false,
                .summary = false,
                .remove_summary = false
            };

            size_t len = 0;
            lua_getglobal( L, "link" );
            const char *str = lua_tolstring( L, 1, &len );
            if( str )
                textarea_options.link = memdup_temp( req, (char *) str, len + 1 );
            lua_pop( L, 1 );

            lua_getglobal( L, "escape" ); textarea_options.escape = lua_toboolean( L, 1 ); lua_pop( L, 1 );
            lua_getglobal( L, "summary" ); textarea_options.summary = lua_toboolean( L, 1 ); lua_pop( L, 1 );
            lua_getglobal( L, "remove_summary" ); textarea_options.remove_summary = lua_toboolean( L, 1 ); lua_pop( L, 1 );

            return memdup_temp( req, &textarea_options, sizeof(textarea_options) );
    
            break;
        }
        
        case NNCMS_FIELD_EDITBOX:
        {
            struct NNCMS_EDITBOX_OPTIONS options =
            {
                .link = NULL
            };
            
            size_t len = 0;
            lua_getglobal( L, "link" );
            const char *str = lua_tolstring( L, 1, &len );
            if( str )
                options.link = memdup_temp( req, (char *) str, len + 1 );
            lua_pop( L, 1 );
    
            return memdup_temp( req, &options, sizeof(options) );
            
            break;
        }
    };

}

// #############################################################################

void field_load_data( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD_DATA_ROW *field_data_row, struct NNCMS_FIELD *field )
{
    GArray *value_array = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD_VALUE) );
    garbage_add( req->loop_garbage, value_array, MEMORY_GARBAGE_GARRAY_FREE );
    
    for( unsigned int j = 0; field_data_row != NULL && field_data_row[j].id != NULL; j = j + 1 )
    {
        struct NNCMS_FIELD_VALUE value =
        {
            .id = field_data_row[j].id,
            .value = field_data_row[j].data,
            .config = field_data_row[j].config,
            .weight = field_data_row[j].weight
        };
        g_array_append_vals( value_array, &value, 1 );
    }

    field->value = ( field_data_row ? field_data_row->data : NULL );
    field->multivalue = (struct NNCMS_FIELD_VALUE *) value_array->data;
}

void field_load_config( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD_CONFIG_ROW *field_config_row, struct NNCMS_FIELD *field )
{
    // Fill field structure
    field->name = field_config_row->name;
    //field->value = NULL;
    //field->multivalue = NULL;
    field->type = filter_str_to_int( field_config_row->type, field_type_list );
    field->editable = true; field->viewable = true;
    field->text_name = NULL;
    field->text_description = NULL;
    field->config = field_config_row->config;
    field->label_type = filter_str_to_int( field_config_row->label, label_type_list );
    field->empty_hide = filter_str_to_int( field_config_row->empty_hide, boolean_list );
    field->values_count = atoi( field_config_row->values_count );
    field->size_min = atoi( field_config_row->size_min );
    field->size_max = atoi( field_config_row->size_max );
    field->char_table = filter_str_to_ptr( field_config_row->char_table, char_table_list );
    
    field->data = field_parse_config( req, field->type, field->config );
}

// #############################################################################

void field_parse( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field, GString *buffer )
{
    if( field->viewable == false )
        return;
    
    if( field->empty_hide == true &&
        (field->value == NULL || field->value[0] == 0) &&
        (field->multivalue == NULL || field->multivalue[0].value == NULL || field->multivalue[0].value[0] == 0) &&
        field->editable == false )
        return;
    
    // Get description from i18n database if set to NULL
    {
        char i18n_name[FIELD_NAME_LEN_MAX + 6];
        
        if( field->text_name == NULL )
        {
            snprintf( i18n_name, sizeof(i18n_name), "%s_name", field->name );
            field->text_name = i18n_string_temp( req, i18n_name, NULL  );
        }

        if( field->text_description == NULL )
        {
            snprintf( i18n_name, sizeof(i18n_name), "%s_desc", field->name );
            field->text_description = i18n_string_temp( req, i18n_name, NULL );
        }
    }

    // Convert label type to label class
    char *label_class = filter_int_to_str( field->label_type, label_type_list );
    
    // Convert single value to one multivalue
    struct NNCMS_FIELD_VALUE *multivalue;
    if( field->multivalue == NULL )
    {
        multivalue = (struct NNCMS_FIELD_VALUE [])
        {
            { .id = "0", .value = field->value, .config = field->config, .weight = "0" },
            { .id = NULL }
        };
    }
    else
    {
        multivalue = field->multivalue;
    }

    // Prepare buffer for field data
    GString *data_html = g_string_sized_new( 10 );

    // Iterate through every field data
    for( unsigned int k = 0; multivalue != NULL && multivalue[k].id != NULL; k = k + 1 )
    {
        // Add [id] for multivalue field
        char *field_name = field->name; 
        char *field_value = multivalue[k].value;
        char temp[FIELD_NAME_LEN_MAX + 10];
        if( field->values_count > 1 || field->values_count == -1 )
        {
            snprintf( temp, sizeof(temp), "%s[%i]", field->name, k );
            field_name = temp;
        }

        struct NNCMS_VARIABLE field_data_tags[] =
        {
            { .name = "field_text_name", .value.string = field->text_name, .type = NNCMS_TYPE_STRING },
            { .name = "field_text_description", .value.string = field->text_description, .type = NNCMS_TYPE_STRING },
            { .name = "field_name", .value.string = field_name, .type = NNCMS_TYPE_STRING },
            { .name = "field_value", .value.string = field_value, .type = NNCMS_TYPE_STRING },
            { .name = "values_count", .value.integer = field->values_count, .type = NNCMS_TYPE_INTEGER },
            { .name = "field_data_id", .value.string = multivalue[k].id, .type = NNCMS_TYPE_STRING },
            { .name = "weight", .value.string = multivalue[k].weight, .type = NNCMS_TYPE_STRING },
            { .name = "link", .value.string = NULL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        // Process according to field type
        switch( field->type )
        {
            case NNCMS_FIELD_EDITBOX:
            {
                if( field->editable == true )
                {
                    template_hparse( req, "template_field_editbox.html", data_html, field_data_tags );
                }
                else
                {
                    struct NNCMS_EDITBOX_OPTIONS *options = (struct NNCMS_EDITBOX_OPTIONS *) field->data;
                    if( options != NULL && options->link != NULL && options->link[0] != 0 )
                    {
                        field_data_tags[7].value.string = options->link;
                        template_hparse( req, "template_field_textlink.html", data_html, field_data_tags );
                    }
                    else
                    {
                        template_hparse( req, "template_field_text.html", data_html, field_data_tags );
                    }
                }
                break;
            }

            case NNCMS_FIELD_CHECKBOX:
            {
                if( field->editable == true )
                    template_hparse( req, "template_field_checkbox.html", data_html, field_data_tags );
                else
                    template_hparse( req, "template_field_text.html", data_html, field_data_tags );
                break;
            }
            
            case NNCMS_FIELD_TEXTAREA:
            {
                struct NNCMS_TEXTAREA_OPTIONS *textarea_options = (struct NNCMS_TEXTAREA_OPTIONS *) field->data;
                
                if( field->editable == true )
                {
                    template_hparse( req, "template_field_textarea.html", data_html, field_data_tags );
                }
                else
                {
                    if( textarea_options != NULL && textarea_options->escape == true )
                    {
                        template_hparse( req, "template_field_code.html", data_html, field_data_tags );
                    }
                    else
                    {
                        // Get summary data from text
                        char *summary_start = strstr( field_value, "<summary>" );
                        char *summary_end = NULL;
                        char *summary = NULL;
                        size_t summary_len = 0;
                        if( summary_start != NULL )
                        {
                            summary_start += 9;
                            summary_end = strstr( field_value, "</summary>" );
                            if( summary_end != NULL )
                            {
                                summary_end -= 1;
                                summary_len = summary_end - summary_start + 1;
                                summary = MALLOC( summary_len + 1 );
                                strlcpy( summary, summary_start, summary_len + 1 );
                            }
                        }
                        
                        if( textarea_options != NULL && textarea_options->summary == true )
                        {
                            field_data_tags[3].value.string = summary;
                        }
                        
                        if( textarea_options != NULL && textarea_options->remove_summary == true )
                        {
                            if( summary_start != NULL && summary_end != NULL )
                            strcpy( summary_start - 9, summary_end + 11 );
                        }
                        
                        template_hparse( req, "template_field_text.html", data_html, field_data_tags );
                        
                        if( summary != NULL )
                        FREE( summary );
                    }
                }
                break;
            }
            
            case NNCMS_FIELD_HIDDEN:
            {
                label_class = "hidden";
                template_hparse( req, "template_field_hidden.html", data_html, field_data_tags );
                break;
            }

            case NNCMS_FIELD_UPLOAD:
            {
                template_hparse( req, "template_field_upload.html", data_html, field_data_tags );
                break;
            }
            
            case NNCMS_FIELD_SUBMIT:
            {
                label_class = "hidden";
                template_hparse( req, "template_field_submit.html", data_html, field_data_tags );
                break;
            }
            
            case NNCMS_FIELD_HELP:
            {
                template_hparse( req, "template_field_help.html", data_html, field_data_tags );
                break;
            }

            case NNCMS_FIELD_PICTURE:
            {
                if( field->value != NULL || field->value[0] != 0 )
                    template_hparse( req, "template_field_picture.html", data_html, field_data_tags );
                    
                break;
            }

            case NNCMS_FIELD_TEXT:
            {
                template_hparse( req, "template_field_text.html", data_html, field_data_tags );
                break;
            }

            case NNCMS_FIELD_AUTHORING:
            {
                if( field->data == NULL )
                    break;

                // Retrieve field specific data
                struct NNCMS_AUTHORING_DATA *authoring_data = (struct NNCMS_AUTHORING_DATA *) field->data;

                // Get information about user
                database_bind_text( req->stmt_find_user_by_id, 1, authoring_data->user_id );
                struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_find_user_by_id );

                // HTML output
                struct NNCMS_VARIABLE authoring_tags[] =
                {
                    { .name = "user_id", .value.string = authoring_data->user_id, .type = NNCMS_TYPE_STRING },
                    { .name = "user_name", .value.string = (user_row ? user_row->name : "guest"), .type = NNCMS_TYPE_STRING },
                    { .name = "user_nick", .value.string = (user_row ? user_row->nick : "Anonymous"), .type = NNCMS_TYPE_STRING },
                    { .name = "timestamp", .value.string = authoring_data->timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP },
                    { .type = NNCMS_TYPE_NONE }
                };
                template_hparse( req, "template_field_authoring.html", data_html, authoring_tags );
                
                // Mem free
                database_free_rows( (struct NNCMS_ROW *) user_row );
                
                break;
            }
            
            case NNCMS_FIELD_LINK:
            {
                template_hparse( req, "template_field_link.html", data_html, field_data_tags );
                break;
            }

            case NNCMS_FIELD_USER:
            {
                if( field->editable == true )
                {
                    //
                    // Editable field
                    //
                    GString *options_str = g_string_sized_new( 1024 );
                    
                    struct NNCMS_VARIABLE option_tags[] =
                    {
                        { .name = "option_value", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_desc", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };

                    database_bind_int( req->stmt_list_users, 1, -1 );
                    database_bind_int( req->stmt_list_users, 2, 0 );
                    struct NNCMS_USER_ROW *user_row = (struct NNCMS_USER_ROW *) database_steps( req, req->stmt_list_users );
                    if( user_row != NULL )
                    {
                        GString *description = g_string_sized_new( 32 );
                        for( unsigned int i = 0; user_row[i].id != NULL; i = i + 1 )
                        {
                            g_string_printf( description, "%s (%s)", user_row[i].nick, user_row[i].name );
                            
                            struct NNCMS_VARIABLE option_tags[] =
                            {
                                { .name = "option_value", .value.string = user_row[i].id, .type = NNCMS_TYPE_STRING },
                                { .name = "option_desc", .value.string = description->str, .type = NNCMS_TYPE_STRING },
                                { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                                { .type = NNCMS_TYPE_NONE }
                            };

                            // Detect selected
                            if( field_value != NULL )
                            {
                                if( strcmp( field_value, user_row[i].id ) == 0 )
                                    option_tags[2].value.string = "selected";
                            }

                            template_hparse( req, "template_field_option.html", options_str, option_tags );
                        }
                        g_string_free( description, TRUE );
                        database_free_rows( (struct NNCMS_ROW *) user_row );
                    }
                    
                    // Put options between <select> ... </select>
                    field_data_tags[3].value.string = options_str->str;
                    template_hparse( req, "template_field_select.html", data_html, field_data_tags );
                    
                    g_string_free( options_str, TRUE );
                }
                else
                {
                    //
                    // Read-only field
                    //

                    database_bind_text( req->stmt_find_user_by_id, 1, field_value );
                    struct NNCMS_ROW *user_row = database_steps( req, req->stmt_find_user_by_id );
                    if( user_row == NULL )
                    {
                        field_data_tags[2].value.string = "0";
                        field_data_tags[3].value.string = "Anonymous";
                    }
                    else
                    {
                        field_data_tags[2].value.string = user_row->value[0];
                        field_data_tags[3].value.string = user_row->value[2];
                    }

                    template_hparse( req, "template_field_user.html", data_html, field_data_tags );
                    
                    database_free_rows( user_row );
                }
                
                break;
            }
            
            case NNCMS_FIELD_GROUP:
            {
                if( field->editable == true )
                {
                    //
                    // Editable field
                    //
                    GString *options_str = g_string_sized_new( 1024 );
                    
                    char *select_a_value = i18n_string( req, "option_select_a_value", NULL );
                    struct NNCMS_VARIABLE option_tags[] =
                    {
                        { .name = "option_value", .value.string = "", .type = NNCMS_TYPE_STRING },
                        { .name = "option_desc", .value.string = select_a_value, .type = NNCMS_TYPE_STRING },
                        { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };
                    template_hparse( req, "template_field_option.html", options_str, option_tags );
                    g_free( select_a_value );

                    database_bind_int( req->stmt_list_group, 1, -1 );
                    database_bind_int( req->stmt_list_group, 2, 0 );
                    struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_list_group );
                    if( group_row != NULL )
                    {
                        GString *description = g_string_sized_new( 32 );
                        for( unsigned int i = 0; group_row[i].id != NULL; i = i + 1 )
                        {
                            g_string_printf( description, "%s (%s)", group_row[i].description, group_row[i].name );
                            
                            struct NNCMS_VARIABLE option_tags[] =
                            {
                                { .name = "option_value", .value.string = group_row[i].id, .type = NNCMS_TYPE_STRING },
                                { .name = "option_desc", .value.string = description->str, .type = NNCMS_TYPE_STRING },
                                { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                                { .type = NNCMS_TYPE_NONE }
                            };

                            // Detect selected
                            if( field_value != NULL )
                            {
                                if( strcmp( field_value, group_row[i].id ) == 0 )
                                    option_tags[2].value.string = "selected";
                            }

                            template_hparse( req, "template_field_option.html", options_str, option_tags );
                        }
                        g_string_free( description, TRUE );
                        database_free_rows( (struct NNCMS_ROW *) group_row );
                    }
                    
                    // Put options between <select> ... </select>
                    field_data_tags[3].value.string = options_str->str;
                    template_hparse( req, "template_field_select.html", data_html, field_data_tags );
                    
                    g_string_free( options_str, TRUE );
                }
                else
                {
                    //
                    // Read-only field
                    //

                    database_bind_text( req->stmt_find_group, 1, field_value );
                    struct NNCMS_GROUP_ROW *group_row = (struct NNCMS_GROUP_ROW *) database_steps( req, req->stmt_find_group );
                    if( group_row != NULL )
                    {
                        template_hparse( req, "template_field_group.html", data_html, field_data_tags );                        
                        database_free_rows( (struct NNCMS_ROW *) group_row );
                    }
                }
                
                break;
            }
            
            case NNCMS_FIELD_TIMEDATE:
            {
                // Convert string to integer and then format it
                gint64 itd = (field_value != NULL ? g_ascii_strtoll( field_value, NULL, 10 ) : 0 );
                GDateTime *gtd = g_date_time_new_from_unix_local( itd );
                char *std = 0;
                if( gtd != 0 )
                {
                    std = g_date_time_format( gtd, szTimestampFormat );
                    g_date_time_unref( gtd );
                    if( std != 0 )
                        field_data_tags[3].value.string = std;
                }
                
                template_hparse( req, "template_field_help.html", data_html, field_data_tags );
                
                if( std != 0 )
                    g_free( std );
                
                break;
            }
            
            case NNCMS_FIELD_VIEW_ID:
            {
                if( field->editable == true )
                {
                    //
                    // Editable field
                    //
                    GString *options_str = g_string_sized_new( 1024 );
                    
                    struct NNCMS_VARIABLE option_tags[] =
                    {
                        { .name = "option_value", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_desc", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };

                    database_bind_int( req->stmt_list_view, 1, -1 );
                    database_bind_int( req->stmt_list_view, 2, 0 );
                    struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_list_view );
                    if( view_row != NULL )
                    {
                        GString *description = g_string_sized_new( 32 );
                        for( unsigned int i = 0; view_row[i].id != NULL; i = i + 1 )
                        {
                            g_string_printf( description, "[%s]: %s", view_row[i].id, view_row[i].name );
                            
                            struct NNCMS_VARIABLE option_tags[] =
                            {
                                { .name = "option_value", .value.string = view_row[i].id, .type = NNCMS_TYPE_STRING },
                                { .name = "option_desc", .value.string = description->str, .type = NNCMS_TYPE_STRING },
                                { .name = "option_selected", .value.string = NULL, .type = NNCMS_TYPE_STRING },
                                { .type = NNCMS_TYPE_NONE }
                            };

                            // Detect selected
                            if( field_value != NULL )
                            {
                                if( strcmp( field_value, view_row[i].id ) == 0 )
                                    option_tags[2].value.string = "selected";
                            }

                            template_hparse( req, "template_field_option.html", options_str, option_tags );
                        }
                        g_string_free( description, TRUE );
                        database_free_rows( (struct NNCMS_ROW *) view_row );
                    }
                    
                    // Put options between <select> ... </select>
                    field_data_tags[3].value.string = options_str->str;
                    template_hparse( req, "template_field_select.html", data_html, field_data_tags );
                    
                    g_string_free( options_str, TRUE );
                }
                else
                {
                    //
                    // Read-only field
                    //

                    if( field_value != NULL )
                    {
                        database_bind_text( req->stmt_find_view, 1, field_value );
                        struct NNCMS_VIEWS_ROW *view_row = (struct NNCMS_VIEWS_ROW *) database_steps( req, req->stmt_find_view );
                        field_data_tags[3].value.string = view_row->name;
                        template_hparse( req, "template_field_text.html", data_html, field_data_tags );
                        database_free_rows( (struct NNCMS_ROW *) view_row );
                    }
                    
                    break;

                }

                break;
            }

            case NNCMS_FIELD_FIELD_ID:
            {
                if( field->editable == true )
                {
                    //
                    // Editable field
                    //
                    GString *options_str = g_string_sized_new( 1024 );
                    
                    struct NNCMS_VARIABLE option_tags[] =
                    {
                        { .name = "option_value", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_desc", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };

                    database_bind_int( req->stmt_list_all_field_config, 1, -1 );
                    database_bind_int( req->stmt_list_all_field_config, 2, 0 );
                    struct NNCMS_FIELD_CONFIG_ROW *field_config = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_list_all_field_config );
                    if( field_config != NULL )
                    {
                        GString *description = g_string_sized_new( 32 );
                        for( unsigned int i = 0; field_config[i].id != NULL; i = i + 1 )
                        {
                            g_string_printf( description, "[%s]: %s", field_config[i].id, field_config[i].name );
                            
                            struct NNCMS_VARIABLE option_tags[] =
                            {
                                { .name = "option_value", .value.string = field_config[i].id, .type = NNCMS_TYPE_STRING },
                                { .name = "option_desc", .value.string = description->str, .type = NNCMS_TYPE_STRING },
                                { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                                { .type = NNCMS_TYPE_NONE }
                            };

                            // Detect selected
                            if( field_value != NULL )
                            {
                                if( strcmp( field_value, field_config[i].id ) == 0 )
                                    option_tags[2].value.string = "selected";
                            }

                            template_hparse( req, "template_field_option.html", options_str, option_tags );
                        }
                        g_string_free( description, TRUE );
                        database_free_rows( (struct NNCMS_ROW *) field_config );
                    }
                    
                    // Put options between <select> ... </select>
                    field_data_tags[3].value.string = options_str->str;
                    template_hparse( req, "template_field_select.html", data_html, field_data_tags );
                    
                    g_string_free( options_str, TRUE );
                }
                else
                {
                    //
                    // Read-only field
                    //

                    if( field_value != NULL )
                    {
                        database_bind_text( req->stmt_find_field_config, 1, field_value );
                        struct NNCMS_FIELD_CONFIG_ROW *field_config = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_find_field_config );
                        field_data_tags[3].value.string = field_config->name;
                        template_hparse( req, "template_field_text.html", data_html, field_data_tags );
                        database_free_rows( (struct NNCMS_ROW *) field_config );
                    }
                    
                    break;

                }

                break;
            }
            
            case NNCMS_FIELD_LANGUAGE:
            {
                if( field->editable == true )
                {
                    //
                    // Editable field
                    //
                    GString *options_str = g_string_sized_new( 1024 );
                    
                    struct NNCMS_VARIABLE option_tags[] =
                    {
                        { .name = "option_value", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_desc", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };

                    struct NNCMS_ROW *lang_list = database_query( req, "select id, name, native, prefix from languages where enabled=1 order by weight asc" );
                    for( struct NNCMS_ROW *cur_row = lang_list; cur_row; cur_row = cur_row->next )
                    {
                        option_tags[0].value.string = cur_row->value[3];
                        option_tags[1].value.string = cur_row->value[2];
                        option_tags[2].value.string = NULL;
                        
                        // Detect selected
                        if( field_value != 0 )
                        {
                            if( strcmp( field_value, cur_row->value[3] ) == 0 )
                                option_tags[2].value.string = "selected";
                        }
                        else
                        {
                            if( strcmp( req->language, cur_row->value[3] ) == 0 )
                                option_tags[2].value.string = "selected";
                        }
                        
                        template_hparse( req, "template_field_option.html", options_str, option_tags );
                    }
                    database_free_rows( lang_list );
                    
                    // Put options between <select> ... </select>
                    field_data_tags[3].value.string = options_str->str;
                    template_hparse( req, "template_field_select.html", data_html, field_data_tags );
                    
                    g_string_free( options_str, TRUE );
                }
                else
                {
                    //
                    // Read-only field
                    //

                    char *lang_prefix = ( field_value ? field_value : req->language );

                    database_bind_text( req->stmt_find_language_by_prefix, 1, field_value );
                    struct NNCMS_LANGUAGE_ROW *lang_row = (struct NNCMS_LANGUAGE_ROW *) database_steps( req, req->stmt_find_language_by_prefix );
                    struct NNCMS_VARIABLE language_field_data_tags[] =
                    {
                        { .name = "field_text_name", .value = field_data_tags[0].value, .type = NNCMS_TYPE_STRING },
                        { .name = "field_text_description", .value = field_data_tags[1].value, .type = NNCMS_TYPE_STRING },
                        { .name = "field_name", .value = field_data_tags[2].value, .type = NNCMS_TYPE_STRING },
                        { .name = "field_value", .value.string = (lang_row ? lang_row->native : NULL ), .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };
                    
                    template_hparse( req, "template_field_help.html", data_html, language_field_data_tags );
                    
                    database_free_rows( (struct NNCMS_ROW *) lang_row );
                    
                    break;

                }

                break;
            }
            
            case NNCMS_FIELD_SELECT:
            {
               
                if( field->editable == true )
                {
                    //
                    // Editable field
                    //
                    GString *options_str = g_string_sized_new( 1024 );
                    
                    struct NNCMS_VARIABLE option_tags[] =
                    {
                        { .name = "option_value", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_desc", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .name = "option_selected", .value.string = 0, .type = NNCMS_TYPE_STRING },
                        { .type = NNCMS_TYPE_NONE }
                    };

                    struct NNCMS_SELECT_OPTIONS *options = (struct NNCMS_SELECT_OPTIONS *) field->data;
                    for( int i = 0; options != NULL && options->items != NULL && options->items[i].name != NULL; i++ )
                    {                    
                        option_tags[0].value.string = options->items[i].name;
                        option_tags[1].value.string = options->items[i].desc;
                        option_tags[2].value.string = NULL;

                        if( option_tags[1].value.string == NULL )
                        {
                            char i18n_name[32];
                            snprintf( i18n_name, sizeof(i18n_name), "option_%s_desc", options->items[i].name );
                            option_tags[1].value.string = i18n_string_temp( req, i18n_name, NULL  );
                        }

                        // Detect selected
                        if( (field_value != NULL && strcmp( options->items[i].name, field_value ) == 0) || options->items[i].selected == true )
                            option_tags[2].value.string = "selected";

                        template_hparse( req, "template_field_option.html", options_str, option_tags );
                    }
                    
                    // Put options between <select> ... </select>
                    field_data_tags[3].value.string = options_str->str;
                    template_hparse( req, "template_field_select.html", data_html, field_data_tags );
                    
                    g_string_free( options_str, TRUE );
                }
                else
                {
                    //
                    // Read-only field
                    //
                    
                    GString *temp = g_string_sized_new( 20 );

                    struct NNCMS_SELECT_OPTIONS *options = (struct NNCMS_SELECT_OPTIONS *) field->data;
                    for( int i = 0; options != NULL && options->items != NULL && options->items[i].name != NULL; i++ )
                    {
                        // Detect selected
                        if( (field_value != 0 && strcmp( options->items[i].name, field_value ) == 0) || options->items[i].selected == true )
                        {
                            struct NNCMS_VARIABLE option_field_data_tags[] =
                            {
                                { .name = "field_text_name", .value = field_data_tags[0].value, .type = NNCMS_TYPE_STRING },
                                { .name = "field_text_description", .value = field_value, .type = NNCMS_TYPE_STRING },
                                { .name = "field_name", .value.string = options->items[i].name, .type = NNCMS_TYPE_STRING },
                                { .name = "field_value", .value.string = options->items[i].desc, .type = NNCMS_TYPE_STRING },
                                { .name = "link", .value.string = NULL, .type = NNCMS_TYPE_STRING },
                                { .type = NNCMS_TYPE_NONE }
                            };
                            
                            if( options && options->link && options->link[0] != 0 )
                            {
                                g_string_assign( temp, "" );
                                template_sparse( req, field->text_name, options->link, temp, option_field_data_tags );
                                
                                option_field_data_tags[4].value.string = temp->str;
                                template_hparse( req, "template_field_textlink.html", data_html, option_field_data_tags );
                            }
                            else
                            {
                                template_hparse( req, "template_field_help.html", data_html, option_field_data_tags );
                            }

                            break;
                        }
                    }
                    
                    g_string_free( temp, true );
                }
                
                break;
            }
        } // end switch
    }

    // Template tags for fields
    struct NNCMS_VARIABLE field_tags[] =
    {
        { .name = "field_text_name", .value.string = field->text_name, .type = NNCMS_TYPE_STRING },
        { .name = "field_text_description", .value.string = field->text_description, .type = NNCMS_TYPE_STRING },
        { .name = "field_name", .value.string = field->name, .type = NNCMS_TYPE_STRING },
        { .name = "field_values", .value.string = data_html->str, .type = NNCMS_TYPE_STRING },
        { .name = "label_class", .value.string = label_class, .type = NNCMS_TYPE_STRING },
        { .name = "values_count", .value.integer = field->values_count, .type = NNCMS_TYPE_INTEGER },
        { .name = "editable", .value.boolean = field->editable, .type = NNCMS_TYPE_BOOL },
        { .type = NNCMS_TYPE_NONE }
    };
    template_hparse( req, "template_field_wrapper.html", buffer, field_tags );

    g_string_free( data_html, TRUE );
}

// #############################################################################

// Get data from $_GET array
void form_get_data( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field )
{
    for( unsigned int i = 0; field[i].type != NNCMS_FIELD_NONE; i++ )
    {
        if( field[i].editable == false )
            continue;
        
        char *httpVar = main_variable_get( req, req->get_tree, field[i].name );

        field[i].value = httpVar;
    }
}

// #############################################################################

// Get data from $_POST array
void form_post_data( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field )
{
    for( unsigned int i = 0; field[i].type != NNCMS_FIELD_NONE; i++ )
    {
        if( field[i].editable == false )
            continue;
            
        // Old
        char *httpVar = main_variable_get( req, req->post_tree, field[i].name );
        field[i].value = httpVar;

        // Convert single value to one multivalue
        GArray *gmultivalue = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD_VALUE) );
        garbage_add( req->loop_garbage, gmultivalue, MEMORY_GARBAGE_GARRAY_FREE );
        struct NNCMS_FIELD_VALUE *multivalue = field[i].multivalue;

        if( field[i].multivalue == NULL && field[i].values_count == 1 )
        {
            // Store existing value in dynamic array
            g_array_append_vals( gmultivalue, & (struct NNCMS_FIELD_VALUE)
            {
                .id = "0",
                .value = httpVar,
                .config = field[i].config,
                .weight = "0"
            }, 1 );
        }

        // If request method is POST, then the form might be subbmited to add new value
        //
        // Add existing values
        unsigned int k;
        for( k = 0; multivalue != NULL && multivalue[k].id != NULL; k = k + 1 )
        {
            // Prepare field name
            char *field_name = field[i].name; 
            char temp[FIELD_NAME_LEN_MAX + 10];
            if( field[i].values_count > 1 || field[i].values_count == -1 )
            {
                snprintf( temp, sizeof(temp), "%s[%i]", field[i].name, k );
                field_name = temp;
            }
            else if( k >= field[i].values_count )
            {
                break;
            }
            
            // Get value from POST data
            char *httpVar = main_variable_get( req, req->post_tree, field_name );
            
            // Store in dynamic array
            g_array_append_vals( gmultivalue, & (struct NNCMS_FIELD_VALUE)
            {
                .id = multivalue[k].id,
                .value = ( httpVar != NULL ? httpVar : multivalue[k].value ),
                .config = multivalue[k].config,
                .weight = multivalue[k].weight
            }, 1 );
        }
        
        // Add previously added values
        if( k < field[i].values_count || field[i].values_count == -1 )
        {
            while( 1 )
            {
                // Prepare field name
                char field_name[FIELD_NAME_LEN_MAX + 10];
                snprintf( field_name, sizeof(field_name), "%s[%i]", field[i].name, k );
                
                // Get value from POST data
                char *httpVar = main_variable_get( req, req->post_tree, field_name );
                
                if( httpVar != NULL )
                {
                    g_array_append_vals( gmultivalue, & (struct NNCMS_FIELD_VALUE)
                    {
                        .id = "0",
                        .value = httpVar,
                        .config = "0",
                        .weight = "0"
                    }, 1 );
                }
                else
                {
                    break;
                }

                k = k + 1;
            }
        }
        
        // Add empty value if "Add more" button was pressed
        char add_button[FIELD_NAME_LEN_MAX + 5];
        snprintf( add_button, sizeof(add_button), "%s_add", field[i].name );
        char *http_add_button = main_variable_get( req, req->post_tree, add_button );
        if( http_add_button != NULL )
        {
            g_array_append_vals( gmultivalue, & (struct NNCMS_FIELD_VALUE) { .id = "0", .value = "", .config = "", .weight = "0" }, 1 );
            log_printf( req, LOG_ACTION, "Field data added for field '%s'", field[i].name );
        }
        
        field[i].multivalue = (struct NNCMS_FIELD_VALUE *) gmultivalue->data;
    }
}

// #############################################################################

bool field_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *fields )
{
    bool result = true;

    // Iterate through every field and validate
    for( unsigned int i = 0; fields[i].type != NNCMS_FIELD_NONE; i++ )
    {
        if( fields[i].editable == false || fields[i].viewable == false )
            continue;

        // Should not be NULL
        if( fields[i].value == NULL )
        {
            fields[i].value = "";
        }

        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_name", .value.string = fields[i].name, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        // Size limit
        if( fields[i].size_max == 0 )
            fields[i].size_max = 10 * 1024 * 1024;
        bool size_validation = filter_size_limit( fields[i].value, fields[i].size_min, fields[i].size_max );
        if( size_validation == false )
        {
            result = false;
            struct NNCMS_VARIABLE vars_size[] =
            {
                { .name = "field_name", .value.string = fields[i].name, .type = NNCMS_TYPE_STRING },
                { .name = "size_min", .value.unsigned_integer = fields[i].size_min, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "size_max", .value.unsigned_integer = fields[i].size_max, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ERROR, "field_validate_size_limit", vars_size );
            continue;
        }
        
        // Invalid_characters
        if( fields[i].char_table != NULL )
        {
            bool table_validation = filter_table_validation( fields[i].value, fields[i].char_table );
            if( table_validation == false )
            {
                result = false;
                log_vdisplayf( req, LOG_ERROR, "field_validate_invalid_characters", vars );
            }
        }

        // Selected options
        switch( fields[i].type )
        {
            case NNCMS_FIELD_SELECT:
            {
                struct NNCMS_SELECT_OPTIONS *options = (struct NNCMS_SELECT_OPTIONS *) fields[i].data;
                if( options != NULL && options->items != NULL )
                {
                    bool option_validation = filter_option_validation( fields[i].value, options );
                    if( option_validation == false )
                    {
                        result = false;
                        log_vdisplayf( req, LOG_ERROR, "field_validate_options", vars );
                    }
                }
                break;
            }
            
            case NNCMS_FIELD_LANGUAGE:
            {
                if( fields[i].value != NULL )
                {
                    database_bind_text( req->stmt_find_language_by_prefix, 1, fields[i].value );
                    struct NNCMS_LANGUAGE_ROW *lang_row = (struct NNCMS_LANGUAGE_ROW *) database_steps( req, req->stmt_find_language_by_prefix );
                    if( lang_row != NULL )
                    {
                        database_free_rows( (struct NNCMS_ROW *) lang_row );
                    }
                    else
                    {
                        result = false;
                        log_vdisplayf( req, LOG_ERROR, "field_validate_invalid_language", vars );
                    }
                }
                else
                {
                    result = false;
                    log_vdisplayf( req, LOG_ERROR, "field_validate_invalid_language", vars );
                }
                break;
            }

            case NNCMS_FIELD_CHECKBOX:
            {
                if( fields[i].value != NULL )
                {
                    // Replace "on", "true" or anything else to just "0" or "1"
                    if( strcmp( fields[i].value, "on" ) == 0 ||
                        strcmp( fields[i].value, "true" ) == 0 ||
                        strcmp( fields[i].value, "1" ) == 0 )
                        fields[i].value = "1";
                    else
                        fields[i].value = "0";
                }
                else
                {
                    //fields[i].value = "0";
                }
                break;
            }
        }

    }
    
    return result;
}

// #############################################################################

void field_set_editable( struct NNCMS_FIELD *fields, bool state )
{
    for( unsigned int i = 0; fields[i].type != NNCMS_FIELD_NONE; i = i + 1 )
    {
        fields[i].editable = false;
    }
}

// #############################################################################
