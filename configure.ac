#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([planc], [0.05])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_HOST

AC_MSG_CHECKING([whether to build with mingw for a Windows host])
AS_CASE([$host_os],
        [*mingw*], [windows_host=yes],
        [windows_host=no])
AC_MSG_RESULT([$windows_host])
AM_CONDITIONAL([WINDOWS_HOST], [test "x$windows_host" = "xyes"])

m4_include([m4/ax_check_enable_debug.m4])

AX_CHECK_ENABLE_DEBUG([no])

# Checks for programs.

if test -z $CFLAGS; then
    CFLAGS='-O2'
fi

AC_PROG_CC_C99
AC_PROG_RANLIB
AM_PROG_AR
AC_SYS_LARGEFILE

# Checks for libraries.
LT_INIT
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memcpy strchr strrchr strstr strtol strtoul strcasestr])

PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.20)
PKG_CHECK_MODULES(WK4, webkit2gtk-4.0 >= 2.18)
PKG_CHECK_MODULES(SQ3, sqlite3 >= 3.00)
AC_CHECK_LIB([webkit2gtk-4.0], [webkit_settings_set_enable_encrypted_media], [AC_DEFINE([WK_MEDIA_ENCRYPT], [1], [WebKitGtk has encrypted_media setting])],[])

GLIB_GSETTINGS

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h string.h sys/socket.h unistd.h netdb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor round inet_ntoa memmove strchr strrchr strstr strtol strtoul])

# Conditionals
AC_ARG_ENABLE([bwrap],
	AS_HELP_STRING([--disable-bwrap], [Disable bubblewrap sandboxing]))
AM_CONDITIONAL([BWRAP], [test "x$enable_bwrap" = "xyes"])
AS_IF([test "x$enable_bwrap" != "xno"],
[
	AC_DEFINE([PLANC_FEATURE_BWRAP], [1], [Build with bubblewrap sandboxing])
])
AC_ARG_ENABLE([dmenu],
	AS_HELP_STRING([--enable-dmenu], [Enable dynamic traditional menus]))
AM_CONDITIONAL([DMENU], [test "x$enable_dmenu" = "xyes"])
AS_IF([test "x$enable_dmenu" = "xyes"],
[
	AC_DEFINE([PLANC_FEATURE_DMENU], [1], [Build with dynamic traditional menus])
])
AC_ARG_ENABLE([dpolc],
	AS_HELP_STRING([--disable-dpolc], [Disable domain policy webkit extension]))
AM_CONDITIONAL([DPOLC], [test "x$enable_dpolc" != "xno"])
AS_IF([test "x$enable_dpolc" != "xno"],
[
	AC_DEFINE([PLANC_FEATURE_DPOLC], [1], [Build with domain policy support])
])
AC_ARG_ENABLE([gnome],
	AS_HELP_STRING([--enable-gnome], [Build Gnome3 features (GMenu)]))
AM_CONDITIONAL([GNOME], [test "x$enable_gnome" = "xyes"])
AS_IF([test "x$enable_gnome" = "xyes"],
[
	AC_DEFINE([PLANC_FEATURE_GNOME], [1], [Build with Gnome3 oriented features like Gmenu])
])
AC_OUTPUT
