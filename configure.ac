# Process this file with autoconf to produce a configure script

AC_PREREQ(2.69)
AC_INIT([libbulk], [0.1.0], [xrdp-devel@googlegroups.com])
AC_DEFINE([VERSION_YEAR], 2026, [Copyright year])
AC_CONFIG_HEADERS(config_ac.h:config_ac-h.in)
AM_INIT_AUTOMAKE([1.7.2 foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
LT_INIT

# Use silent rules by default if supported by Automake
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tests/Makefile
])

AC_OUTPUT

