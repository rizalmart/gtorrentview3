#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# 2004 (c) Alejandro Claro
#

AC_PREREQ([2.72])
AC_INIT([GTorrentViewer],[3.0],[ap0lly0n@users.sourceforge.net])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/utilities.c])
AM_CONFIG_HEADER(config.h)

PACKAGE=$PACKAGE_NAME
VERSION=$PACKAGE_VERSION

# Checks for programs.
AC_PROG_CC

# define desktop and pixmap system directories in the configure script
AC_ARG_WITH([pixmapsdir],
 AS_HELP_STRING([--with-pixmapsdir=DIR],[system's pixmaps files [[DATADIR/pixmaps]]]),
 [
 	case "$withval" in
  	n|no|y|ye|yes) 
      PIXMAPDIR=$datadir/pixmaps
      ;; 
 		*)
 			PIXMAPDIR=$withval
 			;;
 	esac
 ], PIXMAPDIR=$datadir/pixmaps)
  
AC_SUBST(PIXMAPDIR)

AC_ARG_WITH([desktopdir],
 AS_HELP_STRING([--with-desktopdir=DIR],[.desktop files [[DATADIR/applications]]]),
 [
 	case "$withval" in
  	n|no|y|ye|yes) 
      DESKTOPDIR=$datadir/applications
      ;; 
 		*)
 			DESKTOPDIR=$withval
 			;;
 	esac
 ], DESKTOPDIR=$datadir/applications)
  
AC_SUBST(DESKTOPDIR)

#
# Checks for libraries.
#

# check for libcurl.
AC_ARG_WITH([curl-config],
 AS_HELP_STRING([--with-curl-config=PATH],[path to curl-config (e.g. /opt/bin)]),
 [
 	case "$withval" in
  	n|no)
      AC_MSG_NOTICE([$PACKAGE_NAME needs libcurl. It can't be disable.])
      ;;
    y|ye|yes) 
      ;; 
 		*)
 			curl_dir=$withval
 			;;
 	esac
 ])
  
AC_PATH_PROG([CURL_CONFIG], [curl-config], [no],
   [$curl_dir /bin /usr/bin /usr/local/bin $PATH $prefix/bin])
  
if test "$CURL_CONFIG" != "no" ; then
	CURL_LIBS="`$CURL_CONFIG --libs`"
	CURL_CFLAGS="`$CURL_CONFIG --cflags`"
else
 	echo "*** Can't find curl-config. Make sure you have libcurl installed."
 	echo "*** If you do, try using --with-libcurl=PREFIX."
 	exit 1
fi

AC_SUBST(CURL_LIBS)
AC_SUBST(CURL_CFLAGS)

# check for Glib and GTK+
AM_PATH_GTK_3_0(3.0.0,,AC_MSG_ERROR($(PACKAGE_NAME) $(PACKAGE_VERSION) needs GTK+ 3.0), gthread)
#PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0])

#check for glibs gettext
IT_PROG_INTLTOOL([0.23])

ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [gettext domain])

# Checks for header files.
m4_warn([obsolete],
[The preprocessor macro 'STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  C89 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# check for large file support
AC_TYPE_OFF_T
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([memchr memmove memset modf])

# ceil() requires libm
AC_CHECK_LIB([m], [ceil])

AC_CONFIG_FILES([Makefile src/Makefile pixmaps/Makefile data/Makefile po/Makefile.in])
AC_OUTPUT
