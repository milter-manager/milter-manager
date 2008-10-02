#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

m4_define([milter_manager_version_major], [0])
m4_define([milter_manager_version_minor], [0])
m4_define([milter_manager_version_micro], [1])
m4_define([milter_manager_version],
          [milter_manager_version_major.milter_manager_version_minor.milter_manager_version_micro])

AC_INIT(milter-manager, [milter_manager_version], kou@cozmixng.org)
AC_CONFIG_AUX_DIR([config])
# AC_CONFIG_MACRO_DIR([m4macros])

AC_CONFIG_SRCDIR([milter-client/milter-client.h])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

PACKAGE_TITLE="milter manager"
AC_SUBST(PACKAGE_TITLE)

MILTER_MANAGER_VERSION_MAJOR=milter_manager_version_major
MILTER_MANAGER_VERSION_MINOR=milter_manager_version_minor
MILTER_MANAGER_VERSION_MICRO=milter_manager_version_micro
MILTER_MANAGER_VERSION=milter_manager_version
AC_SUBST(MILTER_MANAGER_VERSION_MAJOR)
AC_SUBST(MILTER_MANAGER_VERSION_MINOR)
AC_SUBST(MILTER_MANAGER_VERSION_MICRO)
AC_SUBST(MILTER_MANAGER_VERSION)

AC_ENABLE_SHARED(yes)
AC_ENABLE_STATIC(no)

AC_PROG_LIBTOOL

LT_CURRENT=1
LT_REVISION=0
LT_AGE=1
LT_VERSION_INFO="$LT_CURRENT:$LT_REVISION:$LT_AGE"
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_VERSION_INFO)

LT_CURRENT_MINUS_AGE=`expr $LT_CURRENT - $LT_AGE`
AC_SUBST(LT_CURRENT_MINUS_AGE)

LIBTOOL_EXPORT_OPTIONS='-export-symbols-regex "^[[^_]].*"'
AC_SUBST(LIBTOOL_EXPORT_OPTIONS)


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wmissing-declarations[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-declarations" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wmissing-prototypes[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wpointer-arith[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wpointer-arith" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wcast-align[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wcast-align" ;;
  esac
fi

dnl **************************************************************
dnl Check for headers.
dnl **************************************************************
AC_CHECK_HEADER(sys/socket.h,
                [AC_DEFINE(HAVE_SYS_SOCKET_H, 1,
                           [Define to 1 if you have <sys/socket.h>.])])
AC_CHECK_HEADER(sys/un.h,
                [AC_DEFINE(HAVE_SYS_UN_H, 1,
                           [Define to 1 if you have <sys/un.h>.])])

dnl **************************************************************
dnl Check for GLib.
dnl **************************************************************
GLIB_REQUIRED=2.12.3
AC_SUBST(GLIB_REQUIRED)
AM_PATH_GLIB_2_0($GLIB_REQUIRED,
                 [],
                 [AC_MSG_ERROR([GLib >= $GLIB_REQUIRED required.])],
                 [gobject gthread])

dnl **************************************************************
dnl for Cutter
dnl **************************************************************
m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER(>= 1.0.5)
],
[ac_cv_use_cutter="no"])

AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])
if test "$ac_cv_use_cutter" != "no"; then
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])
fi

m4_ifdef([AC_CHECK_COVERAGE], [AC_CHECK_COVERAGE])
GENHTML_OPTIONS="--title 'milter manager Code Coverage'"

dnl **************************************************************
dnl Configure for maintainer mode.
dnl **************************************************************

AM_MAINTAINER_MODE
AM_CONDITIONAL([MAINTAINER_MODE],
               [test "x$USE_MAINTAINER_MODE" = "xyes"])

dnl **************************************************************
dnl Checks for gtk-doc
dnl **************************************************************

GTK_DOC_CHECK([1.8])

dnl **************************************************************
dnl Check for gettext.
dnl **************************************************************

AC_PROG_INTLTOOL([0.35.5])

AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=milter-manager
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package])

dnl **************************************************************
dnl Check for SF.net.
dnl **************************************************************

SF_PROJECT_NAME="milter manager"
AC_SUBST(SF_PROJECT_NAME)

AC_ARG_WITH([sf-user],
            AS_HELP_STRING([--with-sf-user=NAME],
                           [Use NAME to access sf.net]),
            [SF_USER="$withval"])
AC_SUBST(SF_USER)

SF_USER_OPTION=""
if test -n "$SF_USER"; then
   SF_USER_OPTION="--username $SF_USER"
fi
AC_SUBST(SF_USER_OPTION)

SF_HOST="shell.sourceforge.net"
if test -n "$SF_USER"; then
   SF_HOST="$SF_USER@$SF_HOST"
fi
AC_SUBST(SF_HOST)

SF_HTDOCS_PATH="/home/groups/m/mi/milter_manager/htdocs"
AC_SUBST(SF_HTDOCS_PATH)

SF_HTDOCS="$SF_HOST:$SF_HTDOCS_PATH"
AC_SUBST(SF_HTDOCS)

SF_REPOS="https://milter-manager.svn.sourceforge.net/svnroot/milter-manager/milter-manager"
AC_SUBST(SF_REPOS)

MILTER_MANAGER_CFLAGS="$GLIB_CFLAGS -DG_DISABLE_DEPRECATED"
AC_SUBST(MILTER_MANAGER_CFLAGS)

AC_CONFIG_FILES([Makefile
		 milter-core.pc
		 milter-core/Makefile
		 milter-client.pc
		 milter-client/Makefile
		 milter-server.pc
		 milter-server/Makefile
		 libmilter.pc
		 libmilter/Makefile
		 test/Makefile
		 test/core/Makefile
		 test/client/Makefile
		 test/libmilter/Makefile
		 po/Makefile.in])

AC_OUTPUT
