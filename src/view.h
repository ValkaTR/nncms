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

#define THUMBNAIL_WIDTH_MAX      640
#define THUMBNAIL_WIDTH_MIN      32
#define THUMBNAIL_HEIGTH_MAX     480
#define THUMBNAIL_HEIGTH_MIN     32

// #############################################################################
// function declarations
//

// Pages
void view_show( struct NNCMS_THREAD_INFO *req );
void view_thumbnail( struct NNCMS_THREAD_INFO *req );

void view_advanced_parse( struct NNCMS_THREAD_INFO *req,
    char *lpszSrc, size_t nSrcLen,
    char *lpszDst, size_t nDstLen );
struct NNCMS_ATTRIBUTE *view_decode_attributes( struct NNCMS_THREAD_INFO *req, char *lpszAttributes );
size_t view_math( struct NNCMS_THREAD_INFO *req, char *lpszDst, size_t nDstLen, char *lpszExpression );
void hex_dump( unsigned char *dst, unsigned int dst_size,
               unsigned char *src, unsigned int src_size );

// Aux functions
void view_force_cache( struct NNCMS_THREAD_INFO *req );
void view_iw_exception( struct NNCMS_THREAD_INFO *req, MagickWand *wand, char *lpszError, char *lpBuf );

// #############################################################################

#endif // __view_h__

// #############################################################################
