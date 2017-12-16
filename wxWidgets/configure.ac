#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Uncomplicated Port Forward], [1.0], [nobodys@here.null])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIR([m4])
AX_CHECK_ENABLE_DEBUG([no])
AC_CANONICAL_HOST

AC_MSG_CHECKING([whether to build with mingw for a Windows host])
AS_CASE([$host_os],
        [*mingw*], [windows_host=yes],
        [windows_host=no])
AC_MSG_RESULT([$windows_host])
AM_CONDITIONAL([WINDOWS_HOST], [test "x$windows_host" = "xyes"])

AS_IF([test "x$windows_host" = "xyes"],
	[WIN_LIBS="-lws2_32 -liphlpapi "],
	[])

AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])

# Checks for programs.
#if test -z $CXXFLAGS; then
	#CXXFLAGS='O2'
#fi
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netinet/in.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AM_OPTIONS_WXCONFIG
reqwx=3.0.0
AM_PATH_WXCONFIG($reqwx, wxWin=1,wxWin=0,[core,base,stc,webview],)
if test "$wxWin" != 1; then
	AC_MSG_ERROR([
		wxWidgets must be installed on your system.
 
		Please check that wx-config is in path, the directory
		where wxWidgets libraries are installed (returned by
		'wx-config --libs' or 'wx-config --static --libs' command)
		is in LD_LIBRARY_PATH or equivalent variable and
		wxWidgets version is $reqwx or above.
		])
fi

CPPFLAGS="$CPPFLAGS $WX_CPPFLAGS"
CXXFLAGS="$CXXFLAGS $WX_CXXFLAGS_ONLY"
CFLAGS="$CFLAGS $WX_CFLAGS_ONLY"
LIBS="$LIBS $WX_LIBS $WIN_LIBS"

AC_OUTPUT
