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

#include "xxx.h"
#include "acl.h"
#include "template.h"
#include "user.h"
#include "log.h"
#include "i18n.h"
#include "memory.h"
#include "filter.h"

// #############################################################################
// includes of system headers
//

// #############################################################################
// global variables
//

struct NNCMS_SELECT_ITEM filter_operators[] =
{
    { .name = "equal", .desc = NULL, .selected = false },
    { .name = "notequal", .desc = NULL, .selected = false },
    { .name = "like", .desc = NULL, .selected = false },
    { .name = "glob", .desc = NULL, .selected = false },
    { .name = "less", .desc = NULL, .selected = false },
    { .name = "less_equal", .desc = NULL, .selected = false },
    { .name = "greater", .desc = NULL, .selected = false },
    { .name = "greater_equal", .desc = NULL, .selected = false },
    { .name = NULL, .desc = NULL, .selected = false }
};

// #############################################################################
// functions

void filter_query_prepare( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query )
{
    for( unsigned int i = 0; query->filter[i].operator != NNCMS_OPERATOR_NONE; i = i + 1 )
    {
        if( query->filter[i].filter_exposed == false )
            continue;
        
        char *field_value = main_variable_get( req, req->get_tree, query->filter[i].field_name );
        query->filter[i].field_value = field_value;

        char filter_operator_name[32];
        snprintf( filter_operator_name, sizeof(filter_operator_name), "%s_op", query->filter[i].field_name );
        char *filter_operator = main_variable_get( req, req->get_tree, filter_operator_name );
        if( filter_operator != NULL )
            query->filter[i].operator = filter_str_to_int( filter_operator, filter_operator_list );
    }
}

// #############################################################################

char *filter_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query )
{
    GString *filter = g_string_sized_new( 1024 );
    
    for( unsigned int i = 0; query->filter[i].operator != NNCMS_OPERATOR_NONE; i = i + 1 )
    {
        //struct NNCMS_VARIABLE vars[] =
        //{
        //    { .name = "field_name", .value.string = query->filter[i].field_name, .type = NNCMS_TYPE_STRING },
        //    { .name = "field_value", .value.string = query->filter[i].field_value, .type = NNCMS_TYPE_STRING },
        //};

        char filter_operator_name[32];
        snprintf( filter_operator_name, sizeof(filter_operator_name), "%s_op", query->filter[i].field_name );
        char *filter_operator = main_variable_get( req, req->get_tree, filter_operator_name );

        struct NNCMS_FIELD filter_fields[] =
        {
            { .name = query->filter[i].field_name, .value = query->filter[i].field_value, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL, .size_min = 0, .size_max = 1000 },
            { .name = filter_operator_name, .value = filter_operator, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = filter_operators }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
            { .name = "", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = i18n_string_temp( req, "apply", NULL ), .text_description = "" },
            { .type = NNCMS_FIELD_NONE }
        };
        
        struct NNCMS_FORM filter_form =
        {
            .name = "filter", .action = NULL, .method = "GET",
            .title = NULL, .help = NULL,
            .header_html = NULL, .footer_html = NULL,
            .fields = (struct NNCMS_FIELD *) filter_fields
        };
        
        char *html = form_html( req, &filter_form );
        g_string_append( filter, html );
    }
    
    char *full_html = g_string_free( filter, FALSE );
    garbage_add( req->loop_garbage, full_html, MEMORY_GARBAGE_GFREE );
    return full_html;
}

// #############################################################################

void pager_query_prepare( struct NNCMS_THREAD_INFO *req, struct NNCMS_QUERY *query )
{
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    
    int start = 0;
    if( http_start != 0 )
    {
        start = atoi( http_start );
    }
    
    query->offset = start;
    query->limit = default_pager_quantity;
}

// #############################################################################

