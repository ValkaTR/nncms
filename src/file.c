// #############################################################################
// Source file: file.c

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################
// includes of local headers
//

#include "acl.h"
#include "file.h"
#include "main.h"
#include "mime.h"
#include "user.h"
#include "filter.h"
#include "template.h"
#include "log.h"
#include "smart.h"
#include "cache.h"

#include "strlcpy.h"
#include "strlcat.h"
#include "strdup.h"

#include "memory.h"

// #############################################################################
// includes of system headers
//

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

// #############################################################################
// global variables
//

// Path where files are located
char fileDir[NNCMS_PATH_LEN_MAX] = "./files/";

// How much columns in a row in gallery mode
int nGalleryCols = 4;

// How many items on a single page by default
int nItemsPerPage = 60;

// File icons
struct NNCMS_FILE_ICON fileIcons[] =
{
    { /* szExtension */ "default",  /* szIcon */ "images/mimetypes/text-x-preview.png" },
    { /* szExtension */ "folder",   /* szIcon */ "images/places/folder.png" },

    { /* szExtension */ "txt",      /* szIcon */ "images/mimetypes/text-x-generic.png" },
    { /* szExtension */ "log",      /* szIcon */ "images/mimetypes/text-x-generic.png" },
    { /* szExtension */ "conf",     /* szIcon */ "images/mimetypes/text-x-generic.png" },

    { /* szExtension */ "html",     /* szIcon */ "images/mimetypes/text-html.png" },
    { /* szExtension */ "htm",      /* szIcon */ "images/mimetypes/text-html.png" },

    { /* szExtension */ "mp3",      /* szIcon */ "images/mimetypes/audio-x-mp3.png" },
    { /* szExtension */ "wav",      /* szIcon */ "images/mimetypes/audio-x-wav.png" },

    { /* szExtension */ "mpg",      /* szIcon */ "images/mimetypes/video-mpeg.png" },
    { /* szExtension */ "avi",      /* szIcon */ "images/mimetypes/video-x-msvideo.png" },

    { /* szExtension */ "gif",      /* szIcon */ "images/mimetypes/image-gif.png" },
    { /* szExtension */ "jpg",      /* szIcon */ "images/mimetypes/image-jpeg.png" },
    { /* szExtension */ "png",      /* szIcon */ "images/mimetypes/image-png.png" },
    { /* szExtension */ "svg",      /* szIcon */ "images/mimetypes/image-svg.png" },

    { /* szExtension */ "exe",      /* szIcon */ "images/mimetypes/application-x-executable.png" },
    { /* szExtension */ "com",      /* szIcon */ "images/mimetypes/application-x-executable.png" },
    { /* szExtension */ "bat",      /* szIcon */ "images/mimetypes/application-x-executable.png" },

    { /* szExtension */ "rar",      /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "zip",      /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "gz",       /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "bz2",      /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "lzma",     /* szIcon */ "images/mimetypes/package-x-generic.png" },
    { /* szExtension */ "tar",      /* szIcon */ "images/mimetypes/package-x-generic.png" },

    { /* szExtension */ "doc",      /* szIcon */ "images/mimetypes/x-office-document.png" },

    { /* szExtension */ 0,          /* szIcon */ 0 } // Terminating row
};

// #############################################################################
// functions

