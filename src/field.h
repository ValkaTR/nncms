// #############################################################################
// Header file: xxx.h

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

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __field_h__
#define __field_h__

// #############################################################################
// type and constant definitions
//

#define FIELD_NAME_LEN_MAX        32

// Special fields
#define FIELD_NODE_TYPE_ID        1000
#define FIELD_NODE_TYPE_NAME      1001
#define FIELD_NODE_ID             1002
#define FIELD_NODE_USER_ID        1003
#define FIELD_NODE_LANGUAGE       1004
#define FIELD_NODE_PUBLISHED      1005
#define FIELD_NODE_PROMOTED       1006
#define FIELD_NODE_STICKY         1007
#define FIELD_NODE_REV_ID         1008
#define FIELD_NODE_REV_USER_ID    1009
#define FIELD_NODE_REV_TITLE      1010
#define FIELD_NODE_REV_TIMESTAMP  1011
#define FIELD_NODE_REV_LOG_MSG    1012

struct NNCMS_FIELD_CONFIG_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *node_type_id;
    char *name;
    char *type;
    char *config;
    char *weight;
    char *label;
    char *empty_hide;
    char *values_count;
    char *size_min;
    char *size_max;
    char *char_table;
    char *value[NNCMS_COLUMNS_MAX - 12];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_FIELD_DATA_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *node_rev_id;
    char *field_id;
    char *data;
    char *config;
    char *weight;
    char *value[NNCMS_COLUMNS_MAX - 6];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################

enum NNCMS_FIELD_TYPE
{
    NNCMS_FIELD_NONE = 0,
    NNCMS_FIELD_TEXT,
    NNCMS_FIELD_HELP,
    NNCMS_FIELD_USER,
    NNCMS_FIELD_GROUP,
    NNCMS_FIELD_TIMEDATE,
    NNCMS_FIELD_EDITBOX,
    NNCMS_FIELD_TEXTAREA,
    NNCMS_FIELD_FILE,
    NNCMS_FIELD_SUBMIT,
    NNCMS_FIELD_HIDDEN,
    NNCMS_FIELD_LANGUAGE,
    NNCMS_FIELD_SELECT,
    NNCMS_FIELD_LINK,
    NNCMS_FIELD_UPLOAD,
    NNCMS_FIELD_CHECKBOX,
    NNCMS_FIELD_AUTHORING,
    NNCMS_FIELD_FIELD_ID,
    NNCMS_FIELD_VIEW_ID,
    NNCMS_FIELD_PICTURE,
};

enum NNCMS_FIELD_LABEL_TYPE
{
    FIELD_LABEL_UNKNOWN = 0,
    FIELD_LABEL_ABOVE,
    FIELD_LABEL_INLINE,
    FIELD_LABEL_HIDDEN
};

// Structure for one value
struct NNCMS_FIELD_VALUE
{
    char *id;
    char *value;
    char *config;
    char *weight;
};

struct NNCMS_FIELD
{
    // Id of a field in field_config table
    char *id;
    
    // Information for user
    char *text_name;
    char *text_description;
    
    // Information about field
    char *name;
    char *value;
    struct NNCMS_FIELD_VALUE *multivalue;

    // Data for special field types
    void *data;    
    enum NNCMS_FIELD_TYPE type;
    
    // Access control
    bool viewable;
    bool editable;
    
    // Validation
    char *char_table;
    size_t size_max;
    size_t size_min;
    
    // Display options
    enum NNCMS_FIELD_LABEL_TYPE label_type;
    bool empty_hide;
    int values_count;
    
    // Configuration described by text
    char *config;
};

struct NNCMS_SELECT_OPTIONS
{
    char *link;
    struct NNCMS_SELECT_ITEM *items;
};
struct NNCMS_SELECT_ITEM
{
    char *name;
    char *desc;
    bool selected;
};

struct NNCMS_AUTHORING_DATA
{
    char *user_id;
    char *timestamp;
};

struct NNCMS_TEXTAREA_OPTIONS
{
    char *link;
    
    // When editable is false, escape the text?
    bool escape;
    
    // Display format
    char *format;
    
    // Summary settings
    bool summary;
    bool remove_summary;
};

struct NNCMS_EDITBOX_OPTIONS
{
    char *link;
};

extern struct NNCMS_SELECT_ITEM char_tables[];
extern struct NNCMS_VARIABLE char_table_list[];

extern struct NNCMS_SELECT_ITEM label_types[];
extern struct NNCMS_VARIABLE label_type_list[];

extern struct NNCMS_SELECT_ITEM booleans[];
extern struct NNCMS_VARIABLE boolean_list[];

// #############################################################################
// function declarations
//

struct NNCMS_FIELD *field_find( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field, char *field_name );
char *field_get_value( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field, char *field_name );
void field_set_editable( struct NNCMS_FIELD *fields, bool state );

void *field_parse_config( struct NNCMS_THREAD_INFO *req, enum NNCMS_FIELD_TYPE field_type, char *config );
void field_load_data( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD_DATA_ROW *field_data_row, struct NNCMS_FIELD *field );
void field_load_config( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD_CONFIG_ROW *field_config_row, struct NNCMS_FIELD *field );
void field_parse( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field, GString *buffer );

void form_get_data( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field );
void form_post_data( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *field );

bool field_validate( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *fields );

// #############################################################################

#endif // __field_h__

// #############################################################################
