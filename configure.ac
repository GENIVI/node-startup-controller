dnl ***************************
dnl *** Version information ***
dnl ***************************
m4_define([boot_manager_version_api_major], [1])
m4_define([boot_manager_version_major], [0])
m4_define([boot_manager_version_minor], [0])
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

dnl *********************************************
dnl *** Include GLib/GSettings specific stuff ***
dnl *********************************************
GLIB_GSETTINGS

AC_OUTPUT([
Makefile
luc-handler/Makefile
luc-handler/org.genivi.LUCHandler1.gschema.xml
])
