AC_PREREQ([2.69])
AC_INIT([autotools-memleak], [0.1], [you@example.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/memleak.c])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AC_ARG_ENABLE([asan],
  AS_HELP_STRING([--enable-asan],
    [Build with AddressSanitizer for debugging (optional)]),
  [enable_asan=$enableval],
  [enable_asan=no])
if test "x$enable_asan" = "xyes"; then
  AC_MSG_NOTICE([Building with AddressSanitizer flags])
  CFLAGS="$CFLAGS -fsanitize=address -g -O1"
  LDFLAGS="$LDFLAGS -fsanitize=address"
fi

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