char *pager_html( struct NNCMS_THREAD_INFO *req, int count )
{
    const int pages_displayed = 2;
    
    // Load pager data
    char *http_start = main_variable_get( req, req->get_tree, "start" );
    int start = 0; int end = start + default_pager_quantity;
    if( http_start != 0 )
    {
        start = atoi( http_start );
        end = start + default_pager_quantity;
    }
    if( start >= count ) start = count - 1;
    if( end >= count ) end = count;
    
    // What page is opened?
    int pages = count / default_pager_quantity;
    if( count % default_pager_quantity > 0 ) pages = 1 + pages;
    int cur_page = start / default_pager_quantity + 1;
    if( start % default_pager_quantity > 0 ) cur_page = 0;

    //
    // Generate pager
    //

    GString *pager = g_string_sized_new( 1024 );
    if( pages > 1 )
    {
        g_string_append( pager, "<div class=\"item-list\">\n" );
        g_string_append( pager, "    <ul class=\"pager\">\n" );

        // Print "previous" link if not first page
        if( cur_page > 1 )
        {
            char *temp; char *page_str; char *link;
            
            main_variable_add( req, req->get_tree, "start", "0" );
            link = main_glink( req, req->function_name, NULL, req->get_tree );
            temp = i18n_string( req, "pager_first", NULL );
            g_string_append_printf( pager, "        <li class=\"pager-first\"><a href=\"%s\">%s</a></li>\n", link, temp );
            g_free( temp );
            g_free( link );
            
            page_str = g_strdup_printf( "%u", (cur_page - 2) * default_pager_quantity );
            main_variable_add( req, req->get_tree, "start", page_str );
            link = main_glink( req, req->function_name, NULL, req->get_tree );
            temp = i18n_string( req, "pager_prev", NULL );
            g_string_append_printf( pager, "        <li class=\"pager-prev\"><a href=\"%s\">%s</a></li>\n", link, temp );
            g_free( temp );
            g_free( page_str );
            g_free( link );
        }

        // Limit number of links after and before current page
        // Example: first prev ... 100 101 102 103 104 ... next last
        int pages_start = cur_page - pages_displayed;
        int pages_end = cur_page + pages_displayed;
        if( pages_start < 1 )
        {
            pages_start = 1;
            pages_end = pages_start + pages_displayed * 2;
        }
        if( pages_end > pages )
        {
            pages_end = pages;
            pages_start = pages_end - pages_displayed * 2;
        }
        if( pages_start < 1 )
        {
            pages_start = 1;
        }

        // Dots
        if( pages_start > 1 )
            g_string_append( pager, "        <li>…</li>\n" );

        // Print page links
        for( unsigned int i = pages_start; i < pages_end + 1; i++ )
        {
            if( i == cur_page )
            {
                g_string_append_printf( pager, "        <li class=\"pager-current\">%u</li>\n", i );
            }
            else
            {
                // Generate link
                char *start_str = g_strdup_printf( "%u", (i - 1) * default_pager_quantity );
                main_variable_add( req, req->get_tree, "start", start_str );
                char *link = main_glink( req, req->function_name, NULL, req->get_tree );

                g_string_append_printf( pager, "        <li class=\"pager-item\"><a href=\"%s\">%u</a></li>\n", link, i );
                
                g_free( start_str );
                g_free( link );
            }
        }
        
        // Dots
        if( pages_end < pages )
            g_string_append( pager, "        <li>…</li>\n" );

        // Print "next" link if not first page
        if( cur_page != pages )
        {
            
            char *temp; char *page_str; char *link;

            page_str = g_strdup_printf( "%u", cur_page * default_pager_quantity );
            main_variable_add( req, req->get_tree, "start", page_str );
            link = main_glink( req, req->function_name, NULL, req->get_tree );
            temp = i18n_string( req, "pager_next", NULL );
            g_string_append_printf( pager, "        <li class=\"pager-next\"><a href=\"%s\">%s</a></li>\n", link, temp );
            g_free( temp );
            g_free( page_str );
            g_free( link );
            
            page_str = g_strdup_printf( "%u", (pages - 1) * default_pager_quantity );
            main_variable_add( req, req->get_tree, "start", page_str );
            link = main_glink( req, req->function_name, NULL, req->get_tree );
            temp = i18n_string( req, "pager_last", NULL );
            g_string_append_printf( pager, "        <li class=\"pager-last\"><a href=\"%s\">%s</a></li>\n", link, temp );
            g_free( temp );
            g_free( page_str );
            g_free( link );
        }

        g_string_append( pager, "    </ul>\n" );
        g_string_append( pager, "</div>\n" );
    }
    else
    {
        g_string_assign( pager, "" );
    }
    
    // Return HTML code
    char *pager_html = g_string_free( pager, FALSE );
    garbage_add( req->loop_garbage, pager_html, MEMORY_GARBAGE_GFREE );
    return pager_html;
}


// #############################################################################

//
// Tables
//

// #############################################################################

