// #############################################################################
// Source file: smart.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "smart.h"

// #############################################################################
// includes of system headers
//

#include <stdlib.h>
#include <stdio.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

// test
//
//struct NNCMS_BUFFER buffer =
//    {
//        /* lpBuffer */ malloc( 10 ),
//        /* nSize */ 10,
//        /* nPos */ 0
//    };
//
//smart_cat( &buffer, "1234" );
//smart_cat( &buffer, "1234" );
//smart_cat( &buffer, "1234" );
//smart_cat( &buffer, "1234" );
//smart_cat( &buffer, "1234" );
//
//// Should be "123412341\0"
//printf( "%s\n", buffer.lpBuffer );
//return 0;

// #############################################################################

bool smart_cat( struct NNCMS_BUFFER *buffer, char *lpszString )
{
    // Standart iterator
    unsigned int i = 0;

    // Loop
    while( 1 )
    {
        if( buffer->nPos + 1 >= buffer->nSize )
        {
            // Overflow, place null terminating character
            buffer->lpBuffer[buffer->nPos] = 0;
            return false;
        }

        if( lpszString[i] == 0 )
        {
            // End of string
            buffer->lpBuffer[buffer->nPos] = 0;
            return true;
        }

        // Copy byte
        buffer->lpBuffer[buffer->nPos] = lpszString[i];

        // Next byte
        buffer->nPos++;
        i++;
    }
}

// #############################################################################

bool smart_copy( struct NNCMS_BUFFER *dst, struct NNCMS_BUFFER *src )
{
    // Check if dst buffer fit src buffer
    if( dst->nSize < src->nSize )
    {
        return false;
    }

    // Copy memory area
    memccpy( dst->lpBuffer, src->lpBuffer, 0, src->nSize );
    dst->nPos = src->nPos;
}

// #############################################################################

void smart_reset( struct NNCMS_BUFFER *buffer )
{
    // Quick reset
    *buffer->lpBuffer = 0;
    buffer->nPos = 0;
}

// #############################################################################

bool smart_append_file( struct NNCMS_BUFFER *buffer, char *szFileName )
{
    FILE *pFile = fopen( szFileName, "rb" );
    if( pFile != NULL )
    {
        // Get file size
        fseek( pFile, 0, SEEK_END );
        size_t nSize = ftell( pFile );
        rewind( pFile );

        // Check if file fit in buffer
        if( buffer->nSize - buffer->nPos > nSize )
        {
            // Read
            nSize = fread( &buffer->lpBuffer[buffer->nPos], 1, nSize, pFile );
            buffer->nPos += nSize;

            // Null terminating character
            buffer->lpBuffer[buffer->nPos] = 0;
        }

        fclose( pFile );
    }
}

// #############################################################################
