// #############################################################################
// Header file: file.h

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

#include <sys/stat.h>
#include <dirent.h>

#include <sqlite3.h>

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"
#include "main.h"

// #############################################################################

#ifndef __file_h__
#define __file_h__

// #############################################################################
// type and constant definitions
//

// From ls.c
struct NNCMS_FILE_ENTITY
{
    char fname[NNCMS_PATH_LEN_MAX];
    struct stat f_stat;
};

struct NNCMS_FILE_ICON
{
    char szExtension[16];
    char *szIcon;
};

struct NNCMS_FILE_INFO
{
    char lpszRelFullPath[NNCMS_PATH_LEN_MAX];  // Full relative to fileDir path to file
    char lpszRelPath[NNCMS_PATH_LEN_MAX];      // Relative to fileDir path to file

    char lpszAbsFullPath[NNCMS_PATH_LEN_MAX];  // Absolute full path to file
    char lpszAbsPath[NNCMS_PATH_LEN_MAX];      // Absolute path

    char lpszFileName[NNCMS_PATH_LEN_MAX];     // File name
    char lpszExtension[16]; // File extension
    char *lpszMimeType;     // Mime type of file
    char *lpszIcon;         // Icon for this file
    char lpszParentDir[NNCMS_PATH_LEN_MAX];    // Parent directory

    struct stat fileStat;

    unsigned int nSize;     // File size
    bool bIsDir;   // Is directory?
    bool bIsFile;  // Is file?
    bool bExists;  // Does this file exist in filesystem?
};

// For 'view' module
extern char fileDir[NNCMS_PATH_LEN_MAX];

// #############################################################################
// function declarations
//

// File/Dir Info functions
void file_get_info( struct NNCMS_FILE_INFO *fileInfo, char *lpszFile );

// Pages
void file_add( struct NNCMS_THREAD_INFO *req );
void file_browse( struct NNCMS_THREAD_INFO *req );
void file_delete( struct NNCMS_THREAD_INFO *req );
void file_download( struct NNCMS_THREAD_INFO *req );
void file_edit( struct NNCMS_THREAD_INFO *req );
void file_gallery( struct NNCMS_THREAD_INFO *req );
void file_mkdir( struct NNCMS_THREAD_INFO *req );
void file_rename( struct NNCMS_THREAD_INFO *req );
void file_upload( struct NNCMS_THREAD_INFO *req );

// From ls.c
int file_compare_name_asc( const void *a, const void *b );
int file_compare_name_desc( const void *a, const void *b );
int file_compare_date_asc( const void *a, const void *b );
int file_compare_date_desc( const void *a, const void *b );

// Other usefull functions
struct NNCMS_FILE_ENTITY *file_get_directory( struct NNCMS_THREAD_INFO *req, char *lpszPath, unsigned int *nDirCount, int(*compar)(const void *, const void *) );
bool file_is_exists( char *szFile );
char *file_get_icon( char *lpszFile );
void file_human_size( char *szDest, unsigned int nSize );

// Check if user has access to lpszPerm
bool file_check_perm( struct NNCMS_THREAD_INFO *req, struct NNCMS_FILE_INFO *fileInfo, char *lpszPerm );

// #############################################################################

#endif // __file_h__

// #############################################################################
