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

#include <fcgiapp.h>
#include <pthread.h>
#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <sqlite3.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __threadinfo_h__
#define __threadinfo_h__

// #############################################################################
// type and constant definitions
//

#define NNCMS_USERNAME_LEN      128
#define NNCMS_COLUMNS_MAX       16
#define NNCMS_VARIABLES_MAX     10
#define NNCMS_BLOCK_LEN_MAX     10 * 1024
#define NNCMS_HEADERS_LEN_MAX   1024
#define NNCMS_UPLOAD_LEN_MAX    10 * 1024 * 1024
#define NNCMS_VAR_NAME_LEN_MAX  48
#define NNCMS_BOUNDARY_LEN_MAX  128
#define NNCMS_PATH_LEN_MAX      256

// For file and post module
#define NNCMS_ICONS_MAX         1024
#define NNCMS_ACTIONS_LEN_MAX   2048

// Structure of one row, they can linked using nextRow variable
// number of row is very dynamic value, it can vary from 1 to 1000, so
// we allocate memore for each one, columns are static, only I decide number of
// them.
struct NNCMS_ROW
{
    char *szColName[NNCMS_COLUMNS_MAX];     // Contains a pointer to column name
    char *szColValue[NNCMS_COLUMNS_MAX];    // Contains a pointer to column's value
    struct NNCMS_ROW *next;                 // Pointer to the next row
                                            // If zero, then this is last row
};

// Structure to hold query variables
struct NNCMS_VARIABLE
{
    char *lpszName;
    char *lpszValue;
};

// List of uploaded files
struct NNCMS_FILE
{
    char szName[NNCMS_VAR_NAME_LEN_MAX];
    char szFileName[NNCMS_PATH_LEN_MAX];
    char *lpData;
    unsigned int nSize;
    struct NNCMS_FILE *lpNext;
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
    pthread_t *pThread;

    // Temporary buffers
    char *lpszBuffer;
    char *lpszFrame;
    char *lpszTemp;

    // Reserved for main_output
    // Do not use it as variable for main_output
    char *lpszOutput;

    // FastCGI Request data
    FCGX_Request *fcgi_request;
    int nContentLenght; char *lpszContentType; char *lpszContentData;
    char *lpszRequestURI;
    char szBoundary[NNCMS_BOUNDARY_LEN_MAX]; unsigned int nBoundaryLen;
    struct NNCMS_VARIABLE variables[NNCMS_VARIABLES_MAX];
    struct NNCMS_FILE *lpUploadedFiles;

    // HTTPd info
    char *lpszHTTPd;
    enum NNCMS_HTTPD httpd_type;

    // Response data
    char szResponse[NNCMS_HEADERS_LEN_MAX];
    char szResponseHeaders[NNCMS_HEADERS_LEN_MAX];
    char szResponseContentType[NNCMS_HEADERS_LEN_MAX];
    char bHeadersSent;

    // This holds database query result
    struct NNCMS_ROW *firstRow;
    struct NNCMS_ROW *lastRow;

    // Login data
    bool g_bSessionChecked;
    char g_userid[32];
    char g_username[NNCMS_USERNAME_LEN];
    char *g_sessionid;
    char *g_useraddr, *g_useragent; // For ban and session check
    bool g_isGuest;

    // User block temporary buffers
    char *lpszBlockContent;
    char *lpszBlockTemp;

    // Root directory
    char *lpszRoot;

    // Prepared statements for acl
    sqlite3_stmt *stmtAddACL, *stmtFindACLById, *stmtFindACLByName,
        *stmtEditACL, *stmtDeleteACL, *stmtDeleteObject, *stmtGetACLId,
        *stmtGetObject, *stmtListACL, *stmtFindUser;

    // Prepared statements for log
    sqlite3_stmt *stmtListLogs, *stmtViewLog, *stmtUser, *stmtAddLogReq,
        *stmtAddLog, *stmtClearLog;

    // Prepared statements for user
    sqlite3_stmt *stmtFindUserByName, *stmtFindUserById, *stmtAddUser,
        *stmtLoginUserByName, *stmtLoginUserById, *stmtFindSession,
        *stmtAddSession, *stmtDeleteSession, *stmtCleanSessions,
        *stmtListSessions, *stmtListUsers, *stmtEditUserHash,
        *stmtEditUserNick, *stmtEditUserGroup, *stmtDeleteUser;

    // Prepared statements for user
    sqlite3_stmt *stmtAddPost, *stmtFindPost, *stmtPowerEditPost, *stmtEditPost,
        *stmtDeletePost, *stmtGetParentPost, *stmtRootPosts;
    sqlite3_stmt *stmtTopicPosts;
    sqlite3_stmt *stmtCountChildPosts;

    // Prepared statements for ban
    sqlite3_stmt *stmtAddBan, *stmtListBans, *stmtFindBanById, *stmtEditBan,
        *stmtDeleteBan;

    // Lua VM
    lua_State *L;
};

// #############################################################################
// function declarations
//

// #############################################################################

#endif // __threadinfo_h__

// #############################################################################
