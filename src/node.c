// #############################################################################
// Source file: node.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

/*
 *  Relationship between tables in the database
 *
 *  ┌─[ Figure 1 ]──────────────────────────┐
 *  │                                       │░
 *  │  node_type                            │░
 *  │    │   └──────────────────┐           │░
 *  │    ↓                      ↓           │░
 *  │  node                   field_config  │░
 *  │    │                      │           │░
 *  │    ↓                      │           │░
 *  │  node_rev                 │           │░
 *  │    │                      │           │░
 *  │    │   ┌──────────────────┘           │░
 *  │    ↓   ↓                              │░
 *  │  field_data                           │░
 *  │                                       │░
 *  └───────────────────────────────────────┘░
 *    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
 *
 *  Variable `rev_id` in `node` table is required for fast finding of
 *  the latest revision and for easy reverting.
 *
 *  Variable `user_id` in `node` shows who is the owner. In `rev` it
 *  shows who did the latest update.
 *
 *  <node_type id="1" name="page">
 *      <field_config id="1" name="tags" type="text" />
 *      <field_config id="2" name="body" type="text" />
 *
 *      <node id="1" user_id="2" language="ru" rev_id="1" published="1" promoted="1" sticky="0">
 *          <rev id="1" user_id="2" title="мышь" log_msg="" timestamp="yesterday">
 *              <data field_id="1">писк</data>
 *              <data field_id="2">фуу</data>
 *          </rev>
 *      </node>
 *      <node id="2" user_id="2" language="en" rev_id="2" published="1" promoted="1" sticky="0">
 *          <rev id="2" user_id="2" title="mouse" log_msg="" timestamp="yesterday">
 *              <data field_id="1">squeak</data>
 *              <data field_id="2">foo</data>
 *          </rev>
 *      </node>
 *      <node id="3" user_id="2" language="de" rev_id="4" published="0" promoted="1" sticky="0">
 *          <rev id="3" user_id="2" title="die Maus" log_msg="" timestamp="yesterday">
 *              <data field_id="1">fiepseln</data>
 *              <data field_id="2">föo</data>
 *          </rev>
 *          <rev id="4" user_id="1" title="der Mäuserich" log_msg="ersetzte die männliche Maus" timestamp="today">
 *              <data field_id="1">fieps</data>
 *              <data field_id="2">föö</data>
 *          </rev>
 *      </node>
 *  </node_type>
 */

// #############################################################################
// includes of local headers
//

#include "node.h"

#include "main.h"
#include "database.h"
#include "user.h"
#include "filter.h"
#include "template.h"
#include "acl.h"
#include "log.h"
#include "i18n.h"
#include "memory.h"
#include "view.h"

// #############################################################################
// includes of system headers
//

#include <glib.h>

// #############################################################################
// global variables
//

