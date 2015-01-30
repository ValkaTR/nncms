// #############################################################################
// Header file: filter.h

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

#include "template.h"

// #############################################################################

#ifndef __filter_h__
#define __filter_h__

// #############################################################################
// type and constant definitions
//

extern char unix_file_validate[];
extern char printable_validate[];
extern char numeric_validate[];
extern char alphanum_validate[];
extern char alphanum_underscore_validate[];

extern char alphanum_filter[];
extern char usernameFilter_cp1251[];
extern char nicknameFilter_cp1251[];
extern char passwordFilter_cp1251[];
extern char emailFilter_cp1251[];
extern char uriFilter[256];
extern char pathFilter[];
extern char queryFilter[256];
extern char numericFilter[256];

// #############################################################################
// function declarations
//

char *filter_str_to_ptr( char *str, struct NNCMS_VARIABLE *vars );
int filter_str_to_int( char *str, struct NNCMS_VARIABLE *vars );
char *filter_int_to_str( int val, struct NNCMS_VARIABLE *vars );

// Validation
bool filter_option_validation( char *buff, struct NNCMS_SELECT_OPTIONS *options );
bool filter_table_validation( char *buff, char *table );
bool filter_size_limit( char *buff, size_t low, size_t high );

// Strint functions, these process strings
void filter_table_replace( char *dest, size_t nLen, char *table ); // Table replace
void filter_truncate_string( char *dest, size_t nLen ); // Function for truncating a string

// Filter path name
void filter_path( char *lpszPath, unsigned char bBeginSlash, unsigned char bEndSlash );

// #############################################################################

#endif // __filter_h__

// #############################################################################
