// #############################################################################
// Header file: tree.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// #############################################################################
// includes of local headers
//

// #############################################################################

#ifndef __tree_h__
#define __tree_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_ATTRIBUTE
{
	char *name;     // Attribute name
	char *value;    // Attribute value
	struct NNCMS_ATTRIBUTE *next;   // Pointer to next attribute
};

// #############################################################################
// function declarations
//

// Functions for working with attributes
char *tree_get_attribute( struct NNCMS_ATTRIBUTE *attr, char *name );
void tree_set_attribute( struct NNCMS_ATTRIBUTE **attr, char *name, char *value );

struct NNCMS_ATTRIBUTE *tree_add_attribute( struct NNCMS_ATTRIBUTE **attr );
struct NNCMS_ATTRIBUTE *tree_create_attribute(  );

void tree_destroy_attributes( struct NNCMS_ATTRIBUTE **attr );
void tree_remove_attribute( struct NNCMS_ATTRIBUTE **attr, char *name );
void tree_delete_attribute( struct NNCMS_ATTRIBUTE *a );

// #############################################################################

#endif // __tree_h__

// #############################################################################
