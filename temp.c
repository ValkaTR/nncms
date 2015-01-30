 -DMEM_PROFILE=1 -DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16
########################################################################
cmake . -DCMAKE_BUILD_TYPE=Debug
cd ~/Dokumente/nncms/src/
make && ./spawn-fcgi -a 127.0.0.1 -p 9000 -n ./nncms
gdb --eval-command="break database_tables" --eval-command="continue" --pid `ps -C nncms -o pid=`
gdb --eval-command="break main" --eval-command="continue" --pid `ps -C nncms -o pid=`

handle SIGINT pass

systemctl start lighttpd

export G_SLICE="always-malloc"
make && valgrind --suppressions=nncms.supp --leak-check=full --show-possibly-lost=no --trace-children=yes ./spawn-fcgi -a 127.0.0.1 -p 9000 -n ./nncms
########################################################################

1000 characters
llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllll

    struct NNCMS_FIELD fields[] =
    {
        { .name = "_id", .value = _id, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "_value", .value = _, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL },
        { .name = "referer", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = req->g_sessionid, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };

    struct NNCMS_FORM form =
    {
        .name = "_edit", .action = NULL, .method = "POST",
        .title = NULL, .help = NULL,
        .header_html = NULL, .footer_html = NULL,
        .fields = fields
    };

########################################################################

    char *type;
    struct NNCMS_VARIABLE vars[] =
    {
        { .name = "name", .value.string = "yo", .type = NNCMS_TYPE_RETURN },
        { .name = "type", .value.string = &type, .type = NNCMS_TYPE_PRETURN },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE filter[] =
    {
        { .name = "id", .value.string = "1", .type = NNCMS_TYPE_STRING },
        { .name = "type", .value.string = "f", .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_VARIABLE sort[] =
    {
        { .name = "type", .value.string = "ASC", .type = NNCMS_TYPE_STRING },
        { .name = "name", .value.string = "ASC", .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE }
    };

    struct NNCMS_QUERY query_insert =
    {
        .op = NNCMS_QUERY_INSERT,
        .table = "files",
        .vars = vars
    };

    struct NNCMS_QUERY query_select =
    {
        .op = NNCMS_QUERY_SELECT,
        .table = "files",
        .vars = vars,
        .filter = filter,
        .sort = sort,
        .offset = 50,
        .limit = 25
    };

    struct NNCMS_QUERY query_update =
    {
        .op = NNCMS_QUERY_UPDATE,
        .table = "files",
        .vars = vars,
        .filter = filter
    };

    struct NNCMS_QUERY query_delete =
    {
        .op = NNCMS_QUERY_DELETE,
        .table = "files",
        .filter = filter
    };
    
    database_select( req, &query_insert );
    database_select( req, &query_select );
    database_select( req, &query_update );
    database_select( req, &query_delete );

    struct NNCMS_FILES_TABLE
    {
        char *id;
        char *user_id;
        char *parent_file_id;
        char *name;
        char *type;
        char *title;
        char *description;
    };
    
    struct NNCMS_FILES_TABLE *files_table = database_select( req, &query_select );
    

    // INSERT INTO files (name, type) VALUES (?, ?);
    // SELECT name, type FROM files WHERE id=? AND type=? ORDER BY type ASC, name ASC LIMIT 25 OFFSET 50;
    // UPDATE files SET name=?, type=? WHERE id=? AND type=?;
    // DELETE FROM files WHERE id=? AND type=?;

########################################################################

// Passing variable number of named parameters of different types

enum NNCMS_VARIABLE_TYPE
{
    NNCMS_TYPE_NONE = 0,
    NNCMS_TYPE_STRING,
    NNCMS_TYPE_INTEGER,
    NNCMS_TYPE_UNSIGNED_INTEGER,
    NNCMS_TYPE_BOOL,
    NNCMS_TYPE_MEM,
};

union NNCMS_VARIABLE_UNION
{
    void *pmem;                        // [Pointer] Pointer to variable value
    char *string;
    int integer;
    unsigned int unsigned_integer;
    bool boolean;
};

struct NNCMS_VARIABLE
{
    char name[NNCMS_VAR_NAME_LEN_MAX];     // [String] Variable name
    union NNCMS_VARIABLE_UNION value;      // [Pointer] Pointer to variable value
    enum NNCMS_VARIABLE_TYPE type;         // [Number] ID number describing the value of the variable
};

// Convert typed data to a string
char *main_type( struct NNCMS_THREAD_INFO *req, union NNCMS_VARIABLE_UNION value, enum NNCMS_VARIABLE_TYPE type )
{
    GString *buffer = g_string_sized_new( 32 );
    
    switch( type )
    {
        case NNCMS_TYPE_UNSIGNED_INTEGER:
        {
            int unsigned_integer = value.unsigned_integer;
            g_string_printf( buffer, "%u", unsigned_integer );
            break;
        }

        case NNCMS_TYPE_INTEGER:
        {
            int integer = value.integer;
            g_string_printf( buffer, "%i", integer );
            break;
        }

        case NNCMS_TYPE_BOOL:
        {
            bool bval = value.boolean;
            if( bval == true )
                g_string_assign( buffer, "True" );
            else
                g_string_assign( buffer, "False" );
            break;
        }

        case NNCMS_TYPE_MEM:
        {
            void *pval = value.pmem;
            g_string_printf( buffer, "%08Xh", pval );
            break;
        }

        case NNCMS_TYPE_STRING:
        {
            char *pstring = value.string;
            g_string_assign( buffer, pstring );
            break;
        }

        default:
        {
            char *pstring = value.string;
            g_string_assign( buffer, pstring );
            break;
        }
    }
    
    // Return pointer to memory containing the string
    char *pbuffer = g_string_free( buffer, FALSE );
    return pbuffer;
}

// Specify values for template
struct NNCMS_VARIABLE vars[] =
    {
        { .name = "number", .value.integer = 1337, .type = NNCMS_TYPE_INTEGER },
        { .name = "text", .value.string = "mouse", .type = NNCMS_TYPE_STRING },
        { .type = NNCMS_TYPE_NONE } // Terminating row
    };

char *tmp = main_type( req, vars[0].value, vars[0].type );

// Inline, anonymous
char *tmp = main_type( req, (union NNCMS_VARIABLE_UNION) { .value.integer = 1337 }, NNCMS_TYPE_INTEGER );

########################################################################

    // First we check what permissions do user have
    if( acl_check_perm( req, "node", req->g_username, "delete" ) == false )
    {
        main_vmessage( req, "not_allowed" );
        return;
    }

    // Get Id
    char *httpVarId = main_variable_get( req, req->get_tree, "node_id" );
    if( httpVarId == 0 )
    {
        main_vmessage( req, "no_data" );
        return;
    }
    int node_id = atoi( httpVarId );

    // Get the list of rows
    struct NNCMS_ROW *node_row = database_query( req, "select * from node where id=%i", node_id );
    if( node_row == 0 )
    {
        main_vmessage( req, "not_found" );
        return;
    }
    memory_add_garbage( req, node_row, MEMORY_GARBAGE_DB_FREE );

    //
    // Check if submit button pressed
    //
    char *httpVarSubmit = main_variable_get( req, req->post_tree, "node_add" );
    if( httpVarSubmit )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            main_message( req, "xsrf_fail" );
            return;
        }

        char *httpVarTitle = main_variable_get( req, req->post_tree, "node_title" );
        if( httpVarTitle == NULL )
            httpVarTitle = "";

        char *httpVarLanguage = main_variable_get( req, req->post_tree, "node_language" );
        if( httpVarLanguage == NULL )
            httpVarLanguage = req->language;

        database_query( req, "insert into node_rev (id, node_id, user_id, title, timestamp, log_msg) VALUES(null, %i, '%q', '%q', %u, '')", node_id, req->g_userid, httpVarTitle, raw_time );
        database_query( req, "update node set node_rev_id=%i where id=%i", rev_id, node_id );
        database_query( req, "delete from node where id=%i", node_id );

        unsigned int node_id = database_last_rowid( req );

        log_displayf( req, LOG_INFO, "Node %i was added (revision id = %u)", node_id, rev_id );
        redirect_to_referer( req );

        return;
    }

    //
    // Form
    //

    struct NNCMS_FORM *form = template_form_new( req );
    struct NNCMS_FIELD *field;

    // Add title field
    field = template_field_new( form, NNCMS_FIELD_EDITBOX );
    field->name = "acl_object";
    field->value = "";
    field->text_name = i18n_string_temp( req, "acl_object_name", NULL );
    field->text_description = i18n_string_temp( req, "acl_object_desc", NULL );
    field->editable = true;
    
    // Language
    field = template_field_new( form, NNCMS_FIELD_LANGUAGE );
    field->name = "node_language";
    field->value = req->language;
    field->text_name = "Language";
    field->text_description = "Language selection.";
    field->editable = true;

    // Get user name and user nick
    field = template_field_new( form, NNCMS_FIELD_USER );
    field->value = req->g_userid;
    field->text_name = "Posted by";
    field->text_description = "Authoring information.";
    field->editable = false;

    // Referer field
    field = template_field_new( form, NNCMS_FIELD_HIDDEN );
    field->name = "referer";
    field->value = req->referer;
    
    // fkey
    field = template_field_new( form, NNCMS_FIELD_HIDDEN );
    field->name = "fkey";
    field->value = req->g_sessionid;

    // Submit buttons
    field = template_field_new( node_form, NNCMS_FIELD_SUBMIT );
    field->name = "node_add";
    field->value = i18n_string_temp( req, "node_add_submit", NULL );

    // Html output
    frame_template[1].value.string = form_html( req, form );

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Free memory from query result
    template_form_destroy( req, node_form );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );

########################################################################

    // Create a table
    struct NNCMS_TABLE *table = table_new( req );
    table->caption = header_str;

    // Header cells
    table_new_header_cell( req, table, "id" );
    table_new_header_cell( req, table, "name" );
    table_new_header_cell( req, table, "" );
    table_new_header_cell( req, table, "" );
    table_new_header_cell( req, table, "" );
    table_new_header_cell( req, table, "" );

    // Fetch table data
    for( struct NNCMS_ROW *cur_row = node_type; cur_row; cur_row = cur_row->next )
    {
        struct NNCMS_TABLE_ROW *table_row = table_new_row( req, table );
        
        char *node_id = cur_row->value[0];
        char *node_name = cur_row->value[1];
        
        // Data
        table_new_cell( req, table_row, node_id );
        table_new_cell( req, table_row, node_name );

        // Actions
        struct NNCMS_VARIABLE vars[] =
        {
            { /* name */ "node_type_id", /* value */ node_id },
            { /* name */ NULL, /* value */ NULL }
        };
        char *link;
        link = main_temp_link( req, "node_list", "list", vars ); table_new_cell( req, table_row, link );
        link = main_temp_link( req, "node_type_edit", "edit", vars ); table_new_cell( req, table_row, link );
        link = main_temp_link( req, "node_field_list", "fields", vars ); table_new_cell( req, table_row, link );
        link = main_temp_link( req, "node_type_delete", "delete", vars ); table_new_cell( req, table_row, link );
    }

    // Extra links
    char *link;
    GString *links = g_string_sized_new( 100 );
    memory_add_garbage( req, links, MEMORY_GARBAGE_GSTRING_FREE );
    struct NNCMS_VARIABLE vars[] =
    {
        { /* name */ "node_type_id", /* value */ node_id },
        { /* name */ NULL, /* value */ NULL }
    };
    
    link = main_temp_link( req, "node_type_add", "Add new node type", vars );
    g_string_append( links, link );

    table->header_html = links->str;
    table->footer_html = links->str;

    // Html output
    frame_template[1].value.string = template_table( req, table );

    // Free memory
    table_destroy( req, table );

    // Make a cute frame
    template_hparse( req, "frame.html", req->frame, frame_template );

    // Send generated html to client
    main_output( req, header_str, req->frame->str, 0 );

########################################################################

    struct NNCMS_FIELD field_file_id = { .name = "file_id", .value = file_table.id, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_user_id = { .name = "file_user_id", .value = file_table.user_id, .data = NULL, .type = NNCMS_FIELD_USER, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_parent_file_id = { .name = "file_parent_file_id", .value = file_table.parent_file_id, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_name = { .name = "file_name", .value = file_table.name, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_type = { .name = "file_type", .value = file_table.type, .data = & (struct NNCMS_SELECT_OPTIONS) { .link = NULL, .items = file_types }, .type = NNCMS_FIELD_SELECT, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_size = { .name = "file_size", .value = file_info->size, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_timestamp = { .name = "file_timestamp", .value = file_timestamp, .data = NULL, .type = NNCMS_FIELD_TIMEDATE, .values_count = 1, .editable = false, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_title = { .name = "file_title", .value = file_table.title, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_description = { .name = "file_description", .value = file_table.description, .data = NULL, .type = NNCMS_FIELD_EDITBOX, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD field_file_content = { .name = "file_content", .value = NULL, .data = NULL, .type = NNCMS_FIELD_TEXTAREA, .values_count = 1, .editable = true, .viewable = true, .text_name = NULL, .text_description = NULL };
    struct NNCMS_FIELD fields[] =
    {
        field_file_id,
        field_file_user_id,
        field_file_parent_file_id,
        field_file_name,
        field_file_type,
        field_file_size,
        field_file_timestamp,
        field_file_title,
        field_file_description,
        field_file_content,
        { .name = "referer", .value = req->referer, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "fkey", .value = req->g_sessionid, .data = NULL, .type = NNCMS_FIELD_HIDDEN, .values_count = 1, .editable = false, .viewable = true, .text_name = "", .text_description = "" },
        { .name = "file_edit", .value = NULL, .data = NULL, .type = NNCMS_FIELD_SUBMIT, .editable = false, .viewable = true, .text_name = NULL, .text_description = "" },
        { .type = NNCMS_FIELD_NONE }
    };

########################################################################

; calculate p = sqrt(a^2 + b^2)
fld qword [a]   ; load a into st0
fmul st0, st0   ; st0 = a * a = a^2
fld qword [b]   ; load b into st0
fmul st0, st0   ; st0 = b * b = b^2
faddp st1, st0  ; st1 = a^2 + b^2
                ; and pop st0
fsqrt           ; st0 = sqrt(st0)
fst qword [p]   ; store st0 in c
; end program

// на память из http://en.wikipedia.org/wiki/C_preprocessor
//
//#define dumpme(x, fmt) printf("%s:%u: %s=" fmt, __FILE__, __LINE__, #x, x)
//
//int some_function() {
//    int foo;
//    /* [a lot of complicated code goes here] */
//    dumpme(foo, "%d");
//    /* [more complicated code goes here] */
//}

// ololo

// Test
//void database_xx(){}

0x00007fa1d655bd6d in write () from /lib64/libpthread.so.0
(gdb) bt
#0  0x00007fa1d655bd6d in write () from /lib64/libpthread.so.0
#1  0x00007fa1d6771359 in OS_Write () from /usr/lib64/libfcgi.so.0
#2  0x00007fa1d676d650 in ?? () from /usr/lib64/libfcgi.so.0
#3  0x00007fa1d676e1df in ?? () from /usr/lib64/libfcgi.so.0
#4  0x00007fa1d676f138 in FCGX_FClose () from /usr/lib64/libfcgi.so.0
#5  0x00007fa1d676fa64 in FCGX_Finish_r () from /usr/lib64/libfcgi.so.0
#6  0x000000000041283d in main_loop (var=0x6d3570) at main.c:929
#7  0x00007fa1d6554f05 in start_thread () from /lib64/libpthread.so.0
#8  0x00007fa1d5e854bd in clone () from /lib64/libc.so.6


http://abuse.wtf-no.com/tor.csv
http://abuse.wtf-no.com/proxy.csv
http://abuse.wtf-no.com/bad-ip.csv

Simple cached ban check:
0.007447280
0.006670160
0.006458720
0.006631000
0.006475721
0.006540001
0.007788320
0.006590360

No ban check:
0.003658040
0.003731400
0.002958240
0.002954080
0.002958760
0.003193040
0.002882680
0.002991120

Average ban check time: 0.0036592753 s
for 22300 bans on 800MHz proccessor

== Preventing CSRF and XSRF Attacks ==
( http://www.codinghorror.com/blog/2008/10/preventing-csrf-and-xsrf-attacks.html )
When a user visits a site, the site should generate a (cryptographically strong) pseudorandom value and set it as a cookie on the user's machine. The site should require every form submission to include this pseudorandom value as a form value and also as a cookie value. When a POST request is sent to the site, the request should only be considered valid if the form value and the cookie value are the same. When an attacker submits a form on behalf of a user, he can only modify the values of the form. An attacker cannot read any data sent from the server or modify cookie values, per the same-origin policy. This means that while an attacker can send any value he wants with the form, he will be unable to modify or read the value stored in the cookie. Since the cookie value and the form value must be the same, the attacker will be unable to successfully submit a form unless he is able to guess the pseudorandom value.
The advantage of this approach is that it requires no server state; you simply set the cookie value once, then every HTTP POST checks to ensure that one of the submitted <input> values contains the exact same cookie value. Any difference between the two means a possible XSRF attack.
An even stronger, albeit more complex, prevention method is to leverage server state -- to generate (and track, with timeout) a unique random key for every single HTML FORM you send down to the client. We use a variant of this method on Stack Overflow with great success. That's why with every <form> you'll see the following:

<input type="hidden" name="fkey" value="<?= fkey ?>">
{ /* szName */ "fkey", /* szValue */ 0 },
Template[3].szValue = req->g_sessionid;
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

x
)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))
о_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________
о_____________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________
PRIVMSG #pik-pik :

c4forc( c4charc *saveptr = c4NULLc, *str = inputStr; ; str = c4NULLc ) {
    c4charc *token = strtok_r( str, c7" "c, &saveptr );
    c4ifc( token == c4NULLc ) c4breakc;
    printf( c7"%s\n"c, token );
}


Dir
            <?lua if(file_acl(var.file_path, "rename") == true) then return "<a href=\"" .. var.homeURL .. "/file_rename.fcgi?file_name=" .. escape(var.file_path) .. "\"><img src=\"" .. var.homeURL .. "/images/actions/gtk-edit.png\" width=\"16\" height=\"16\" alt=\"rename\" title=\"Rename this folder\" border=\"0\"></a>" end ?>
			<?lua if(file_acl(var.file_path, "delete") == true) then return "<a href=\"" .. var.homeURL .. "/file_delete.fcgi?file_name=" .. escape(var.file_path) .. "\"><img src=\"" .. var.homeURL .. "/images/actions/edit-delete.png\" width=\"16\" height=\"16\" alt=\"delete\" title=\"Delete this folder\" border=\"0\"></a>" end ?>
			<?lua if(acl("acl", "edit") == true) then return "<a href=\"" .. var.homeURL .. "/acl_view.fcgi?acl_object=file_" .. escape(var.file_path) .. "\"><img src=\"" .. var.homeURL .. "/images/apps/preferences-system-authentication.png\" width=\"16\" height=\"16\" alt=\"view acl\" title=\"View ACL of this folder\" border=\"0\"></a>" end ?>

File
            <?lua if(file_acl(var.file_path .. var.file_name, "download") == true) then return "<a href=\"" .. var.homeURL .. escape(var.file_path .. var.file_name) .. "\"><img src=\"" .. var.homeURL .. "/images/stock/stock_download.png\" width=\"16\" height=\"16\" alt=\"download\" title=\"Download this file\" border=\"0\"></a>" end ?>
            <?lua if(file_acl(var.file_path .. var.file_name, "edit") == true) then return "<a href=\"" .. var.homeURL .. "/file_edit.fcgi?file_name=" .. escape(var.file_path .. var.file_name) .. "\"><img src=\"" .. var.homeURL .. "/images/stock/stock_edit.png\" width=\"16\" height=\"16\" alt=\"edit\" title=\"Edit this file\" border=\"0\"></a>" end ?>
            <?lua if(file_acl(var.file_path .. var.file_name, "rename") == true) then return "<a href=\"" .. var.homeURL .. "/file_rename.fcgi?file_name=" .. escape(var.file_path .. var.file_name) .. "\"><img src=\"" .. var.homeURL .. "/images/actions/gtk-edit.png\" width=\"16\" height=\"16\" alt=\"rename\" title=\"Rename this file\" border=\"0\"></a>" end ?>
			<?lua if(file_acl(var.file_path .. var.file_name, "delete") == true) then return "<a href=\"" .. var.homeURL .. "/file_delete.fcgi?file_name=" .. escape(var.file_path .. var.file_name) .. "\"><img src=\"" .. var.homeURL .. "/images/actions/edit-delete.png\" width=\"16\" height=\"16\" alt=\"delete\" title=\"Delete this file\" border=\"0\"></a>" end ?>
			<?lua if(acl("acl", "edit") == true) then return "<a href=\"" .. var.homeURL .. "/acl_view.fcgi?acl_object=file_" .. escape(var.file_path .. var.file_name) .. "\"><img src=\"" .. var.homeURL .. "/images/apps/preferences-system-authentication.png\" width=\"16\" height=\"16\" alt=\"view acl\" title=\"View ACL of this file\" border=\"0\"></a>" end ?>

rewrite ^/$ /nncms.fcgi?page=index break;
#rewrite ^(.*).html$ /nncms.fcgi?page=view&file=$1.html break;
rewrite ^/view/(.*)$ /nncms.fcgi?page=view&file=/$1 break;
rewrite ^(.*)/$ /nncms.fcgi?page=file_browse&file_path=$1/ break;
rewrite ^/tn64/(.*)$ /nncms.fcgi?page=thumbnail&file=/$1&width=64&height=64 break;
rewrite ^/tn/(.*)$ /nncms.fcgi?page=thumbnail&file=/$1 break;
rewrite ^/(index|info|acl_add|acl_edit|acl_delete|acl_view|cfg_view|cfg_edit|log_view|log_clear|post_add|post_edit|post_modify|post_news|post_delete|post_view|post_remove|post_reply|post_root|user_register|user_login|user_logout|user_sessions|user_view|user_edit|user_profile|user_delete|view|thumbnail|file_browse|file_rename|file_edit|file_upload|file_delete|file_mkdir|file_add).html$ /nncms.fcgi?page=$1 break;

rewrite ^/$ /index.fcgi break;
rewrite ^/view/([^\?]+)$ /view.fcgi?file=/$1 break;
rewrite ^([^\?]+)/$ /file_browse.fcgi?file_path=$1/ break;
rewrite ^/tn64/([^\?]+)$ /thumbnail.fcgi?file=/$1&width=64&height=64 break;
rewrite ^/tn/([^\?]+)$ /thumbnail.fcgi?file=/$1 break;

url.rewrite = (
    "^/$" => "/nncms.fcgi?page=index",
    "^/view/([^\?]+)(\?(.*))?$" => "/nncms.fcgi?page=view&file=/$1&$3",
    "^(.*)/$" => "/nncms.fcgi?page=file_browse&file_path=$1/",
    "^/tn64/(.*)$" => "/nncms.fcgi?page=thumbnail&file=/$1&width=64&height=64",
    "^/tn/([^\?]+)(\?(.*))?$" => "/nncms.fcgi?page=thumbnail&file=/$1&$3",
    "^/(index|info|acl_add|acl_edit|acl_delete|acl_view|cfg_view|cfg_edit|log_view|log_clear|post_add|post_edit|post_modify|post_news|post_delete|post_view|post_remove|post_reply|post_root|user_register|user_login|user_logout|user_sessions|user_view|user_edit|user_profile|user_delete|view|thumbnail|file_browse|file_rename|file_edit|file_upload|file_delete|file_mkdir|file_add).html(\?(.*))?$" => "/nncms.fcgi?page=$1&$3"
)

url.rewrite = (
    "^/$" => "/index.fcgi",
    "^/view/([^\?]+)(\?(.*))?$" => "/view.fcgi?file=/$1&$3",
    "^([^\?]+)/$" => "/file_browse.fcgi?file_path=$1/",
    "^/tn64/(.*)$" => "/thumbnail.fcgi?file=/$1&width=64&height=64",
    "^/tn/([^\?]+)(\?(.*))?$" => "/thumbnail.fcgi?file=/$1&$3"
)

#rewrite ^(.*).html$ /nncms.fcgi?page=view&file=$1.html break;

    static int nCounter = 0;
    static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

        char *lpszServerName = FCGX_GetParam( "SERVER_NAME", request.envp );

        FCGX_FPrintF( request.out,
            "Content-type: text/html\r\n"
            "\r\n"
            "<title>FastCGI Hello! (multi-threaded C, fcgiapp library)</title>"
            "<h1>FastCGI Hello! (multi-threaded C, fcgiapp library)</h1>"
            "Thread %d, Process %ld<p>"
            "Request count for thread running on host <i>%s</i>: <code>%u</code><p>",
            nThreadID, pid, lpszServerName ? lpszServerName : "?", nCounter );

        sleep(2);

        pthread_mutex_lock( &count_mutex );
        ++nCounter;
        pthread_mutex_unlock( &count_mutex );

http://127.0.0.1/nncms.fcgi?page=user_view
http://127.0.0.1/nncms.fcgi?page=thumbnail&file=/lol/3pcbgbc31znm.jpg
http://127.0.0.1/nncms.fcgi?page=thumbnail&file=/lol/tails_s.png
http://127.0.0.1/nncms.fcgi?page=post_view&post_id=1
http://127.0.0.1/nncms.fcgi?page=post_add&post_id=1

// Leave only neccesary image files script
cd ~/nncms/files
mkdir -v images2
grep -oh "images/[^\\\"]*" ~/nncms/src/*.c | sort -u | xargs -I {} cp --parents -v {} images2/
grep -oh "images/[^\\\"]*" templates/* | sort -u | xargs -I {} cp --parents -v {} images2/
*/

Intel Pentium III 800MHz           27.2 W or 26.4 W
Intel Pentium 4 HT 3.2GHz          82 W
AMD Turion 64 X2 TL-56 1800 MHz    33 W
Intel Atom Z550                    2.4 W


sudo /etc/init.d/apache2 stop
sudo /etc/init.d/lighttpd stop
sudo /etc/init.d/nginx restart
cd ~/nncms/src
make && ./spawn-fcgi -a 194.168.1.5 -p 9000 -n ./nncms


########################################################################
cmake . -DCMAKE_BUILD_TYPE=Debug
cd ~/Документы/nncms/src/
make && ./spawn-fcgi -a 127.0.0.1 -p 9000 -n ./nncms
########################################################################




cd /home/valka/nncms/src; make && ./spawn-fcgi -a 194.168.1.5 -p 9000 -n ./nncms
gdb --eval-command="break ban.c:81" --eval-command="continue" --pid `ps -C nncms -o pid=`
gdb --eval-command="continue" --pid `ps -C nncms -o pid=`
CFLAGS="-O0 -g" ./configure --with-imagemagick=yes --with-fam=no
make && valgrind --verbose --leak-check=full --suppressions=nncms.supp --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.5 -p 9000 -n ./nncms

make && valgrind --verbose --suppressions=nncms.supp --leak-check=full --trace-children=yes spawn-fcgi -a 127.0.0.1 -p 9000 -n ./nncms
make && valgrind --suppressions=nncms.supp --malloc-fill=23 --log-file=valgrind_%p.log --trace-children=yes ./spawn-fcgi -a 194.168.1.128 -p 9000 -n ./nncms
make && valgrind --suppressions=nncms.supp --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.128 -p 9000 -n ./nncms
make && valgrind --suppressions=nncms.supp --gen-suppressions=yes --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.128 -p 9000 -n ./nncms
make && valgrind --gen-suppressions=yes --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.128 -p 9000 -n ./nncms
make && valgrind --verbose --leak-check=full --suppressions=nncms.supp --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.128 -p 9000 -n ./nncms
make && valgrind --verbose --log-file=valgrind_%p.log --leak-check=full --suppressions=nncms.supp --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.128 -p 9000 -n ./nncms
make && valgrind --verbose --log-file=valgrind_%p.log --tool=memcheck --leak-check=full --suppressions=nncms.supp --malloc-fill=23 --trace-children=yes ./spawn-fcgi -a 194.168.1.5 -p 9000 -n ./nncms

indent -bl -bli0 -bls -blf -prs -bap -nsaf -nsai -nsaw -npcs -i4 -ncdw -nce -ts4 -nut

indent --braces-after-if-line --brace-indent0 --braces-after-struct-decl-line --braces-after-func-def-line --space-after-parentheses --blank-lines-after-procedures --no-space-after-for --no-space-after-if --no-space-after-while --no-space-after-function-call-names --indent-level4 --dont-cuddle-do-while --dont-cuddle-else --tab-size4 --no-tabs --procnames-start-lines tree.c

flex --reentrant --noline --full html_parser.lex

########################################################################
########################################################################
// libtest
float sum(float a, float b) {
    return a + b;
}
########################################################################
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

int main(void) {
    char *error; /* сюда будут помещаться сообщения об ошибках */
    void *libHandle; /* через этот указатель будем "общаться" с библиотекой */
    /* указатель на библиотечную функцию, через который мы сможем её вызвать */
    float (*sum) (float, float);

    /* пытаемся открыть файл с библиотечным кодом и задействовать его */
    libHandle = dlopen("./lib.so", RTLD_NOW);
    /* проверяем результат работы dlopen() */
    if (NULL == libHandle) {
        /* если мы попали внутрь этого if, то у dlopen() ничего не получилось */
        fprintf(stderr, "%s\n", dlerror());
		/* пробуем узнать, что именно не получилось */
        exit(EXIT_FAILURE); /* больше сделать ничего не можем, поэтому завершаемся */
    }

    /* связываем наш указатель с кодом конкретной функции по её имени */
    *(void **) (&sum) = dlsym(libHandle, "sum");
    if ((error = dlerror()) != NULL) { /* проверяем результат работы dlsym() */
        /* снова неудача, пытаемся выяснить, из-за чего */
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }
    /* вот тут запускаем код из библиотеки */
    fprintf(stdout, "2 + 3 = %f\n", (*sum)(2, 3));
	/* вот тут запускаем код из библиотеки */
    dlclose(libHandle); /* "отключаем" библиотеку от программы */
    exit(EXIT_SUCCESS); /* завершаем программу */
}

Флаг RTLD_NOW в вызове dlopen() заставляет run-time среду:

    * убедиться в том, что все ссылки из нашей программы на библиотечный код
      актуальны ("разрешаются");
    * выполнить такую проверку до возврата из dlopen(), иначе – вернуться из неё
      с ошибкой.

Также имеется флаг RTLD_LAZY ("ленивый"), благодаря которому такие проверки
производятся по мере исполнения программы только при обнаружении ещё не
связанного кода. Первый способ надёжнее (мы сразу узнаем о проблемах), но может
застопорить работу программы на длительное время для проверки всех ссылок (если
их много). Второй "размазывает" задержку по тем моментам, когда эти ссылки
встретятся при выполнении кода, но создаёт эффект "русской рулетки".

Сохраняем код программы в файл с именем main.c и приступаем к компиляции
библиотеки:

$ gcc -shared -fPIC -o lib.so lib.c

Параметры "-shared" и "-fPIC" как раз и сообщают компилятору о нашем желании
построить разделяемую библиотеку, а не "обычный" исполняемый файл. В случае
успеха в текущем каталоге у нас появится файл с именем lib.so. Теперь собираем
саму программу:

$ gcc -rdynamic -o main main.c -ldl

Параметр "-rdynamic" сообщает компилятору о необходимости добавить в исполняемый
код поддержку dlopen(), а "-ldl" подключает стандартную библиотеку, в которой
находится код нужных нам функций (dlopen() и прочих). Эта библиотека – libdl –
является частью GNU-библиотеки функций языка C в GNU/Linux и, кстати, сама
является разделяемой библиотекой, а указание её через параметр командной строки
решает проблему "курицы и яйца".

В случае успеха в текущем каталоге должен появиться исполняемый файл main.
Попросим систему выполнить содержащийся в нём код:

$ ./main
2 + 3 = 5.000000

Если система отвечает чем-то похожим на "bash: ./main: Отказано в доступе", то
убедитесь в том, что у вас есть права на запуск этого файла. Команда

$ ls -l main

должна выдать что-то вроде "-rwx... main", здесь важно наличие "x" в первой
группе прав ("eXecutable" – исполняемый). Если это не так, то можно набрать
следующую команду:

$ chmod u+x main

для исправления ситуации. Итак, базовые навыки создания и использования
разделяемых библиотек "на лету" у нас есть.
########################################################################
########################################################################

GET /nncms/nncms.fcgi?page=post_root HTTP/1.1
Host: 194.168.1.5
User-Agent: Mozilla/5.0 (X11; U; Linux x86_64; en-US; rv:1.9.1.3) Gecko/20091005 Gentoo Shiretoko/3.5.3
Accept: text/html
Accept-Language: ru,en-us;q=0.7,en;q=0.3
Accept-Encoding: gzip,deflate
Accept-Charset: windows-1251,utf-8;q=0.7,*;q=0.7
Keep-Alive: 300
Connection: keep-alive
Cookie: session_id=4qV0cfkHFFurOPjljvvWUF5CPJoLyeCpQvo8QZv0f5tNdzj4hRUM3PmfqwuYW9GFNBcocAiWFPpqrnyzyAJu4lxyx1l8qic8ekY0eSEn7A1QPt4HvxNXRK7qLGiXM309

########################################################################

#ifdef HAVE_LIBMAGICK

    extern ExceptionInfo *lpException;

    //
    // Initialize the image info structure and read an image.
    //
    ImageInfo *lpImageInfo = CloneImageInfo( (ImageInfo *) 0 );
    (void) strcpy( lpImageInfo->filename, fileInfo->lpszAbsFullPath );
    Image *lpImages = ReadImage( lpImageInfo, lpException );
    if( lpException->severity != UndefinedException )
        CatchException( lpException );
    if( lpImages == (Image *) 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Image not loaded";
        log_printf( req, LOG_ERROR, "View::Thumbnail: %s", frameTemplate[1].szValue );
        goto cleanup;
    }

    //
    // Convert the image to a thumbnail.
    //
    //Image *lpThumbnails = NewImageList( );
    Image *lpResizeImage, *lpImage;
    //while( (lpImage = RemoveFirstImageFromList( &lpImages )) != (Image *) 0 )
    //{
        lpResizeImage = ResizeImage( lpImages, 106, 80, LanczosFilter, 1.0, lpException );
        //lpResizeImage = ThumbnailImage( lpImage, 106, 80, lpException );
    //    if( lpResizeImage == (Image *) 0 )
    //        MagickError( lpException->severity,lpException->reason,lpException->description );
    //    (void) AppendImageToList( &lpThumbnails, lpResizeImage );
    //    DestroyImage( lpImage );
    //}

    //
    // Write the image thumbnail.
    //
    (void) strcpy( lpResizeImage->filename, "test.png" );
    WriteImage( lpImageInfo, lpResizeImage );

    // Cleanup
    //lpThumbnails = DestroyImageList( lpThumbnails );
    DestroyImage( lpResizeImage );
    lpImageInfo = DestroyImageInfo( lpImageInfo );

    frameTemplate[0].szValue = "Error";
    frameTemplate[1].szValue = "Image not loaded";
    goto cleanup;

#endif // HAVE_LIBMAGICK


supp

{
             Error name
             Type
             fun:function name, which contains the error to suppress
        fun:function name, which calls the function specified above
}

Error name can be any name.
             type=ValueN, if the error is an uninitialized value error.
                 =AddrN, if it is an address error.(N=sizeof(data type))
                 =Free, if it is a free error (eg:mismatched free)
                 =Cond, if error is due to uninitialized CPU condition code.
                 =Param, if it is an invalid system call parameter error.




char *strncat_s(char *dest, const char *src, size_t n)
{
   size_t dest_len = strlen(dest);
   size_t i;

   for (i = 0 ; i + dest_len < n && src[i] != '\0' ; i++)
       dest[dest_len + i] = src[i];
   dest[dest_len + i] = '\0';

   return dest;
}


char *temp = malloc( 10 );
/*for( int i = 0; i < 50; i++ )
{
    char c = temp[i];
}*/

strncpy( temp, "123456", 7 );
strncat_s( temp, "1234567890", 10 );

printf( "%s\n", temp );

free( temp );
return 0;


// This routine is needed to avoid code duplicating
// If you have full path to file, then you dont have to split it
// just put it as first or second parameter, the other one then shall be zero.
bool file_check_perm( struct NNCMS_THREAD_INFO *req, struct NNCMS_FILE_INFO *fileInfo, char *permission )
{

    char szFullPath[NNCMS_PATH_LEN_MAX];

    // Test
    /*printf( TERM_BG_BLUE "fileInfo->lpszRelFullPath: " TERM_RESET "%s\n"
            TERM_BG_BLUE "permission: " TERM_RESET "%s\n"
            TERM_BG_BLUE "req->g_username: " TERM_RESET "%s\n\n", fileInfo->lpszRelFullPath, permission, req->g_username );//*/

    // Попробуем проверить доступ у предыдущего (корневого) каталога
    char *lpszPath = strdup( fileInfo->lpszRelFullPath );
    char *lpszTemp = lpszPath;
    int i = 0;
    while( 1 )
    {
        if( i != 1 || fileInfo->bIsFile == true )
        {
            // Название в базе
            if( i == 0 )
                snprintf( szFullPath, sizeof(szFullPath), "file_%s", lpszPath );
            else
                snprintf( szFullPath, sizeof(szFullPath), "file_%s/", lpszPath );

            // Проверка доступа
            if( acl_check_perm( req, szFullPath, req->g_username, permission ) == true )
            {
                free( lpszPath );

                // Test
                //printf( TERM_FG_GREEN "szFullPath: " TERM_RESET "%s\n", szFullPath );

                return true;
            }
        }

        // Взять следующую дочернюю папку
        lpszTemp = strrchr( lpszTemp, '/' );
        if( lpszTemp == 0 )
        {
            break;
        }
        else
        {
            *lpszTemp = 0;
            lpszTemp = lpszPath;
        }

        i++;
    }

    free( lpszPath );

    // Test
    //printf( TERM_FG_GREEN "szFullPath: " TERM_RESET "%s\n", szFullPath );

    return false;
}


static void
load_esk(void)
{
    FILE *f;
    char *name, *value, *line;

    f = fopen("/etc/sysconfig/keyboard", "r");
    if (!f)
	exit(1);

    while (fscanf(f, "%as\n", &line) == 1) {
	if (sscanf(line, "%as=\"%as\"\n", &name, &value) != 2) {
	    free(line);
	    continue;
	}
	setenv(name, value, 1);
	free(name);
	free(value);
	free(line);
    }

    fclose(f);
}


        /*
        if( varPage == 0 )                                              main_index( &threadInfo );
        else if( strcmp( varPage->lpszValue, "index" ) == 0 )           main_index( &threadInfo );
        else if( strcmp( varPage->lpszValue, "info" ) == 0 )            main_info( &threadInfo );
        else if( strcmp( varPage->lpszValue, "acl_add" ) == 0 )         acl_add( &threadInfo );
        else if( strcmp( varPage->lpszValue, "acl_edit" ) == 0 )        acl_edit( &threadInfo );
        else if( strcmp( varPage->lpszValue, "acl_delete" ) == 0 )      acl_delete( &threadInfo );
        else if( strcmp( varPage->lpszValue, "acl_view" ) == 0 )        acl_view( &threadInfo );
        else if( strcmp( varPage->lpszValue, "cfg_view" ) == 0 )        cfg_view( &threadInfo );
        else if( strcmp( varPage->lpszValue, "cfg_edit" ) == 0 )        cfg_edit( &threadInfo );
        else if( strcmp( varPage->lpszValue, "log_view" ) == 0 )        log_view( &threadInfo );
        else if( strcmp( varPage->lpszValue, "log_clear" ) == 0 )       log_clear( &threadInfo );
        else if( strcmp( varPage->lpszValue, "post_add" ) == 0 )        post_add( &threadInfo );
        else if( strcmp( varPage->lpszValue, "post_edit" ) == 0 )       post_edit( &threadInfo );
        else if( strcmp( varPage->lpszValue, "post_delete" ) == 0 )     post_delete( &threadInfo );
        else if( strcmp( varPage->lpszValue, "post_view" ) == 0 )       post_view( &threadInfo );
        else if( strcmp( varPage->lpszValue, "post_news" ) == 0 )       post_news( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_register" ) == 0 )   user_register( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_login" ) == 0 )      user_login( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_logout" ) == 0 )     user_logout( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_sessions" ) == 0 )   user_sessions( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_view" ) == 0 )       user_view( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_edit" ) == 0 )       user_edit( &threadInfo );
        else if( strcmp( varPage->lpszValue, "user_delete" ) == 0 )     user_delete( &threadInfo );
        else if( strcmp( varPage->lpszValue, "view" ) == 0 )            view_show( &threadInfo );
        else if( strcmp( varPage->lpszValue, "thumbnail" ) == 0 )       view_thumbnail( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_browse" ) == 0 )     file_browse( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_rename" ) == 0 )     file_rename( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_edit" ) == 0 )       file_edit( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_upload" ) == 0 )     file_upload( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_delete" ) == 0 )     file_delete( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_mkdir" ) == 0 )      file_mkdir( &threadInfo );
        else if( strcmp( varPage->lpszValue, "file_add" ) == 0 )        file_add( &threadInfo );
        else                                                            main_index( &threadInfo );
        */



==================================================================================================
==================================================================================================
==================================================================================================
==================================================================================================
==================================================================================================
==================================================================================================

// memcspn: returns the first location in buf of any character in find
// extra performance derived from pam_mysql.c
char *memcspn(const char *ibuf,const char *iend,const char *ifind,unsigned findl) {
  const unsigned char *find=(const unsigned char *)ifind;
  unsigned char *buf=(unsigned char *)ibuf,*end=(unsigned char *)iend;
  switch(findl) {
  case 0: break;
  case 1: if (buf<end) {
      char *rv;
      if ((rv=(char *)memchr(buf,*find,end-buf))) return(rv);
    } break;
  case 2: {
      unsigned char c1=find[0],c2=find[1];
      for(; buf<end; buf++) if (*buf==c1 || *buf==c2) return((char *)buf);
    } break;
  default: if (buf<end) {
      const unsigned char *findt,*finde=find+findl;
      unsigned char min=(unsigned char)-1,max=0,and_mask=(unsigned char)~0,or_mask=0;
      for(findt=find; findt<finde; findt++) {
        if (max<*findt) max=*findt;
        if (min>*findt) min=*findt;
        and_mask &= *findt;
        or_mask |= *findt;
      }
      for(; buf<end; buf++) {
        if (*buf>=min && *buf<=max && (*buf & and_mask) == and_mask && (*buf | or_mask) && memchr(find,*buf,findl)) return((char *)buf);
      }
    } break;
  }
  return((char *)end);
}

// because so many c string functions must call strlen() before operating, these mem...
// functions are more efficient on huge buffers. If you wish to provide a \0 terminated C-string
// calculate the strlen() yourself as most c string functions do every time they
// are called.
void memcqspnstart(const char *find,unsigned findl,unsigned *quick) {
  unsigned q;
  for(q=0; q<256/(sizeof(unsigned)*8); q++) quick[q]=0; // how many bits can we store in unsigned?
  while(findl) {
    quick[(unsigned)*(unsigned char *)find/(sizeof(unsigned)*8)] |= 1<<(unsigned)*(unsigned char *)find%(sizeof(unsigned)*8);
    find++;
    findl--;
  }
}

// strcqspn quick version for large find strings used multiple times
// end=buf+buflen
// for C strings this is the position of the \0
// for buffers, this is 1 character beyond the end (same place)
// returns end when the search fails
char *memcqspn(const char *buf,const char *end,const unsigned *quick) {
  if (buf<end) for(; buf<end; buf++) {
    if (quick[(unsigned)*(unsigned char *)buf/(sizeof(unsigned)*8)] & 1<<((unsigned)*(unsigned char *)buf%(sizeof(unsigned)*8))) return((char *)buf);
  } else if (buf>end) for(; buf>end; buf--) {
    if (quick[(unsigned)*(unsigned char *)buf/(sizeof(unsigned)*8)] & 1<<((unsigned)*(unsigned char *)buf%(sizeof(unsigned)*8))) return((char *)buf);
  }
  return((char *)end);
}

size_t roundtonextpower(size_t numo) { /* round to next power */
  size_t pow,num;
  num=numo;
  if (num>=32768) {pow=15; num >>= 15;}
  else if (num>1024) {pow=10; num >>=10;}
  else if (num>128) {pow=5; num>>=5;}
  else pow=0;
  while(num>1) {pow++; num>>=1;}
  num=1<<pow;
  if (num<numo) num<<=1;
  return(num);
}

// When enabled, this will ensure that string lengths are being maintained properly by the highly detailed
// string transforms. When they are known to be working, turning off NPPDEBUG will disable all this
// slow code.
#if NPPDEBUG && 1 /* { binary safe functions no longer permit this testing */
void *memmovetest(void *dest,void *src,size_t n) {
  size_t m;
  m=strlen((const char *)src);
  if (m+1 != n) {
#if NPPWINDOWS /* { */
      MessageBoxFree(g_nppData._nppHandle,smprintf("memmovetest wrong SLN strlen(+1):%u != SLN:%u",strlen((const char *)src)+1,n),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#else /* } else { */ /* Borland C for DOS */
     printf("Wrong SLN:%u != strlen(+1):%d\n",n,m+1);
#endif /* } */
     n=m;
  }
  return(memmove(dest,src,n));
}
#else /* } else { */
#define memmovetest memmove
#endif /* } */

// returns the number of bytes *dest was changed by so the caller can update their pointers too, including when *dest becomes NULL by accident.
// If you are certain that *dest started non null, you can be sure that it hasn't become NULL if the return value is 0
/* Constant realloc()'s are abusive to memory because realloc() must copy the data to a new malloc()'d area.
   On the other hand, slack space on buffers that will never use it is wasteful.
   Choose strategy to minimize realloc()'s while making good use of memory */
#define ARMSTRATEGY_INCREASE 0 //increase buffer with slack space when necessary; good for a constantly expanding buffer
#define ARMSTRATEGY_MAINTAIN 1 //increase buffer only the minimum amount, buffer will not be reduced if too large; good for a buffer that will be reused with some large and some small allocations
#define ARMSTRATEGY_REDUCE 2   //increase buffer only the minimum amount, reduce buffer if too large; good for buffers of known size or that will only be used once
// If choosing ARMSTRATEGY_REDUCE for a new buffer it is usually better to just use malloc() yourself and not call armrealloc()
// clear=1, memset new space to 0
//size_t g_armrealloc_cutoff=16384; /* sizeof(buf)<=, next size doubled, >, added */
// new feature: ARMSTRATEGY_REDUCE should only reduce when half can be reclaimed, much like ARMSTRATEGY_INCREASE only doubles when necessary
int armrealloc(char **dest,unsigned *destsz,unsigned newsize,int strategy,int clear
#if NPPDEBUG
,char *title
#define armreallocsafe armrealloc
#define THETITLE title
#else
#define armreallocsafe(dt,ds,ns,st,cl,ti) armrealloc(dt,ds,ns,st,cl)
#define THETITLE "armrealloc" /* the macros discards this */
#endif
) {
  unsigned rv=0,oldsz=(*dest)?(*destsz):0;
  char *dest1;
  if (strategy==ARMSTRATEGY_REDUCE && *destsz != newsize) { *destsz = newsize; goto doit;} // without this goto, the conditionals were so convoluted that they never worked
  if (strategy==ARMSTRATEGY_INCREASE){
    if (newsize < 64) newsize=64;
    if (!*dest && newsize < *destsz) newsize=*destsz;
  }
  if (!*dest || *destsz <  newsize) {
    *destsz = (strategy==ARMSTRATEGY_INCREASE)?roundtonextpower(newsize):newsize;
doit:
    dest1=*dest;
    dest1= (dest1)?(char *)reallocsafe(dest1,*destsz,THETITLE):(char *)mallocsafe(*destsz,THETITLE);
      //if (dest1) strcpy((dest1+*destsz-1),"#$"); // $ is beyond string
    rv=dest1-*dest;
    if (dest1 && clear && *destsz>oldsz) memset(dest1+oldsz,0,*destsz-oldsz);
    *dest=dest1;
  }
  return(rv);
}
#undef THETITLE

// unlike other arm routines, destlen is required here because memmovearm() is not interested in appending anything.
// returns the number of bytes *dest was changed by during a realloc() so the caller can adjust their pointers
// memmovearm always moves one extra byte assuming the caller is trying to maintain a \0 terminated C-string
int memmovearm(char **dest,unsigned *destsz,unsigned *destlen,char *destp,char *sourcep
#if NPPDEBUG
,int notest
#endif
){  unsigned rv=0;
  if (destp != sourcep) {
    if ((rv=armreallocsafe(dest,destsz,*destlen+1+destp-sourcep,ARMSTRATEGY_INCREASE,0,"memmovearm"))) {
      destp += rv;
      sourcep += rv;
    }
    if (*dest) {
      //memset(*dest+*destlen,'@',*destsz-*destlen); *(*dest+*destlen)='%'; // % is \0
#if NPPDEBUG
      if (notest) memmove(destp,sourcep,(*destlen)-(sourcep-*dest)+1); else
#endif
      memmovetest(destp,sourcep,(*destlen)-(sourcep-*dest)+1);
      *destlen += destp-sourcep;
    }
  }
  return(rv);
}
#if NPPDEBUG
#define memmovearmtest memmovearm
#else
#define memmovearmtest(dt,ds,dl,dp,sp,nt) memmovearm(dt,ds,dl,dp,sp)
#endif

//strchrstrans: Replaces any in a set of single characters with corresponding strings
//adapted from memstrtran() && memchrtran()
//chrs: the set of chars to search for, and all chrs must be found in repls
//repls: deli + setof[char+string+deli] the first character of repls is the delimiter
//which must be chosen such that it doesn't occur in the data of repls
//stopeol: process only a single line, provide a pointer to the line beginning and it will return a pointer to the line ending \r\n
//  NULL to process entire str, set to NULL if starting at beginning,
// if using stopeol mode, slndx is required and returns the amount that the string was reduced by moving forwards minus alterations so the caller can update their counters
unsigned strchrstrans(char **dest,unsigned *destsz,unsigned *destlen,char **stopeol,const char *chrs,unsigned chrsl,const char *repls)
{
#define lold 1
  unsigned n=0,lnew;
  char *d,*newst,*dnew,*end; char ckst[3];
  unsigned quick[256/(sizeof(unsigned)*8)];

  if ((d=*dest) && lold && chrsl) {
    end=d+*destlen;
    if (stopeol) end=memcspn(d=*stopeol,end,"\r\n",2);
    ckst[0]=*repls;
    ckst[2]='\0';
    memcqspnstart(chrs,chrsl,quick);
    while ( (d=memcqspn(d,end,quick))<end) {
      ckst[1]=*d;
      if (!(newst=(char *)strstr(repls,ckst))) {
#if NPPDEBUG /* { */
        MessageBoxFree(g_nppData._nppHandle,smprintf("strchrstrans: Improper String Format (#%d)\r\n%s",1,repls),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif /* } */
        goto failbreak;
      }
      newst+=2;
      if (!(dnew=strchr(newst,*repls))) { // find the termination character (typically ;)
#if NPPDEBUG /* { */
        MessageBoxFree(g_nppData._nppHandle,smprintf("strchrstrans: Improper String Format (#%d)\r\n%s",2,repls),PLUGIN_NAME, MB_OK|MB_ICONSTOP);
#endif /* } */
        goto failbreak;
      }
      lnew=dnew-newst;
      if (lnew != lold) {
        unsigned dx;
        dx=memmovearmtest(dest,destsz,destlen,d+lnew,d+lold,1); if (!*dest) goto failbreak;
        d+=dx; // thanks to stopeol we can't use the easier method here
        end+=dx+lnew-lold;
      }
      if (lnew) {
        memcpy(d,newst,lnew);
        d+=lnew;
      }
      n++;
    }
    if (stopeol) *stopeol = d;
  }
failbreak:
  return(n);
#undef lold
}

/*      case 0:    memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&#0;"  ,4,"strdupXMLXlatToEnti
      case '&':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&amp;" ,5,"strdupXMLXlatToEnti
      case '"':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&quot;",6,"strdupXMLXlatToEnti
      case '<':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&lt;"  ,4,"strdupXMLXlatToEnti
      case '>':  memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&gt;"  ,4,"strdupXMLXlatToEnti
      case '\'': memcpyarmsafe(&sValueConverted,&uValueConvertedSz,&uValueConvertedLen,"&apos;",6,"strdu

rvu=strchrstrans(&tx,&txsz,&destlen,NULL,"&<>\"",4,",&&amp;,<&lt;,>&gt;,\"&quot;,")*/

———————————————————————————————————————————————————————————————————————————————————————————————————
———————————————————————————————————————————————————————————————————————————————————————————————————
———————————————————————————————— わぜふあけ ——————————————————————————————————————————————————————————————————
———————————————————————————————————————————————————————————————————————————————————————————————————
———————————————————————————————————————————————————————————————————————————————————————————————————
———————————————————————————————————————————————————————————————————————————————————————————————————

void template_escape_html( struct NNCMS_THREAD_INFO *req, struct NNCMS_BUFFER *sbDst, struct NNCMS_BUFFER *sbSrc )
{
    // Find minimum size
    size_t nSize = sbDst->nSize;
    if( nSize > sbSrc->nSize ) nSize = sbSrc->nSize;

    // Prepare destination buffer
    *sbDst->lpBuffer = 0;
    sbDst->nPos = 0;

    // Start table replace
    for( size_t i = 0; i < nSize; i++ )
    {
        if( sbSrc->lpBuffer[i] == '&' ) { smart_cat( sbDst, "&amp;" ); }
        else if( sbSrc->lpBuffer[i] == '<' ) { smart_cat( sbDst, "&lt;" ); }
        else if( sbSrc->lpBuffer[i] == '>' ) { smart_cat( sbDst, "&gt;" ); }
        else if( sbSrc->lpBuffer[i] == '"' ) { smart_cat( sbDst, "&quot;" ); }
        else if( sbSrc->lpBuffer[i] == '\'' ) { smart_cat( sbDst, "&apos;" ); }
        else if( sbSrc->lpBuffer[i] == 0 )
        {
            break;
        }
        else
        {
            // Copy character by default
            sbDst->lpBuffer[sbDst->nPos] = sbSrc->lpBuffer[i];
            sbDst->nPos++;
            sbDst->lpBuffer[sbDst->nPos] = 0;
        }
    }
}

    // Create smart buffers
    struct NNCMS_BUFFER sbPostText =
        {
            /* lpBuffer */ postRow->szColValue[4],
            /* nSize */ 0,
            /* nPos */ 0
        };
    sbPostText.nSize = strlen( postRow->szColValue[4] );
    sbPostText.nPos = sbPostText.nSize - 1;

    struct NNCMS_BUFFER sbPostTextEscaped =
        {
            /* lpBuffer */ req->lpszTemp,
            /* nSize */ NNCMS_PAGE_SIZE_MAX,
            /* nPos */ 0
        };
    *sbPostTextEscaped.lpBuffer = 0;

    // Escape post text
    template_escape_html( req, &sbPostTextEscaped, &sbPostText );
    editTemplate[10].szValue = sbPostTextEscaped.lpBuffer; // Body