struct NNCMS_SELECT_ITEM field_types[] =
{
    { .name = "editbox", .desc = NULL, .selected = false },
    { .name = "textarea", .desc = NULL, .selected = false },
    { .name = "timedate", .desc = NULL, .selected = false },
    { .name = "checkbox", .desc = NULL, .selected = false },
    { .name = "select", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

struct NNCMS_VARIABLE field_type_list[] =
{
    { .name = "none", .value.integer = NNCMS_FIELD_NONE, .type = NNCMS_TYPE_INTEGER },
    { .name = "editbox", .value.integer = NNCMS_FIELD_EDITBOX, .type = NNCMS_TYPE_INTEGER },
    { .name = "textarea", .value.integer = NNCMS_FIELD_TEXTAREA, .type = NNCMS_TYPE_INTEGER },
    { .name = "timedate", .value.integer = NNCMS_FIELD_TIMEDATE, .type = NNCMS_TYPE_INTEGER },
    { .name = "checkbox", .value.integer = NNCMS_FIELD_CHECKBOX, .type = NNCMS_TYPE_INTEGER },
    { .name = "language", .value.integer = NNCMS_FIELD_LANGUAGE, .type = NNCMS_TYPE_INTEGER },
    { .name = "select", .value.integer = NNCMS_FIELD_SELECT, .type = NNCMS_TYPE_INTEGER },
    { .type = NNCMS_TYPE_NONE }
};

struct NNCMS_NODE_FIELDS
{
    struct NNCMS_FIELD node_id;
    struct NNCMS_FIELD node_title;
    struct NNCMS_FIELD node_authoring;
    struct NNCMS_FIELD node_type;
    struct NNCMS_FIELD node_language;
    struct NNCMS_FIELD node_user_id;
    struct NNCMS_FIELD node_rev_timestamp;
    struct NNCMS_FIELD node_log_msg;
    struct NNCMS_FIELD node_published;
    struct NNCMS_FIELD node_promoted;
    struct NNCMS_FIELD node_sticky;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD node_add;
    struct NNCMS_FIELD node_edit;
    struct NNCMS_FIELD none;
}
node_fields =
{
    { .name = "node_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = false, .text_name = NULL, .text_description = NULL },
    { .name = "node_title", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = false, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "node_authoring", .value = NULL, .data = NULL, .type = NNCMS_FIELD_AUTHORING, .values_count = 1, .editable = false, .viewable = false, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "node_type", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = false, .text_name = NULL, .text_description = NULL },
    { .name = "node_language", .value = NULL, .data = NULL, .type = NNCMS_FIELD_LANGUAGE, .values_count = 1, .editable = true, .viewable = false, .text_name = NULL, .text_description = NULL },
    { .name = "node_user_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = false, .viewable = false, .text_name = NULL, .text_description = NULL },
    { .name = "node_rev_timestamp", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = false, .text_name = NULL, .text_description = NULL },
    { .name = "node_log_msg", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = false, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "node_published", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = false, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "node_promoted", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = false, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "node_sticky", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = false, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = false, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = false, .text_name = "", .text_description = "" },
    { .name = "node_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "node_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_FIELD_CONFIG_FIELDS
{
    struct NNCMS_FIELD field_id;
    struct NNCMS_FIELD node_type_id;
    struct NNCMS_FIELD field_name;
    struct NNCMS_FIELD field_type;
    struct NNCMS_FIELD field_config;
    struct NNCMS_FIELD field_weight;
    struct NNCMS_FIELD field_label;
    struct NNCMS_FIELD field_empty_hide;
    struct NNCMS_FIELD field_values_count;
    struct NNCMS_FIELD field_size_min;
    struct NNCMS_FIELD field_size_max;
    struct NNCMS_FIELD field_char_table;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD field_add;
    struct NNCMS_FIELD field_edit;
    struct NNCMS_FIELD none;
}
field_config_fields =
{
    { .name = "field_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "node_type_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 100, .char_table = numeric_validate },
    { .name = "field_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = alphanum_underscore_validate },
    { .name = "field_type", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = field_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "field_config", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TEXTAREA, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "field_weight", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = numeric_validate },
    { .name = "field_label", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = label_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = alphanum_validate },
    { .name = "field_empty_hide", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = booleans }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = numeric_validate },
    { .name = "field_values_count", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "field_size_min", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = numeric_validate },
    { .name = "field_size_max", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = numeric_validate },
    { .name = "field_char_table", .value = NULL, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = char_tables }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000, .char_table = alphanum_underscore_validate },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "field_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "field_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

struct NNCMS_NODE_TYPE_FIELDS
{
    struct NNCMS_FIELD node_type_id;
    struct NNCMS_FIELD node_type_name;
    struct NNCMS_FIELD node_type_authoring;
    struct NNCMS_FIELD node_type_comments;
    struct NNCMS_FIELD referer;
    struct NNCMS_FIELD fkey;
    struct NNCMS_FIELD node_type_add;
    struct NNCMS_FIELD node_type_edit;
    struct NNCMS_FIELD none;
}
node_type_fields =
{
    { .name = "node_type_id", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
    { .name = "node_type_name", .value = NULL, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 1, .size_max = 1000, .char_table = alphanum_underscore_validate },
    { .name = "node_type_authoring", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "node_type_comments", .value = NULL, .data = NULL, .type = NNCMS_FIELD_CHECKBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
    { .name = "referer", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "fkey", .value = NULL, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
    { .name = "node_type_add", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .name = "node_type_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = false, .text_name = NULL, .text_description = "" },
    { .type = NNCMS_FIELD_NONE }
};

// #############################################################################
// function declarations

char *node_field_links( struct NNCMS_THREAD_INFO *req, char *node_id, char *node_rev_id, char *field_id, char *node_type_id );
char *node_type_links( struct NNCMS_THREAD_INFO *req, char *node_id, char *node_rev_id, char *field_id, char *node_type_id );
char *node_links( struct NNCMS_THREAD_INFO *req, char *node_id, char *node_rev_id, char *field_id, char *node_type_id );
bool field_config_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD_CONFIG_FIELDS *fields );

// #############################################################################
// functions

bool node_global_init( )
{
    main_local_init_add( &node_local_init );
    main_local_destroy_add( &node_local_destroy );

    main_page_add( "node_add", &node_add );
    main_page_add( "node_edit", &node_edit );
    main_page_add( "node_view", &node_view );
    main_page_add( "node_delete", &node_delete );
    main_page_add( "node_list", &node_list );
    main_page_add( "node_rev", &node_rev );
    main_page_add( "node_field_add", &node_field_add );
    main_page_add( "node_field_delete", &node_field_delete );
    main_page_add( "node_field_edit", &node_field_edit );
    main_page_add( "node_field_list", &node_field_list );
    main_page_add( "node_type_add", &node_type_add );
    main_page_add( "node_type_view", &node_type_view );
    main_page_add( "node_type_edit", &node_type_edit );
    main_page_add( "node_type_delete", &node_type_delete );
    main_page_add( "node_type_list", &node_type_list );

    return true;
}

// #############################################################################

bool node_global_destroy( )
{
    return true;
}

// #############################################################################

bool node_local_init( struct NNCMS_THREAD_INFO *req )
{
    // Prepared statements
    /*req->stmt_list_folders = database_prepare( req, "SELECT id, user_id, parent_node_id, name, type, title, description FROM nodes WHERE type='d'" );
    req->stmt_list_nodes = database_prepare( req, "SELECT id, user_id, parent_node_id, name, type, title, description FROM nodes WHERE id=?" );
    req->stmt_count_node_childs = database_prepare( req, "SELECT COUNT(*) FROM nodes WHERE parent_node_id=?" );
    req->stmt_list_node_childs = database_prepare( req, "SELECT id, user_id, parent_node_id, name, type, title, description FROM nodes WHERE parent_node_id=?" );
    req->stmt_find_node_by_id = database_prepare( req, "SELECT id, user_id, parent_node_id, name, type, title, description FROM nodes WHERE id=?" );
    req->stmt_add_node = database_prepare( req, "INSERT INTO nodes (id, user_id, parent_node_id, name, type, title, description) VALUES(null, ?, ?, ?, ?, ?, ?)" );
    req->stmt_edit_node = database_prepare( req, "UPDATE nodes SET user_id=?, parent_node_id=?, name=?, type=?, title=?, description=? WHERE id=?" );*/
    
    req->stmt_add_field_config = database_prepare( req, "INSERT INTO field_config (id, node_type_id, name, type, config, weight, label, empty_hide, values_count, size_min, size_max, char_table) VALUES(null, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)" );
    req->stmt_update_field_config = database_prepare( req, "UPDATE field_config SET node_type_id=?, name=?, type=?, config=?, weight=?, label=?, empty_hide=?, values_count=?, size_min=?, size_max=?, char_table=? WHERE id=?" );
    req->stmt_delete_field_config = database_prepare( req, "DELETE FROM field_config WHERE id=?" );
    req->stmt_count_field_config = database_prepare( req, "SELECT COUNT(*) FROM field_config WHERE node_type_id=?" );
    req->stmt_list_field_config = database_prepare( req, "SELECT id, node_type_id, name, type, config, weight, label, empty_hide, values_count, size_min, size_max, char_table FROM field_config WHERE node_type_id=? ORDER BY weight LIMIT ? OFFSET ?" );
    req->stmt_list_all_field_config = database_prepare( req, "SELECT id, node_type_id, name, type, config, weight, label, empty_hide, values_count, size_min, size_max, char_table FROM field_config ORDER BY weight LIMIT ? OFFSET ?" );
    req->stmt_find_field_config = database_prepare( req, "SELECT id, node_type_id, name, type, config, weight, label, empty_hide, values_count, size_min, size_max, char_table FROM field_config WHERE id=?" );
    
    req->stmt_add_node = database_prepare( req, "INSERT INTO node (id, user_id, node_type_id, language, node_rev_id, published, promoted, sticky) VALUES(null, ?, ?, ?, ?, ?, ?, ?)" );
    req->stmt_delete_node = database_prepare( req, "DELETE FROM node WHERE id=?" );
    req->stmt_count_node = database_prepare( req, "SELECT COUNT(*) FROM node WHERE node_type_id=?" );
    req->stmt_list_node = database_prepare( req, "SELECT id, user_id, node_type_id, language, node_rev_id, published, promoted, sticky FROM node WHERE node_type_id=? LIMIT ? OFFSET ?" );    
    req->stmt_find_node = database_prepare( req, "SELECT id, user_id, node_type_id, language, node_rev_id, published, promoted, sticky FROM node WHERE id=?" );
    req->stmt_edit_node = database_prepare( req, "UPDATE node SET user_id=?, node_type_id=?, language=?, node_rev_id=?, published=?, promoted=?, sticky=? WHERE id=?" );
    req->stmt_update_node_node_rev_id = database_prepare( req, "UPDATE node SET node_rev_id=? WHERE id=?" );
    
    req->stmt_add_node_rev = database_prepare( req, "INSERT INTO node_rev (id, node_id, user_id, title, timestamp, log_msg) VALUES(null, ?, ?, ?, ?, ?)" );
    req->stmt_delete_node_rev = database_prepare( req, "DELETE FROM node_rev WHERE id=?" );
    req->stmt_count_node_rev = database_prepare( req, "SELECT COUNT(*) FROM node_rev WHERE node_id=?" );
    req->stmt_list_node_rev = database_prepare( req, "SELECT id, node_id, user_id, title, timestamp, log_msg FROM node_rev WHERE node_id=? LIMIT ? OFFSET ?" );
    req->stmt_find_node_rev = database_prepare( req, "SELECT id, node_id, user_id, title, timestamp, log_msg FROM node_rev WHERE id=?" );
    
    req->stmt_add_field_data = database_prepare( req, "INSERT INTO field_data (id, node_rev_id, field_id, data, config, weight) VALUES(null, ?, ?, ?, ?, ?)" );
    req->stmt_delete_field_data_by_field_id = database_prepare( req, "DELETE FROM field_data WHERE field_id=?" );
    req->stmt_delete_field_data_by_node_rev_id = database_prepare( req, "DELETE FROM field_data WHERE node_rev_id=?" );
    req->stmt_find_field_data = database_prepare( req, "SELECT id, node_rev_id, field_id, data, config, weight FROM field_data WHERE node_rev_id=? AND field_id=?" );

    req->stmt_list_node_type = database_prepare( req, "SELECT id, name, authoring, comments FROM node_type LIMIT ? OFFSET ?" );
    req->stmt_count_node_type = database_prepare( req, "SELECT COUNT(*) FROM node_type" );
    req->stmt_find_node_type = database_prepare( req, "SELECT id, name, authoring, comments FROM node_type WHERE id=?" );
    req->stmt_add_node_type = database_prepare( req, "INSERT INTO node_type (id, name, authoring, comments) VALUES(null, ?, ?, ?)" );
    req->stmt_edit_node_type = database_prepare( req, "UPDATE node_type SET name=?, authoring=?, comments=? WHERE id=?" );
    req->stmt_delete_node_type = database_prepare( req, "DELETE FROM node_type WHERE id=?" );

    return true;
}

// #############################################################################

bool node_local_destroy( struct NNCMS_THREAD_INFO *req )
{
    // Free prepared statements
    database_finalize( req, req->stmt_add_field_config );
    database_finalize( req, req->stmt_update_field_config );
    database_finalize( req, req->stmt_delete_field_config );
    database_finalize( req, req->stmt_count_field_config );
    database_finalize( req, req->stmt_list_field_config );
    database_finalize( req, req->stmt_list_all_field_config );
    database_finalize( req, req->stmt_find_field_config );

    database_finalize( req, req->stmt_add_node );
    database_finalize( req, req->stmt_delete_node );
    database_finalize( req, req->stmt_count_node );
    database_finalize( req, req->stmt_list_node );
    database_finalize( req, req->stmt_edit_node );
    database_finalize( req, req->stmt_find_node );
    database_finalize( req, req->stmt_update_node_node_rev_id );

    database_finalize( req, req->stmt_add_node_rev );
    database_finalize( req, req->stmt_delete_node_rev );
    database_finalize( req, req->stmt_count_node_rev );
    database_finalize( req, req->stmt_list_node_rev );
    database_finalize( req, req->stmt_find_node_rev );

    database_finalize( req, req->stmt_add_field_data );
    database_finalize( req, req->stmt_delete_field_data_by_field_id );
    database_finalize( req, req->stmt_delete_field_data_by_node_rev_id );
    database_finalize( req, req->stmt_find_field_data );

    database_finalize( req, req->stmt_find_node_type );
    database_finalize( req, req->stmt_list_node_type );
    database_finalize( req, req->stmt_count_node_type );
    database_finalize( req, req->stmt_add_node_type );
    database_finalize( req, req->stmt_edit_node_type );
    database_finalize( req, req->stmt_delete_node_type );

    return true;
}

// #############################################################################

struct NNCMS_NODE *node_load( struct NNCMS_THREAD_INFO *req, char *node_id )
{
    // Find node
    database_bind_text( req->stmt_find_node, 1, node_id );
    struct NNCMS_NODE_ROW *node_row = (struct NNCMS_NODE_ROW *) database_steps( req, req->stmt_find_node );
    if( node_row == NULL )
        return NULL;

    // Find node type
    database_bind_text( req->stmt_find_node_type, 1, node_row->node_type_id );
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_steps( req, req->stmt_find_node_type );
    if( node_type_row == NULL )
    {
        database_free_rows( (struct NNCMS_ROW *) node_row );
        return NULL;
    }

    // Get latest revision
    database_bind_text( req->stmt_find_node_rev, 1, node_row->node_rev_id );
    struct NNCMS_NODE_REV_ROW *node_rev_row = (struct NNCMS_NODE_REV_ROW *) database_steps( req, req->stmt_find_node_rev );
    if( node_rev_row == NULL )
    {
        log_printf( req, LOG_ERROR, "Latest revision for node (id = %s) not found", node_id );
        database_free_rows( (struct NNCMS_ROW *) node_row );
        database_free_rows( (struct NNCMS_ROW *) node_type_row );
        return NULL;
    }

    // Get fields list
    database_bind_text( req->stmt_list_field_config, 1, node_row->node_type_id );
    database_bind_int( req->stmt_list_field_config, 2, -1 );
    database_bind_int( req->stmt_list_field_config, 3, 0 );
    struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_list_field_config );
    if( field_config_row == NULL )
    {
        log_printf( req, LOG_ERROR, "Field config for node '%s' (id = %s) not found", node_rev_row->title, node_id );
        database_free_rows( (struct NNCMS_ROW *) node_row );
        database_free_rows( (struct NNCMS_ROW *) node_type_row );
        database_free_rows( (struct NNCMS_ROW *) node_rev_row );
        return NULL;
    }

    // Prepare node array
    struct NNCMS_NODE *node = MALLOC( sizeof(struct NNCMS_NODE) );
    node->id = node_id;
    node->title = node_rev_row->title;
    node->language = node_row->language;
    node->user_id = node_row->user_id;
    node->type_id = node_row->node_type_id;
    node->type_name = node_type_row->name;
    node->rev_id = node_rev_row->id;
    node->rev_timestamp = node_rev_row->timestamp;
    node->rev_user_id = node_rev_row->user_id;
    node->rev_log_msg = node_rev_row->log_msg;
    node->db_data = g_ptr_array_new_with_free_func( (GDestroyNotify) database_free_rows );

    // Prepare array for fields
    GArray *field_array = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );

    // Load data into form
    for( unsigned int i = 0; field_config_row != NULL && field_config_row[i].id != NULL; i = i + 1 )
    {
        // Get field data
        database_bind_text( req->stmt_find_field_data, 1, node_rev_row->id );
        database_bind_text( req->stmt_find_field_data, 2, field_config_row[i].id );
        struct NNCMS_FIELD_DATA_ROW *field_data_row = (struct NNCMS_FIELD_DATA_ROW *) database_steps( req, req->stmt_find_field_data );
        
        // Fill field structure
        struct NNCMS_FIELD field;
        field_load_config( req, &field_config_row[i], &field );
        field_load_data( req, field_data_row, &field );

        // Add field to array
        g_array_append_vals( field_array, &field, 1 );

        // Free memory from query result... in future
        g_ptr_array_add( node->db_data, field_data_row );
    }

    // Store array in node
    node->fields_count = field_array->len;
    node->fields = (struct NNCMS_FIELD *) g_array_free( field_array, FALSE );

    // Free memory from query result
    node->field_config_row = field_config_row;
    node->node_rev_row = node_rev_row;
    node->node_type_row = node_type_row;
    node->node_row = node_row;

    return node;
}

// #############################################################################

void node_free( struct NNCMS_NODE *node )
{
    //for( unsigned int i = 0; node->fields[i].type != NNCMS_FIELD_NONE; i = i + 1 )
    //{
    //    g_free( node->fields[i].multivalue );
    //}
    g_free( node->fields );
    g_ptr_array_free( node->db_data, TRUE );
    database_free_rows( (struct NNCMS_ROW *) node->field_config_row );
    database_free_rows( (struct NNCMS_ROW *) node->node_rev_row );
    database_free_rows( (struct NNCMS_ROW *) node->node_type_row );
    database_free_rows( (struct NNCMS_ROW *) node->node_row );
    FREE( node );
}

// #############################################################################

void node_view( struct NNCMS_THREAD_INFO *req )
{

    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get id
    char *node_id = main_variable_get( req, req->get_tree, "node_id" );
    if( node_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }
    char *node_rev_id = main_variable_get( req, req->get_tree, "rev_id" );

    // Find row associated with our object by 'id'
    struct NNCMS_NODE *node_info = node_load( req, node_id );
    if( node_info == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_info, MEMORY_GARBAGE_NODE_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
            { .name = "node_title", .value.string = node_info->title, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "node_view_header", vars );


    // Generate links
    char *links = node_links( req, node_info->id, NULL, NULL, NULL );
    char *node_type_links = node_links( req, NULL, NULL, NULL, node_info->type_id );

    //
    // View node form
    //
    struct NNCMS_NODE_FIELDS *fields = memdup_temp( req, &node_fields, sizeof(node_fields) );
    field_set_editable( (struct NNCMS_FIELD *) fields, false );
    fields->node_title.viewable = false;
    fields->node_authoring.viewable = true;
    fields->node_type.viewable = false;
    fields->node_language.viewable = true;
    fields->node_user_id.viewable = false;
    fields->node_rev_timestamp.viewable = false;
    fields->node_log_msg.viewable = false;
    
    struct NNCMS_AUTHORING_DATA authoring_data =
    {
        .user_id = node_info->user_id,
        .timestamp = node_info->rev_timestamp
    };
    fields->node_title.value = node_info->title;
    fields->node_authoring.data = &authoring_data;
    fields->node_type.value = node_info->type_name;
    fields->node_language.value = node_info->language;
    fields->node_user_id.value = node_info->user_id;
    fields->node_rev_timestamp.value = node_info->rev_timestamp;
    
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->referer.viewable = true;
    fields->fkey.viewable = true;

    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );
    //g_array_append_vals( gfields, &(fields->node_title), 1 );
    //g_array_append_vals( gfields, &(fields->node_authoring), 1 );
    g_array_append_vals( gfields, &(fields->node_type), 1 );
    g_array_append_vals( gfields, &(fields->node_language), 1 );
    g_array_append_vals( gfields, &(fields->node_user_id), 1 );
    g_array_append_vals( gfields, &(fields->node_rev_timestamp), 1 );

    //
    // Copy node fields
    //
    for( unsigned int i = 0; i < node_info->fields_count; i = i + 1 )
    {
        node_info->fields[i].editable = false;
        g_array_append_vals( gfields, &node_info->fields[i], 1 );
    }

    struct NNCMS_FORM form =
    {
        .name = "node_view", .action = NULL, .method = NULL,
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) gfields->data
    };

    // Prepare form according to view and field_view tables
    view_prepare_fields( req, form.fields, "1" );

    // Html output
    char *node_html = form_html( req, &form );

    // Specify values for template
    struct NNCMS_VARIABLE node_template[] =
        {
            { .name = "header", .value.string = NULL, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = node_html, .type = NNCMS_TYPE_STRING },
            { .name = "authoring", .value.string = node_info->node_type_row->authoring, .type = NNCMS_TYPE_STRING },
            { .name = "node_id", .value.string = node_info->node_row->id, .type = NNCMS_TYPE_STRING },
            { .name = "user_id", .value.string = node_info->node_row->user_id, .type = TEMPLATE_TYPE_USER },
            { .name = "timestamp", .value.string = node_info->node_rev_row->timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP },
            { .name = "links", .value.string = links, .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    g_string_assign( req->temp, "" );
    template_hparse( req, "node.html", req->temp, node_template );

    // Specify values for template
    struct NNCMS_VARIABLE frame_template[] =
        {
            { .name = "header", .value.string = header_str, .type = NNCMS_TYPE_STRING },
            { .name = "content", .value.string = req->temp->str, .type = NNCMS_TYPE_STRING },
            { .name = "icon", .value.string = "images/categories/applications-internet.png", .type = NNCMS_TYPE_STRING },
            { .name = "links", .value.string = node_type_links, .type = NNCMS_TYPE_STRING },
            { .name = "homeURL", .value.string = homeURL, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );
}

// #################################################################################

void node_delete( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "delete" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get id
    char *node_id = main_variable_get( req, req->get_tree, "node_id" );
    if( node_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'id'
    struct NNCMS_NODE *node_info = node_load( req, node_id );
    if( node_info == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_info, MEMORY_GARBAGE_NODE_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_id", .value.string = node_info->id, .type = NNCMS_TYPE_STRING },
        { .name = "node_title", .value.string = node_info->title, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_delete_header", vars );

    //
    // Submit button pressed?
    //
    char *delete_submit = main_variable_get( req, req->post_tree, "delete_submit" );
    if( delete_submit )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }
        
        // TODO: Iterate all revisions associated with this node and
        // delete the field data

        // Delete the node
        database_bind_text( req->stmt_delete_node, 1, node_id );
        database_steps( req, req->stmt_delete_node );
        
        // Loop through all node revisions
        database_bind_text( req->stmt_list_node_rev, 1, node_id );
        database_bind_int( req->stmt_list_node_rev, 2, -1 ); // LIMIT
        database_bind_int( req->stmt_list_node_rev, 3, 0 ); // OFFSET
        struct NNCMS_NODE_REV_ROW *node_rev = (struct NNCMS_NODE_REV_ROW *) database_steps( req, req->stmt_list_node_rev );
        if( node_rev != NULL )
        {
            garbage_add( req->loop_garbage, node_rev, MEMORY_GARBAGE_DB_FREE );
            for( unsigned int i = 0; node_rev[i].id != NULL; i = i + 1 )
            {
                // Delete data
                database_bind_text( req->stmt_delete_field_data_by_node_rev_id, 1, node_rev[i].id );
                database_steps( req, req->stmt_delete_field_data_by_node_rev_id );
                
                // Delete node rev
                database_bind_text( req->stmt_delete_node_rev, 1, node_rev[i].id );
                database_steps( req, req->stmt_delete_node_rev );
            }
        }

        log_vdisplayf( req, LOG_ACTION, "node_delete_success", vars );
        log_printf( req, LOG_ACTION, "Node '%s' was deleted", node_info->title );
        redirect_to_referer( req );

        return;
    }

    // Generate links
    char *links = node_links( req, node_info->id, NULL, NULL, node_info->type_id );

    struct NNCMS_FORM *node_form = template_confirm( req, node_info->title );

    // Html output
    char *html = form_html( req, node_form );

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

// #################################################################################

void node_add( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Get fields list
    database_bind_text( req->stmt_list_field_config, 1, node_type_id );
    database_bind_int( req->stmt_list_field_config, 2, -1 ); // LIMIT
    database_bind_int( req->stmt_list_field_config, 3, -1 ); // OFFSET
    struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_list_field_config );
    if( field_config_row == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, field_config_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_add_header", vars );

    //
    // Node add form
    //
    struct NNCMS_NODE_FIELDS *fields = memdup_temp( req, &node_fields, sizeof(node_fields) );
    fields->node_title.viewable = true;
    fields->node_language.viewable = true;
    fields->node_user_id.viewable = true;
    fields->node_published.viewable = true;
    fields->node_promoted.viewable = true;
    fields->node_sticky.viewable = true;
    fields->node_language.value = req->language;
    fields->node_user_id.value = req->user_id;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->referer.viewable = true;
    fields->fkey.viewable = true;
    fields->node_add.viewable = true;

    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );
    g_array_append_vals( gfields, &(fields->node_title), 1 );
    g_array_append_vals( gfields, &(fields->node_language), 1 );
    g_array_append_vals( gfields, &(fields->node_user_id), 1 );
    g_array_append_vals( gfields, &(fields->node_published), 1 );
    g_array_append_vals( gfields, &(fields->node_promoted), 1 );
    g_array_append_vals( gfields, &(fields->node_sticky), 1 );

    // Load data into form
    for( unsigned int i = 0; field_config_row != NULL && field_config_row[i].id != NULL; i = i + 1 )
    {
        // Fill field structure
        struct NNCMS_FIELD field;
        field_load_config( req, &field_config_row[i], &field );
        field.value = NULL;
        field.multivalue = NULL;

        g_array_append_vals( gfields, &field, 1 );
    }
    g_array_append_vals( gfields, &(fields->referer), 1 );
    g_array_append_vals( gfields, &(fields->fkey), 1 );
    g_array_append_vals( gfields, &(fields->node_add), 1 );

    //
    // Actual node edit
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "node_add" );
    if( httpVarEdit )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Get POST data
        form_post_data( req, (struct NNCMS_FIELD *) gfields->data );
        
        // Validate
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) gfields->data );
        if( field_valid == true )
        {
            struct NNCMS_FIELD *field_node_title = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_title" );
            struct NNCMS_FIELD *field_node_language = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_language" );
            struct NNCMS_FIELD *field_node_published = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_published" );
            struct NNCMS_FIELD *field_node_promoted = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_promoted" );
            struct NNCMS_FIELD *field_node_sticky = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_sticky" );

            // Add the node
            database_bind_text( req->stmt_add_node, 1, req->user_id ); // user_id
            database_bind_text( req->stmt_add_node, 2, node_type_id ); // node_type_id
            database_bind_text( req->stmt_add_node, 3, field_node_language->value ); // language
            database_bind_text( req->stmt_add_node, 4, "" ); // node_rev_id
            database_bind_text( req->stmt_add_node, 5, field_node_published->value );
            database_bind_text( req->stmt_add_node, 6, field_node_promoted->value );
            database_bind_text( req->stmt_add_node, 7, field_node_sticky->value );
            database_steps( req, req->stmt_add_node );
            unsigned int node_id = database_last_rowid( req );

            // Add revision
            time_t raw_time = time( 0 );
            database_bind_int( req->stmt_add_node_rev, 1, node_id ); // node_id
            database_bind_text( req->stmt_add_node_rev, 2, req->user_id ); // user_id
            database_bind_text( req->stmt_add_node_rev, 3, field_node_title->value ); // title
            database_bind_int( req->stmt_add_node_rev, 4, raw_time ); // timestamp
            database_bind_text( req->stmt_add_node_rev, 5, "" ); // log_msg
            database_steps( req, req->stmt_add_node_rev );
            unsigned int rev_id = database_last_rowid( req );

            // Update the node to make it pointing to the latest revision
            database_bind_int( req->stmt_update_node_node_rev_id, 1, rev_id ); // node_rev_id
            database_bind_int( req->stmt_update_node_node_rev_id, 2, node_id ); // where id
            database_steps( req, req->stmt_update_node_node_rev_id );

            //
            // Store field data
            //
            for( unsigned int i = 0; field_config_row != NULL && field_config_row[i].id != NULL; i = i + 1 )
            {
                struct NNCMS_FIELD *field_data = field_find( req, (struct NNCMS_FIELD *) gfields->data, field_config_row[i].name );
                //char *httpVarData = main_variable_get( req, req->post_tree, field_config_row[i].name );
                if( field_data != NULL && field_data->value != NULL )
                {
                    for( unsigned int j = 0; field_data->multivalue[j].value != NULL && field_data->multivalue[j].value[0] != 0; j = j + 1 )
                    {
                        database_bind_int( req->stmt_add_field_data, 1, rev_id ); // node_rev_id
                        database_bind_text( req->stmt_add_field_data, 2, field_config_row[i].id ); // field_id
                        database_bind_text( req->stmt_add_field_data, 3, field_data->multivalue[j].value ); // data
                        database_steps( req, req->stmt_add_field_data );
                    }
                    /*database_bind_int( req->stmt_add_field_data, 1, rev_id ); // node_rev_id
                    database_bind_text( req->stmt_add_field_data, 2, field_config_row[i].id ); // field_id
                    database_bind_text( req->stmt_add_field_data, 3, field_data->value ); // data
                    database_steps( req, req->stmt_add_field_data );*/
                }
            }

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "node_id", .value.unsigned_integer = node_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
                { .name = "node_rev_id", .value.unsigned_integer = rev_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "node_add_success", vars  );
            log_printf( req, LOG_ACTION, "Node '%u' was added (rev_id = %u)", node_id, rev_id );
            
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = node_links( req, NULL, NULL, NULL, node_type_id );

    struct NNCMS_FORM form =
    {
        .name = "node_add", .action = main_temp_link( req, "node_add", NULL, vars ), .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) gfields->data
    };

    // Html output
    char *html = form_html( req, &form );

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

// #################################################################################

void node_edit( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get id
    char *node_id = main_variable_get( req, req->get_tree, "node_id" );
    if( node_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'id'
    struct NNCMS_NODE *node_info = node_load( req, node_id );
    if( node_info == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_info, MEMORY_GARBAGE_NODE_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
        { .name = "node_title", .value.string = node_info->title, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_edit_header", vars );

    //
    // Node edit form
    //
    struct NNCMS_NODE_FIELDS *fields = memdup_temp( req, &node_fields, sizeof(node_fields) );
    fields->node_title.viewable = true;
    fields->node_type.viewable = true;
    fields->node_language.viewable = true;
    fields->node_user_id.viewable = true;
    fields->node_rev_timestamp.viewable = true;
    fields->node_log_msg.viewable = true;
    fields->node_published.viewable = true;
    fields->node_promoted.viewable = true;
    fields->node_sticky.viewable = true;
    
    fields->node_title.value = node_info->title;
    fields->node_type.value = node_info->type_name;
    fields->node_language.value = node_info->language;
    fields->node_user_id.value = node_info->user_id;
    fields->node_rev_timestamp.value = node_info->rev_timestamp;
    fields->node_published.value = node_info->node_row->published;
    fields->node_promoted.value = node_info->node_row->promoted;
    fields->node_sticky.value = node_info->node_row->sticky;
    
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->referer.viewable = true;
    fields->fkey.viewable = true;
    
    fields->node_edit.viewable = true;

    GArray *gfields = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_FIELD) );
    garbage_add( req->loop_garbage, gfields, MEMORY_GARBAGE_GARRAY_FREE );
    g_array_append_vals( gfields, &(fields->node_title), 1 );
    g_array_append_vals( gfields, &(fields->node_type), 1 );
    g_array_append_vals( gfields, &(fields->node_language), 1 );
    g_array_append_vals( gfields, &(fields->node_user_id), 1 );
    g_array_append_vals( gfields, &(fields->node_rev_timestamp), 1 );

    // Node fields
    g_array_append_vals( gfields, node_info->fields, node_info->fields_count );

    g_array_append_vals( gfields, &(fields->node_published), 1 );
    g_array_append_vals( gfields, &(fields->node_promoted), 1 );
    g_array_append_vals( gfields, &(fields->node_sticky), 1 );
    g_array_append_vals( gfields, &(fields->node_log_msg), 1 );
    g_array_append_vals( gfields, &(fields->referer), 1 );
    g_array_append_vals( gfields, &(fields->fkey), 1 );
    g_array_append_vals( gfields, &(fields->node_edit), 1 );

    //
    // Actual node edit
    //
    char *httpVarEdit = main_variable_get( req, req->post_tree, "node_edit" );
    if( httpVarEdit )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            node_free( node_info );
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Get POST data
        form_post_data( req, (struct NNCMS_FIELD *) gfields->data );
        
        // Validate
        bool field_valid = field_validate( req, (struct NNCMS_FIELD *) gfields->data );
        if( field_valid == true )
        {
            struct NNCMS_FIELD *field_node_title = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_title" );
            struct NNCMS_FIELD *field_node_language = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_language" );
            struct NNCMS_FIELD *field_node_log_msg = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_log_msg" );
            struct NNCMS_FIELD *field_node_published = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_published" );
            struct NNCMS_FIELD *field_node_promoted = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_promoted" );
            struct NNCMS_FIELD *field_node_sticky = field_find( req, (struct NNCMS_FIELD *) gfields->data, "node_sticky" );

            // Add revision
            time_t raw_time = time( 0 );
            database_bind_text( req->stmt_add_node_rev, 1, node_id ); // node_id
            database_bind_text( req->stmt_add_node_rev, 2, req->user_id ); // user_id
            database_bind_text( req->stmt_add_node_rev, 3, field_node_title->value ); // title
            database_bind_int( req->stmt_add_node_rev, 4, raw_time ); // timestamp
            database_bind_text( req->stmt_add_node_rev, 5, field_node_log_msg->value ); // log_msg
            database_steps( req, req->stmt_add_node_rev );
            unsigned int rev_id = database_last_rowid( req );

            // Update the node to make it pointing to the latest revision
            //database_bind_int( req->stmt_update_node_node_rev_id, 1, rev_id ); // node_rev_id
            //database_bind_text( req->stmt_update_node_node_rev_id, 2, node_id ); // where id
            //database_steps( req, req->stmt_update_node_node_rev_id );
            
            database_bind_text( req->stmt_edit_node, 1, fields->node_user_id.value );
            database_bind_text( req->stmt_edit_node, 2, node_info->type_id );
            database_bind_text( req->stmt_edit_node, 3, field_node_language->value );
            database_bind_int( req->stmt_edit_node, 4, rev_id );
            database_bind_text( req->stmt_edit_node, 5, field_node_published->value );
            database_bind_text( req->stmt_edit_node, 6, field_node_promoted->value );
            database_bind_text( req->stmt_edit_node, 7, field_node_sticky->value );
            database_bind_text( req->stmt_edit_node, 8, node_id );
            database_steps( req, req->stmt_edit_node );

            //
            // Store field data
            //
            struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) (node_info->field_config_row);
            for( unsigned int i = 0; field_config_row != NULL && field_config_row[i].id != NULL; i = i + 1 )
            {
                struct NNCMS_FIELD *field_data = field_find( req, (struct NNCMS_FIELD *) gfields->data, field_config_row[i].name );
                if( field_data != NULL && field_data->multivalue != NULL )
                {
                    for( unsigned int j = 0; field_data->multivalue[j].value != NULL && field_data->multivalue[j].value[0] != 0; j = j + 1 )
                    {
                        database_bind_int( req->stmt_add_field_data, 1, rev_id ); // node_rev_id
                        database_bind_text( req->stmt_add_field_data, 2, field_config_row[i].id ); // field_id
                        database_bind_text( req->stmt_add_field_data, 3, field_data->multivalue[j].value ); // data
                        database_steps( req, req->stmt_add_field_data );
                    }
                }
                /*if( field_data != NULL && field_data->value != NULL )
                {
                    database_bind_int( req->stmt_add_field_data, 1, rev_id ); // node_rev_id
                    database_bind_text( req->stmt_add_field_data, 2, field_config_row[i].id ); // field_id
                    database_bind_text( req->stmt_add_field_data, 3, field_data->value ); // data
                    database_steps( req, req->stmt_add_field_data );
                }*/
            }

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
                { .name = "node_type_id", .value.string = node_info->type_id, .type = NNCMS_TYPE_STRING },
                { .name = "node_rev_id", .value.unsigned_integer = rev_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE }
            };
            log_vdisplayf( req, LOG_ACTION, "node_edit_success", vars  );
            log_printf( req, LOG_ACTION, "Node '%s' was updated (rev_id = %u)", node_id, rev_id );

            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = node_links( req, node_info->id, NULL, NULL, node_info->type_id );

    struct NNCMS_FORM form =
    {
        .name = "node_edit", .action = main_temp_link( req, "node_edit", NULL, vars ), .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) gfields->data
    };

    // Html output
    char *html = form_html( req, &form );

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

// #################################################################################

void node_field_edit( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "field_edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get id
    char *field_id = main_variable_get( req, req->get_tree, "field_id" );
    if( field_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Get node type id from the field
    database_bind_text( req->stmt_find_field_config, 1, field_id );
    struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_find_field_config );
    if( field_config_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, field_config_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_id", .value.string = field_id, .type = NNCMS_TYPE_STRING },
            { .name = "node_type_id", .value.string = field_config_row->node_type_id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "node_field_edit_header", vars );

    // Generate links
    char *links = node_field_links( req, NULL, NULL, field_id, field_config_row->node_type_id );

    //
    // Field edit form
    //
    struct NNCMS_FIELD_CONFIG_FIELDS *fields = memdup_temp( req, &field_config_fields, sizeof(field_config_fields) );
    fields->field_id.value = field_id;
    fields->node_type_id.value = field_config_row->node_type_id;
    fields->field_name.value = field_config_row->name;
    fields->field_type.value = field_config_row->type;
    fields->field_config.value = field_config_row->config;
    fields->field_weight.value = field_config_row->weight;
    fields->field_label.value = field_config_row->label;
    fields->field_empty_hide.value = field_config_row->empty_hide;
    fields->field_values_count.value = field_config_row->values_count;
    fields->field_size_min.value = field_config_row->size_min;
    fields->field_size_max.value = field_config_row->size_max;
    fields->field_char_table.value = field_config_row->char_table;
    
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->referer.viewable = true;
    fields->fkey.viewable = true;
    
    fields->field_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "field_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Actual node edit
    //
    char *field_edit = main_variable_get( req, req->post_tree, "field_edit" );
    if( field_edit )
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
        bool field_config_valid = field_config_validate( req, fields );
        if( field_valid == true && field_config_valid == true )
        {
            // Edit the field
            database_bind_text( req->stmt_update_field_config, 1, fields->node_type_id.value );
            database_bind_text( req->stmt_update_field_config, 2, fields->field_name.value );
            database_bind_text( req->stmt_update_field_config, 3, fields->field_type.value );
            database_bind_text( req->stmt_update_field_config, 4, fields->field_config.value );
            database_bind_text( req->stmt_update_field_config, 5, fields->field_weight.value );
            database_bind_text( req->stmt_update_field_config, 6, fields->field_label.value );
            database_bind_text( req->stmt_update_field_config, 7, fields->field_empty_hide.value );
            database_bind_text( req->stmt_update_field_config, 8, fields->field_values_count.value );
            database_bind_text( req->stmt_update_field_config, 9, fields->field_size_min.value );
            database_bind_text( req->stmt_update_field_config, 10, fields->field_size_max.value );
            database_bind_text( req->stmt_update_field_config, 11, fields->field_char_table.value );
            database_bind_text( req->stmt_update_field_config, 12, field_id );
            database_steps( req, req->stmt_update_field_config );

            log_vdisplayf( req, LOG_ACTION, "field_edit_success", vars  );
            log_printf( req, LOG_ACTION, "Field '%s' was updated", fields->field_name.value );

            redirect_to_referer( req );
            return;
        }
    }

    // Html output
    char *html = form_html( req, &form );
    
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

// #################################################################################

void node_field_delete( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "field_delete" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get id
    char *field_id = main_variable_get( req, req->get_tree, "field_id" );
    if( field_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Get node type id from the field
    database_bind_text( req->stmt_find_field_config, 1, field_id );
    struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_find_field_config );
    if( field_config_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, field_config_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_id", .value.string = field_id, .type = NNCMS_TYPE_STRING },
            { .name = "node_type_id", .value.string = field_config_row->node_type_id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "node_field_delete_header", vars );

    // Generate links
    char *links = node_field_links( req, NULL, NULL, field_id, field_config_row->node_type_id );

    //
    // Actual field delete
    //
    char *delete_submit = main_variable_get( req, req->post_tree, "delete_submit" );
    if( delete_submit )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_vmessage( req, "xsrf_fail" );
            return;
        }

        // Delete the field
        database_bind_text( req->stmt_delete_field_config, 1, field_id );
        database_steps( req, req->stmt_delete_field_config );
        database_bind_text( req->stmt_delete_field_data_by_field_id, 1, field_id );
        database_steps( req, req->stmt_delete_field_data_by_field_id );

        log_vdisplayf( req, LOG_ACTION, "field_delete_success", vars  );
        log_printf( req, LOG_ACTION, "Field '%s' was deleted", field_config_row->name );
        
        redirect_to_referer( req );
        return;
    }
    
    struct NNCMS_FORM *form = template_confirm( req, field_config_row->name );

    // Html output
    char *html = form_html( req, form );

    
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

// #################################################################################

void node_field_add( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "field_add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Try to get id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
        {
            { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE } // Terminating row
        };
    char *header_str = i18n_string_temp( req, "node_field_add_header", vars );

    // Generate links
    char *links = node_field_links( req, NULL, NULL, NULL, node_type_id );

    //
    // Field edit form
    //
    struct NNCMS_FIELD_CONFIG_FIELDS *fields = memdup_temp( req, &field_config_fields, sizeof(field_config_fields) );
    fields->field_id.viewable = false;

    fields->node_type_id.value = node_type_id;
    
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->referer.viewable = true;
    fields->fkey.viewable = true;
    
    fields->field_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "field_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Actual node edit
    //
    char *field_add = main_variable_get( req, req->post_tree, "field_add" );
    if( field_add )
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
        bool field_config_valid = field_config_validate( req, fields );
        if( field_valid == true && field_config_valid == true )
        {
            // Edit the field
            database_bind_text( req->stmt_add_field_config, 1, fields->node_type_id.value );
            database_bind_text( req->stmt_add_field_config, 2, fields->field_name.value );
            database_bind_text( req->stmt_add_field_config, 3, fields->field_type.value );
            database_bind_text( req->stmt_add_field_config, 4, fields->field_config.value );
            database_bind_text( req->stmt_add_field_config, 5, fields->field_weight.value );
            database_bind_text( req->stmt_add_field_config, 6, fields->field_label.value );
            database_bind_text( req->stmt_add_field_config, 7, fields->field_empty_hide.value );
            database_bind_text( req->stmt_add_field_config, 8, fields->field_values_count.value );
            database_bind_text( req->stmt_add_field_config, 9, fields->field_size_min.value );
            database_bind_text( req->stmt_add_field_config, 10, fields->field_size_max.value );
            database_bind_text( req->stmt_add_field_config, 11, fields->field_char_table.value );
            database_steps( req, req->stmt_add_field_config );
            unsigned int field_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "field_id", .value.unsigned_integer = field_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "field_add_success", vars  );
            log_printf( req, LOG_ACTION, "Field '%s' was added", fields->field_name.value );

            redirect_to_referer( req );
            return;
        }
    }

    // Html output
    char *html = form_html( req, &form );
    
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

// #################################################################################

void node_field_list( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *header_str = i18n_string_temp( req, "node_type_fields_header", NULL );

    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "type_fields" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get node type id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Generate links
    char *links = node_field_links( req, NULL, NULL, NULL, node_type_id );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_field_config );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_text( req->stmt_list_field_config, 1, node_type_id );
    database_bind_int( req->stmt_list_field_config, 2, default_pager_quantity );
    database_bind_text( req->stmt_list_field_config, 3, (http_start != NULL ? http_start : "0") );
    struct NNCMS_FIELD_CONFIG_ROW *field_config_row = (struct NNCMS_FIELD_CONFIG_ROW *) database_steps( req, req->stmt_list_field_config );
    if( field_config_row != NULL )
    {
        garbage_add( req->loop_garbage, field_config_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "field_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_type_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_config_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "field_weight_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; field_config_row != NULL && field_config_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "field_id", .value.string = field_config_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *edit = main_temp_link( req, "node_field_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "node_field_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = field_config_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = field_config_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = field_config_row[i].type, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = field_config_row[i].config, .type = TEMPLATE_TYPE_UNSAFE_STRING, .options = NULL },
            { .value.string = field_config_row[i].weight, .type = NNCMS_TYPE_STRING, .options = NULL },
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

// #################################################################################

void node_type_list( struct NNCMS_THREAD_INFO *req )
{
    // Check access
    if( acl_check_perm( req, "node", NULL, "type_list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "node_type_list_header", NULL );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_node_type );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_int( req->stmt_list_node_type, 1, default_pager_quantity );
    database_bind_text( req->stmt_list_node_type, 2, (http_start != NULL ? http_start : "0") );
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_steps( req, req->stmt_list_node_type );
    if( node_type_row != NULL )
    {
        garbage_add( req->loop_garbage, node_type_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "node_type_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_type_name_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; node_type_row != NULL && node_type_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "node_type_id", .value.string = node_type_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *list = main_temp_link( req, "node_list", i18n_string_temp( req, "list", NULL ), vars );
        char *fields = main_temp_link( req, "node_field_list", i18n_string_temp( req, "fields", NULL ), vars );
        char *view = main_temp_link( req, "node_type_view", i18n_string_temp( req, "view", NULL ), vars );
        char *edit = main_temp_link( req, "node_type_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "node_type_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = node_type_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = node_type_row[i].name, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = list, .type = NNCMS_TYPE_STRING, .options = NULL },
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
        .column_count = sizeof(header_cells) / sizeof(struct NNCMS_TABLE_CELL) - 1,
        .pager_quantity = 0, .pages_displayed = 0,
        .headerz = header_cells,
        .cells = (struct NNCMS_TABLE_CELL *) gcells->data
    };

    // Html output
    char *html = table_html( req, &table );

    // Generate links
    char *links = node_type_links( req, NULL, NULL, NULL, NULL );

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

void node_type_add( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "node", NULL, "type_add" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Page header
    char *header_str = i18n_string_temp( req, "node_type_add_header", NULL );

    //
    // Form
    //
    struct NNCMS_NODE_TYPE_FIELDS *fields = memdup_temp( req, &node_type_fields, sizeof(node_type_fields) );
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->node_type_add.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "node_type_add", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *node_type_add = main_variable_get( req, req->post_tree, "node_type_add" );
    if( node_type_add != 0 )
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
            database_bind_text( req->stmt_add_node_type, 1, fields->node_type_name.value );
            database_bind_text( req->stmt_add_node_type, 2, fields->node_type_authoring.value );
            database_bind_text( req->stmt_add_node_type, 3, fields->node_type_comments.value );
            database_steps( req, req->stmt_add_node_type );
            unsigned int node_type_id = database_last_rowid( req );

            struct NNCMS_VARIABLE vars[] =
            {
                { .name = "node_type_id", .value.unsigned_integer = node_type_id, .type = NNCMS_TYPE_UNSIGNED_INTEGER },
                { .type = NNCMS_TYPE_NONE } // Terminating row
            };
            log_vdisplayf( req, LOG_ACTION, "node_type_add_success", vars );
            log_printf( req, LOG_ACTION, "Node type was added (id = %u)", node_type_id );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = node_type_links( req, NULL, NULL, NULL, NULL );

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

void node_type_edit( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "node", NULL, "type_edit" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_node_type, 1, node_type_id );
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_steps( req, req->stmt_find_node_type );
    if( node_type_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_type_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_type_edit_header", vars );

    //
    // Form
    //
    struct NNCMS_NODE_TYPE_FIELDS *fields = memdup_temp( req, &node_type_fields, sizeof(node_type_fields) );
    fields->node_type_id.value = node_type_row->id;
    fields->node_type_name.value = node_type_row->name;
    fields->node_type_authoring.value = node_type_row->authoring;
    fields->node_type_comments.value = node_type_row->comments;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;
    fields->node_type_edit.viewable = true;

    struct NNCMS_FORM form =
    {
        .name = "node_type_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    //
    // Check if user commit changes
    //
    char *node_type_edit = main_variable_get( req, req->post_tree, "node_type_edit" );
    if( node_type_edit != 0 )
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
            database_bind_text( req->stmt_edit_node_type, 1, fields->node_type_name.value );
            database_bind_text( req->stmt_edit_node_type, 2, fields->node_type_authoring.value );
            database_bind_text( req->stmt_edit_node_type, 3, fields->node_type_comments.value );
            database_bind_text( req->stmt_edit_node_type, 4, node_type_id );
            database_steps( req, req->stmt_edit_node_type );

            log_vdisplayf( req, LOG_ACTION, "node_type_edit_success", vars );
            log_printf( req, LOG_ACTION, "Node type '%s' was editted", node_type_row->name );

            // Redirect back
            redirect_to_referer( req );
            return;
        }
    }

    // Generate links
    char *links = node_type_links( req, NULL, NULL, NULL, node_type_id );

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

void node_type_view( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission to edit ACLs
    if( acl_check_perm( req, "node", NULL, "type_view" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row by id
    database_bind_text( req->stmt_find_node_type, 1, node_type_id );
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_steps( req, req->stmt_find_node_type );
    if( node_type_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_type_row, MEMORY_GARBAGE_DB_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_type_view_header", vars );

    //
    // Form
    //
    struct NNCMS_NODE_TYPE_FIELDS *fields = memdup_temp( req, &node_type_fields, sizeof(node_type_fields) );
    for( unsigned int i = 0; ((struct NNCMS_FIELD *) fields)[i].type != NNCMS_FIELD_NONE; i = i + 1 ) ((struct NNCMS_FIELD *) fields)[i].editable = false;
    fields->node_type_id.value = node_type_row->id;
    fields->node_type_name.value = node_type_row->name;
    fields->node_type_authoring.value = node_type_row->authoring;
    fields->node_type_comments.value = node_type_row->comments;
    fields->referer.value = req->referer;
    fields->fkey.value = req->session_id;

    struct NNCMS_FORM form =
    {
        .name = "node_type_view", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = (struct NNCMS_FIELD *) fields
    };

    // Generate links
    char *links = node_type_links( req, NULL, NULL, NULL, node_type_id );

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

void node_type_delete( struct NNCMS_THREAD_INFO *req )
{
    // Check user permission
    if( acl_check_perm( req, "node", NULL, "type_delete" ) == false )
    {
        main_message( req, "not_allowed" );
        return;
    }

    // Get id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = TEMPLATE_TYPE_UNSAFE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_type_delete_header", NULL );

    // Find selected id in database
    database_bind_text( req->stmt_find_node_type, 1, node_type_id );
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_steps( req, req->stmt_find_node_type );
    if( node_type_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_type_row, MEMORY_GARBAGE_DB_FREE );

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
        database_bind_text( req->stmt_delete_node_type, 1, node_type_id );
        database_steps( req, req->stmt_delete_node_type );
        //log_printf( req, LOG_DEBUG, "Deleted node_type (where id = %s)", node_type_id );

        // Loop through all nodes
        database_bind_text( req->stmt_list_node, 1, node_type_id );
        database_bind_int( req->stmt_list_node, 2, -1 ); // LIMIT
        database_bind_int( req->stmt_list_node, 3, 0 ); // OFFSET
        struct NNCMS_NODE_ROW *node_rows = (struct NNCMS_NODE_ROW *) database_steps( req, req->stmt_list_node );
        if( node_rows != NULL )
        {
            garbage_add( req->loop_garbage, node_rows, MEMORY_GARBAGE_DB_FREE );
            for( unsigned int i = 0; node_rows[i].id != NULL; i = i + 1 )
            {
                // Delete the node
                database_bind_text( req->stmt_delete_node, 1, node_rows[i].id );
                database_steps( req, req->stmt_delete_node );
                //log_printf( req, LOG_DEBUG, "Deleted node (where node_id = %s)", node_rows[i].id );
                
                // Loop through all node revisions
                database_bind_text( req->stmt_list_node_rev, 1, node_rows[i].id );
                database_bind_int( req->stmt_list_node_rev, 2, -1 ); // LIMIT
                database_bind_int( req->stmt_list_node_rev, 3, 0 ); // OFFSET
                struct NNCMS_NODE_REV_ROW *node_rev = (struct NNCMS_NODE_REV_ROW *) database_steps( req, req->stmt_list_node_rev );
                if( node_rev != NULL )
                {
                    garbage_add( req->loop_garbage, node_rev, MEMORY_GARBAGE_DB_FREE );
                    for( unsigned int i = 0; node_rev[i].id != NULL; i = i + 1 )
                    {
                        // Delete data
                        database_bind_text( req->stmt_delete_field_data_by_node_rev_id, 1, node_rev[i].id );
                        database_steps( req, req->stmt_delete_field_data_by_node_rev_id );
                        //log_printf( req, LOG_DEBUG, "Deleted field_data (where node_id = %s)", node_rev[i].id );
                        
                        // Delete node rev
                        database_bind_text( req->stmt_delete_node_rev, 1, node_rev[i].id );
                        database_steps( req, req->stmt_delete_node_rev );
                        //log_printf( req, LOG_DEBUG, "Deleted node_rev (where id = %s)", node_rev[i].id );
                    }
                }
            }
        }

        log_vdisplayf( req, LOG_ACTION, "node_type_delete_success", vars );
        log_printf( req, LOG_ACTION, "Node type '%s' was deleted", node_type_row->name );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    struct NNCMS_FORM *form = template_confirm( req, node_type_id );

    // Generate links
    char *links = node_type_links( req, NULL, NULL, NULL, node_type_id );

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

// #################################################################################

void node_rev( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "rev" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get id
    char *node_id = main_variable_get( req, req->get_tree, "node_id" );
    if( node_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Find row associated with our object by 'id'
    struct NNCMS_NODE *node_info = node_load( req, node_id );
    if( node_info == NULL )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    garbage_add( req->loop_garbage, node_info, MEMORY_GARBAGE_NODE_FREE );

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
        { .name = "node_title", .value.string = node_info->title, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_rev_list_header", vars );

    // Generate links
    char *links = node_links( req, node_info->id, NULL, NULL, node_info->type_id );

    // Find rows
    struct NNCMS_ROW *row_count = database_steps( req, req->stmt_count_node_rev );
    garbage_add( req->loop_garbage, row_count, MEMORY_GARBAGE_DB_FREE );
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    database_bind_text( req->stmt_list_node_rev, 1, node_id );
    database_bind_int( req->stmt_list_node_rev, 2, default_pager_quantity );
    database_bind_text( req->stmt_list_node_rev, 3, (http_start != NULL ? http_start : "0") );
    struct NNCMS_NODE_REV_ROW *node_rev_row = (struct NNCMS_NODE_REV_ROW *) database_steps( req, req->stmt_list_node_rev );
    if( node_rev_row != NULL )
    {
        garbage_add( req->loop_garbage, node_rev_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "node_rev_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_rev_user_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_rev_title_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_rev_timestamp_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_rev_log_msg_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; node_rev_row != NULL && node_rev_row[i].id != NULL; i = i + 1 )
    {
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "node_rev_id", .value.string = node_rev_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "node_rev_view", i18n_string_temp( req, "view", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = node_rev_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = node_rev_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
            { .value.string = node_rev_row[i].title, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = node_rev_row[i].timestamp, .type = NNCMS_TYPE_STR_TIMESTAMP, .options = NULL },
            { .value.string = node_rev_row[i].log_msg, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
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

// #################################################################################

void node_list( struct NNCMS_THREAD_INFO *req )
{
    // First we check what permissions do user have
    if( acl_check_perm( req, "node", NULL, "list" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get node type id
    char *node_type_id = main_variable_get( req, req->get_tree, "node_type_id" );
    if( node_type_id == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }

    // Generate links
    char *links = node_links( req, NULL, NULL, NULL, node_type_id );

    // Find rows
    struct NNCMS_FILTER filter[] =
    {
        { .field_name = "node_type_id", .operator = NNCMS_OPERATOR_EQUAL, .field_value = NULL, .filter_exposed = true, .operator_exposed = false, .required = false },
        { .operator = NNCMS_OPERATOR_NONE }
    };
    
    struct NNCMS_SORT sort[] =
    {
        { .field_name = "id", .direction = NNCMS_SORT_DESCENDING },
        { .direction = NNCMS_SORT_NONE }
    };

    struct NNCMS_QUERY query_select =
    {
        .table = "node",
        .filter = filter,
        .sort = sort,
        .offset = 0,
        .limit = 25
    };

    // Find rows
    filter_query_prepare( req, &query_select );
    int row_count = database_count( req, &query_select );
    pager_query_prepare( req, &query_select );
    struct NNCMS_NODE_ROW *node_row = (struct NNCMS_NODE_ROW *) database_select( req, &query_select );
    if( node_row != NULL )
    {
        garbage_add( req->loop_garbage, node_row, MEMORY_GARBAGE_DB_FREE );
    }

    // Page header
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    char *header_str = i18n_string_temp( req, "node_list_header", vars );

    // Header cells
    struct NNCMS_TABLE_CELL header_cells[] =
    {
        { .value = i18n_string_temp( req, "node_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_title_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_user_id_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_timestamp_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = i18n_string_temp( req, "node_language_name", NULL ), .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .value = "", .type = NNCMS_TYPE_STRING, .options = NULL },
        { .type = NNCMS_TYPE_NONE }
    };

    // Fetch table data
    GArray *gcells = g_array_new( TRUE, FALSE, sizeof(struct NNCMS_TABLE_CELL) );
    garbage_add( req->loop_garbage, gcells, MEMORY_GARBAGE_GARRAY_FREE );
    for( unsigned int i = 0; node_row != NULL && node_row[i].id != NULL; i = i + 1 )
    {
        database_bind_text( req->stmt_find_node_rev, 1, node_row[i].node_rev_id );
        struct NNCMS_NODE_REV_ROW *node_rev_row = (struct NNCMS_NODE_REV_ROW *) database_steps( req, req->stmt_find_node_rev );
        if( node_rev_row != 0 )
        {
            garbage_add( req->loop_garbage, node_rev_row, MEMORY_GARBAGE_DB_FREE );
        }
        
        // Actions
        char *link;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "node_id", .value.string = node_row[i].id, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };

        char *view = main_temp_link( req, "node_view", i18n_string_temp( req, "view", NULL ), vars );
        char *rev = main_temp_link( req, "node_rev", i18n_string_temp( req, "rev", NULL ), vars );
        char *edit = main_temp_link( req, "node_edit", i18n_string_temp( req, "edit", NULL ), vars );
        char *delete = main_temp_link( req, "node_delete", i18n_string_temp( req, "delete", NULL ), vars );

        // Data
        struct NNCMS_TABLE_CELL cells[] =
        {
            { .value.string = node_row[i].id, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = (node_rev_row ? node_rev_row->title : ""), .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = node_row[i].user_id, .type = TEMPLATE_TYPE_USER, .options = NULL },
            { .value.string = (node_rev_row ? node_rev_row->timestamp : ""), .type = NNCMS_TYPE_STR_TIMESTAMP, .options = NULL },
            { .value.string = node_row[i].language, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = view, .type = NNCMS_TYPE_STRING, .options = NULL },
            { .value.string = rev, .type = NNCMS_TYPE_STRING, .options = NULL },
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

// #############################################################################

char *node_type_links( struct NNCMS_THREAD_INFO *req, char *node_id, char *node_rev_id, char *field_id, char *node_type_id )
{
    struct NNCMS_VARIABLE vars_node_id[] =
    {
        { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_node_rev_id[] =
    {
        { .name = "node_rev_id", .value.string = node_rev_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_field_id[] =
    {
        { .name = "field_id", .value.string = field_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_node_type_id[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
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

    //
    // Node types
    //

    link.function = "node_type_list";
    link.title = i18n_string_temp( req, "node_type_list_link", NULL );
    link.vars = NULL;
    g_array_append_vals( links, &link, 1 );

    if( node_id == NULL )
    {
        link.function = "node_type_add";
        link.title = i18n_string_temp( req, "node_type_add_link", NULL );
        link.vars = NULL;
        g_array_append_vals( links, &link, 1 );

        if( node_type_id != NULL )
        {
            link.function = "node_type_view";
            link.title = i18n_string_temp( req, "node_type_view_link", NULL );
            link.vars = vars_node_type_id;
            g_array_append_vals( links, &link, 1 );

            link.function = "node_type_edit";
            link.title = i18n_string_temp( req, "node_type_edit_link", NULL );
            link.vars = vars_node_type_id;
            g_array_append_vals( links, &link, 1 );

            link.function = "node_type_delete";
            link.title = i18n_string_temp( req, "node_type_delete_link", NULL );
            link.vars = vars_node_type_id;
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

char *node_field_links( struct NNCMS_THREAD_INFO *req, char *node_id, char *node_rev_id, char *field_id, char *node_type_id )
{
    struct NNCMS_VARIABLE vars_node_id[] =
    {
        { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_node_rev_id[] =
    {
        { .name = "node_rev_id", .value.string = node_rev_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_field_id[] =
    {
        { .name = "field_id", .value.string = field_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_node_type_id[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
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

    //
    // Fields
    //

    if( node_type_id != NULL )
    {
        link.function = "node_field_list";
        link.title = i18n_string_temp( req, "field_list_link", NULL );
        link.vars = vars_node_type_id;
        g_array_append_vals( links, &link, 1 );

        if( node_id == NULL )
        {
            link.function = "node_field_add";
            link.title = i18n_string_temp( req, "field_add_link", NULL );
            link.vars = vars_node_type_id;
            g_array_append_vals( links, &link, 1 );
        }
    }

    if( node_id == NULL )
    {
        if( field_id != NULL )
        {
            link.function = "node_field_edit";
            link.title = i18n_string_temp( req, "field_edit_link", NULL );
            link.vars = vars_field_id;
            g_array_append_vals( links, &link, 1 );

            link.function = "node_field_delete";
            link.title = i18n_string_temp( req, "field_delete_link", NULL );
            link.vars = vars_field_id;
            g_array_append_vals( links, &link, 1 );
        }
    }

    //
    // Node types
    //

    if( node_type_id != NULL )
    {
        link.function = "node_type_view";
        link.title = i18n_string_temp( req, "node_type_view_link", NULL );
        link.vars = vars_node_type_id;
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

char *node_links( struct NNCMS_THREAD_INFO *req, char *node_id, char *node_rev_id, char *field_id, char *node_type_id )
{
    struct NNCMS_VARIABLE vars_node_id[] =
    {
        { .name = "node_id", .value.string = node_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_node_rev_id[] =
    {
        { .name = "node_rev_id", .value.string = node_rev_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_field_id[] =
    {
        { .name = "field_id", .value.string = field_id, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE vars_node_type_id[] =
    {
        { .name = "node_type_id", .value.string = node_type_id, .type = NNCMS_TYPE_STRING },
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

    //
    // Nodes
    //

    if( acl_check_perm( req, "node", NULL, "list" ) == true )
    {
        link.function = "node_type_list";
        link.title = i18n_string_temp( req, "node_type_list_link", NULL );
        link.vars = NULL;
        g_array_append_vals( links, &link, 1 );
    }

    if( node_type_id != NULL )
    {
        if( acl_check_perm( req, "node", NULL, "list" ) == true )
        {
            link.function = "node_list";
            link.title = i18n_string_temp( req, "node_list_link", NULL );
            link.vars = vars_node_type_id;
            g_array_append_vals( links, &link, 1 );
        }
        
        if( acl_check_perm( req, "node", NULL, "add" ) == true )
        {
            link.function = "node_add";
            link.title = i18n_string_temp( req, "node_add_link", NULL );
            link.vars = vars_node_type_id;
            g_array_append_vals( links, &link, 1 );
        }
    }

    if( node_id != NULL )
    {
        if( acl_check_perm( req, "node", NULL, "view" ) == true )
        {
            link.function = "node_view";
            link.title = i18n_string_temp( req, "node_view_link", NULL );
            link.vars = vars_node_id;
            g_array_append_vals( links, &link, 1 );
        }

        if( acl_check_perm( req, "node", NULL, "rev" ) == true )
        {
            link.function = "node_rev";
            link.title = i18n_string_temp( req, "node_rev_link", NULL );
            link.vars = vars_node_id;
            g_array_append_vals( links, &link, 1 );
        }

        if( acl_check_perm( req, "node", NULL, "edit" ) == true )
        {
            link.function = "node_edit";
            link.title = i18n_string_temp( req, "node_edit_link", NULL );
            link.vars = vars_node_id;
            g_array_append_vals( links, &link, 1 );
        }

        if( acl_check_perm( req, "node", NULL, "delete" ) == true )
        {
            link.function = "node_delete";
            link.title = i18n_string_temp( req, "node_delete_link", NULL );
            link.vars = vars_node_id;
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

bool field_config_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD_CONFIG_FIELDS *fields )
{
    bool result = true;
    
    // Check if node_type_id exists
    database_bind_text( req->stmt_find_node_type, 1, fields->node_type_id.value );
    struct NNCMS_NODE_TYPE_ROW *node_type_row = (struct NNCMS_NODE_TYPE_ROW *) database_steps( req, req->stmt_find_node_type );
    if( node_type_row != NULL )
    {
        database_free_rows( (struct NNCMS_ROW *) node_type_row );
    }
    else
    {
        result = false;
        struct NNCMS_VARIABLE vars[] =
        {
            { .name = "node_type_id", .value.string = fields->node_type_id.value, .type = NNCMS_TYPE_STRING },
            { .type = NNCMS_TYPE_NONE }
        };
        log_vdisplayf( req, LOG_ERROR, "invalid_node_type_id", vars );
    }
}

// #############################################################################
