// #############################################################################
// Source file: tree.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "tree.h"
#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #############################################################################
// global variables
//

// #############################################################################
// functions

/*
void tree_attribute_test(  )
{
    struct NNCMS_ATTRIBUTE *attr = 0;
    tree_set_attribute( &attr, "test", "blah" );
    tree_set_attribute( &attr, "xxx", "blah2" );
    printf( "test: %s\nxxx: %s\n\n",
            tree_get_attribute( attr, "test" ),
            tree_get_attribute( attr, "xxx" ) );
    tree_destroy_attributes( &attr );
}
*/

// #############################################################################

struct NNCMS_ATTRIBUTE *tree_create_attribute(  )
{
    struct NNCMS_ATTRIBUTE *attr =
        ( struct NNCMS_ATTRIBUTE * )
        MALLOC( sizeof( struct NNCMS_ATTRIBUTE ) );
    bzero( attr, sizeof( struct NNCMS_ATTRIBUTE ) );
    return attr;
};

// #############################################################################

void tree_delete_attribute( struct NNCMS_ATTRIBUTE *a )
{
    if( a->name )
    {
        free( a->name );
    }
    if( a->value )
    {
        free( a->value );
    }
    FREE( a );
};

// #############################################################################

void tree_destroy_attributes( struct NNCMS_ATTRIBUTE **attr )
{
    struct NNCMS_ATTRIBUTE *current = *attr;
    while( current )
    {
        struct NNCMS_ATTRIBUTE *next = current->next;
        tree_delete_attribute( current );
        current = next;
    }
    *attr = 0;
};

// #############################################################################

char *tree_get_attribute( struct NNCMS_ATTRIBUTE *attr, char *name )
{
    while( attr )
    {
        if( attr->name )
            if( !strcasecmp( attr->name, name ) )
            {
                return attr->value;
            }
        attr = attr->next;
    }

    return NULL;
}

// #############################################################################

void tree_set_attribute( struct NNCMS_ATTRIBUTE **attr, char *name, char *value )
{
    if( !name )
        return;

    struct NNCMS_ATTRIBUTE *current = *attr;
    while( current )
    {
        if( current->name )
            if( !strcasecmp( current->name, name ) )
            {
                break;
            }
        current = current->next;
    }

    if( !current )
    {
        current = tree_add_attribute( attr );
        current->name = strdup( name );
    }

    if( current->value )
    {
        free( current->value );
    }

    if( value )
        current->value = strdup( value );
    else
        current->value = strdup( "" );
}

// #############################################################################

void tree_remove_attribute( struct NNCMS_ATTRIBUTE **attr, char *name )
{
    if( ( *attr )->name )
        if( !strcasecmp( ( *attr )->name, name ) )
        {
            *attr = ( *attr )->next;
            tree_delete_attribute( *attr );
            return;
        }

    struct NNCMS_ATTRIBUTE *current = *attr;
    while( current->next )
    {
        if( current->next->name )
            if( !strcasecmp( current->next->name, name ) )
            {
                struct NNCMS_ATTRIBUTE *todel = current->next;
                current->next = todel->next;
                tree_delete_attribute( todel );
                return;
            }
        current = current->next;
    }
}

// #############################################################################

struct NNCMS_ATTRIBUTE *tree_add_attribute( struct NNCMS_ATTRIBUTE **attr )
{
    struct NNCMS_ATTRIBUTE *newattr = tree_create_attribute(  );
    struct NNCMS_ATTRIBUTE *current = *attr;

    if( !current )
    {
        ( *attr ) = newattr;
    }
    else
    {
        while( current->next )
        {
            current = current->next;
        }
        current->next = newattr;
    }
    return newattr;
};

// #############################################################################
