#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([browser], [0.01])
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
AC_PROG_CC_C99
AC_PROG_RANLIB
AC_SYS_LARGEFILE

# Checks for libraries.
LT_INIT
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memcpy strchr strrchr strstr strtol strtoul])

#PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= 2.24)
PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.20)
PKG_CHECK_MODULES(WK4, webkit2gtk-4.0 >= 2.18)
#PKG_CHECK_MODULES(WK3G3, webkit2gtk-3.0 >= 2.4)
#PKG_CHECK_MODULES(WK3G2, webkitgtk-3.0 >= 2)



# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h string.h sys/socket.h unistd.h netdb.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor inet_ntoa memmove strchr strrchr strstr strtol strtoul])

# Condictionals
AC_OUTPUT
