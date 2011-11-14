// #############################################################################
// Source file: view.c

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

#define _GNU_SOURCE
#include <string.h>

#ifdef HAVE_IMAGEMAGICK
//#include <magick/MagickCore.h>
#include <wand/MagickWand.h>
#endif // HAVE_IMAGEMAGICK

//#include <mgl/mgl_c.h>

// #############################################################################
// includes of local headers
//

#include "view.h"
#include "main.h"
#include "template.h"
#include "mime.h"
#include "file.h"
#include "log.h"
#include "cache.h"
#include "tree.h"

#include "strlcpy.h"
#include "strlcat.h"

#include "memory.h"

// #############################################################################
// global variables
//

// #############################################################################
// functions

// Show file
void view_show( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "View file";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ req->lpszBuffer },
            { /* szName */ "icon",  /* szValue */ "images/mimetypes/application-octet-stream.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    // Initialize fileInfo for anti-crash trick
    struct NNCMS_FILE_INFO fileInfo;

    // Get filename to be opened
    struct NNCMS_VARIABLE *httpVarFile = main_get_variable( req, "file" );
    if( httpVarFile == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No file";
        goto output;
    }
    file_get_info( &fileInfo, httpVarFile->lpszValue );
    if( fileInfo.bExists == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        goto output;
    }

    // Check user permission to view files
    if( file_check_perm( req, &fileInfo, "view" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "View::Show: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Tag values
    frameTemplate[0].szValue = fileInfo.lpszRelFullPath;
    frameTemplate[2].szValue = fileInfo.lpszIcon;

    //
    // Read file
    //
    FILE *pFile = fopen( fileInfo.lpszAbsFullPath, "r" );
    if( pFile == 0 )
    {
        // File not found
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        goto output;
    }

    // Get file size
    fseek( pFile, 0, SEEK_END );
    size_t nSize = ftell( pFile );
    rewind( pFile );

    // Do not accept zero files
    if( nSize == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File is empty";
        goto output;
    }

    // Truncate (limit) the size
    if( nSize > NNCMS_PAGE_SIZE_MAX )
        nSize = NNCMS_PAGE_SIZE_MAX;

    char *szTempBuffer = MALLOC( nSize + 1 );
    if( szTempBuffer == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Unable to allocate block of memory for file";
        goto output;
    }

    // Copy file to req->szBuffer
    unsigned int nOutputSize = fread( szTempBuffer, 1, nSize, pFile );
    szTempBuffer[nOutputSize] = 0; // 0 terminating character
    fclose( pFile );

    // Create string with human readable file size
    char szHumanSize[32];
    if( fileInfo.bIsDir == true ) *szHumanSize = 0;
    else file_human_size( szHumanSize, fileInfo.nSize );

    // Get file status
    struct stat stFile;
    stat( fileInfo.lpszAbsFullPath, &stFile );

    // Strings
    char szSize[32];
    snprintf( szSize, sizeof(szSize), "%u", fileInfo.nSize );

    // Specify values for file preview template
    struct NNCMS_TEMPLATE_TAGS fileTemplate[] =
        {
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ "file_icon",  /* szValue */ fileInfo.lpszIcon },
            { /* szName */ "file_path", /* szValue */ fileInfo.lpszRelPath },
            { /* szName */ "file_name", /* szValue */ fileInfo.lpszFileName },
            { /* szName */ "file_fullpath", /* szValue */ fileInfo.lpszRelFullPath },
            { /* szName */ "file_mimetype",  /* szValue */ fileInfo.lpszMimeType },
            { /* szName */ "file_human_size",  /* szValue */ szHumanSize },
            { /* szName */ "file_size",  /* szValue */ szSize },
            { /* szName */ "file_actions",  /* szValue */ 0 },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    //
    // Filter it somehow
    //
    unsigned int i, j;
    if( strcmp( fileInfo.lpszMimeType, "text/html" ) == 0 )
    {
        // Remove everything outside <body></body> tag
        char *lpszHtmlBegin = strcasestr( szTempBuffer, "<body" );
        if( lpszHtmlBegin != 0 )
        {
            lpszHtmlBegin = strchr( lpszHtmlBegin, '>' ) + 1;
            if( lpszHtmlBegin == 0 )
            {
                frameTemplate[0].szValue = "Error";
                frameTemplate[1].szValue = "Error parsing html file, body tag is unfinished";
                log_printf( req, LOG_ERROR, "View::Show: %s", frameTemplate[1].szValue );
                goto output;
            }
            char *lpszHtmlEnd = strcasestr( szTempBuffer, "</body>" );
            if( lpszHtmlEnd != 0 )
                *lpszHtmlEnd = 0;

            // Parse as template
            template_sparse( req, fileInfo.lpszFileName, lpszHtmlBegin, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, fileTemplate );
        }
        else
        {
            // Parse as template
            template_sparse( req, fileInfo.lpszFileName, szTempBuffer, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, fileTemplate );
        }
    }
    else if( strcmp( fileInfo.lpszMimeType, "text/plain" ) == 0 )
    {
        j = 0;
        for( i = 0; i < nOutputSize; i++ )
        {
            if( (j + 16) > NNCMS_PAGE_SIZE_MAX )
                break;

            // Convert CRLF to <BR>
            if( szTempBuffer[i] == '\r' )
                continue;
            if( szTempBuffer[i] == '\n' )
            {
                strcpy( &req->lpszBuffer[j], "<br>\r\n" ); j += 6;
                continue;
            }

            // Replace some special characters
            if( szTempBuffer[i] == '<' )
            {
                strcpy( &req->lpszBuffer[j], "&lt;" ); j += 4;
                continue;
            }
            if( szTempBuffer[i] == '>' )
            {
                strcpy( &req->lpszBuffer[j], "&gt;" ); j += 4;
                continue;
            }
            if( szTempBuffer[i] == '&' )
            {
                strcpy( &req->lpszBuffer[j], "&amp;" ); j += 5;
                continue;
            }
            if( szTempBuffer[i] == ' ' && szTempBuffer[i + 1] == ' ' )
            {
                strcpy( &req->lpszBuffer[j], "&nbsp;" ); j += 6;
                continue;
            }

            // Copy other characters without modify
            req->lpszBuffer[j] = szTempBuffer[i];
            j++;
        }
        req->lpszBuffer[j] = 0;
    }
    //else if( strcmp( fileInfo.lpszMimeType, "application/octet-stream" ) == 0 )
    //{
    //    hex_dump( req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, szTempBuffer, nOutputSize );
    //}
#ifdef HAVE_IMAGEMAGICK
    else if( strncmp( fileInfo.lpszMimeType, "image/", 6 ) == 0 )
    {
        //
        // Initialize the image info structure and read an image.
        //
        MagickWand *magick_wand = NewMagickWand( );
        MagickBooleanType result = MagickPingImage( magick_wand, fileInfo.lpszAbsFullPath );
        //MagickBooleanType result = MagickReadImage( magick_wand, fileInfo.lpszAbsFullPath );
        if( result == MagickFalse )
        {
            view_iw_exception( req, magick_wand, "Image not loaded", req->lpszBuffer );
            frameTemplate[0].szValue = "Error";
            frameTemplate[1].szValue = req->lpszBuffer;
            log_printf( req, LOG_ERROR, "View::Show: %s", frameTemplate[1].szValue );
            goto output;
        }

        // Compression
        struct IMAGEMAGICK_COMPRESSIONS
            {
                CompressionType compression;
                char *szName;
            }
            strCompressions[] =
            {
                { /* compression */ UndefinedCompression, /* szName */ "Undefined compression" },
                { /* compression */ NoCompression, /* szName */ "No compression" },
                { /* compression */ BZipCompression, /* szName */ "BZip compression" },
                { /* compression */ DXT1Compression, /* szName */ "DXT1 compression" },
                { /* compression */ DXT3Compression, /* szName */ "DXT3 compression" },
                { /* compression */ DXT5Compression, /* szName */ "DXT5 compression" },
                { /* compression */ FaxCompression, /* szName */ "Fax compression" },
                { /* compression */ Group4Compression, /* szName */ "Group4 compression" },
                { /* compression */ JPEGCompression, /* szName */ "JPEG compression" },
                { /* compression */ JPEG2000Compression, /* szName */ "JPEG2000 compression" },
                { /* compression */ LosslessJPEGCompression, /* szName */ "Lossless JPEG compression" },
                { /* compression */ LZWCompression, /* szName */ "LZW compression" },
                { /* compression */ RLECompression, /* szName */ "RLE compression" },
                { /* compression */ ZipCompression, /* szName */ "Zip compression" },
                { /* compression */ 0, /* szName */ 0 }
            };
        char *lpszCompression = "Unknown compression";
        CompressionType imgCompression = MagickGetCompression( magick_wand );
        for( unsigned int i = 0; strCompressions[i].szName != 0; i++ )
        {
            if( strCompressions[i].compression == imgCompression )
            {
                lpszCompression = strCompressions[i].szName;
                break;
            }
        }

        // Orientation
        struct IMAGEMAGICK_ORIENTATIONS
            {
                OrientationType orientation;
                char *szName;
            }
            strOrientations[] =
            {
                { /* orientation */ UndefinedOrientation, /* szName */ "Undefined orientation" },
                { /* orientation */ TopLeftOrientation, /* szName */ "Top-left orientation" },
                { /* orientation */ TopRightOrientation, /* szName */ "Top-right orientation" },
                { /* orientation */ BottomRightOrientation, /* szName */ "Bottom-right orientation" },
                { /* orientation */ BottomLeftOrientation, /* szName */ "Bottom-left orientation" },
                { /* orientation */ LeftTopOrientation, /* szName */ "Left-top orientation" },
                { /* orientation */ RightTopOrientation, /* szName */ "Right-top orientation" },
                { /* orientation */ RightBottomOrientation, /* szName */ "Right-bottom orientation" },
                { /* orientation */ LeftBottomOrientation, /* szName */ "Left-bottom orientation" },
                { /* orientation */ 0, /* szName */ 0 }
            };
        char *lpszOrientation = "Unknown orientation";
        OrientationType imgOrientation = MagickGetOrientation( magick_wand );
        for( unsigned int i = 0; strOrientations[i].szName != 0; i++ )
        {
            if( strOrientations[i].orientation == imgOrientation )
            {
                lpszOrientation = strOrientations[i].szName;
                break;
            }
        }

        // Image type
        struct IMAGEMAGICK_IMAGETYPES
            {
                ImageType type;
                char *szName;
            }
            strImageTypes[] =
            {
                { /* type */ UndefinedType, /* szName */ "Undefined type" },
                { /* type */ BilevelType, /* szName */ "Bilevel type" },
                { /* type */ GrayscaleType, /* szName */ "Grayscale type" },
                { /* type */ GrayscaleMatteType, /* szName */ "Grayscale matte type" },
                { /* type */ PaletteType, /* szName */ "Palette type" },
                { /* type */ PaletteMatteType, /* szName */ "Palette matte type" },
                { /* type */ TrueColorType, /* szName */ "True color type" },
                { /* type */ TrueColorMatteType, /* szName */ "True color matte type" },
                { /* type */ ColorSeparationType, /* szName */ "Color separation type" },
                { /* type */ ColorSeparationMatteType, /* szName */ "Color separation matte type" },
                { /* type */ OptimizeType, /* szName */ "Optimize type" },
                { /* type */ PaletteBilevelMatteType, /* szName */ "Palette bi-level matte type" },
                { /* type */ 0, /* szName */ 0 }
            };
        char *lpszImageType = "Unknown image type";
        ImageType imgType = MagickGetType( magick_wand );
        for( unsigned int i = 0; strImageTypes[i].szName != 0; i++ )
        {
            if( strImageTypes[i].type == imgType )
            {
                lpszImageType = strImageTypes[i].szName;
                break;
            }
        }

        // Interlace type
        struct IMAGEMAGICK_INTERLACES
            {
                InterlaceType interlace;
                char *szName;
            }
            strInterlaceTypes[] =
            {
                { /* type */ UndefinedInterlace, /* szName */ "Undefined interlace" },
                { /* type */ NoInterlace, /* szName */ "No interlace" },
                { /* type */ LineInterlace, /* szName */ "Line interlace" },
                { /* type */ PartitionInterlace, /* szName */ "Partition interlace" },
                { /* type */ GIFInterlace, /* szName */ "GIF interlace" },
                { /* type */ JPEGInterlace, /* szName */ "JPEG interlace" },
                { /* type */ PNGInterlace, /* szName */ "PNG interlace" },
                { /* type */ 0, /* szName */ 0 }
            };
        char *lpszInterlaceType = "Unknown interlace type";
        InterlaceType imgInterlace = MagickGetInterlaceScheme( magick_wand );
        for( unsigned int i = 0; strInterlaceTypes[i].szName != 0; i++ )
        {
            if( strInterlaceTypes[i].interlace == imgInterlace )
            {
                lpszInterlaceType = strInterlaceTypes[i].szName;
                break;
            }
        }

        // Resolution type
        struct IMAGEMAGICK_RESOLUTIONS
            {
                ResolutionType units;
                char *szName;
            }
            strResolutionTypes[] =
            {
                { /* type */ UndefinedResolution, /* szName */ "(undefined resolution)" },
                { /* type */ PixelsPerInchResolution, /* szName */ "pixels per inch" },
                { /* type */ PixelsPerCentimeterResolution, /* szName */ "pixels per centimeter" },
                { /* type */ 0, /* szName */ 0 }
            };
        char *lpszResolution = "(unknown resolution)";
        ResolutionType imgResolution = MagickGetImageUnits( magick_wand );
        for( unsigned int i = 0; strResolutionTypes[i].szName != 0; i++ )
        {
            if( strResolutionTypes[i].units == imgResolution )
            {
                lpszResolution = strResolutionTypes[i].szName;
                break;
            }
        }

        // Colorspace
        struct IMAGEMAGICK_COLORSPACES
            {
                ColorspaceType colorspace;
                char *szName;
            }
            strColorspaceTypes[] =
            {
                { /* type */ UndefinedColorspace, /* szName */ "Undefined colorspace" },
                { /* type */ RGBColorspace, /* szName */ "RGB colorspace" },
                { /* type */ GRAYColorspace, /* szName */ "GRAY colorspace" },
                { /* type */ TransparentColorspace, /* szName */ "Transparent colorspace" },
                { /* type */ OHTAColorspace, /* szName */ "OHTA colorspace" },
                { /* type */ LabColorspace, /* szName */ "Lab colorspace" },
                { /* type */ XYZColorspace, /* szName */ "XYZ colorspace" },
                { /* type */ YCbCrColorspace, /* szName */ "YCbCr colorspace" },
                { /* type */ YCCColorspace, /* szName */ "YCC colorspace" },
                { /* type */ YIQColorspace, /* szName */ "YIQ colorspace" },
                { /* type */ YPbPrColorspace, /* szName */ "YPbPr colorspace" },
                { /* type */ YUVColorspace, /* szName */ "YUV colorspace" },
                { /* type */ CMYKColorspace, /* szName */ "CMYK colorspace" },
                { /* type */ sRGBColorspace, /* szName */ "sRGB colorspace" },
                { /* type */ HSBColorspace, /* szName */ "HSB colorspace" },
                { /* type */ HSLColorspace, /* szName */ "HSL colorspace" },
                { /* type */ HWBColorspace, /* szName */ "HWB colorspace" },
                { /* type */ Rec601LumaColorspace, /* szName */ "Rec601Luma colorspace" },
                { /* type */ Rec601YCbCrColorspace, /* szName */ "Rec601YCbCr colorspace" },
                { /* type */ Rec709LumaColorspace, /* szName */ "Rec709Luma colorspace" },
                { /* type */ Rec709YCbCrColorspace, /* szName */ "Rec709YCbCr colorspace" },
                { /* type */ LogColorspace, /* szName */ "Log colorspace" },
                { /* type */ CMYColorspace, /* szName */ "CMY colorspace" },
                { /* type */ 0, /* szName */ 0 }
            };
        char *lpszColorspace = "Unknown colorspace";
        ColorspaceType imgColorspace = MagickGetColorspace( magick_wand );
        for( unsigned int i = 0; strColorspaceTypes[i].szName != 0; i++ )
        {
            if( strColorspaceTypes[i].colorspace == imgColorspace )
            {
                lpszColorspace = strColorspaceTypes[i].szName;
                break;
            }
        }

        // Resolution
        double x_resolution, y_resolution;
        MagickGetImageResolution( magick_wand, &x_resolution, &y_resolution );

        // Other strings
        char szSize[32], szWidth[32], szHeight[32],
            szXResolution[32], szYResolution[32],
            szDepth[32], szColors[32], szQuality[32];
        snprintf( szSize, sizeof(szSize), "%u", fileInfo.nSize );
        snprintf( szWidth, sizeof(szWidth), "%lu", MagickGetImageWidth( magick_wand ) );
        snprintf( szHeight, sizeof(szHeight), "%lu", MagickGetImageHeight( magick_wand ) );
        snprintf( szXResolution, sizeof(szXResolution), "%0.2f", x_resolution );
        snprintf( szYResolution, sizeof(szYResolution), "%0.2f", y_resolution );
        snprintf( szDepth, sizeof(szDepth), "%lu", MagickGetImageDepth( magick_wand ) );
        snprintf( szColors, sizeof(szColors), "%lu", MagickGetImageColors( magick_wand ) );
        snprintf( szQuality, sizeof(szQuality), "%lu", MagickGetCompressionQuality( magick_wand ) );

        // Specify values for image preview template
        struct NNCMS_TEMPLATE_TAGS imageTemplate[] =
            {
                { /* szName */ "file_icon",  /* szValue */ fileInfo.lpszIcon },
                { /* szName */ "homeURL",  /* szValue */ homeURL },
                { /* szName */ "file_path", /* szValue */ fileInfo.lpszRelPath },
                { /* szName */ "file_name", /* szValue */ fileInfo.lpszFileName },
                { /* szName */ "file_fullpath", /* szValue */ fileInfo.lpszRelFullPath },
                { /* szName */ "file_mimetype",  /* szValue */ fileInfo.lpszMimeType },
                { /* szName */ "file_human_size",  /* szValue */ szHumanSize },
                { /* szName */ "file_size",  /* szValue */ szSize },
                { /* szName */ "file_actions",  /* szValue */ 0 },
                { /* szName */ "image_orientation",  /* szValue */ lpszOrientation },
                { /* szName */ "image_compression",  /* szValue */ lpszCompression },
                { /* szName */ "image_imagetype",  /* szValue */ lpszImageType },
                { /* szName */ "image_interlace",  /* szValue */ lpszInterlaceType },
                { /* szName */ "image_colorspace",  /* szValue */ lpszColorspace },
                { /* szName */ "image_dither",  /* szValue */ "n/a" }, //(lpImageInfo->dither == MagickTrue ? "Yes" : "No" ) },
                { /* szName */ "image_width",  /* szValue */ szWidth },
                { /* szName */ "image_height",  /* szValue */ szHeight },
                { /* szName */ "image_x_resolution",  /* szValue */ szXResolution },
                { /* szName */ "image_y_resolution",  /* szValue */ szYResolution },
                { /* szName */ "image_resolutiontype",  /* szValue */ lpszResolution },
                { /* szName */ "image_depth",  /* szValue */ szDepth },
                { /* szName */ "image_colors",  /* szValue */ szColors },
                { /* szName */ "image_quality",  /* szValue */ szQuality },
                { /* szName */ 0, /* szValue */0 } // Terminating row
            };

        // Parse image preview template
        template_iparse( req, TEMPLATE_VIEW_IMAGE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, imageTemplate );

        // Cleanup
        magick_wand = DestroyMagickWand( magick_wand );
    }
#endif // HAVE_IMAGEMAGICK
    else
    {
        // Parse image preview template
        template_iparse( req, TEMPLATE_VIEW_FILE, req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, fileTemplate );
    }

    // Free temp buffer
    if( szTempBuffer != 0 )
        FREE( szTempBuffer )

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, stFile.st_mtime );
}

// #############################################################################

void view_advanced_parse( struct NNCMS_THREAD_INFO *req,
    char *lpszSrc, size_t nSrcLen,
    char *lpszDst, size_t nDstLen )
{
    // Examples:
    //  {tag}blabla{/tag}
    //  {tag}{/tag}
    //  {tag attr="value"}{/tag}
    //  {tag attr=value /}

    // Backup offsets
    char *lpszTempSrc = lpszSrc;
    char *lpszTempDst = lpszDst;

    // Prepare output buffer
    //memset( lpszDst, 0, nDstLen );

    // Specify values for file preview template
    struct NNCMS_TEMPLATE_TAGS testTemplate[] =
        {
            { /* szName */ "homeURL",  /* szValue */ /*"http://valka.dyndns.org/"*/ homeURL },
            { /* szName */ "xxx",  /* szValue */ "yyy" /*homeURL*/ },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

    //char *lpszResult = template_advanced_parse( req, &lpszTempSrc, 0, testTemplate, 0 );
    //strlcpy( lpszDst, lpszResult, nDstLen );
    //FREE( lpszResult );
    strlcpy( lpszDst, lpszSrc, nDstLen );
}

// #############################################################################

// NOT A PAGE!
size_t view_math( struct NNCMS_THREAD_INFO *req, char *lpszDst, size_t nDstLen, char *lpszExpression )
{
    // Generate sha512 from lpszExpression
    unsigned char digest[SHA512_DIGEST_SIZE]; unsigned int i;
    unsigned char hash[2 * SHA512_DIGEST_SIZE + 1];
    sha512( lpszExpression, strlen( (char *) lpszExpression ), digest );
    hash[2 * SHA512_DIGEST_SIZE] = '\0';
    for( i = 0; i < (int) SHA512_DIGEST_SIZE ; i++ )
        sprintf( (char *) hash + (2 * i), "%02x", digest[i] );

    // Generate path to cached image
    char szPath[NNCMS_PATH_LEN_MAX];
    snprintf( szPath, NNCMS_PATH_LEN_MAX, "/cache/math/%s.png", hash );

    // Get file info
    struct NNCMS_FILE_INFO fileInfo;
    file_get_info( &fileInfo, szPath );

    // Generate image
    if( fileInfo.bExists == false )
    {
/*
        // Parse expression with MathGL
        struct mglGraph *lpGraph = mgl_create_graph_zb( 640, 480 );
        //mgl_puts_ext( lpGraph, 0.0, 0.0, 0.0, lpszExpression, "rC", 0, 't' );
        mgl_puts( lpGraph, 0.0, 0.0, 0.0, lpszExpression );
        unsigned char *lpPixels = (unsigned char *) mgl_get_rgba( lpGraph );

        // Put image in to imagemagik
        extern ExceptionInfo *lpException;
        ImageInfo *lpImageInfo = CloneImageInfo( (ImageInfo *) 0 );
        Image *lpImage = ConstituteImage( 640, 480, "RGBA", CharPixel, lpPixels, lpException );
        if( lpException->severity != UndefinedException )
            CatchException( lpException );
        if( lpImage == (Image *) 0 )
        {
            log_printf( req, LOG_ERROR, "View::Math: Could not constitute image" );
            return 0;
        }
        Image *lpTrim = TrimImage( lpImage, lpException );
        if( lpException->severity != UndefinedException )
            CatchException( lpException );

        // Output PNG image
        (void) strcpy( lpTrim->filename, lpfileInfo.lpszAbsFullPath );
        if( WriteImage( lpImageInfo, lpTrim ) == MagickFalse )
        {
            log_printf( req, LOG_ERROR, "View::Math: Could not write PNG image to file" );
            return 0;
        }

        // Cleanup
        //mgl_write_png( lpGraph, lpfileInfo.lpszAbsFullPath, lpszExpression );
        DestroyImage( lpImage );
        DestroyImage( lpTrim );
        lpImageInfo = DestroyImageInfo( lpImageInfo );
        mgl_delete_graph( lpGraph );
*/
    }

    // Output
    return snprintf( lpszDst, nDstLen, "<img src=\"%s%s\" alt=\"{math}%s{/math}\" title=\"{math}%s{/math}\">", homeURL, szPath, lpszExpression, lpszExpression );
}

// #############################################################################

void view_force_cache( struct NNCMS_THREAD_INFO *req )
{
    char szTimeBuf[NNCMS_TIME_STR_LEN_MAX];
    char szExpireBuf[9 + NNCMS_TIME_STR_LEN_MAX + 1];

    // Make browser cache the image:
    //    Last-Modified: Wed, 08 Sep 2010 14:59:39 GMT
    //    Expires: Thu, 31 Dec 2037 23:55:55 GMT
    //    Cache-Control: max-age=315360000
    main_add_header( req, "Cache-Control: max-age=315360000" );
    main_format_time_string( req, szTimeBuf, time( 0 ) + 315360000 );
    snprintf( szExpireBuf, sizeof(szExpireBuf), "Expires: %s", szTimeBuf );
    main_add_header( req, szExpireBuf );
}

// #############################################################################

void view_iw_exception( struct NNCMS_THREAD_INFO *req, MagickWand *wand, char *lpszError, char *lpBuf )
{
    ExceptionType severity;
    char *description = MagickGetException( wand, &severity );
    (void) snprintf( req->lpszBuffer, NNCMS_PAGE_SIZE_MAX, "%s: %s %s %lu %s\n", lpszError, GetMagickModule(), description );
    description = (char *) MagickRelinquishMemory( description );
    wand = DestroyMagickWand( wand );
}

// #############################################################################

void view_thumbnail( struct NNCMS_THREAD_INFO *req )
{
    // Page header
    char *szHeader = "Image thumbnail";

    // Specify values for template
    struct NNCMS_TEMPLATE_TAGS frameTemplate[] =
        {
            { /* szName */ "header", /* szValue */ szHeader },
            { /* szName */ "content",  /* szValue */ "ok" },
            { /* szName */ "icon",  /* szValue */ "images/mimetypes/application-octet-stream.png" },
            { /* szName */ "homeURL",  /* szValue */ homeURL },
            { /* szName */ 0, /* szValue */0 } // Terminating row
        };

#ifndef HAVE_IMAGEMAGICK
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = PACKAGE_STRING " is compiled without ImageMagick support";
        goto output;

#endif // !HAVE_IMAGEMAGICK
#ifdef HAVE_IMAGEMAGICK

    // Get filename to be thumbnailed
    struct NNCMS_VARIABLE *httpVarFile = main_get_variable( req, "file" );
    if( httpVarFile == 0 )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "No file";
        goto output;
    }
    struct NNCMS_FILE_INFO fileInfo;
    file_get_info( &fileInfo, httpVarFile->lpszValue );
    if( fileInfo.bExists == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "File not found";
        goto output;
    }

    // Set icon on template
    frameTemplate[2].szValue = fileInfo.lpszIcon;

    // Check user permission to view files
    if( file_check_perm( req, &fileInfo, "thumbnail" ) == false )
    {
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = "Not allowed";
        log_printf( req, LOG_NOTICE, "View::Thumbnail: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Get thumbnail size
    unsigned long int ulNewWidth = THUMBNAIL_WIDTH_MAX, ulNewHeight = THUMBNAIL_HEIGTH_MAX;
    struct NNCMS_VARIABLE *httpVarWidth = main_get_variable( req, "width" );
    struct NNCMS_VARIABLE *httpVarHeight = main_get_variable( req, "height" );
    if( httpVarWidth != 0 && httpVarHeight != 0 )
    {
        // Convert string to integer
        ulNewWidth = atol( httpVarWidth->lpszValue );
        ulNewHeight = atol( httpVarHeight->lpszValue );

        // Limit
             if( ulNewWidth  > THUMBNAIL_WIDTH_MAX )  { ulNewWidth = THUMBNAIL_WIDTH_MAX; }
        else if( ulNewWidth  < THUMBNAIL_WIDTH_MIN )  { ulNewWidth = THUMBNAIL_WIDTH_MIN; }
             if( ulNewHeight > THUMBNAIL_HEIGTH_MAX ) { ulNewHeight = THUMBNAIL_HEIGTH_MAX; }
        else if( ulNewHeight < THUMBNAIL_HEIGTH_MIN ) { ulNewHeight = THUMBNAIL_HEIGTH_MIN; }
    }

    // Get pseudohash of this thumbnail
    char szPseudoHash[NNCMS_PATH_LEN_MAX];
    snprintf( szPseudoHash, NNCMS_PATH_LEN_MAX, "%s_%ldx%ld.%s", fileInfo.lpszRelFullPath, ulNewWidth, ulNewHeight, fileInfo.lpszExtension );

    //
    // Find image thumbnail in cache
    //
    struct NNCMS_CACHE *lpCache = cache_find( req, szPseudoHash );
    if( lpCache != 0 )
    {
        // Output cached image
        main_set_content_type( req, fileInfo.lpszMimeType );
        view_force_cache( req );
        main_send_headers( req, lpCache->nSize, lpCache->fileTimeStamp );
        FCGX_PutStr( lpCache->lpData, lpCache->nSize, req->fcgi_request->out );

        // Free mem
        FREE( lpCache );
        return;
    }

    //
    // Initialize the image info structure and read an image.
    //
    MagickWand *magick_wand = NewMagickWand( );
    MagickBooleanType result = MagickReadImage( magick_wand, fileInfo.lpszAbsFullPath );
    if( result == MagickFalse )
    {
        view_iw_exception( req, magick_wand, "Image not loaded", req->lpszBuffer );
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = req->lpszBuffer;
        log_printf( req, LOG_ERROR, "View::Thumbnail: %s", frameTemplate[1].szValue );
        goto output;
    }

    //
    // Calculate new width and height for thumblnail, keeping the
    // same aspect ratio.
    //
    double dWidth = (double) MagickGetImageWidth( magick_wand );
    double dHeight = (double) MagickGetImageHeight( magick_wand );
    double dRatio = dWidth / dHeight;
    if( dRatio > 4.0f / 3.0f )
        ulNewHeight = (unsigned long int) (dHeight * ((double) ulNewWidth / dWidth));
    else
        ulNewWidth = (unsigned long int) (dWidth * ((double) ulNewHeight / dHeight));

    //
    // Set format to JPG, if image is not PNG, but keep PNG for PNG, or
    // transparent background will look bad, when resized
    //
    main_set_content_type( req, fileInfo.lpszMimeType );
    if( strcmp( fileInfo.lpszExtension, "png" ) != 0 )
    {
        MagickSetFormat( magick_wand, "JPEG" );
        main_set_content_type( req, "image/jpeg" );
    }

    //
    //  Turn the images into a thumbnail sequence.
    //
    MagickResetIterator( magick_wand );
    MagickNextImage( magick_wand );
    MagickResizeImage( magick_wand, ulNewWidth, ulNewHeight, LanczosFilter, 1.0 );

    size_t nLenght;
    unsigned char *lpBlob = MagickGetImageBlob( magick_wand, &nLenght );
    if( lpBlob == 0 || nLenght == 0 )
    {
        view_iw_exception( req, magick_wand, "Unable to convert image to blob", req->lpszBuffer );
        frameTemplate[0].szValue = "Error";
        frameTemplate[1].szValue = req->lpszBuffer;
        log_printf( req, LOG_ERROR, "View::Thumbnail: %s", frameTemplate[1].szValue );
        goto output;
    }

    // Memory garbage collector workaround in DEBUG mode
#ifdef DEBUG
    //unsigned char *lpTemp = MALLOC( nLenght );
    //memcpy( lpTemp, lpBlob, nLenght );
    //free( lpBlob );
    //lpBlob = lpTemp;
#endif // DEBUG

    // Output image
    //main_set_content_type( req, "text/plain" );
    view_force_cache( req );
    main_send_headers( req, nLenght, 0 );
    FCGX_PutStr( lpBlob, nLenght, req->fcgi_request->out );

    // Store in cache
    cache_store( req, szPseudoHash, lpBlob, nLenght );

    // Cleanup
    MagickRelinquishMemory( lpBlob );
    magick_wand = DestroyMagickWand( magick_wand );
    return;

#endif // HAVE_IMAGEMAGICK

output:
    // Generate the page
    template_iparse( req, TEMPLATE_FRAME, req->lpszFrame, NNCMS_PAGE_SIZE_MAX, frameTemplate );

    // Send generated html to client
    main_output( req, szHeader, req->lpszFrame, 0 );
}

// #############################################################################

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <time.h>
//#include <magick/MagickCore.h>

//int main(int argc,char **argv)
//{
  //ExceptionInfo
    //*exception;

  //Image
    //*image,
    //*images,
    //*resize_image,
    //*thumbnails;

  //ImageInfo
    //*image_info;

  //if (argc != 3)
    //{
      //(void) fprintf(stdout,"Usage: %s image thumbnail\n",argv[0]);
      //exit(0);
    //}
  ///*
    //Initialize the image info structure and read an image.
  //*/
  //MagickCoreGenesis(*argv,MagickTrue);
  //exception=AcquireExceptionInfo();
  //image_info=CloneImageInfo((ImageInfo *) NULL);
  //(void) strcpy(image_info->filename,argv[1]);
  //images=ReadImage(image_info,exception);
  //if (exception->severity != UndefinedException)
    //CatchException(exception);
  //if (images == (Image *) NULL)
    //exit(1);
  ///*
    //Convert the image to a thumbnail.
  //*/
  //thumbnails=NewImageList();
  //while ((image=RemoveFirstImageFromList(&images)) != (Image *) NULL)
  //{
    //resize_image=ResizeImage(image,106,80,LanczosFilter,1.0,exception);
    //if (resize_image == (Image *) NULL)
      //MagickError(exception->severity,exception->reason,exception->description);
    //(void) AppendImageToList(&thumbnails,resize_image);
    //DestroyImage(image);
  //}
  ///*
    //Write the image thumbnail.
  //*/
  //(void) strcpy(thumbnails->filename,argv[2]);
  //WriteImage(image_info,thumbnails);
  ///*
    //Destroy the image thumbnail and exit.
  //*/
  //thumbnails=DestroyImageList(thumbnails);
  //image_info=DestroyImageInfo(image_info);
  //exception=DestroyExceptionInfo(exception);
  //MagickCoreTerminus();
  //return(0);
//}

// #############################################################################

void hex_dump( unsigned char *dst, unsigned int dst_size,
               unsigned char *src, unsigned int src_size )
{
    unsigned int len, i, j, c, bufpos = 0;

    if( (dst_size - 7) < bufpos ) { return; }
    bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "<pre>\n" );

    for( i = 0; i < src_size; i += 16 )
    {
        len = src_size - i;
        if( len > 16 )
            len = 16;

        // Print address
        if( (dst_size - 17 ) < bufpos) { return; }
        bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "<b>%08x</b> ", i );

        // Print 16 bytes
        for( j = 0; j < 16; j++ )
        {
            if( (dst_size - 4 ) < bufpos) { return; }
            if( j < len )
                bufpos += snprintf( &dst[bufpos], dst_size - bufpos, " %02x", src[i + j] );
            else
                bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "   " );
        }
        if( (dst_size - 2 ) < bufpos) { return; }
        bufpos += snprintf( &dst[bufpos], dst_size - bufpos, " " );

        // Print character
        for( j = 0; j < len; j++ )
        {
            c = src[i + j];

            if( c < ' ' || c > '~' )
                c = '.';

            if( (dst_size - 6 ) < bufpos ) { return; }

            if( c == '<' )
                bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "&lt;" );
            else if( c == '>' )
                bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "&gt;" );
            else if( c == '&' )
                bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "&amp;" );
            else
                bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "%c", c );
        }
        if( (dst_size - 2 ) < bufpos) { return; }
        bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "\n" );
    }

    if( (dst_size - 8 ) < bufpos) { return; }
    bufpos += snprintf( &dst[bufpos], dst_size - bufpos, "</pre>\n" );
}

// #############################################################################
