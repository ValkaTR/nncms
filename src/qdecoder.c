// #############################################################################
// Source file: qdecoder.c

/******************************************************************************
 * qDecoder - http://www.qdecoder.org
 *
 * Copyright (c) 2000-2012 Seungyoung Kim.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

// #############################################################################
// includes of local headers
//

#include "qdecoder.h"
#include "threadinfo.h"

// #############################################################################
// includes of system headers
//

#include <fcgiapp.h>
#include <glib.h>
#include <string.h>

// #############################################################################
// global variables
//

extern char *_q_makeword(char *str, char stop);
extern size_t _q_urldecode(char *str);
extern char *_q_strtrim(char *str);
extern char  _q_x2c(char hex_up, char hex_low);

GHashTable *_parse_query(GHashTable *hash, const char *query, char equalchar, char sepchar, int *count);

// Log module status
static bool b_qdecoder = false;

// #############################################################################
// functions

bool qdecoder_init( )
{
    if( b_qdecoder == true ) return false;
    b_qdecoder = true;

    // Initialization code

    return true;
}

// #############################################################################

bool qdecoder_deinit( )
{
    if( b_qdecoder == false ) return false;
    b_qdecoder = false;

    // DeInitialization code

    return true;
}

// #############################################################################

/**
 * Parse one or more request(COOKIE/POST/GET) queries.
 *
 * @param request   qentry_t container pointer that parsed key/value pairs
 *                  will be stored. NULL can be used to create a new container.
 * @param method    Target mask consists of one or more of Q_CGI_COOKIE,
 *                  Q_CGI_POST and Q_CGI_GET. Q_CGI_ALL or 0 can be used for
 *                  parsing all of those types.
 *
 * @return qentry_t* handle if successful, NULL if there was insufficient
 *         memory to allocate a new object.
 *
 * @code
 *   qentry_t *req = qcgireq_parse(NULL, 0);
 * @endcode
 *
 * @code
 *   qentry_t *req = qcgireq_parse(req, Q_CGI_COOKIE | Q_CGI_POST);
 * @endcode
 *
 * @note
 * When multiple methods are specified, it'll be parsed in the order of
 * (1)COOKIE, (2)POST (3)GET unless you call it separately multiple times.
 */
GHashTable *qcgireq_parse(struct NNCMS_THREAD_INFO *req, GHashTable *hash, Q_CGI_T method)
{
    // initialize entry structure
    if (hash == NULL) {
        GHashTable* hash = g_hash_table_new(g_str_hash, g_str_equal);
        if(hash == NULL) return NULL;
    }

    // parse COOKIE
    if (method == Q_CGI_ALL || (method & Q_CGI_COOKIE) != 0) {
        if (req->lpszCookie != NULL) {
            _parse_query(hash, req->lpszCookie, '=', ';', NULL);
        }
    }

    //  parse POST method
    if (method == Q_CGI_ALL || (method & Q_CGI_POST) != 0) {
        if (req->lpszContentType != NULL)
        {
            if (!strncmp(req->lpszContentType, "application/x-www-form-urlencoded", 33)) {
                //char *query = qcgireq_getquery(Q_CGI_POST);
                if (req->lpszContentData != NULL) {
                    _parse_query(hash, req->lpszContentData, '=', '&', NULL);
                }
            } else if (!strncmp(req->lpszContentType, "multipart/form-data", 19)) {
                _parse_multipart(req, hash);
            }
        }
    }

    // parse GET method
    if (method == Q_CGI_ALL || (method & Q_CGI_GET) != 0) {
        //char *query = qcgireq_getquery(Q_CGI_GET);
        if (req->lpszQueryString != NULL) {
            _parse_query(hash, req->lpszQueryString, '=', '&', NULL);
        }
    }

    return hash;
}

// #############################################################################

GHashTable *_parse_query(GHashTable *hash, const char *query, char equalchar, char sepchar, int *count)
{
    if (hash == NULL) {
        GHashTable* hash = g_hash_table_new(g_str_hash, g_str_equal);
        if(hash == NULL) return NULL;
    }

    char *newquery = NULL;
    int cnt = 0;

    if (query != NULL) newquery = g_strdup(query);
    while (newquery && *newquery) {
        char *value = _q_makeword(newquery, sepchar);
        char *name = _q_strtrim(_q_makeword(value, equalchar));
        _q_urldecode(name);
        _q_urldecode(value);

        //if (request->putstr(request, name, value, false) == true)
        g_hash_table_insert(hash, name, value);
        
        cnt++;

        //free(name);
        //free(value);
    }
    if (newquery != NULL) g_free(newquery);
    if (count != NULL) *count = cnt;

    return hash;
}

