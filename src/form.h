// #############################################################################
// Header file: xxx.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of system headers
//

#include <stdbool.h>

// #############################################################################
// includes of local headers
//

#include "threadinfo.h"
#include "database.h"
#include "field.h"

// #############################################################################

#ifndef __form_h__
#define __form_h__

// #############################################################################
// type and constant definitions
//

struct NNCMS_FORM
{
    // Form options
    char *name;
    char *action;
    char *method;
    
    // Information about form
    char *title;
    char *help;

    // Extra html added before and after form
    char *header_html;
    char *footer_html;

    // Fields
    struct NNCMS_FIELD *fields;
};

// #############################################################################
// function declarations
//

char *form_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_FORM *form );

// #############################################################################

#endif // __form_h__

// #############################################################################
