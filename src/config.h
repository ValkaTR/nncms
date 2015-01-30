// #############################################################################
// Header file: config.h

// The author disclaims copyright to this source code.  In place of
// a legal notice, here is a blessing:
//
//    May you do good and not evil.
//    May you find forgiveness for yourself and forgive others.
//    May you share freely, never taking more than you give.

// #############################################################################

#ifndef __config_h__
#define __config_h__

// #############################################################################
// type and constant definitions
//

#define CMAKE_PROJECT_NAME "nncms"
#define CMAKE_INSTALL_PREFIX "/usr/local"
#define CMAKE_VERSION_STRING "0.4.dev-20120707"
#ifdef NDEBUG
    #define CMAKE_BUILD_TYPE "Release"
#else
    #define CMAKE_BUILD_TYPE "Debug"
#endif
#define CMAKE_BUILD_INFO "VER=0.4.dev-20120707 BUILD_TYPE="CMAKE_BUILD_TYPE" RUN_IN_PLACE= USE_GETTEXT= USE_SOUND= INSTALL_PREFIX=/usr/local"

#define HAVE_IMAGEMAGICK 

// #############################################################################

#endif // __config_h__

// #############################################################################
