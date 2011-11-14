// #############################################################################
// Header file: main.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <config.h>

#include <fcgiapp.h>
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

#include "threadinfo.h"
#include "cfg.h"

// #############################################################################

#ifndef __main_h__
#define __main_h__

// #############################################################################
// type and constant definitions
//

//
// Debug stuff
//
//#define VERBOSE_DEBUG   // Prints all memory allocations and at exit prints unfreed
//#define DEBUG           // Minor debug (remembers memory allocations and frees at exit)

// #############################################################################

#define NNCMS_PAGE_SIZE_MAX     64 * 1024 * 10
#define NNCMS_TIME_STR_LEN_MAX  40

// Structure for function
struct NNCMS_FUNCTION
{
    char szName[32];
    char *szDesc;

    // <return-type> (*functionName)(<parameter-list>);
    void (*lpFunc)(struct NNCMS_THREAD_INFO *);
};

// Prefix for all links
extern char homeURL[NNCMS_PATH_LEN_MAX];

// Global Timestamp Format
char szTimestampFormat[128];

// Settings
extern struct NNCMS_OPTION g_options[];

// Default value of quiet attribute in "include" tag
extern bool bIncludeDefaultQuiet;

extern bool bRewrite;

// #############################################################################
// function declarations
//

// Main
int main( int argc, char *argv[] );

// Accept thread
static void *main_loop( void *var );

// Signal handlers
void atquit( );
void quit( int nSignal );

// HTML output and process functions
unsigned int main_output( struct NNCMS_THREAD_INFO *req, char *lpszTitle, char *lpszContent, time_t nModTime );
void main_send_headers( struct NNCMS_THREAD_INFO *req, int nContentLength, time_t nModTime );
void main_set_response( struct NNCMS_THREAD_INFO *req, char *lpszMsg );
void main_set_content_type( struct NNCMS_THREAD_INFO *req, char *type );
void main_add_header( struct NNCMS_THREAD_INFO *req, char *lpszMsg );
void main_set_cookie( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszValue );

// Request handlers
int main_add_variable( struct NNCMS_THREAD_INFO *req, char *lpszName, char *lpszValue);
struct NNCMS_VARIABLE *main_get_variable( struct NNCMS_THREAD_INFO *req, char *lpszName );
char main_from_hex( char c );
char *main_unescape( char *str );
void main_store_data( struct NNCMS_THREAD_INFO *req, char *query );
void main_store_data_rfc1867( struct NNCMS_THREAD_INFO *req, char *query );

// Pages
void main_index( struct NNCMS_THREAD_INFO *req ); // Index page
void main_info( struct NNCMS_THREAD_INFO *req );
void redirect_to_referer( struct NNCMS_THREAD_INFO *req );
void redirectf( struct NNCMS_THREAD_INFO *req, char *lpszUrlFormat, ... );
void redirect( struct NNCMS_THREAD_INFO *req, char *lpszURL );

// Usefull functions
char *main_get_file( char *lpszFileName, size_t *nLimit );
char *main_get_var( char *lpBuf, char *lpszDest, unsigned int nDestSize, char *lpszVarName );
void main_format_time_string( struct NNCMS_THREAD_INFO *req, char *ptr, time_t clock);

// #############################################################################

#endif // __main_h__

// #############################################################################

