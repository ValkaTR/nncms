// #############################################################################
// Header file: view.h

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

#include <sqlite3.h>

#ifdef HAVE_IMAGEMAGICK
//#include <magick/MagickCore.h>
#include <wand/MagickWand.h>
#endif // HAVE_IMAGEMAGICK

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"
#include "sha2.h"

// #############################################################################

#ifndef __view_h__
#define __view_h__

// #############################################################################
// type and constant definitions
//

extern struct NNCMS_VARIABLE view_format_list[];

#define THUMBNAIL_WIDTH_MAX      640
#define THUMBNAIL_WIDTH_MIN      32
#define THUMBNAIL_HEIGTH_MAX     480
#define THUMBNAIL_HEIGTH_MIN     32

struct NNCMS_VIEWS_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *name;
    char *format;
    char *header;
    char *footer;
    char *pager_items;
    char *value[NNCMS_COLUMNS_MAX - 6];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_VIEWS_FILTER_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *view_id;
    char *field_id;
    char *operator;
    char *data;
    char *data_exposed;
    char *filter_exposed;
    char *operator_exposed;
    char *required ;
    char *value[NNCMS_COLUMNS_MAX - 9];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_VIEWS_SORT_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *view_id;
    char *field_id;
    char *direction;
    char *value[NNCMS_COLUMNS_MAX - 4];
    
    struct NNCMS_ROW *next; // BC
};

struct NNCMS_FIELD_VIEW_ROW
{
    char *col_name[NNCMS_COLUMNS_MAX];
    
    char *id;
    char *view_id;
    char *field_id;
    char *label;
    char *empty_hide;
    char *config;
    char *weight;
    char *value[NNCMS_COLUMNS_MAX - 7];
    
    struct NNCMS_ROW *next; // BC
};

// #############################################################################
// function declarations
//

// Modular functions
bool view_global_init( );
bool view_global_destroy( );
bool view_local_init( struct NNCMS_THREAD_INFO *req );
bool view_local_destroy( struct NNCMS_THREAD_INFO *req );

// Pages
void view_list( struct NNCMS_THREAD_INFO *req );
void view_add( struct NNCMS_THREAD_INFO *req );
void view_edit( struct NNCMS_THREAD_INFO *req );
void view_view( struct NNCMS_THREAD_INFO *req );
void view_delete( struct NNCMS_THREAD_INFO *req );

void field_view_list( struct NNCMS_THREAD_INFO *req );
void field_view_add( struct NNCMS_THREAD_INFO *req );
void field_view_edit( struct NNCMS_THREAD_INFO *req );
void field_view_view( struct NNCMS_THREAD_INFO *req );
void field_view_delete( struct NNCMS_THREAD_INFO *req );

void view_filter_list( struct NNCMS_THREAD_INFO *req );
void view_filter_add( struct NNCMS_THREAD_INFO *req );
void view_filter_edit( struct NNCMS_THREAD_INFO *req );
void view_filter_view( struct NNCMS_THREAD_INFO *req );
void view_filter_delete( struct NNCMS_THREAD_INFO *req );

// Functions
char *view_links( struct NNCMS_THREAD_INFO *req, char *view_id );

// Pages
void view_display( struct NNCMS_THREAD_INFO *req );

void view_prepare_fields( struct NNCMS_THREAD_INFO *req, struct NNCMS_FIELD *fields, char *view_id );

// #############################################################################

#endif // __view_h__

// #############################################################################
