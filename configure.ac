#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ourvle], [0.1])
AC_CONFIG_SRCDIR([src/ourvle.c])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_AUX_DIR(build-aux)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h netdb.h stddef.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 memset mkdir socket])

AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT
