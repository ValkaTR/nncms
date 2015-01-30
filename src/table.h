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
#include "database.h"

// #############################################################################

#ifndef __table_h__
#define __table_h__

// #############################################################################
// type and constant definitions
//

extern struct NNCMS_SELECT_ITEM filter_operators[];

struct NNCMS_TABLE_CELL
{
    //char *data; // old
    union NNCMS_VARIABLE_UNION value; // Advanced :3
    enum NNCMS_VARIABLE_TYPE type;
    
    struct NNCMS_VARIABLE *options;
};

struct NNCMS_TABLE_ROW
{
    GPtrArray *cells;
    
    struct NNCMS_VARIABLE *options;
};

struct NNCMS_TABLE
{
    struct NNCMS_VARIABLE *options;
    char *caption;
    char *header_html;
    char *footer_html;
    
    // Table settings
    char *cellpadding;
    char *cellspacing;
    char *border;
    char *bgcolor;
    char *width;
    char *height;
    
    // Pages
    int pager_quantity;
    int pages_displayed;
    
    // Output
    GString *html;
    
    // Storage
    struct NNCMS_TABLE_CELL *headerz;
    struct NNCMS_TABLE_CELL *cells;
    int row_count;
    int column_count;
};

// #############################################################################
// function declarations
//

// Filter
void filter_query_prepare( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query );
char *filter_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query );

// Pager
void pager_query_prepare( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query );
char *pager_html( struct NNCMS_THREAD_INFO *req, int count );

// Table
char *table_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_TABLE *table );

// #############################################################################

#endif // __table_h__

// #############################################################################