// #############################################################################

static int _parse_multipart(struct NNCMS_THREAD_INFO *req)
{

    char buf[MAX_LINEBUF];
    int  amount = 0;

    /*
     * For parse multipart/form-data method
     */

    

    // Force to check the boundary string length to defense overflow attack
    char boundary[256];
    int maxboundarylen = CONST_STRLEN("--");
    maxboundarylen += strlen(strstr(getenv("CONTENT_TYPE"), "boundary=") + CONST_STRLEN("boundary="));
    maxboundarylen += CONST_STRLEN("--");
    maxboundarylen += CONST_STRLEN("\r\n");
    if (maxboundarylen >= sizeof(boundary)) {
        DEBUG("The boundary string is too long(Overflow Attack?). stopping process.");
        return amount;
    }

    // find boundary string - Hidai Kenichi made this patch for handling quoted boundary string
    char boundary_orig[256];
    _q_strcpy(boundary_orig, sizeof(boundary_orig), strstr(getenv("CONTENT_TYPE"), "boundary=") + CONST_STRLEN("boundary="));
    _q_strtrim(boundary_orig);
    _q_strunchar(boundary_orig, '"', '"');
    snprintf(boundary, sizeof(boundary), "--%s", boundary_orig);

    // If you want to observe the string from stdin, uncomment this section.
    /*
    if (true) {
        int i, j;
        qcgires_setcontenttype(request, "text/html");

        printf("Content Length = %s<br>\n", getenv("CONTENT_LENGTH"));
        printf("Boundary len %zu : %s<br>\n", strlen(boundary), boundary);
        for (i = 0; boundary[i] != '\0'; i++) printf("%02X ", boundary[i]);
        printf("<p>\n");

        for (j = 1; _q_fgets(buf, sizeof(buf), stdin) != NULL; j++) {
            printf("Line %d, len %zu : %s<br>\n", j, strlen(buf), buf);
            //for (i = 0; buf[i] != '\0'; i++) printf("%02X ", buf[i]);
            printf("<br>\n");
        }
        exit(EXIT_SUCCESS);
    }
    */

    // check boundary
    if (_q_fgets(buf, sizeof(buf), stdin) == NULL) {
        DEBUG("Bbrowser sent a non-HTTP compliant message.");
        return amount;
    }

    // for explore 4.0 of NT, it sent \r\n before starting.
    if (!strcmp(buf, "\r\n")) _q_fgets(buf, sizeof(buf), stdin);

    if (strncmp(buf, boundary, strlen(boundary)) != 0) {
        DEBUG("Invalid string format.");
        return amount;
    }

    // check file save mode
    bool upload_filesave = false; // false: save into memory, true: save into file
    const char *upload_basepath = request->getstr(request, "_Q_UPLOAD_BASEPATH", false);
    if (upload_basepath != NULL) upload_filesave = true;

    bool  finish;
    for (finish = false; finish == false; amount++) {
        char *name = NULL, *value = NULL, *filename = NULL, *contenttype = NULL;
        int valuelen = 0;

        // get information
        while (_q_fgets(buf, sizeof(buf), stdin)) {
            if (!strcmp(buf, "\r\n")) break;
            else if (!strncasecmp(buf, "Content-Disposition: ", CONST_STRLEN("Content-Disposition: "))) {
                int c_count;

                // get name field
                name = strdup(buf + CONST_STRLEN("Content-Disposition: form-data; name=\""));
                for (c_count = 0; (name[c_count] != '\"') && (name[c_count] != '\0'); c_count++);
                name[c_count] = '\0';

                // get filename field
                if (strstr(buf, "; filename=\"") != NULL) {
                    int erase;
                    filename = strdup(strstr(buf, "; filename=\"") + CONST_STRLEN("; filename=\""));
                    for (c_count = 0; (filename[c_count] != '\"') && (filename[c_count] != '\0'); c_count++);
                    filename[c_count] = '\0';
                    // remove directory from path, erase '\'
                    for (erase = 0, c_count = strlen(filename) - 1; c_count >= 0; c_count--) {
                        if (erase == 1) filename[c_count] = ' ';
                        else {
                            if (filename[c_count] == '\\') {
                                erase = 1;
                                filename[c_count] = ' ';
                            }
                        }
                    }
                    _q_strtrim(filename);

                    // empty attachment
                    if (strlen(filename) == 0) {
                        free(filename);
                        filename = NULL;
                    }
                }
            } else if (!strncasecmp(buf, "Content-Type: ", CONST_STRLEN("Content-Type: "))) {
                contenttype = strdup(buf + CONST_STRLEN("Content-Type: "));
                _q_strtrim(contenttype);
            }
        }

        // check
        if (name == NULL) {
            DEBUG("bug or invalid format.");
            continue;
        }

        // get value field
        if (filename != NULL && upload_filesave == true) {
            char *tp, *savename = strdup(filename);
            for (tp = savename; *tp != '\0'; tp++) {
                if (*tp == ' ') *tp = '_'; // replace ' ' to '_'
            }
            value = _parse_multipart_value_into_disk(boundary, upload_basepath, savename, &valuelen, &finish);
            free(savename);

            if (value != NULL) request->putstr(request, name, value, false);
            else request->putstr(request, name, "(parsing failure)", false);
        } else {
            value = _parse_multipart_value_into_memory(boundary, &valuelen, &finish);

            if (value != NULL) request->put(request, name, value, valuelen+1, false);
            else request->putstr(request, name, "(parsing failure)", false);
        }

        // store some additional info
        if (value != NULL && filename != NULL) {
            char ename[255+10+1];

            // store data length, 'NAME.length'
            snprintf(ename, sizeof(ename), "%s.length", name);
            request->putint(request, ename, valuelen, false);

            // store filename, 'NAME.filename'
            snprintf(ename, sizeof(ename), "%s.filename", name);
            request->putstr(request, ename, filename, false);

            // store contenttype, 'NAME.contenttype'
            snprintf(ename, sizeof(ename), "%s.contenttype", name);
            request->putstr(request, ename, ((contenttype!=NULL)?contenttype:""), false);

            if (upload_filesave == true) {
                snprintf(ename, sizeof(ename), "%s.savepath", name);
                request->putstr(request, ename, value, false);
            }
        }

        // free stuffs
        if (name != NULL) free(name);
        if (value != NULL) free(value);
        if (filename != NULL) free(filename);
        if (contenttype != NULL) free(contenttype);
    }

    return amount;
}