char *table_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_TABLE *table )
{
    // Variables
    unsigned int i;
    unsigned int row_count;
    unsigned int column_count;
    
    GString *html = g_string_sized_new( 1024 );
    
    // Defaults
    if( table->pager_quantity == 0 ) table->pager_quantity = default_pager_quantity;
    if( table->pages_displayed == 0 ) table->pages_displayed = 2;
    if( table->caption == NULL ) table->caption = "";
    if( table->cellpadding == NULL ) table->cellpadding = "5";
    if( table->cellspacing == NULL ) table->cellspacing = "0";
    if( table->border == NULL ) table->border = "0";
    if( table->bgcolor == NULL ) table->bgcolor = "#FFFFFF";
    if( table->width == NULL ) table->width = "100%";
    if( table->height == NULL ) table->height = "";
    if( table->header_html == NULL ) table->header_html = "";
    if( table->footer_html == NULL ) table->footer_html = "";
    
    // Add header to html
    g_string_append( html, table->header_html );

    // Add pager to html
    char *pager = pager_html( req, table->row_count );
    g_string_append( html, pager );

    //
    // HTML Table start
    //
    g_string_append( html, "<table class=\"template-table\"" );
    
    // Print variables
    if( table->options != 0 )
    {
        for( unsigned int i = 0; table->options[i].type != NNCMS_TYPE_NONE; i++ )
        {
            // Value is unescaped!
            g_string_append_printf( html, " %s=\"%s\"", table->options[i].name, table->options[i].value.string );
        }
    }
    
    if( table->cellpadding ) g_string_append_printf( html, " cellpadding=\"%s\"", table->cellpadding );
    if( table->cellspacing ) g_string_append_printf( html, " cellspacing=\"%s\"", table->cellspacing );
    if( table->border ) g_string_append_printf( html, " border=\"%s\"", table->border );
    if( table->bgcolor ) g_string_append_printf( html, " bgcolor=\"%s\"", table->bgcolor );
    if( table->width ) g_string_append_printf( html, " width=\"%s\"", table->width );
    if( table->height ) g_string_append_printf( html, " height=\"%s\"", table->height );
    
    g_string_append( html, ">\n" );
    
    // Print caption if there is
    if( table->caption )
        g_string_append_printf( html, "    <caption>%s</caption>\n", table->caption );

    //
    // Header cells
    //
    g_string_append( html, "    <thead class=\"template-table\">\n" );
    g_string_append( html, "    <tr>\n" );
    for( i = 0; true; i++ )
    {
        struct NNCMS_TABLE_CELL *cell = &table->headerz[i];
        if( cell == NULL ) break;
        if( cell->type == NNCMS_TYPE_NONE ) break;
        
        // Type convert
        char *pbuffer = main_type( req, cell->value, cell->type );
        
        // Print variables
        if( cell->options != NULL )
        {
            g_string_append( html, "        <th class=\"template-table\"" );
            for( unsigned int k = 0; cell->options[k].type != NNCMS_TYPE_NONE; k++ )
            {
                // Value is unescaped!
                g_string_append_printf( html, " %s=\"%s\"", cell->options[k].name, cell->options[k].value.string );
            }
            g_string_append( html, ">" );
            g_string_append( html, pbuffer );
            g_string_append( html, "</th>\n" );
        }
        else
        {
            g_string_append_printf( html, "        <th class=\"template-table\">%s</th>\n", pbuffer );
        }

        g_free( pbuffer );
    }
    g_string_append( html, "    </tr>\n" );
    g_string_append( html, "    </thead>\n" );
    
    // Remember column count
    column_count = i;

    g_string_append( html, "    <tbody class=\"template-table\">\n" );

    //
    // Rows
    //
    // Iterate throught rows
    // #define g_array_index(a,t,i)      ( ((t*) (void *) (a)->data) [(i)] )
    for( i = 0; i < table->pager_quantity; i++ )
    {
        struct NNCMS_TABLE_CELL *row = &table->cells[i * column_count];
        if( row == NULL ) break;
        if( row->type == NNCMS_TYPE_NONE ) break;

        // Determine class
        char *class = "";
        if( i == 0 )
            class = "template-table-first template-table-odd";
        else if( i % 2 == 0 )
            if( row[1].type == NNCMS_TYPE_NONE )
                class = "template-table-last template-table-odd";
            else
                class = "template-table-odd";
        else if( i % 2 == 1 )
            if( row[1].type == NNCMS_TYPE_NONE )
                class = "template-table-last template-table-even";
            else
                class = "template-table-even";

        g_string_append_printf( html, "    <tr class=\"%s\">\n", class );
        
        //
        // Print cells
        //
        for( unsigned int j = 0; j < column_count; j++ )
        {
            struct NNCMS_TABLE_CELL *cell = &row[j];
            if( cell == NULL ) break;
            if( cell->type == NNCMS_TYPE_NONE ) break;
            
            //g_string_append_printf( html, "        <td>%s</td>\n", cell->data );
            
            // Type convert
            char *pbuffer = main_type( req, cell->value, cell->type );

            // Print variables
            if( cell->options != NULL )
            {
                g_string_append( html, "        <td" );
                if( cell->options != NULL )
                {
                    for( unsigned int k = 0; cell->options[k].type != NNCMS_TYPE_NONE; k++ )
                    {
                        // Value is unescaped!
                        g_string_append_printf( html, " %s=\"%s\"", cell->options[k].name, cell->options[k].value.string );
                    }
                }
                g_string_append( html, ">" );
                g_string_append( html, pbuffer );
                g_string_append( html, "</td>\n" );
            }
            else
            {
                g_string_append_printf( html, "        <td>%s</td>\n", pbuffer );
            }
            
            g_free( pbuffer );
        }
        g_string_append( html, "    </tr>\n" );

    }
    g_string_append( html, "    </tbody>\n" );

    // Remember row count
    row_count = i;

    //
    // Table ends
    //
    g_string_append( html, "</table>\n" );

    // Add footer to html
    g_string_append( html, table->footer_html );

    // Add pager to html
    g_string_append( html, pager );
    //g_string_free( pager, TRUE );

    garbage_add( req->loop_garbage, html, MEMORY_GARBAGE_GSTRING_FREE );

    return html->str;
}

// #############################################################################
