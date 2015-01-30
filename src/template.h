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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#  include <getopt.h>
#else
#  include <sys/time.h>
#endif

#include <glib.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"
//#include "main.h"
#include "smart.h"

#include "form.h"
#include "field.h"
#include "table.h"

// #############################################################################

#ifndef __template_h__
#define __template_h__

// #############################################################################
// type and constant definitions
//

#define NNCMS_TAG_NAME_MAX_SIZE 64

struct NNCMS_LINK
{
    char *function;
    char *title;
    struct NNCMS_VARIABLE *vars;
};

// #############################################################################

// Path to tempates
extern char templateDir[NNCMS_PATH_LEN_MAX];

// Default number of items per page
extern int default_pager_quantity;

// #############################################################################
// function declarations
//

// Module
bool template_global_init( );
bool template_global_destroy( );
bool template_local_init( struct NNCMS_THREAD_INFO *req );
bool template_local_destroy( struct NNCMS_THREAD_INFO *req );

void lua_stackdump( lua_State *L );

// This fuction parses template file and template structure
// it replaces tags in structure with its data in template file
size_t template_sparse( struct NNCMS_THREAD_INFO *req, char *template_name, char *template_data, GString *buffer, struct NNCMS_VARIABLE *template_tags );
size_t template_hparse( struct NNCMS_THREAD_INFO *req, char *template_name, GString *buffer, struct NNCMS_VARIABLE *template_tags );

//
// Generators
//

// Extra functions
char *template_links( struct NNCMS_THREAD_INFO *req, struct NNCMS_LINK *links );
struct NNCMS_FORM *template_confirm( struct NNCMS_THREAD_INFO *req, char *name );

char *template_escape_html( char *src );
char *template_escape_uri( char *lpszSource );

// LUA functions
static int template_lua_load( lua_State *L );
static int template_lua_parse( lua_State *L );
static int template_lua_print( lua_State *L );
static int template_lua_escape_html( lua_State *L );
static int template_lua_escape_uri( lua_State *L );
static int template_lua_escape_js( lua_State *L );
static int template_lua_acl( lua_State *L );
static int template_lua_post_acl( lua_State *L );
static int template_lua_file_acl( lua_State *L );
static int template_lua_login( lua_State *L );
static int template_lua_profile( lua_State *L );
static int template_lua_i18n_string( lua_State *L );


// #############################################################################

#endif // __template_h__

// #############################################################################

