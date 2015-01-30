// #############################################################################
// Header file: database.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// rofl includes of system headers
//



#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#  include <getopt.h>
#else
#  include <sys/time.h>
#endif

#include <sqlite3.h>

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"

// #############################################################################

#ifndef __database_h__
#define __database_h__

// #############################################################################
// type and constant definitions
//

extern struct NNCMS_VARIABLE filter_operator_list[];
extern struct NNCMS_VARIABLE filter_operator_sql_list[];

struct NNCMS_TABLE_INFO_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *cid;          // 0 ... n
    char *name;         // column name
    char *type;         // integer / string
    char *notnull;      // 0
    char *dflt_value;   // 
    char *pk;           // primary key
    char *value[NNCMS_COLUMNS_MAX - 6];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_MASTER_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *type;         // table
    char *name;         // xxx
    char *tbl_name;     // xxx
    char *rootpage;     // 25
    char *sql;          // CREATE TABLE 'xxx' ( 'id' integer primary key, 'data' text )
    char *value[NNCMS_COLUMNS_MAX - 5];
    
    struct NNCMS_ROW *next; // BC
};

extern struct NNCMS_ROW empty_row;

#define NNCMS_QUERY_LEN_MAX    512

// SQLite3 sturcture
extern sqlite3 *db;

// Path to database
extern char szDatabasePath[NNCMS_PATH_LEN_MAX];

/*enum NNCMS_QUERY_WHERE_BOOLEAN
{
    NNCMS_BOOL_NONE = 0,
    NNCMS_BOOL_CONJUNCTION,  // And
    NNCMS_BOOL_DISJUNCTION   // Or
};*/

enum NNCMS_QUERY_OPERATION
{
    NNCMS_QUERY_NONE = 0,
    NNCMS_QUERY_INSERT,     // table name, variables
    NNCMS_QUERY_SELECT,     // table name, variables, filter, sort, offset, limit,
    NNCMS_QUERY_UPDATE,     // table name, variables, filter
    NNCMS_QUERY_DELETE      // table name, filter
};

/*
<option value="=">Is equal to</option>
<option value="!=">Is not equal to</option>
<option selected="selected" value="contains">Contains</option>
<option value="word">Contains any word</option>
<option value="allwords">Contains all words</option>
<option value="starts">Starts with</option>
<option value="not_starts">Does not start with</option>
<option value="ends">Ends with</option>
<option value="not_ends">Does not end with</option>
<option value="not">Does not contain</option>
<option value="shorterthan">Length is shorter than</option>
<option value="longerthan">Length is longer than</option>
<option value="regular_expression">Regular expression</option>
*/

enum NNCMS_OPERATOR
{
    NNCMS_OPERATOR_NONE = 0,
    NNCMS_OPERATOR_EQUAL,
    NNCMS_OPERATOR_NOTEQUAL,
    NNCMS_OPERATOR_LIKE,
    NNCMS_OPERATOR_GLOB,
    NNCMS_OPERATOR_REGEXP,
    NNCMS_OPERATOR_LESS,
    NNCMS_OPERATOR_LESS_EQUAL,
    NNCMS_OPERATOR_GREATER,
    NNCMS_OPERATOR_GREATER_EQUAL,
};

enum NNCMS_SORT_DIRECTION
{
    NNCMS_SORT_NONE = 0,
    NNCMS_SORT_ASCENDING,
    NNCMS_SORT_DESCENDING,
};

struct NNCMS_SORT
{
    char *field_name;
    enum NNCMS_SORT_DIRECTION direction;
};

struct NNCMS_FILTER
{
    // Query related variables
    char *field_name;
    enum NNCMS_OPERATOR operator;
    char *field_value;

    // User interface related variables
    bool filter_exposed;
    bool operator_exposed;
    bool required;
};

struct NNCMS_QUERY
{
    // Table name
    char *table;

    // Filter
    struct NNCMS_FILTER *filter;

    // Sort
    struct NNCMS_SORT *sort;

    // Offset and limit of row selection
    int offset;
    int limit;
};

// #############################################################################
// function declarations
//

// Module functions
bool database_global_init( );
bool database_global_destroy( );
bool database_local_init( struct NNCMS_THREAD_INFO *req );
bool database_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void database_tables( struct NNCMS_THREAD_INFO *req );
void database_rows( struct NNCMS_THREAD_INFO *req );
void database_edit( struct NNCMS_THREAD_INFO *req );
void database_add( struct NNCMS_THREAD_INFO *req );
void database_delete( struct NNCMS_THREAD_INFO *req );
void database_view( struct NNCMS_THREAD_INFO *req );

// Row fetching functions
struct NNCMS_ROW *database_vquery( struct NNCMS_THREAD_INFO *req, char *query_format, va_list vArgs );
struct NNCMS_ROW *database_query( struct NNCMS_THREAD_INFO *req, char *query_format, ... );
int database_callback( void *NotUsed, int argc, char **argv, char **azColName ); // Custom row fetcher for SQLite
void database_traverse( struct NNCMS_ROW *row ); // Debug and template row traverse function
void database_free_rows( struct NNCMS_ROW *row ); // Free memory from allocated memory for rows
void database_cleanup( struct NNCMS_THREAD_INFO *req ); // Cleanup
unsigned long int database_last_rowid( struct NNCMS_THREAD_INFO *req );

// Prepared row fetching
sqlite3_stmt *database_prepare( struct NNCMS_THREAD_INFO *req, const char *zSql );
bool database_bind_text( sqlite3_stmt *pStmt, int iParam, char *lpText );
bool database_bind_int( sqlite3_stmt *pStmt, int iParam, int iNum );
struct NNCMS_ROW *database_steps( struct NNCMS_THREAD_INFO *req, sqlite3_stmt *stmt );
int database_finalize( struct NNCMS_THREAD_INFO *req, sqlite3_stmt *pStmt );

// Binary interface to database
struct NNCMS_ROW *database_select( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query );
int database_count( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query );

char *database_links( struct NNCMS_THREAD_INFO *req, char *row_id, char *table_name );

// #############################################################################

#endif // __database_h__

// #############################################################################
