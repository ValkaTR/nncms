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
#include <ftw.h>

#include <sqlite3.h>

#include <stdbool.h>

#include <glib.h>

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

#define THUMBNAIL_WIDTH_MAX      640
#define THUMBNAIL_WIDTH_MIN      32
#define THUMBNAIL_HEIGTH_MAX     480
#define THUMBNAIL_HEIGTH_MIN     32

#define	NNCMS_IRUSR	0x0400	/* Read by owner.  */
#define	NNCMS_IWUSR	0x0200	/* Write by owner.  */
#define	NNCMS_IXUSR	0x0100	/* Execute by owner.  */
/* Read, write, and execute by owner.  */
#define	NNCMS_IRWXU	(NNCMS_IRUSR|NNCMS_IWUSR|NNCMS_IXUSR)

#define	NNCMS_IRGRP	(NNCMS_IRUSR >> 4)	/* Read by group.  */
#define	NNCMS_IWGRP	(NNCMS_IWUSR >> 4)	/* Write by group.  */
#define	NNCMS_IXGRP	(NNCMS_IXUSR >> 4)	/* Execute by group.  */
/* Read, write, and execute by group.  */
#define	NNCMS_IRWXG	(NNCMS_IRWXU >> 4)

#define	NNCMS_IROTH	(NNCMS_IRGRP >> 4)	/* Read by others.  */
#define	NNCMS_IWOTH	(NNCMS_IWGRP >> 4)	/* Write by others.  */
#define	NNCMS_IXOTH	(NNCMS_IXGRP >> 4)	/* Execute by others.  */
/* Read, write, and execute by others.  */
#define	NNCMS_IRWXO	(NNCMS_IRWXG >> 4)

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

struct NNCMS_FILE_INFO_V2
{
    char *mime_type;     // Mime type of file
    char *icon;          // Icon for this file

    struct stat fstat;

    char *size;    // File size
    time_t timestamp;
    
    bool is_dir;   // Is directory?
    bool is_file;  // Is file?
    bool exists;   // Does this file exist in filesystem?
    bool binary;   // Binary or text content
};

// For 'view' module
extern char fileDir[NNCMS_PATH_LEN_MAX];

// #############################################################################

struct NNCMS_FILE_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *user_id;
    char *parent_file_id;
    char *name;
    char *type;
    char *title;
    char *description;
    char *group;
    char *mode;
    char *value[NNCMS_COLUMNS_MAX - 9];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

bool file_global_init( );
bool file_global_destroy( );
bool file_local_init( struct NNCMS_THREAD_INFO *req );
bool file_local_destroy( struct NNCMS_THREAD_INFO *req );

// File/Dir Info functions
void file_get_info( struct NNCMS_FILE_INFO *fileInfo, char *lpszFile );
struct NNCMS_FILE_INFO_V2 *file_get_info_v2( struct NNCMS_THREAD_INFO *req, char *file_path );

// Pages
void file_upload( struct NNCMS_THREAD_INFO *req );
void file_mkdir( struct NNCMS_THREAD_INFO *req );
void file_add( struct NNCMS_THREAD_INFO *req );
void file_view( struct NNCMS_THREAD_INFO *req );
void file_gallery( struct NNCMS_THREAD_INFO *req );
void file_list( struct NNCMS_THREAD_INFO *req );
void file_delete( struct NNCMS_THREAD_INFO *req );
void file_edit( struct NNCMS_THREAD_INFO *req );

// Other usefull functions
char *file_get_icon( char *lpszFile );
void file_human_size( char *szDest, unsigned int nSize );
GString *file_dbfs_path( struct NNCMS_THREAD_INFO *req, int file_id );
struct NNCMS_SELECT_OPTIONS *file_folder_list( struct NNCMS_THREAD_INFO *req, char *file_id );
char *file_thumbnail( struct NNCMS_THREAD_INFO *req, char *file_id, unsigned int width, unsigned int height );

// Check if user has access to permission
bool file_check_perm( struct NNCMS_THREAD_INFO *req, char *user_id, char *file_id, bool read_check, bool write_check, bool exec_check );

// #############################################################################

#endif // __file_h__

// #############################################################################
