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

#include "config.h"

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

#define NNCMS_QUERY_LEN_MAX    512

// SQLite3 sturcture
extern sqlite3 *db;

// Path to database
extern char szDatabasePath[NNCMS_PATH_LEN_MAX];

// #############################################################################
// function declarations
//

// Module functions
bool database_init( );
bool database_deinit( );

// Row fetching functions
struct NNCMS_ROW *database_vquery( struct NNCMS_THREAD_INFO *req, char *lpszQueryFormat, va_list vArgs );
struct NNCMS_ROW *database_query( struct NNCMS_THREAD_INFO *req, char *lpszQueryFormat, ... );
int database_callback( void *NotUsed, int argc, char **argv, char **azColName ); // Custom row fetcher for SQLite
void database_traverse( struct NNCMS_ROW *row ); // Debug and template row traverse function
void database_freeRows( struct NNCMS_ROW *row ); // Free memory from allocated memory for rows
void database_cleanup( struct NNCMS_THREAD_INFO *req ); // Cleanup

// Prepared row fetching
sqlite3_stmt *database_prepare( const char *zSql );
bool database_bind_text( sqlite3_stmt *pStmt, int iParam, char *lpText );
bool database_bind_int( sqlite3_stmt *pStmt, int iParam, int iNum );
struct NNCMS_ROW *database_steps( sqlite3_stmt *pStmt );
int database_finalize( sqlite3_stmt *pStmt );

// #############################################################################

#endif // __database_h__

// #############################################################################
