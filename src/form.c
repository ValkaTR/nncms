// #############################################################################
// Source file: xxx.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "form.h"
#include "acl.h"
#include "template.h"
#include "user.h"
#include "log.h"
#include "i18n.h"
#include "memory.h"

// #############################################################################
// includes of system headers
//

// #############################################################################
// global variables
//

// #############################################################################
// functions

//
// Forms
//

// Outputs HTML formatted form
// Uses req->temp buffer
char *form_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_FORM *form )
{
    GString *html = g_string_sized_new( 1024 );

    // Create an HTML form for user input
    struct NNCMS_VARIABLE form_tags[] =
    {
        { .name = "form_name", .value.string = form->name, .type = NNCMS_TYPE_STRING },
        { .name = "form_action", .value.string = form->action, .type = NNCMS_TYPE_STRING },
        { .name = "form_method", .value.string = form->method, .type = NNCMS_TYPE_STRING },
        { .name = "form_fields", .value.string = 0, .type = NNCMS_TYPE_STRING },
        { .name = "form_title", .value.string = form->title, .type = NNCMS_TYPE_STRING },
        { .name = "form_help", .value.string = form->help, .type = NNCMS_TYPE_STRING },
        { .name = "form_header_html", .value.string = form->header_html, .type = NNCMS_TYPE_STRING },
        { .name = "form_footer_html", .value.string = form->footer_html, .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };
    
    // Update fields from POST request
    if( strcmp( req->request_method, "POST" ) == 0 )
    {
        form_post_data( req, form->fields );
    }

    // Iterate throught fields
    //
    // The output html is written to req->temp
    //
    g_string_assign( req->temp, "" );
    if( form->fields != NULL )
    {
        for( unsigned int i = 0; form->fields[i].type != NNCMS_FIELD_NONE; i++ )
        {
            field_parse( req, &form->fields[i], req->temp );
        }
    }

    // Place fields in form template
    form_tags[3].value.string = req->temp->str;

    template_hparse( req, "template_form.html", html, form_tags );
    
    char *html_str = g_string_free( html, FALSE );
    garbage_add( req->loop_garbage, html_str, MEMORY_GARBAGE_GFREE );
    
    return html_str;
}

// #############################################################################
