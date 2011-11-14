// #############################################################################
// Source file: strcasestr.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "strcasestr.h"

// #############################################################################
// includes of system headers
//

#include <string.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

#ifndef HAVE_STRCASESTR
char *strcasestr(register char *where, char *what)
{
	char c1[] = {0, 0};
	char c2[] = {0, 0};

	*c1 = *what;
	*c2 = *what;

#ifdef	HAVE_STRUPR
	strupr(c1);
#else
#	ifdef HAVE__STRUPR
	_strupr(c1);
#	endif
#endif

#ifdef	HAVE_STRLWR
	strlwr(c2);
#else
#	ifdef HAVE__STRLWR
	_strlwr(c2);
#	endif
#endif

	while(*where)
	{
		while(*where && *where != *c1 && *where != *c2)
			where ++;
#ifdef	HAVE_STRCASECMP
		if(!strcasecmp(where, what))
#else
#	ifdef	HAVE_STRICMP
		if(!stricmp(where, what))
#	endif
#endif
			return where;
	}
}
#endif

// #############################################################################