// #############################################################################

char *_q_makeword(char *str, char stop)
{
    char *word;
    int  len, i;

    for (len = 0; ((str[len] != stop) && (str[len])); len++);
    word = (char *) g_malloc(sizeof(char) * (len + 1));

    for (i = 0; i < len; i++) word[i] = str[i];
    word[i] = '\0';

    if (str[len])len++;
    for (i = len; str[i]; i++) str[i - len] = str[i];
    str[i - len] = '\0';

    return word;
}

// #############################################################################

char *_q_strtrim(char *str)
{
    int i, j;

    if (str == NULL) return NULL;
    for (j = 0; str[j] == ' ' || str[j] == '\t' || str[j] == '\r' || str[j] == '\n'; j++);
    for (i = 0; str[j] != '\0'; i++, j++) str[i] = str[j];
    for (i--; (i >= 0) && (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n'); i--);
    str[i+1] = '\0';

    return str;
}

// #############################################################################

size_t _q_urldecode(char *str)
{
    if (str == NULL) {
        return 0;
    }

    char *pEncPt, *pBinPt = str;
    for (pEncPt = str; *pEncPt != '\0'; pEncPt++) {
        switch (*pEncPt) {
            case '+': {
                *pBinPt++ = ' ';
                break;
            }
            case '%': {
                *pBinPt++ = _q_x2c(*(pEncPt + 1), *(pEncPt + 2));
                pEncPt += 2;
                break;
            }
            default: {
                *pBinPt++ = *pEncPt;
                break;
            }
        }
    }
    *pBinPt = '\0';

    return (pBinPt - str);
}

// #############################################################################

// Change two hex character to one hex value.
char _q_x2c(char hex_up, char hex_low)
{
    char digit;

    digit = 16 * (hex_up >= 'A' ? ((hex_up & 0xdf) - 'A') + 10 : (hex_up - '0'));
    digit += (hex_low >= 'A' ? ((hex_low & 0xdf) - 'A') + 10 : (hex_low - '0'));

    return digit;
}

// #############################################################################