void file_add( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Create file";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/document-new.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS addTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_path", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    addTemplate[3].szValue = req->g_sessionid;

    // Cleanup trick
    struct NNCMS_FILE_INFO dirInfo;
    struct NNCMS_FILE_INFO fileInfo;

    // Get path
    struct NNCMS_VARIABLE *httpVarDir = main_get_variable( req, "file_path" );
    if( httpVarDir == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Directory not specified";
        log_printf( req, LOG_NOTICE, "File::Add: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter file name and make a full path to it
    file_get_info( &dirInfo, httpVarDir->lpszValue );
    if( dirInfo.bExists == false )
    {
        // Oops, no file
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Path not found";
        log_printf( req, LOG_ERROR, "File::Add: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to create files
    if( file_check_perm( req, &dirInfo, "add" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Add: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "file_add" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get POST data
        struct NNCMS_VARIABLE *httpVarFilename = main_get_variable( req, "file_name" );
        struct NNCMS_VARIABLE *httpVarText = main_get_variable( req, "file_text" );
        if( httpVarFilename == 0 || httpVarText == 0 )
        {

            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No form data";
            log_printf( req, LOG_ERROR, "File::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Filter
        filter_table_replace( httpVarFilename->lpszValue, (unsigned int) strlen( httpVarFilename->lpszValue ), pathFilter );

        // Get info, full absolute path
        char szFilename[NNCMS_PATH_LEN_MAX];
        strlcpy( szFilename, dirInfo.lpszRelFullPath, sizeof(szFilename) );
        strlcat( szFilename, httpVarFilename->lpszValue, sizeof(szFilename) );
        file_get_info( &fileInfo, szFilename );

        // Validate
        /*if( fileInfo.bExists == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Could not validate file";
            log_printf( req, LOG_ERROR, "File::Add: %s", frameTemplate[1].szValue );
            goto output;
        }*/

        // Already exists?
        if( fileInfo.bExists == true )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "File already exists";
            log_printf( req, LOG_NOTICE, "File::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        if( fileInfo.bIsDir == true )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Directory specified, file expected, use mkdir instead";
            log_printf( req, LOG_NOTICE, "File::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Truncate  file  to  zero length or create text file for writing.
        // The stream is positioned at the beginning of the file.
        FILE *pFile = fopen( fileInfo.lpszAbsFullPath, "wb" );
        if( pFile == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unable to create file";
            log_printf( req, LOG_ERROR, "File::Add: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Write data
        fwrite( httpVarText->lpszValue, strlen( httpVarText->lpszValue ), 1, pFile );

        // Close
        fclose( pFile );

        // Log action
        log_printf( req, LOG_ACTION, "File::Add: File \"%s\" created", fileInfo.lpszRelFullPath );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Some static form info
    addTemplate[2].szValue = dirInfo.lpszRelFullPath;

    // Load template
    template_iparse( req, TEMPLATE_FILE_ADD, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, addTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

// Browse files
void file_browse( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char szHeader[NNCMS_PATH_LEN_MAX] = "File Browser";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/categories/applications-internet.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS fileTemplate[] =
        {
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ "file_size",  /* szValue */ 0 },
            { /* szName */ "file_time",  /* szValue */ 0 },
            { /* szName */ "file_name",  /* szValue */ 0 },
            { /* szName */ "file_action",  /* szValue */ 0 },
            { /* szName */ "file_icon",  /* szValue */ 0 },
            { /* szName */ "file_path",  /* szValue */ 0 },
            { /* szName */ "image_thumbnail",  /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS footerTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_path", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // This sets as false at the end, so we wont cache error page
    bool bError = true;

    // Create smart buffers
    struct NNCMS_BUFFER buffer =
        {
            /* lpBuffer */ req->lpszBuffer,
            /* nSize */ NNCMS_PAGE_SIZE_MAX,
            /* nPos */ 0
        };

    // Check session
    user_check_session( req );

    //
    // Get selected directory, filter it and create header string
    //
    struct NNCMS_FILE_INFO dirInfo;
    struct NNCMS_VARIABLE *httpVarDir = main_get_variable( req, "file_path" );
    if( httpVarDir != 0 )
        file_get_info( &dirInfo, httpVarDir->lpszValue );
    else
        file_get_info( &dirInfo, "/" );

    // Error or directory not found?
    if( dirInfo.bExists == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Directory not found";
        log_printf( req, LOG_ERROR, "File::Browse: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to browse files
    if( file_check_perm( req, &dirInfo, "browse" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Browse: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check this page in cache
    //
    char szCacheFile[NNCMS_PATH_LEN_MAX];
    strlcpy( szCacheFile, dirInfo.lpszRelFullPath, NNCMS_PATH_LEN_MAX );
    strlcat( szCacheFile, "file_browse.html", NNCMS_PATH_LEN_MAX );
    struct NNCMS_CACHE *lpCache = cache_find( req, szCacheFile );
    if( lpCache != 0 )
    {
        // Send cached html to client
        main_output( req, szHeader, lpCache->lpData, lpCache->fileTimeStamp );
        return;
    }

    //
    // List files in current directory
    //
    unsigned int nDirCount = 0;
    struct NNCMS_FILE_ENTITY *lpEnts = file_get_directory( req, dirInfo.lpszAbsFullPath, &nDirCount, file_compare_name_asc );
    if( lpEnts == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Could not open directory";
        log_printf( req, LOG_ALERT, "File::Browse: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Put path in title
    snprintf( szHeader, NNCMS_PATH_LEN_MAX, "%s - File Browser", dirInfo.lpszRelFullPath );

    // Load header of browsed directory
    char szTemp[NNCMS_PATH_LEN_MAX];
    strlcpy( szTemp, dirInfo.lpszAbsFullPath, NNCMS_PATH_LEN_MAX );
    strlcat( szTemp, "HEADER.txt", NNCMS_PATH_LEN_MAX );
    smart_append_file( &buffer, szTemp );

    // Load header of file browser
    buffer.nPos += template_iparse( req, TEMPLATE_FILE_BROWSER_HEAD, &buffer.lpBuffer[buffer.nPos], NNCMS_PAGE_SIZE_MAX - 1 - buffer.nPos, 0 ) - 1;

    //
    // Entity process loop
    //
    for( unsigned int i = 0; i < nDirCount; i++ )
    {
        // Fill FILE_INFO structure
        char szTemp[NNCMS_PATH_LEN_MAX];
        strlcpy( szTemp, dirInfo.lpszRelPath, NNCMS_PATH_LEN_MAX );
        strlcat( szTemp, lpEnts[i].fname, NNCMS_PATH_LEN_MAX );
        if( S_ISDIR( lpEnts[i].f_stat.st_mode ) ||
            S_ISLNK( lpEnts[i].f_stat.st_mode ) )
            strlcat( szTemp, "/", NNCMS_PATH_LEN_MAX );
        struct NNCMS_FILE_INFO fileInfo;
        file_get_info( &fileInfo, szTemp );

        // Error?
        if( fileInfo.bExists == false ) continue;

        // Get Time string
        char szTime[64];
        struct tm tim;
        localtime_r( &(lpEnts[i].f_stat.st_ctime), &tim );
        strftime( szTime, 64, szTimestampFormat, &tim );

        // Ignore files and directories begining with "."
        if( lpEnts[i].fname[0] == '.' && lpEnts[i].fname[1] == '.' && lpEnts[i].fname[2] == 0 )
        {
            // Skip parent dir if we are at root directory
            if( dirInfo.lpszRelFullPath[0] == '/' && dirInfo.lpszRelFullPath[1] == 0 )
            {
                continue;
            }

            // Modify file info
            file_get_info( &fileInfo, dirInfo.lpszParentDir );
            if( fileInfo.bExists == false ) continue;
        }
        else if( lpEnts[i].fname[0] == '.' )
        {
            continue;
        }

        // Create string with human readable file size
        char szSize[32];
        if( fileInfo.bIsDir == true ) *szSize = 0;
        else file_human_size( szSize, fileInfo.nSize );

        //
        // Create a row for table
        //
        fileTemplate[1].szValue = szSize;
        fileTemplate[2].szValue = szTime;
        fileTemplate[3].szValue = lpEnts[i].fname;
        fileTemplate[5].szValue = fileInfo.lpszIcon;
        fileTemplate[6].szValue = fileInfo.lpszRelPath;

        // No actions for parent (..) directory
        if( lpEnts[i].fname[0] == '.' && lpEnts[i].fname[1] == '.' && lpEnts[i].fname[2] == 0 )
        {
            fileTemplate[4].szValue = 0;

            // Set parent directory link
            fileTemplate[6].szValue = dirInfo.lpszParentDir;
        }
        else
        {
            fileTemplate[4].szValue = 0; //file_get_actions( req, fileInfo );
        }

        // Create thumbnail
        char szThumbnail[NNCMS_PATH_LEN_MAX + 128] = { 0 };
        fileTemplate[7].szValue = szThumbnail;
        if( strncmp( fileInfo.lpszMimeType, "image/", 6 ) == 0 &&
            file_check_perm( req, &fileInfo, "thumbnail" ) == true )
        {
            if( bRewrite == true )
            {
                snprintf( szThumbnail, sizeof(szThumbnail),
                    "<img align=\"middle\" src=\"%s/tn64%s\" alt=\"thumbnail\" title=\"Image Thumbnail\">",
                    homeURL, fileInfo.lpszRelFullPath );
            }
            else
            {
                snprintf( szThumbnail, sizeof(szThumbnail),
                    "<img align=\"middle\" src=\"%s/thumbnail.fcgi?file=%s&amp;width=64&amp;height=64\" alt=\"thumbnail\" title=\"Image Thumbnail\">",
                    homeURL, fileInfo.lpszRelFullPath );
            }
        }

        // Parse template
        if( fileInfo.bIsDir == true )
            template_iparse( req, TEMPLATE_FILE_BROWSER_DIR, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, fileTemplate );
        else
            template_iparse( req, TEMPLATE_FILE_BROWSER_FILE, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, fileTemplate );
        if( smart_cat( &buffer, req->lpszFrame ) == 0 )
        {
            // Buffer overflow :<
            break;
        }

        // Free memory from useless info
        if( fileTemplate[4].szValue != 0 )
        {
            FREE( fileTemplate[4].szValue );
            fileTemplate[4].szValue = 0;
        }
    }

    // Load footer of file browser
    footerTemplate[1].szValue = dirInfo.lpszRelPath;
    template_iparse( req, TEMPLATE_FILE_BROWSER_FOOT, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, footerTemplate );
    smart_cat( &buffer, req->lpszFrame );

    // Load footer of browsed directory
    //char szTemp[NNCMS_PATH_LEN_MAX];
    strlcpy( szTemp, dirInfo.lpszAbsFullPath, NNCMS_PATH_LEN_MAX );
    strlcat( szTemp, "FOOTER.txt", NNCMS_PATH_LEN_MAX );
    smart_append_file( &buffer, szTemp );

    bError = false;

cleanup:
    // Free the memory
    FREE( lpEnts );

    size_t nOutputSize = 0;
output:
    // Generate the page
    nOutputSize = template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Cache this page
    if( bError == false )
        cache_store( req, szCacheFile, req->lpszFrame, nOutputSize );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void file_gallery( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char szHeader[NNCMS_PATH_LEN_MAX] = "Gallery";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/categories/applications-internet.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS fileTemplate[] =
        {
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ "file_size",  /* szValue */ 0 },
            { /* szName */ "file_time",  /* szValue */ 0 },
            { /* szName */ "file_name",  /* szValue */ 0 },
            { /* szName */ "file_action",  /* szValue */ 0 },
            { /* szName */ "file_icon",  /* szValue */ 0 },
            { /* szName */ "file_path",  /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS rowTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "content", /* szValue */ req->lpszBuffer },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS structureTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_path", /* szValue */ 0 },
            { /* szName */ "content", /* szValue */ req->lpszTemp },
            { /* szName */ "pages", /* szValue */ 0 },
            { /* szName */ "sort", /* szValue */ 0 },
            { /* szName */ "order", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS pageTemplate[] =
        {
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_path", /* szValue */ 0 },
            { /* szName */ "page_total", /* szValue */ 0 },
            { /* szName */ "page_number", /* szValue */ 0 },
            { /* szName */ "sort", /* szValue */ 0 },
            { /* szName */ "order", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Create smart buffers
    struct NNCMS_BUFFER cellBuffer =
        {
            /* lpBuffer */ req->lpszBuffer,
            /* nSize */ NNCMS_PAGE_SIZE_MAX,
            /* nPos */ 0
        };
    struct NNCMS_BUFFER rowBuffer =
        {
            /* lpBuffer */ req->lpszTemp,
            /* nSize */ NNCMS_PAGE_SIZE_MAX,
            /* nPos */ 0
        };

    // This sets as false at the end, so we wont cache error page
    bool bError = true;

    // Check session
    user_check_session( req );

    //
    // Get selected directory, filter it and create header string
    //
    struct NNCMS_FILE_INFO dirInfo;
    struct NNCMS_VARIABLE *httpVarDir = main_get_variable( req, "file_path" );
    if( httpVarDir != 0 )
        file_get_info( &dirInfo, httpVarDir->lpszValue );
    else
        file_get_info( &dirInfo, "/" );

    // Error or directory not found?
    if( dirInfo.bExists == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Directory not found";
        log_printf( req, LOG_ERROR, "File::Gallery: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to browse files
    if( file_check_perm( req, &dirInfo, "gallery" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Gallery: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check which page is opened
    struct NNCMS_VARIABLE *httpVarPage = main_get_variable( req, "page" );
    int nPage = 1;
    if( httpVarPage != 0 )
    {
        nPage = atoi( httpVarPage->lpszValue );
        if( nPage < 1 )
            nPage = 1;
    }

    //
    // Sorting functions
    //
    // quite ugly
    struct NNCMS_VARIABLE *httpVarSort = main_get_variable( req, "sort" );
    struct NNCMS_VARIABLE *httpVarOrder = main_get_variable( req, "order" );
    void *compar = file_compare_name_asc;
    char *szSort = "name";
    char *szOrder = "asc";
    if( httpVarSort != 0 )
    {
        if( strcmp( httpVarSort->lpszValue, "name" ) == 0 )
        {
            szSort = "name";
            if( httpVarOrder != 0 )
            {
                if( strcmp( httpVarOrder->lpszValue, "desc" ) == 0 )
                {
                    szOrder = "desc";
                    compar = file_compare_name_desc;
                }
            }
        }
        else if( strcmp( httpVarSort->lpszValue, "date" ) == 0 )
        {
            szSort = "date";
            compar = file_compare_date_asc;
            if( httpVarOrder != 0 )
            {
                if( strcmp( httpVarOrder->lpszValue, "desc" ) == 0 )
                {
                    szOrder = "desc";
                    compar = file_compare_date_desc;
                }
            }
        }
    }

    // Fill templates with sorting data
    structureTemplate[4].szValue = szSort;
    structureTemplate[5].szValue = szOrder;
    pageTemplate[4].szValue = szSort;
    pageTemplate[5].szValue = szOrder;

    //
    // Check this page in cache
    //
    char szCacheFile[NNCMS_PATH_LEN_MAX];
    snprintf( szCacheFile, NNCMS_PATH_LEN_MAX, "%sfile_gallery_%i_%s_%s.html", dirInfo.lpszRelFullPath, nPage, szSort, szOrder );
    //strlcpy( szCacheFile, , NNCMS_PATH_LEN_MAX );
    //strlcat( szCacheFile, "file_gallery.html", NNCMS_PATH_LEN_MAX );
    struct NNCMS_CACHE *lpCache = cache_find( req, szCacheFile );
    if( lpCache != 0 )
    {
        // Send cached html to client
        main_output( req, szHeader, lpCache->lpData, lpCache->fileTimeStamp );
        return;
    }

    //
    // List files in current directory
    //
    unsigned int nDirCount = 0;
    struct NNCMS_FILE_ENTITY *lpEnts = file_get_directory( req, dirInfo.lpszAbsFullPath, &nDirCount, compar );
    if( lpEnts == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Could not open directory";
        log_printf( req, LOG_ALERT, "File::Gallery: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Put path in title
    snprintf( szHeader, NNCMS_PATH_LEN_MAX, "%s - Gallery", dirInfo.lpszRelFullPath );

    // Make first cell as a link to parent directory
    fileTemplate[1].szValue = 0;
    fileTemplate[2].szValue = "n/a";
    fileTemplate[3].szValue = "..";
    fileTemplate[4].szValue = 0; // file_actions
    fileTemplate[5].szValue = fileIcons[1].szIcon;
    fileTemplate[6].szValue = dirInfo.lpszParentDir;
    template_iparse( req, TEMPLATE_GALLERY_DIR, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, fileTemplate );
    smart_cat( &cellBuffer, req->lpszFrame );

    //
    // Entity process loop
    //
    int nCell = 1, nFiles = 0;
    for( unsigned int i = 0; i < nDirCount; i++ )
    {

        // Ignore files and directories begining with "."
        if( lpEnts[i].fname[0] == '.' )
        {
            continue;
        }

        // Show files that are supposed to shown on this page
        nFiles = nFiles + 1;
        if( ((nPage - 1) * nItemsPerPage >= nFiles) ||
            (nPage * nItemsPerPage < nFiles) )
        {
            continue;
        }

        // Got some file info. We do that outside the counting for optimization
        // Fill NNCMS_FILE_INFO structure
        char szTemp[NNCMS_PATH_LEN_MAX];
        strlcpy( szTemp, dirInfo.lpszRelPath, NNCMS_PATH_LEN_MAX );
        strlcat( szTemp, lpEnts[i].fname, NNCMS_PATH_LEN_MAX );
        if( S_ISDIR( lpEnts[i].f_stat.st_mode ) ||
            S_ISLNK( lpEnts[i].f_stat.st_mode ) )
            strlcat( szTemp, "/", NNCMS_PATH_LEN_MAX );
        struct NNCMS_FILE_INFO fileInfo;
        file_get_info( &fileInfo, szTemp );
        //if( fileInfo.bExists == false ) continue;

        // Get Time string
        char szTime[64];
        struct tm tim;
        localtime_r( &(lpEnts[i].f_stat.st_ctime), &tim );
        strftime( szTime, 64, szTimestampFormat, &tim );

        // Create string with human readable file size
        char szSize[32];
        if( fileInfo.bIsDir == true ) *szSize = 0;
        else file_human_size( szSize, fileInfo.nSize );

        //
        // Create a row for table
        //
        fileTemplate[1].szValue = szSize;
        fileTemplate[2].szValue = szTime;
        fileTemplate[3].szValue = lpEnts[i].fname;
        fileTemplate[4].szValue = 0; // file_actions
        fileTemplate[5].szValue = fileInfo.lpszIcon;
        fileTemplate[6].szValue = fileInfo.lpszRelPath;

        // Parse template
        enum TEMPLATE_INDEX tempNum = 0;
        if( fileInfo.bIsDir == true )
            tempNum = TEMPLATE_GALLERY_DIR;
        else if( strncmp( fileInfo.lpszMimeType, "image/", 6 ) == 0 && file_check_perm( req, &fileInfo, "thumbnail" ) == true )
            tempNum = TEMPLATE_GALLERY_IMAGE;
        else
            tempNum = TEMPLATE_GALLERY_FILE;
        template_iparse( req, tempNum, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, fileTemplate );

        if( smart_cat( &cellBuffer, req->lpszFrame ) == 0 )
        {
            // Buffer overflow :<
            break;
        }

        // Make a row if column limit reached
        if( nCell % nGalleryCols >= nGalleryCols - 1 )
        {
            template_iparse( req, TEMPLATE_GALLERY_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, rowTemplate );
            if( smart_cat( &rowBuffer, req->lpszFrame ) == 0 )
            {
                // Buffer overflow :<
                break;
            }
            smart_reset( &cellBuffer );
        }

        // Limit number items per page
        nCell = nCell + 1;
    }

    // Fill empty cells and finish the row if neccesary
    for( unsigned int i = 0; i < nCell % nGalleryCols; i++ )
    {
        template_iparse( req, TEMPLATE_GALLERY_EMPTY, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, 0 );
        if( smart_cat( &cellBuffer, req->lpszFrame ) == 0 )
        {
            // Buffer overflow :<
            break;
        }

        if( nCell % nGalleryCols >= nGalleryCols - 1 )
        {
            // Finish last row
            template_iparse( req, TEMPLATE_GALLERY_ROW, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, rowTemplate );
            smart_cat( &rowBuffer, req->lpszFrame );
        }

        nCell = nCell + 1;
    }

    // Put rows in a table
    structureTemplate[1].szValue = dirInfo.lpszRelPath;
    template_iparse( req, TEMPLATE_GALLERY_STRUCTURE, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, structureTemplate );

    //
    // Generate page list
    //
    smart_reset( &cellBuffer );

    // Make string for total pages
    char szPageTotal[8];
    int nPageTotal = (int) ceil((double) (nFiles - 1) / (double) nItemsPerPage);
    snprintf( szPageTotal, sizeof(szPageTotal), "%i", nPageTotal );
    pageTemplate[2].szValue = szPageTotal;
    pageTemplate[1].szValue = dirInfo.lpszRelPath;

    // Loop
    for( int i = 0; i < nPageTotal; i++ )
    {
        // Make string for page number
        char szPageNumber[8];
        snprintf( szPageNumber, sizeof(szPageNumber), "%i", i + 1 );
        pageTemplate[3].szValue = szPageNumber;

        // Parse with template
        enum TEMPLATE_INDEX tempNum = TEMPLATE_PAGE_NUMBER;
        if( (nPage - 1) == i )
            tempNum = TEMPLATE_PAGE_CURRENT;
        template_iparse( req, tempNum, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, pageTemplate );

        // Add to stream
        smart_cat( &cellBuffer, req->lpszFrame );
    }

    // Duplicate "pages" in other buffer to put in variable
    char *lpszPages = MALLOC( cellBuffer.nPos + 1 );
    strlcpy( lpszPages, cellBuffer.lpBuffer, cellBuffer.nPos + 1 );
    structureTemplate[3].szValue = lpszPages;

    //
    // Reset buffer to put everything
    //
    smart_reset( &cellBuffer );

    // Load header of browsed directory
    char szTemp[NNCMS_PATH_LEN_MAX];
    strlcpy( szTemp, dirInfo.lpszAbsFullPath, NNCMS_PATH_LEN_MAX );
    strlcat( szTemp, "HEADER.txt", NNCMS_PATH_LEN_MAX );
    smart_append_file( &cellBuffer, szTemp );

    // Put rows in a table
    structureTemplate[1].szValue = dirInfo.lpszRelPath;
    template_iparse( req, TEMPLATE_GALLERY_STRUCTURE, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, structureTemplate );
    smart_cat( &cellBuffer, req->lpszFrame );

    // Load footer of browsed directory
    strlcpy( szTemp, dirInfo.lpszAbsFullPath, NNCMS_PATH_LEN_MAX );
    strlcat( szTemp, "FOOTER.txt", NNCMS_PATH_LEN_MAX );
    smart_append_file( &cellBuffer, szTemp );

    FREE( lpszPages );
    bError = false;

cleanup:
    // Free the memory
    FREE( lpEnts );

    size_t nOutputSize = 0;
output:
    // Generate the page
    nOutputSize = template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Cache this page
    if( bError == false )
        cache_store( req, szCacheFile, req->lpszFrame, nOutputSize );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void file_delete( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Delete File";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/edit-delete.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS delTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_icon", /* szValue */ 0 },
            { /* szName */ "file_name", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    delTemplate[4].szValue = req->g_sessionid;

    // Cleanup trick
    struct NNCMS_FILE_INFO fileInfo;

    // Get file name
    struct NNCMS_VARIABLE *httpVarName = main_get_variable( req, "file_name" );
    if( httpVarName == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No file specified";
        log_printf( req, LOG_NOTICE, "File::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get info about file
    file_get_info( &fileInfo, httpVarName->lpszValue );
    if( fileInfo.bExists == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        log_printf( req, LOG_ERROR, "File::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to delete files
    if( file_check_perm( req, &fileInfo, "delete" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Delete: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "file_delete" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        if( remove( fileInfo.lpszAbsFullPath ) == -1 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = strerror( errno );
            log_printf( req, LOG_ERROR, "File::Delete: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Log action
        log_printf( req, LOG_ACTION, "File::Delete: %s \"%s\" deleted", (fileInfo.bIsDir == true ? "Directory" : "File"), fileInfo.lpszRelFullPath );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Retreive data of acl
    delTemplate[2].szValue = fileInfo.lpszIcon;
    delTemplate[3].szValue = fileInfo.lpszRelFullPath;

    // Load editor
    template_iparse( req, TEMPLATE_FILE_DELETE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, delTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

// User want to download a file
void file_download( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Download File";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/stock_download.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );

    // Cleanup trick
    struct NNCMS_FILE_INFO fileInfo;

    // Get file name
    char *lpszRequestPath = 0;//httpdRequestPath( server );
    if( lpszRequestPath == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No file specified";
        log_printf( req, LOG_NOTICE, "File::Download: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get info about file
    file_get_info( &fileInfo, lpszRequestPath + 6 /* remove '/files' */ );
    if( fileInfo.bExists == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        log_printf( req, LOG_ERROR, "File::Download: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to download files
    if( file_check_perm( req, &fileInfo, "download" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Download: %s", frameTemplate[1].szValue );
        goto output;
    }
    return;

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

// Edit file
void file_edit( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Edit file";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/stock_edit.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS editTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_icon", /* szValue */ 0 },
            { /* szName */ "file_name", /* szValue */ 0 },
            { /* szName */ "file_text", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    editTemplate[5].szValue = req->g_sessionid;

    // File info
    struct NNCMS_FILE_INFO fileInfo;

    // Get file name
    struct NNCMS_VARIABLE *httpVarFile = main_get_variable( req, "file_name" );
    if( httpVarFile == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No file specified";
        log_printf( req, LOG_NOTICE, "File::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter file name and make a full path to it
    file_get_info( &fileInfo, httpVarFile->lpszValue );
    if( fileInfo.bExists == false )
    {
        // Oops, no file
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        log_printf( req, LOG_ERROR, "File::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to edit files
    if( file_check_perm( req, &fileInfo, "edit" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Edit: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "file_edit" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get new data
        struct NNCMS_VARIABLE *httpVarText = main_get_variable( req, "file_text" );
        if( httpVarText == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_ERROR, "File::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Remove old file
        if( remove( fileInfo.lpszAbsFullPath ) == -1 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = strerror( errno );
            log_printf( req, LOG_ERROR, "File::Edit: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Open for reading and writing.  The file is created  if  it  does not
        // exist, otherwise it is truncated.  The stream is positioned at the
        // beginning of the file.
        FILE *pFile = fopen( fileInfo.lpszAbsFullPath, "w+b" );
        if( pFile == 0 )
        {
            // Oops, no file
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Could not open file";
            log_printf( req, LOG_ERROR, "File::Edit: %s to write", frameTemplate[1].szValue );
            goto output;
        }

        // Write changes to file
        fwrite( httpVarText->lpszValue, strlen( httpVarText->lpszValue ), 1, pFile );

        // Close the file associated with the specified pFile stream after
        // flushing all buffers associated with it.
        fclose( pFile );

        // Log action
        log_printf( req, LOG_ACTION, "File::Edit: File \"%s\" was modified", fileInfo.lpszRelFullPath );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Open an existing file for both reading and writing.  The
    // initial contents of the file are unchanged and the initial
    // file position is at the beginning of the file.
    FILE *pFile = fopen( fileInfo.lpszAbsFullPath, "rb" );
    if( pFile == 0 )
    {
        // Oops, no file
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Could not open file";
        log_printf( req, LOG_ERROR, "File::Edit: %s to read", frameTemplate[1].szValue );
        goto output;
    }

    // Obtain file size.
    fseek( pFile, 0, SEEK_END );
    unsigned int lSize = ftell( pFile );
    rewind( pFile );

    // Allocate memory to contain the whole file.
    char *szTemp = (char *) MALLOC( lSize + 1 );
    if( szTemp == 0 ) return;

    // Copy the file into the buffer.
    fread( szTemp, 1, lSize, pFile );
    szTemp[lSize] = 0;

    // Close the file associated with the specified pFile stream after flushing
    // all buffers associated with it.
    fclose( pFile );

    //
    // The whole file is loaded in the buffer.
    //
    editTemplate[2].szValue = fileInfo.lpszIcon;
    editTemplate[3].szValue = fileInfo.lpszRelFullPath;
    editTemplate[4].szValue = szTemp; //template_escape_html( req, szTemp );

    // Load editor
    template_iparse( req, TEMPLATE_FILE_EDIT, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, editTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

    // Deallocate block of memory for file
    FREE( szTemp );
    //FREE( editTemplate[4].szValue );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void file_mkdir( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Create folder";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/folder-new.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS mkdirTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_path", /* szValue */ 0 },
            { /* szName */ "file_icon", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    mkdirTemplate[4].szValue = req->g_sessionid;

    // File info
    struct NNCMS_FILE_INFO dirInfo;

    // Get path
    struct NNCMS_VARIABLE *httpVarDir = main_get_variable( req, "file_path" );
    if( httpVarDir == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Directory not specified";
        log_printf( req, LOG_ERROR, "File::Mkdir: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter file name and make a full path to it
    file_get_info( &dirInfo, httpVarDir->lpszValue );
    if( dirInfo.bExists == false )
    {
        // Oops, no file
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Path not found";
        log_printf( req, LOG_ERROR, "File::Mkdir: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to create directories
    if( file_check_perm( req, &dirInfo, "mkdir" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Mkdir: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user commit changes
    //
    struct NNCMS_VARIABLE *httpVarEdit = main_get_variable( req, "file_mkdir" );
    if( httpVarEdit != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get POST data
        struct NNCMS_VARIABLE *httpVarFolderName = main_get_variable( req, "file_name" );
        if( httpVarFolderName == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "No data";
            log_printf( req, LOG_ERROR, "File::Mkdir: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Filter
        filter_table_replace( httpVarFolderName->lpszValue, (unsigned int) strlen( httpVarFolderName->lpszValue ), pathFilter );

        // Get info, full absolute path
        char szFolderName[NNCMS_PATH_LEN_MAX];
        strlcpy( szFolderName, dirInfo.lpszRelFullPath, sizeof(szFolderName) );
        strlcat( szFolderName, httpVarFolderName->lpszValue, sizeof(szFolderName) );
        struct NNCMS_FILE_INFO newDirInfo;
        file_get_info( &newDirInfo, szFolderName );

        // Already exists?
        if( newDirInfo.bExists == true )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "File or directory already exists";
            log_printf( req, LOG_ERROR, "File::Mkdir: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Try to create directory
        if( mkdir( newDirInfo.lpszAbsFullPath,
            511 /* 777 (drwxrwxrwx) & umask */ ) == -1 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = strerror( errno );
            log_printf( req, LOG_ERROR, "File::Mkdir: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Log action
        log_printf( req, LOG_ACTION, "File::Mkdir: Directory \"%s\" created", newDirInfo.lpszRelFullPath );

        // Redirect back
        redirect_to_referer( req );
        return;
    }

    // Some static form info
    mkdirTemplate[2].szValue = httpVarDir->lpszValue;
    mkdirTemplate[3].szValue = dirInfo.lpszIcon;

    // Load template
    template_iparse( req, TEMPLATE_FILE_MKDIR, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, mkdirTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void file_rename( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Rename file";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/actions/gtk-edit.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS renameTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_name", /* szValue */ 0 },
            { /* szName */ "file_new", /* szValue */ 0 },
            { /* szName */ "file_icon", /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    renameTemplate[5].szValue = req->g_sessionid;

    // Init file info to zero for cleanup trick
    struct NNCMS_FILE_INFO fileInfo;

    // Get path
    struct NNCMS_VARIABLE *httpVarFile = main_get_variable( req, "file_name" );
    if( httpVarFile == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No path specified";
        log_printf( req, LOG_NOTICE, "File::Rename: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter file name and make a full path to it
    file_get_info( &fileInfo, httpVarFile->lpszValue );
    if( fileInfo.bExists == false )
    {
        // Oops, no file
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        log_printf( req, LOG_ERROR, "File::Rename: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to rename files
    if( file_check_perm( req, &fileInfo, "rename" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Rename: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Check if user pushed big gray shiny "rename" button
    //
    struct NNCMS_VARIABLE *httpVarRename = main_get_variable( req, "file_rename" );
    if( httpVarRename != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Check if new path provided and new file it does not exist
        struct NNCMS_VARIABLE *httpVarRenameTo = main_get_variable( req, "file_new" );
        if( httpVarRenameTo == 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Rename to what?";
            log_printf( req, LOG_ERROR, "File::Rename: %s", frameTemplate[1].szValue );
            goto output;
        }
        if( file_is_exists( httpVarRenameTo->lpszValue ) == true )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "The \"rename_to\" file already exists";
            log_printf( req, LOG_ERROR, "File::Rename: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Make full paths
        char szOldPath[NNCMS_PATH_LEN_MAX];
        char szNewPath[NNCMS_PATH_LEN_MAX];
        strlcpy( szOldPath, fileDir, NNCMS_PATH_LEN_MAX );
        strlcpy( szNewPath, fileDir, NNCMS_PATH_LEN_MAX );
        strlcat( szOldPath, fileInfo.lpszRelFullPath, NNCMS_PATH_LEN_MAX );
        strlcat( szNewPath, httpVarRenameTo->lpszValue, NNCMS_PATH_LEN_MAX );

        // Call rename function
        int nResult = rename( szOldPath, szNewPath );
        if( nResult != 0 )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Could not rename file.";
            log_printf( req, LOG_ERROR, "File::Rename: %s (err%u)", frameTemplate[1].szValue, nResult );
            goto output;
        }

        // Log action
        log_printf( req, LOG_ACTION, "File::Rename: Renamed \"%s\" to \"%s\"", szOldPath, szNewPath );

        // Redirect back
        redirect_to_referer( req );

        return;
    }

    // Make "rename_to" and "rename_from" strings
    renameTemplate[2].szValue = fileInfo.lpszRelFullPath;
    renameTemplate[3].szValue = fileInfo.lpszRelFullPath;
    renameTemplate[4].szValue = fileInfo.lpszIcon;

    // Load rename page
    template_iparse( req, TEMPLATE_FILE_RENAME, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, renameTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

void file_upload( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Upload file";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/stock/stock_upload.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };
    struct NNCMS_TEMPLATE_TAGS uploadTemplate[] =
        {
            { /* szName */ "referer", /* szValue */ FCGX_GetParam( "HTTP_REFERER", req->fcgi_request->envp ) },
            { /* szName */ "homeURL", /* szValue */ homeURL },
            { /* szName */ "file_path",  /* szValue */ 0 },
            { /* szName */ "file_icon",  /* szValue */ 0 },
            { /* szName */ "fkey", /* szValue */ 0 },
            { /* szName */ 0, /* szValue */ 0 } // Terminating row
        };

    // Check session
    user_check_session( req );
    uploadTemplate[4].szValue = req->g_sessionid;

    // Dir info
    struct NNCMS_FILE_INFO dirInfo;

    // Get path
    struct NNCMS_VARIABLE *httpVarPath = main_get_variable( req, "file_path" );
    if( httpVarPath == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No path specified";
        log_printf( req, LOG_NOTICE, "File::Upload: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter file name and make a full path to it
    file_get_info( &dirInfo, httpVarPath->lpszValue );
    if( dirInfo.bExists == false )
    {
        // Oops, no file
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Path not found";
        log_printf( req, LOG_ERROR, "File::Upload: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Check user permission to upload files
    if( file_check_perm( req, &dirInfo, "upload" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "File::Upload: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Filter the path
    char lpszPath[NNCMS_PATH_LEN_MAX];
    strlcpy( lpszPath, httpVarPath->lpszValue, NNCMS_PATH_LEN_MAX );
    filter_path( lpszPath, 0, 1 );

    //
    // Check if user pushed big gray shiny "upload" button
    //
    struct NNCMS_VARIABLE *httpVarUpload = main_get_variable( req, "file_upload" );
    if( httpVarUpload != 0 )
    {
        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Anti CSRF / XSRF vulnerabilities
        if( user_xsrf( req ) == false )
        {
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Unequal keys";
            goto output;
        }

        // Get uploaded file
        struct NNCMS_FILE *lpUploadedFile = req->lpUploadedFiles;
        if( lpUploadedFile == 0 )
        {
            // If there was no first file, then it might be an attack
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "Got multipart/form-data without file!!";
            log_printf( req, LOG_ALERT, "File::Upload: %s", frameTemplate[1].szValue );

            goto output;
        }

file_upload_retry:
        if( lpUploadedFile == 0 )
        {
            // Ok, done, redirect
            redirect_to_referer( req );
            return;
        }

        // Save it on disk
        char lpszFullPath[NNCMS_PATH_LEN_MAX];
        strlcpy( lpszFullPath, fileDir, NNCMS_PATH_LEN_MAX );
        strlcat( lpszFullPath, lpszPath, NNCMS_PATH_LEN_MAX );
        strlcat( lpszFullPath, lpUploadedFile->szFileName, NNCMS_PATH_LEN_MAX );
        filter_path( lpszFullPath, -1, 0 );
        FILE *pFile = fopen( lpszFullPath, "r" );
        if( pFile == 0 )
        {
            pFile = fopen( lpszFullPath, "w" );
            if( pFile != 0 )
            {
                fwrite( lpUploadedFile->lpData, lpUploadedFile->nSize, 1, pFile );
                fclose( pFile );
            }
            else
            {
                frameTemplate[0].szValue = "Error";
                frameTemplate[1].szValue = "Unable to open file for writing";
                log_printf( req, LOG_ERROR, "File::Upload: %s", frameTemplate[1].szValue );
                goto output;
            }
        }
        else
        {
            fclose( pFile );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = "File already exists";
            log_printf( req, LOG_ERROR, "File::Upload: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Log action
        log_printf( req, LOG_ACTION, "File::Upload: File \"%s%s\" uploaded", lpszPath, lpUploadedFile->szFileName );

        // Check if uploaded more than one
        lpUploadedFile = lpUploadedFile->lpNext;
        goto file_upload_retry;
    }

    // Upload path
    uploadTemplate[2].szValue = lpszPath;
    uploadTemplate[3].szValue = dirInfo.lpszIcon;

    // Load upload page
    template_iparse( req, TEMPLATE_FILE_UPLOAD, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, uploadTemplate );
    //strlcpy( req->lpszBuffer, req->lpszFrame, NNCMS_PAGE_SIZE_MAX );

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################
// From ls.c

int file_compare_name_asc( const void *a, const void *b )
{
    if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) )
    {
        return -1;
    }
    else if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) )
    {
        return 1;
    }
    else
    {
        return strcmp( ((struct NNCMS_FILE_ENTITY *)a)->fname, ((struct NNCMS_FILE_ENTITY *)b)->fname );
    }
}

int file_compare_name_desc( const void *a, const void *b )
{
    if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) )
    {
        return -1;
    }
    else if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) )
    {
        return 1;
    }
    else
    {
        return - strcmp( ((struct NNCMS_FILE_ENTITY *)a)->fname, ((struct NNCMS_FILE_ENTITY *)b)->fname );
    }
}

int file_compare_date_asc( const void *a, const void *b )
{
    if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) )
    {
        return -1;
    }
    else if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) )
    {
        return 1;
    }
    else
    {
        if( ((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mtime > ((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mtime )
        {
            return 1;
        }
        else if( ((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mtime < ((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mtime )
        {
            return -1;
        }
        return 0;
    }
}

int file_compare_date_desc( const void *a, const void *b )
{
    if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) )
    {
        return -1;
    }
    else if( S_ISDIR(((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mode) &&
        S_ISREG(((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mode) )
    {
        return 1;
    }
    else
    {
        if( ((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mtime > ((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mtime )
        {
            return -1;
        }
        else if( ((struct NNCMS_FILE_ENTITY *)a)->f_stat.st_mtime < ((struct NNCMS_FILE_ENTITY *)b)->f_stat.st_mtime )
        {
            return 1;
        }
        return 0;
    }
}

// #############################################################################

struct NNCMS_FILE_ENTITY *file_get_directory( struct NNCMS_THREAD_INFO *req, char *lpszPath, unsigned int *nDirCount, int(*compar)(const void *, const void *) )
{
    //
    // List files in current directory
    //
    DIR *lpDirStream = opendir( lpszPath );
    if( lpDirStream == 0 )
        return 0;

    // Get entity count
    *nDirCount = 0;
    rewinddir( lpDirStream );
    while( readdir( lpDirStream ) != 0 )
        (*nDirCount)++;

    //
    // Fetch entities
    //
    rewinddir( lpDirStream );

    // Allocate memory
    struct NNCMS_FILE_ENTITY *lpEnts = MALLOC( sizeof(struct NNCMS_FILE_ENTITY) * *nDirCount + 1 )
    if( lpEnts == 0 )
    {
        log_printf( req, LOG_ERROR, "File::GetDirectory: Could not fetch entities" );
        return 0;
    }

    // Put every directory in own sturcture
    unsigned i = 0;
    struct dirent *lpEntry;
    while( (lpEntry = readdir( lpDirStream )) != 0 )
    {
        // Retreive attributes of file
        char szTemp[NNCMS_PATH_LEN_MAX];
        strlcpy( szTemp, lpszPath, NNCMS_PATH_LEN_MAX );
        strlcat( szTemp, lpEntry->d_name, NNCMS_PATH_LEN_MAX );
        if( lstat( szTemp, &(lpEnts[i].f_stat) ) != 0 )
        {
            FREE( lpEnts );
            log_printf( req, LOG_ERROR, "File::GetDirectory: Could not get \"%s\" file status", szTemp );
            return 0;
        }
        strcpy( lpEnts[i].fname, lpEntry->d_name );
        i++;
    }

    // Close directory stream
    if( closedir( lpDirStream ) == -1 )
    {
        FREE( lpEnts );
        log_printf( req, LOG_ERROR, "File::Browse: Could not close stream" );
        return 0;
    }

    //
    // Entity process loop
    //
    qsort( lpEnts, *nDirCount, sizeof(struct NNCMS_FILE_ENTITY), compar ); // Sort the entries

    return lpEnts;
}

// #############################################################################

// This routine is needed to avoid code duplicating
// If you have full path to file, then you dont have to split it
// just put it as first or second parameter, the other one then shall be zero.
bool file_check_perm( struct NNCMS_THREAD_INFO *req, struct NNCMS_FILE_INFO *fileInfo, char *lpszPerm )
{

    char szFullPath[NNCMS_PATH_LEN_MAX];

    // Check session
    user_check_session( req );

    // Test
    /*printf( "file_check_perm\n"
            TERM_BG_BLUE "fileInfo->lpszRelFullPath: " TERM_RESET "%s\n"
            TERM_BG_BLUE "lpszPerm: " TERM_RESET "%s\n"
            TERM_BG_BLUE "req->g_username: " TERM_RESET "%s\n\n", fileInfo->lpszRelFullPath, lpszPerm, req->g_username );//*/

    // Попробуем проверить доступ у предыдущего (корневого) каталога
    char *lpszPath = strdup_d( fileInfo->lpszRelFullPath );
    char *lpszTemp = lpszPath;
    int i = 0;
    while( 1 )
    {
        // Название в базе
        snprintf( szFullPath, sizeof(szFullPath), "file_%s", lpszPath );

        // Test
        //printf( TERM_FG_GREEN "acl_check_perm — szFullPath: " TERM_RESET "%s\n", szFullPath );

        // Проверка доступа
        if( acl_check_perm( req, szFullPath, req->g_username, lpszPerm ) == true )
        {
            FREE( lpszPath );

            return true;
        }

        // Взять следующую дочернюю папку
fetch_parent:
        lpszTemp = strrchr( lpszPath, '/' );
        if( lpszTemp == 0 )
        {
            break;
        }
        else
        {
            if( lpszTemp[1] == 0 )
            {
                // This is a directory, we are at null
                // terminating character
                lpszTemp[0] = 0;
                goto fetch_parent;
            }
            else
            {
                lpszTemp[1] = 0;
            }
        }

        i++;
    }

    FREE( lpszPath );

    // Test
    //printf( TERM_FG_GREEN "szFullPath: " TERM_RESET "%s\n", szFullPath );

    return false;
}

// #############################################################################

// Check if given file exists in file system. Returns 1 if it exists and
// zero if not.
bool file_is_exists( char *szFile )
{
    FILE *pFile = fopen( szFile, "r" );
    if( pFile == 0 )
    {
        return false;
    }
    else
    {
        fclose( pFile );
        return true;
    }
}

// #############################################################################

// This function will return string with path to icon for given filename.
char *file_get_icon( char *lpszFile )
{
    char *lpszExtension = get_extension( lpszFile );
    static char lpszIcon[NNCMS_PATH_LEN_MAX];
    strlcpy( lpszIcon, fileIcons[0].szIcon, NNCMS_PATH_LEN_MAX );
    if( lpszExtension != 0 )
    {
        lpszExtension++;
        for( unsigned int i = 0; i < NNCMS_ICONS_MAX; i++ )
        {
            // Check for terminating row
            if( fileIcons[i].szExtension == 0 ||
                fileIcons[i].szIcon == 0 ) break;

            // Compare extension and current icon extension
            if( strcasecmp( lpszExtension,
                fileIcons[i].szExtension ) == 0 )
            {
                strlcpy( lpszIcon, fileIcons[i].szIcon, NNCMS_PATH_LEN_MAX );
            }
        }
    }
    return lpszIcon;
}

// #############################################################################

void file_get_info( struct NNCMS_FILE_INFO *fileInfo, char *lpszFile )
{
    char szTempPath[NNCMS_PATH_LEN_MAX];
    size_t nLen;

    // Allocate memory for FILE_INFO structure
    memset( fileInfo, 0, sizeof(struct NNCMS_FILE_INFO) );

    // Copy path to temp buffer and filter it from possible evil
    strlcpy( szTempPath, lpszFile, NNCMS_PATH_LEN_MAX );
    filter_path( szTempPath, 1, -1 );

    // szTempPath is now relative fullpath
    unsigned int nRelFullPathSize = strlen( szTempPath );
    strlcpy( fileInfo->lpszRelFullPath, szTempPath, NNCMS_PATH_LEN_MAX );

    // Create absolute full path
    nLen = strlcpy( fileInfo->lpszAbsFullPath, fileDir, NNCMS_PATH_LEN_MAX );
    strlcpy( &fileInfo->lpszAbsFullPath[nLen], szTempPath + sizeof(char), NNCMS_PATH_LEN_MAX - nLen );

    // Get relative path, we need to just remove filename from szTempPath
    char *lpszFileName = rindex( szTempPath, '/' );
    if( lpszFileName == 0 ) return;
    lpszFileName++; // Skip slash
    unsigned int nRelPathLen = lpszFileName - szTempPath;
    strlcpy( fileInfo->lpszRelPath, szTempPath, nRelPathLen + 1 );

    // Create absolute path
    nLen = strlcpy( fileInfo->lpszAbsPath, fileDir, NNCMS_PATH_LEN_MAX );
    strlcat( &fileInfo->lpszAbsPath[nLen], fileInfo->lpszRelPath + 1, NNCMS_PATH_LEN_MAX - nLen );

    // Get file name
    strlcpy( fileInfo->lpszFileName, szTempPath + nRelPathLen, NNCMS_PATH_LEN_MAX );

    // Get file extension
    char *lpszExtension = rindex( fileInfo->lpszFileName, '.' );
    if( lpszExtension != 0 )
    {
        // Skip dot and copy to extension var
        lpszExtension++;
        strlcpy( fileInfo->lpszExtension, lpszExtension, sizeof(fileInfo->lpszExtension) );
    }
    else
    {
        *fileInfo->lpszExtension = 0;
    }

    // Now, we must know, is it a directory or a file?
    if( szTempPath[nRelFullPathSize - sizeof(char)] == '/' )
    {
        fileInfo->bIsDir = true;
        fileInfo->bIsFile = false;
    }
    else
    {
        fileInfo->bIsDir = false;
        fileInfo->bIsFile = true;
    }

    // Get mimetype (simple)
    fileInfo->lpszMimeType = get_mime( fileInfo->lpszFileName );

    // Get file size
    if( lstat( fileInfo->lpszAbsFullPath, &fileInfo->fileStat ) == 0 )
    {
        // Check what is this
        if( S_ISREG( fileInfo->fileStat.st_mode ) == 1 )
        {
            fileInfo->bIsDir = false;
            fileInfo->bIsFile = true;
        }
        else if( S_ISDIR( fileInfo->fileStat.st_mode ) == 1 )
        {
            fileInfo->bIsDir = true;
            fileInfo->bIsFile = false;
        }

        // File/directory exists
        fileInfo->bExists = true;
    }
    fileInfo->nSize = fileInfo->fileStat.st_size;

    // Get icon (simple)
    if( fileInfo->bIsDir == true )
        fileInfo->lpszIcon = file_get_icon( ".folder" );
    else
        fileInfo->lpszIcon = file_get_icon( fileInfo->lpszFileName );

    // Get parent relative directory path
    strlcpy( fileInfo->lpszParentDir, szTempPath, nRelFullPathSize );
    char *lpszDirName = strrchr( fileInfo->lpszParentDir, '/' );
    if( lpszDirName != 0 )
        lpszDirName[1] = 0; // [1] - means that we leave '/' character, so
                            // parent directory would look like: "/temp/\0"

    // Test
    /*printf( "-----\nRelFullPath: %s\nAbsFullPath: %s\nRelPath: %s\n"
            "AbsPath: %s\nFileName: %s\nExtension: %s\nMimeType: %s\n"
            "IsDir: %s\nIsFile: %s\nIsExists: %s\nSize: %u\nIcon: %s\n"
            "ParentDir: %s\n-----\n",
        fileInfo->lpszRelFullPath, fileInfo->lpszAbsFullPath, fileInfo->lpszRelPath, fileInfo->lpszAbsPath,
        fileInfo->lpszFileName, fileInfo->lpszExtension, fileInfo->lpszMimeType, (fileInfo->bIsDir == true ? "Yes" : "No"),
        (fileInfo->bIsFile == true ? "Yes" : "No"), (fileInfo->bExists == true ? "Yes" : "No"),
        fileInfo->nSize, fileInfo->lpszIcon, fileInfo->lpszParentDir ); // */

    return;
}

// #############################################################################

static char *szSizePrefix[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "PiB" };
void file_human_size( char *szDest, unsigned int nSize )
{
    unsigned char i = 0;
    double nSizeTemp = nSize;

    while( nSizeTemp > 1024 )
    {
        nSizeTemp /= 1024;
        i++;
    }

    if( i == 0 ) sprintf( szDest, "%0.0f %s", nSizeTemp, szSizePrefix[i] );
    else sprintf( szDest, "%0.1f %s", nSizeTemp, szSizePrefix[i] );
}

// #############################################################################
