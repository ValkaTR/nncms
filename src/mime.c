// #############################################################################
// Source file: mime.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <libgen.h>

// #############################################################################
// includes of local headers
//

#include "mime.h"

// #############################################################################
// global variables
//

// Is this module initialized?
char is_mimeInit = 0;

/* $Id: mimetype.c 122 2007-06-10 11:12:21Z byshen $ */
// From orzhttpd

/* NB: the following table must be sorted lexically. */
struct NNCMS_MIME mime_tab[] = {
    {".asc",    "text/plain"},
    {".asf",    "video/x-ms-asf"},
    {".asx",    "video/x-ms-asf"},
    {".avi",    "video/x-msvideo"},
    {".bz2",    "application/x-bzip"},
    {".c",      "text/plain"},
    {".class",  "application/octet-stream"},
    {".conf",   "text/plain"},
    {".cpp",    "text/plain"},
    {".css",    "text/css"},
    {".dtd",    "text/xml"},
    {".dvi",    "application/x-dvi"},
    {".gif",    "image/gif"},
    {".gz",     "application/x-gzip"},
    {".h",      "text/plain"},
    {".htm",    "text/html"},
    {".html",   "text/html"},
    {".jpeg",   "image/jpeg"},
    {".jpg",    "image/jpeg"},
    {".js",     "text/javascript"},
    {".log",    "text/plain"},
    {".m3u",    "audio/x-mpegurl"},
    {".mov",    "video/quicktime"},
    {".mp3",    "audio/mpeg"},
    {".mpeg",   "video/mpeg"},
    {".mpg",    "video/mpeg"},
    {".ogg",    "application/ogg"},
    {".pac",    "application/x-ns-proxy-autoconfig"},
    {".pdf",    "application/pdf"},
    {".png",    "image/png"},
    {".ps",     "application/postscript"},
    {".qt",     "video/quicktime"},
    {".s",      "text/plain"},
    {".sig",    "application/pgp-signature"},
    {".spl",    "application/futuresplash"},
    {".swf",    "application/x-shockwave-flash"},
    {".tar",    "application/x-tar"},
    {".tar.bz2",    "application/x-bzip-compressed-tar"},
    {".tar.gz", "application/x-tgz"},
    {".tbz",    "application/x-bzip-compressed-tar"},
    {".text",   "text/plain"},
    {".tgz",    "application/x-tgz"},
    {".torrent",    "application/x-bittorrent"},
    {".txt",    "text/plain"},
    {".wav",    "audio/x-wav"},
    {".wax",    "audio/x-ms-wax"},
    {".wma",    "audio/x-ms-wma"},
    {".wmv",    "video/x-ms-wmv"},
    {".xbm",    "image/x-xbitmap"},
    {".xml",    "text/xml"},
    {".xpm",    "image/x-xpixmap"},
    {".xwd",    "image/x-xwindowdump"},
    {".zip",    "application/zip"}
};

char mime_default[] = "application/octet-stream";

// #############################################################################
// functions

// Init module
bool mime_init( )
{
    if( is_mimeInit == false )
    {
        is_mimeInit = true;

        //
        // Initilization code
        //

        return true;
    }
    else
    {
        return false;
    }
}

// #############################################################################

// DeInit module
bool mime_deinit( )
{
    if( is_mimeInit == true )
    {
        is_mimeInit = false;

        //
        // DeInitilization code
        //

        return true;
    }
    else
    {
        return false;
    }
}

// #############################################################################

static int mimecmp( const void *a, const void *b )
{
    return ( strcasecmp(((const struct NNCMS_MIME *)a)->extension,
        ((const struct NNCMS_MIME *)b)->extension) );
}

// #############################################################################

char *get_extension( char *fpath )
{
    return strrchr(basename((char *)fpath), '.');
}

// #############################################################################

char *get_mime( char *fpath )
{
    struct NNCMS_MIME tmp, *mime;
    char *ext;

    if( fpath && (ext = get_extension(fpath)) != 0 )
    {
        tmp.extension = ext;
        if( (mime = bsearch(&tmp, mime_tab, sizeof(mime_tab)/sizeof(struct NNCMS_MIME), sizeof(struct NNCMS_MIME), mimecmp)) != 0 )
            return mime->mimetype;
    }
    return mime_default;
}

// #############################################################################
