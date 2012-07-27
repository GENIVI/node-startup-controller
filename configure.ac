dnl ***************************
dnl *** Version information ***
dnl ***************************
m4_define([boot_manager_version_api_major], [1])
m4_define([boot_manager_version_major], [0])
m4_define([boot_manager_version_minor], [1])
m4_define([boot_manager_version_micro], [0])
m4_define([boot_manager_version], [boot_manager_version_major().boot_manager_version_minor().boot_manager_version_micro()])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2012 GENIVI <genivi-dev@mail.genivi.org>])
AC_INIT([boot-manager], [boot_manager_version], [genivi-dev@mail.genivi.org])
AC_PREREQ([2.50])
AC_CANONICAL_TARGET()

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([ACLOCAL_AMFLAGS], ["$ACLOCAL_FLAGS -I m4"])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl **********************************
dnl *** Subst boot-manager version ***
dnl **********************************
BOOT_MANAGER_VERSION_MAJOR=boot_manager_version_major()
BOOT_MANAGER_VERSION_MINOR=boot_manager_version_minor()
BOOT_MANAGER_VERSION_MICRO=boot_manager_version_micro()
BOOT_MANAGER_VERSION_API_MAJOR=boot_manager_version_api_major()
BOOT_MANAGER_VERSION_API=$BOOT_MANAGER_VERSION_API_MAJOR
AC_SUBST([BOOT_MANAGER_VERSION_MAJOR])
AC_SUBST([BOOT_MANAGER_VERSION_MINOR])
AC_SUBST([BOOT_MANAGER_VERSION_MICRO])
AC_SUBST([BOOT_MANAGER_VERSION_API_MAJOR])
AC_SUBST([BOOT_MANAGER_VERSION_API])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AM_PROG_AS()
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_INSTALL()

dnl ***********************
dnl *** Prepare libtool ***
dnl ***********************
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl ***************************************
dnl *** Check for standard header files ***
dnl ***************************************
AC_HEADER_STDC()
AC_CHECK_HEADERS([stdlib.h])

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
AC_CHECK_FUNCS([])

dnl *************************
dnl *** Check for GTK-Doc ***
dnl *************************
GTK_DOC_CHECK(1.9)

dnl *******************************
dnl *** Check for gdbus-codegen ***
dnl *******************************
AC_ARG_VAR([GDBUS_CODEGEN], [Tool to generate C bindings from XML D-Bus interface definitions])
AC_PATH_PROG([GDBUS_CODEGEN], [gdbus-codegen], [no])
if test x"$GDBUS_CODEGEN" = x"no"; then
  AC_MSG_ERROR([could not find gdbus-codegen in \$PATH. You can run
  ./configure GDBUS_CODEGEN=/path/to/gdbus-codegen to define 
  a custom location for it.])
fi

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.30.0])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.30.0])
PKG_CHECK_MODULES([GIO_UNIX], [gio-unix-2.0 >= 2.30.0])
PKG_CHECK_MODULES([SYSTEMD_DAEMON], [libsystemd-daemon >= 37])
PKG_CHECK_MODULES([DLT], [automotive-dlt >= 2.2.0])

dnl *********************************************
dnl *** Include GLib/GSettings specific stuff ***
dnl *********************************************
GLIB_GSETTINGS

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug@:@=no|yes@:>@],
                             [Build with debugging support @<:@default=m4_default([$1], [no])@:>@]),
              [enable_debug=$enableval], [enable_debug=m4_default([$1], [no])])
AC_MSG_CHECKING([whether to build with debugging support])
if test x"$enable_debug" = x"yes"; then
  AC_DEFINE([DEBUG], [1], [Define for debugging support])

  CPPFLAGS="$CPPFLAGS -D_FORTIFY_SOURCE=2"
  CFLAGS="$CFLAGS -Werror -Wall -Wextra \
            -Wno-missing-field-initializers \
            -Wno-unused-parameter -Wold-style-definition \
            -Wdeclaration-after-statement \
            -Wmissing-declarations \
            -Wmissing-noreturn -Wpointer-arith \
            -Wcast-align -Wformat-security \
            -Winit-self -Wmissing-include-dirs -Wundef \
            -Wmissing-format-attribute -Wnested-externs \
            -fstack-protector"

  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

dnl ***************************************************
dnl *** Configure option for prioritising LUC types ***
dnl ***************************************************
AC_ARG_WITH([prioritised-luc-types],
            [AS_HELP_STRING([--with-prioritised-luc-types=LIST],
                            [Comma-separated list of LUC types to be prioritised during start-up])],
            [with_prioritised_luc_types=$withval], [with_prioritised_luc_types=])
AC_DEFINE_UNQUOTED([PRIORITISED_LUC_TYPES],
                   ["$with_prioritised_luc_types"],
                   [Prioritised LUC types])

AC_OUTPUT([
Makefile
common/Makefile
boot-manager/busconf/Makefile
boot-manager/Makefile
legacy-app-handler/Makefile
nsm-dummy/busconf/Makefile
nsm-dummy/Makefile
tests/Makefile
tests/boot-manager/Makefile
tests/legacy-app-handler/Makefile
])
