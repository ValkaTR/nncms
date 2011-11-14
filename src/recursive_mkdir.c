// #############################################################################
// Source file: recursive_mkdir.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "recursive_mkdir.h"
#include "strlcpy.h"

// #############################################################################
// includes of system headers
//

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

void recursive_mkdir( const char *path )
{
    char opath[256];
    char *p;
    size_t len;

    strlcpy( opath, path, sizeof(opath) );
    len = strlen( opath );
    if( opath[len - 1] == '/' )
        opath[len - 1] = '\0';
    for( p = opath; *p; p++ )
        if( *p == '/' )
        {
            *p = '\0';
            if( access( opath, F_OK ) )
                mkdir( opath, S_IRWXU );
            *p = '/';
        }
    if( access( opath, F_OK ) ) /* if path is not terminated with / */
        mkdir( opath, S_IRWXU );
}

// #############################################################################
