// #############################################################################
// Header file: node.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <stdbool.h>

#include <glib.h>

// #############################################################################
// includes of local headers
//

#include "main.h"

// #############################################################################

#ifndef __node_h__
#define __node_h__

// #############################################################################
// type and constant definitions
//

extern struct NNCMS_VARIABLE field_type_list[];

struct NNCMS_NODE
{
    // Node information
    char *id;
    char *title;
    char *language;
    char *user_id;
    
    // Node type information
    char *type_id;
    char *type_name;
    
    // Revision information
    char *rev_id;
    char *rev_timestamp;
    char *rev_user_id;
    char *rev_log_msg;

    // Field data
    struct NNCMS_FIELD *fields;
    unsigned int fields_count;
    
    // Database data
    struct NNCMS_FIELD_CONFIG_ROW *field_config_row;
    struct NNCMS_NODE_REV_ROW *node_rev_row;
    struct NNCMS_NODE_TYPE_ROW *node_type_row;
    struct NNCMS_NODE_ROW *node_row;
    GPtrArray *db_data;
};

// #############################################################################

struct NNCMS_NODE_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *user_id;
    char *node_type_id;
    char *language;
    char *node_rev_id;
    char *published;
    char *promoted;
    char *sticky;
    char *value[NNCMS_COLUMNS_MAX - 8];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_NODE_REV_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *node_id;
    char *user_id;
    char *title;
    char *timestamp;
    char *log_msg;
    char *value[NNCMS_COLUMNS_MAX - 6];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_NODE_TYPE_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *name;
    char *authoring;
    char *comments;
    char *value[NNCMS_COLUMNS_MAX - 4];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

// Modular functions
bool node_global_init( );
bool node_global_destroy( );
bool node_local_init( struct NNCMS_THREAD_INFO *req );
bool node_local_destroy( struct NNCMS_THREAD_INFO *req );

struct NNCMS_NODE *node_load( struct NNCMS_THREAD_INFO *req, char *s_node_id );
void node_free( struct NNCMS_NODE *node );

void node_view( struct NNCMS_THREAD_INFO *req );
void node_rev( struct NNCMS_THREAD_INFO *req );
void node_edit( struct NNCMS_THREAD_INFO *req );
void node_add( struct NNCMS_THREAD_INFO *req );
void node_delete( struct NNCMS_THREAD_INFO *req );
void node_field_add( struct NNCMS_THREAD_INFO *req );
void node_field_delete( struct NNCMS_THREAD_INFO *req );
void node_field_edit( struct NNCMS_THREAD_INFO *req );
void node_field_list( struct NNCMS_THREAD_INFO *req );
void node_type_add( struct NNCMS_THREAD_INFO *req );
void node_type_view( struct NNCMS_THREAD_INFO *req );
void node_type_edit( struct NNCMS_THREAD_INFO *req );
void node_type_delete( struct NNCMS_THREAD_INFO *req );
void node_type_list( struct NNCMS_THREAD_INFO *req );
void node_list( struct NNCMS_THREAD_INFO *req );

// #############################################################################

#endif // __node_h__

// #############################################################################
