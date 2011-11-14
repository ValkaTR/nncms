// #############################################################################
// Source file: database.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// Test
void database_xx(){}

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "database.h"
#include "log.h"

#include "memory.h"

// #############################################################################
// global variables
//

// SQLite3 sturcture
sqlite3 *db = 0;

// Is this module initialized?
char is_databaseInit = 0;

// Path to database
char szDatabasePath[NNCMS_PATH_LEN_MAX] = "./nncms.db";

char *zEmpty = "";

// #############################################################################
// functions

// Init module
bool database_init( )
{
    if( is_databaseInit == false )
    {
        is_databaseInit = true;

        //
        // Open database
        //
        if( sqlite3_open( szDatabasePath, &db ) )
        {
            log_printf( 0, LOG_CRITICAL, "Database::Init: Can't open database: %s", sqlite3_errmsg( db ) );
            sqlite3_close( db );
            db = 0;
            return false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

// #############################################################################

// DeInit module
bool database_deinit( )
{
    if( is_databaseInit == true )
    {
        is_databaseInit = false;

        //
        // After we finish, we should close database
        //

        sqlite3_close( db );
        db = 0;

        return true;
    }
    else
    {
        return false;
    }
}

// #############################################################################

sqlite3_stmt *database_prepare( const char *zSql )
{
    const char *zLeftOver;      /* Tail of unprocessed SQL */
    sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
    int nResult = sqlite3_prepare_v2(
            db,             /* Database handle */
            zSql,           /* SQL statement, UTF-8 encoded */
            -1,             /* Maximum length of zSql in bytes. */
            &pStmt,         /* OUT: Statement handle */
            &zLeftOver      /* OUT: Pointer to unused portion of zSql */
        );

    if( nResult != SQLITE_OK )
    {
        log_printf( 0, LOG_ERROR, "Database::Prepare: %s", sqlite3_errmsg( db ) );
        return NULL;
    }
    return pStmt;
}

// #############################################################################

int database_finalize( sqlite3_stmt *pStmt )
{
    return sqlite3_finalize( pStmt );
}

// #############################################################################

/*
gdb programm `ps -C nncms -o pid=`
break database_steps
continue

*/
struct NNCMS_ROW *database_steps( sqlite3_stmt *pStmt )
{
    // Don't do a thing if DB is not connected
    if( db == 0 ) return NULL;

    // Temporary node for first row
    struct NNCMS_ROW *firstRow = 0, *curRow = 0;

    // The loop
    while( 1 )
    {
        int nResult = sqlite3_step( pStmt );
        if( nResult == SQLITE_ROW )
        {
            // If the SQL statement being executed returns any data, then
            // SQLITE_ROW is returned each time a new row of data is ready for
            // processing by the caller. The values may be accessed using the
            // column access functions.
            if( firstRow == 0 )
            {
                curRow = MALLOC( sizeof(struct NNCMS_ROW) );
                memset( curRow, 0, sizeof(struct NNCMS_ROW) );
                firstRow = curRow;
            }
            else
            {
                // Prepare next row
                curRow->next = MALLOC( sizeof(struct NNCMS_ROW) );
                curRow = curRow->next;
                memset( curRow, 0, sizeof(struct NNCMS_ROW) );
            }

            // Number of columns in the result set returned by the prepared statement.
            int iColCount = sqlite3_column_count( pStmt );

            // Loop
            for( int iCol = 0; iCol < iColCount; iCol++ )
            {
                // Make memory for column value
                int nBytes = sqlite3_column_bytes( pStmt, iCol );
                if( nBytes == 0 )
                {
                    curRow->szColValue[iCol] = zEmpty;
                    continue;
                }
                curRow->szColValue[iCol] = MALLOC( nBytes + 1 );

                // Copy column
                char *lpText = (char *) sqlite3_column_text( pStmt, iCol );
                memcpy( curRow->szColValue[iCol], lpText, nBytes );
                curRow->szColValue[iCol][nBytes] = 0; // Zero terminators at the end of the string
            }
        }
        else if( nResult == SQLITE_BUSY )
        {
            // SQLITE_BUSY means that the database engine was unable to acquire
            // the database locks it needs to do its job.
            continue;
        }
        else if( nResult == SQLITE_DONE )
        {
            // SQLITE_DONE means that the statement has finished executing
            // successfully. sqlite3_step() should not be called again on this
            // virtual machine without first calling sqlite3_reset() to reset
            // the virtual machine back to its initial state.
            sqlite3_reset( pStmt );
            return firstRow;
        }
        else //if( nResult == SQLITE_ERROR )
        {
            // SQLITE_ERROR means that a run-time error (such as a constraint
            // violation) has occurred. sqlite3_step() should not be called
            // again on the VM.
            log_printf( 0, LOG_DEBUG, "Database::Steps: %s", sqlite3_errmsg( db ) );
            sqlite3_reset( pStmt );
            return firstRow;
        }
    }
}

// #############################################################################

bool database_bind_text( sqlite3_stmt *pStmt, int iParam, char *lpText )
{
    // Don't do a thing if DB is not connected
    if( db == 0 ) return false;

    int nResult;
    if( lpText != 0 )
    {
        size_t nLen = strlen( lpText );
        nResult = sqlite3_bind_text( pStmt, iParam, lpText, nLen, SQLITE_STATIC );
    }
    else
    {
        nResult = sqlite3_bind_text( pStmt, iParam, "", -1, SQLITE_STATIC );
    }
    if( nResult != SQLITE_OK )
    {
        log_printf( 0, LOG_DEBUG, "Database::BindText: %s", sqlite3_errmsg( db ) );
        return false;
    }
    return true;

}

// #############################################################################

bool database_bind_int( sqlite3_stmt *pStmt, int iParam, int iNum )
{
    // Don't do a thing if DB is not connected
    if( db == 0 ) return false;

    int nResult = sqlite3_bind_int( pStmt, iParam, iNum );
    if( nResult != SQLITE_OK )
    {
        log_printf( 0, LOG_DEBUG, "Database::BindInt: %s", sqlite3_errmsg( db ) );
        return false;
    }
    return true;
}

// #############################################################################

struct NNCMS_ROW *database_vquery( struct NNCMS_THREAD_INFO *req, char *lpszQueryFormat, va_list vArgs )
{
    // This should hold sqlite error message
    char *zErrMsg = 0;

    if( db == 0 ) return 0;

    // Format query string
    //char szQuery[NNCMS_QUERY_LEN_MAX];
    //vsnprintf( szQuery, NNCMS_QUERY_LEN_MAX, lpszQueryFormat, vArgs );
    char *lpszSQL = sqlite3_vmprintf( lpszQueryFormat, vArgs );

    // Debug
    //printf( "Query: %s\n", szQuery );
    //return 0;

    // Exec
    int nResult = sqlite3_exec( db, lpszSQL, database_callback, req, &zErrMsg );
    sqlite3_free( lpszSQL );

    if( nResult != SQLITE_OK )
    {
        // Error. No data.
        log_printf( req, LOG_ALERT, "Database::Query: SQL Error: %s", zErrMsg );
        sqlite3_free( zErrMsg );
        return 0;
    }

    if( req != 0 )
    {
        struct NNCMS_ROW *tempRow = req->firstRow;
        req->firstRow = 0;
        return tempRow;
    }
    return 0;
}

// #############################################################################

struct NNCMS_ROW *database_query( struct NNCMS_THREAD_INFO *req, char *lpszQueryFormat, ... )
{
    va_list vArgs;
    va_start( vArgs, lpszQueryFormat );
    struct NNCMS_ROW *lpResult = database_vquery( req, lpszQueryFormat, vArgs );
    va_end( vArgs );

    return lpResult;
}

// #############################################################################

// Custom row fetcher
int database_callback( void *NotUsed, int argc, char **argv, char **azColName )
{
    // Global per thread structure
    struct NNCMS_THREAD_INFO *req = NotUsed;

    // If req == 0, then we are probably outside thread
    if( req == 0 )
        return 0;

    // Default indice
    int i;

    // Temporary node for current row, prepare it
    struct NNCMS_ROW *currentRow = MALLOC( sizeof(struct NNCMS_ROW) );
    memset( currentRow, 0, sizeof(struct NNCMS_ROW) );

    // When I was writing this code I was listening
    // to Armin Van Buuren - A State Of Trace, edit at
    // your risk (it may harm your brain)

    // Loop thru all columns (limited to NNCMS_COLUMNS_MAX)
    for( i = 0; i < (argc >= NNCMS_COLUMNS_MAX - 1 ? NNCMS_COLUMNS_MAX - 1 : argc); i++ )
    {
        // Fill currentRow with column name
        currentRow->szColName[i] = MALLOC( strlen(azColName[i]) + 1 );
        strcpy( currentRow->szColName[i], azColName[i] );

        // Fill currentRow with data
        currentRow->szColValue[i] = MALLOC( strlen(argv[i] != 0 ? argv[i] : "0") + 1 );
        strcpy( currentRow->szColValue[i], argv[i] != 0 ? argv[i] : "0" );

        // Debug:
        //printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "0");
    }

    if( req->firstRow )
        // Next of last row should know about current row
        req->lastRow->next = currentRow;
    else
        // Save the first row, it will be used by default, so we wont
        // traverse backwards
        req->firstRow = currentRow;

    // Now current row becomes last row
    req->lastRow = currentRow;

    // Debug:
    //printf("===\n");
    return 0;
}

// #############################################################################

// Debug and template row traverse function
void database_traverse( struct NNCMS_ROW *row )
{
    int i = 0;

    // Check if row exists
    if( row != 0 )
    {
        // Loop thru columns
        for( i = 0; i < NNCMS_COLUMNS_MAX - 1; i++ )
        {
            // If data is empty, then skip this column and quit loop
            if( row->szColValue[i] == 0 )
            {
                break;
            }
            else
            {
                // Otherwise print its data in stdout
                printf( "%s = %s\n", row->szColName[i], row->szColValue[i] );
            }
        }

        // Simple divider, so we can extinguish different rows
        printf( "---\n" );

        // This function traverse forward. We can easilly change row->next to
        // row->prev and function will traverse backward.
        database_traverse( row->next );
    }
}

// #############################################################################

// Clean up after SQL query
void database_cleanup( struct NNCMS_THREAD_INFO *req )
{
    // Clean up
    database_freeRows( req->firstRow );
    req->firstRow = 0;
}

// #############################################################################

// Free memory from allocated memory for rows
void database_freeRows( struct NNCMS_ROW *row )
{
    // This function does simple thing, from current row it traverse forward or
    // backward (if you change smth.) and frees memory alocated for column names
    // and data util last row. Function structure is almost the same as traverse.
    int i = 0;

    // Check if row exists
    if( row != 0 )
    {
        // Loop thru columns
        for( i = 0; i < NNCMS_COLUMNS_MAX - 1; i++ )
        {
            // Check if variable is used and free memory
            if( row->szColName[i] != 0 )
            {
                FREE( row->szColName[i] );
            }
            if( row->szColValue[i] != 0 && row->szColValue[i] != zEmpty )
            {
                FREE( row->szColValue[i] );
            }

        }

        // This function traverse forward. We can easilly change row->next to
        // row->prev and function will traverse backward.
        database_freeRows( row->next );

        // Now, don't forget to free row it self
        FREE( row );
    }
}

// #############################################################################
