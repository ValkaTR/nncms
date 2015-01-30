########################################################################
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
########################################################################

AC_INIT(nncms, 0.4, valkur@gmail.com)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror])

########################################################################
# Checks for programs.
########################################################################
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

########################################################################
# Checks for header files.
########################################################################
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([pwd.h getopt.h sys/wait.h sys/un.h])

AC_CHECK_HEADER(libgen.h, [
    AC_DEFINE([HAVE_LIBGEN_H], [1], [Define to 1 if you have the <libgen.h> header file.])
    have_libgen=yes
    ], )

########################################################################
# Checks for typedefs, structures, and compiler characteristics.
########################################################################
AC_CHECK_TYPES(socklen_t,,, [#include <sys/types.h>
#include <sys/socket.h>])

########################################################################
# ImageMagick
########################################################################
AC_ARG_WITH(
	[imagemagick],
	[AC_HELP_STRING([--without-imagemagick], [disable imagemagick support])],
	[with_imagemagick=$withval],
	[with_imagemagick=$have_x])

if test "$with_imagemagick" = "yes"; then
    PKG_CHECK_MODULES(MAGICKWAND, [MagickWand >= 6.5.1], have_imagemagick=yes, have_imagemagick=no)
	if test "$have_imagemagick" = "yes"; then
		AC_DEFINE(HAVE_IMAGEMAGICK,1,Define if you have ImageMagick library)
		CFLAGS="$MAGICKWAND_CFLAGS $CFLAGS"
	fi
fi

AC_SUBST(MAGICKWAND_CFLAGS)
AC_SUBST(MAGICKWAND_LIBS)
#AC_CHECK_LIB([imagemagick], [main], [], AC_MSG_ERROR([*** Unable to link with imagemagick!]))

########################################################################
# dl
########################################################################
# FIXME: Replace `main' with a function in `-ldl':
AC_CHECK_LIB([dl], [main], [], AC_MSG_ERROR([*** Unable to link with dl!]))

########################################################################
# pthread
########################################################################
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main], [], AC_MSG_ERROR([*** Unable to link with pthread!]))

########################################################################
# expat
########################################################################
# FIXME: Replace `main' with a function in `-lpthread':
#AC_CHECK_LIB([expat], [main], [], AC_MSG_ERROR([*** Unable to link with expat!]))

########################################################################
# fcgi
########################################################################
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([fcgi], [main], [], AC_MSG_ERROR([*** Unable to link with fcgi!]))

########################################################################
# fam
########################################################################
AC_ARG_WITH(
	[fam],
	[AC_HELP_STRING([--without-fam], [disable fam support])],
	[with_fam=$withval],
	[with_fam=$have_x])

if test "$with_fam" = "yes"; then
    AC_CHECK_LIB([fam], [FAMOpen], [], AC_MSG_ERROR([*** Unable to link with fam!]))
	if test "$have_fam" = "yes"; then
		AC_DEFINE(HAVE_FAM,1,Define if you have FAM library)
	fi
fi

########################################################################
# sqlite
########################################################################
PKG_CHECK_MODULES(SQLITE3, [sqlite3 >= 3.6.14], have_sqlite3=yes, have_sqlite3=no)
if test "$have_sqlite3" != "yes"; then
	AC_MSG_ERROR([*** Unable to link with sqlite3!])
fi

AC_DEFINE(HAVE_SQLITE3,1,Define if you have sqlite3 library)
AC_SUBST(SQLITE3_CFLAGS)
AC_SUBST(SQLITE3_LIBS)
CFLAGS="$SQLITE3_CFLAGS $CFLAGS"

########################################################################
# lua
########################################################################
PKG_CHECK_MODULES(LUA, [lua >= 5.1.4], have_lua=yes, have_lua=no)
if test "$have_lua" != "yes"; then
	AC_MSG_ERROR([*** Unable to link with lua!])
fi

AC_DEFINE(HAVE_LUA,1,Define if you have lua library)
AC_SUBST(LUA_CFLAGS)
AC_SUBST(LUA_LIBS)
CFLAGS="$LUA_CFLAGS $CFLAGS"

########################################################################
# uriparser
########################################################################

#URIPARSER_MISSING="Please install uriparser 0.6.4 or later. On a Debian-based system enter 'sudo apt-get install liburiparser-dev'."
#AC_CHECK_LIB(uriparser, uriParseUriA,, AC_MSG_ERROR(${URIPARSER_MISSING}))
#AC_CHECK_HEADER(uriparser/Uri.h,, AC_MSG_ERROR(${URIPARSER_MISSING}))
#
#URIPARSER_TOO_OLD="uriparser 0.6.4 or later is required, your copy is too old."
#AC_COMPILE_IFELSE([
##include <uriparser/Uri.h>
##if (defined(URI_VER_MAJOR) && defined(URI_VER_MINOR) && defined(URI_VER_RELEASE) \
#&& ((URI_VER_MAJOR > 0) \
#|| ((URI_VER_MAJOR == 0) && (URI_VER_MINOR > 6)) \
#|| ((URI_VER_MAJOR == 0) && (URI_VER_MINOR == 6) && (URI_VER_RELEASE >= 4)) \
#))
#/* FINE */
##else
## error uriparser not recent enough
##endif
#],,AC_MSG_ERROR(${URIPARSER_TOO_OLD}))

########################################################################

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.24.1])

########################################################################

CFLAGS="-I/usr/include/fastcgi/ $CFLAGS"

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

echo \
"------------------------------------------------------------------------
nncms  is configured as follows. Please verify that this configuration
matches your expectations.

                  Option                        Value
------------------------------------------------------------------------
  ImageMagick     --with-imagemagick=$with_imagemagick	$have_imagemagick
  FAM             --with-fam=$with_fam	$have_fam

Options used to compile and link:
  Source code location  = ${srcdir}
  Compiler              = ${CC}
  Compiler flags        = ${CFLAGS}
  Host System Type      = ${host}
  Install path          = ${prefix}

  See config.h for further configuration information.
------------------------------------------------------------------------"
