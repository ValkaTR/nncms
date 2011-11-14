// #############################################################################
// Header file: template.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// Best viewed without word wrap, cuz this file contains very long lines.
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

// #############################################################################
// includes of local headers
//

#include "main.h"
#include "smart.h"
#include "tree.h"
#include "threadinfo.h"

// #############################################################################

#ifndef __template_h__
#define __template_h__

// #############################################################################
// type and constant definitions
//

#define NNCMS_TAG_NAME_MAX_SIZE 64

struct NNCMS_TEMPLATE_TAGS
{
    char szName[NNCMS_TAG_NAME_MAX_SIZE];     // [String] Search this tag
    char *szValue;                            // [Pointer] And replace it with this value
    //struct NNCMS_TEMPLATE_TAGS *next;    // [Pointer] Pointer to next structure
};

// Path to tempates
extern char templateDir[NNCMS_PATH_LEN_MAX];

// #############################################################################
// function declarations
//

// Module
bool template_init( struct NNCMS_THREAD_INFO *req );
bool template_deinit( struct NNCMS_THREAD_INFO *req );

void lua_stackdump( lua_State *L );

// For fast access to templates
enum TEMPLATE_INDEX
{
    TEMPLATE_ACL_ADD = 0,
    TEMPLATE_ACL_DELETE,
    TEMPLATE_ACL_EDIT,
    TEMPLATE_ACL_OBJECT_FOOT,
    TEMPLATE_ACL_OBJECT_HEAD,
    TEMPLATE_ACL_OBJECT_ROW,
    TEMPLATE_ACL_VIEW_FOOT,
    TEMPLATE_ACL_VIEW_HEAD,
    TEMPLATE_ACL_VIEW_ROW,
    TEMPLATE_BAN_ADD,
    TEMPLATE_BAN_VIEW,
    TEMPLATE_BAN_VIEW_ROW,
    TEMPLATE_BAN_VIEW_STRUCTURE,
    TEMPLATE_BAN_EDIT,
    TEMPLATE_BAN_DELETE,
    TEMPLATE_BLOCK,
    TEMPLATE_CACHE_ADMIN,
    TEMPLATE_CFG_EDIT,
    TEMPLATE_CFG_VIEW_FOOT,
    TEMPLATE_CFG_VIEW_HEAD,
    TEMPLATE_CFG_VIEW_ROW,
    TEMPLATE_FILE_ADD,
    TEMPLATE_FILE_BROWSER_DIR,
    TEMPLATE_FILE_BROWSER_FILE,
    TEMPLATE_FILE_BROWSER_FOOT,
    TEMPLATE_FILE_BROWSER_HEAD,
    TEMPLATE_FILE_DELETE,
    TEMPLATE_FILE_EDIT,
    TEMPLATE_FILE_MKDIR,
    TEMPLATE_FILE_RENAME,
    TEMPLATE_FILE_UPLOAD,
    TEMPLATE_FORM,
    TEMPLATE_FRAME,
    TEMPLATE_GALLERY_DIR,
    TEMPLATE_GALLERY_EMPTY,
    TEMPLATE_GALLERY_FILE,
    TEMPLATE_GALLERY_IMAGE,
    TEMPLATE_GALLERY_ROW,
    TEMPLATE_GALLERY_STRUCTURE,
    TEMPLATE_LOG_CLEAR,
    TEMPLATE_LOG_VIEW_FOOT,
    TEMPLATE_LOG_VIEW_HEAD,
    TEMPLATE_LOG_VIEW,
    TEMPLATE_LOG_VIEW_ROW,
    TEMPLATE_PAGE_CURRENT,
    TEMPLATE_PAGE_NUMBER,
    TEMPLATE_POST_ADD,
    TEMPLATE_POST_BOARD_ROW,
    TEMPLATE_POST_BOARD_STRUCTURE,
    TEMPLATE_POST_DELETE,
    TEMPLATE_POST_EDIT,
    TEMPLATE_POST_MODIFY,
    TEMPLATE_POST_NEWS_ROW,
    TEMPLATE_POST_NEWS_STRUCTURE,
    TEMPLATE_POST_PARENT,
    TEMPLATE_POST_REMOVE,
    TEMPLATE_POST_REPLY,
    TEMPLATE_POST_ROOT,
    TEMPLATE_POST_RSS_ENTRY,
    TEMPLATE_POST_RSS_STRUCTURE,
    TEMPLATE_POST_VIEW,
    TEMPLATE_POST_TOPICS_ROW,
    TEMPLATE_POST_TOPICS_STRUCTURE,
    TEMPLATE_REDIRECT,
    TEMPLATE_STRUCTURE,
    TEMPLATE_USER_BLOCK,
    TEMPLATE_USER_DELETE,
    TEMPLATE_USER_EDIT,
    TEMPLATE_USER_LOGIN,
    TEMPLATE_USER_PROFILE,
    TEMPLATE_USER_REGISTER,
    TEMPLATE_USER_SESSIONS_FOOT,
    TEMPLATE_USER_SESSIONS_HEAD,
    TEMPLATE_USER_SESSIONS_ROW,
    TEMPLATE_USER_VIEW_FOOT,
    TEMPLATE_USER_VIEW_HEAD,
    TEMPLATE_USER_VIEW,
    TEMPLATE_USER_VIEW_ROW,
    TEMPLATE_VIEW_FILE,
    TEMPLATE_VIEW_GALLERY_CELL,
    TEMPLATE_VIEW_GALLERY_ROW_BEGIN,
    TEMPLATE_VIEW_GALLERY_ROW_END,
    TEMPLATE_VIEW_GALLERY_TABLE_BEGIN,
    TEMPLATE_VIEW_GALLERY_TABLE_END,
    TEMPLATE_VIEW_IMAGE
};

// This fuction parses template file and template structure
// it replaces tags in structure with its data in template file
size_t template_parse( struct NNCMS_THREAD_INFO *req,
    const char *szFileName, char *szBuffer, size_t nLen,
    struct NNCMS_TEMPLATE_TAGS *templateTags );
size_t template_sparse( struct NNCMS_THREAD_INFO *req,
    char *lpName, char *pTemplate, char *szBuffer, size_t nLen,
    struct NNCMS_TEMPLATE_TAGS *templateTags );
size_t template_iparse( struct NNCMS_THREAD_INFO *req,
    enum TEMPLATE_INDEX tempNum, char *szBuffer, size_t nLen,
    struct NNCMS_TEMPLATE_TAGS *templateTags );

char *template_escape_html( char *lpszSource );
char *template_escape_uri( char *lpszSource );

// #############################################################################

#endif // __template_h__

// #############################################################################

