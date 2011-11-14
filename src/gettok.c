// #############################################################################
// Source file: gettok.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "gettok.h"
#include "strlcpy.h"

// #############################################################################
// includes of system headers
//

#include <string.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

// Разделяет буффер lpszSrc по знаку chDelimiter, находит ячейку с указанным
// индексом nIndex и копирует её в буффер lpszDest. Отрицательный индекс
// копирует начиная с найденной ячейкой до последней.
void gettok( char *lpszSrc, char *lpszDst, int nIndex, char chDelimiter )
{
    char *lpszStart = lpszSrc;
    char *lpszEnd = strchr( lpszSrc, chDelimiter );

    bool bWasNeg = false;

    if( nIndex < 0 )
    {
        bWasNeg = true;
        nIndex = -nIndex;
    }

    for( unsigned int i = 0; i < nIndex; i++ )
    {
        if( lpszStart != 0 )
            lpszStart = strchr( lpszStart + 1, chDelimiter );

        if( lpszEnd != 0 )
            lpszEnd = strchr( lpszEnd + 1, chDelimiter );
    }

    *lpszDst = 0;
    if( lpszStart != 0 )
    {
        if( *lpszStart == ' ' ) lpszStart++;
        if( lpszEnd == 0 || bWasNeg == true )
        {
            strcpy( lpszDst, lpszStart );
        }
        else
        {
            strlcpy( lpszDst, lpszStart, lpszEnd - lpszStart + 1 );
            //lpszDst[lpszEnd - lpszStart] = 0;
        }
    }
}

// #############################################################################
