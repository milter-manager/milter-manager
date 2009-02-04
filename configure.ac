#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

m4_define([milter_manager_version_major], [0])
m4_define([milter_manager_version_minor], [8])
m4_define([milter_manager_version_micro], [0])
m4_define([milter_manager_version],
          [milter_manager_version_major.milter_manager_version_minor.milter_manager_version_micro])

AC_INIT(milter-manager, [milter_manager_version],
        milter-manager-users-en@lists.sourceforge.net)
AC_CONFIG_AUX_DIR([config])

AC_CONFIG_SRCDIR([milter/manager.h])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

PACKAGE_TITLE="milter-manager"
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

LIBMILTER_LT_CURRENT=1
LIBMILTER_LT_REVISION=1
LIBMILTER_LT_AGE=0
LIBMILTER_LT_VERSION_INFO="$LIBMILTER_LT_CURRENT:$LIBMILTER_LT_REVISION:$LIBMILTER_LT_AGE"
AC_SUBST(LIBMILTER_LT_CURRENT)
AC_SUBST(LIBMILTER_LT_REVISION)
AC_SUBST(LIBMILTER_LT_AGE)
AC_SUBST(LIBMILTER_LT_VERSION_INFO)

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
                 [gobject gmodule gthread])

dnl **************************************************************
dnl Check for Cutter
dnl **************************************************************
m4_ifdef([AC_CHECK_GCUTTER], [
AC_CHECK_GCUTTER(>= 1.0.6)
],
[ac_cv_use_cutter="no"])

AM_CONDITIONAL([WITH_CUTTER], [test "$ac_cv_use_cutter" != "no"])
if test "$ac_cv_use_cutter" != "no"; then
  AC_DEFINE(WITH_CUTTER, 1, [Define to 1 if you use Cutter])
fi

m4_ifdef([AC_CHECK_COVERAGE], [AC_CHECK_COVERAGE])
GENHTML_OPTIONS="--title 'milter manager Code Coverage'"

dnl **************************************************************
dnl Check for Ruby
dnl **************************************************************
ruby_available="no"
AC_ARG_WITH([ruby],
            AS_HELP_STRING([--with-ruby=PATH],
                           [Ruby interpreter path (default: auto-detect)]),
            [RUBY="$withval"])

if test "$RUBY" = "no"; then
  : # ignore
elif test "$RUBY" = ""; then
  AC_PATH_PROG(RUBY, ruby, none)
  if test "$RUBY" != "none"; then
    ruby_available="yes"
  fi
else
  AC_CHECK_FILE([$RUBY],
                [ruby_available="yes"],
                [AC_MSG_WARN([$RUBY is not found. Disable milter manager.])])
fi

if test "$ruby_available" = "yes"; then
  rbconfig="$RUBY -rrbconfig -e "

  changequote(<<, >>)
  for var_name in archdir sitearchdir vendorarchdir arch \
      	       	  CFLAGS LIBRUBYARG LIBS RUBY_SO_NAME libdir; do
    rbconfig_tmp=`$rbconfig "print Config::CONFIG['$var_name']"`
    eval "rbconfig_$var_name=\"$rbconfig_tmp\""
  done
  changequote([, ])

  RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitearchdir "
  RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorarchdir "
  RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_archdir "
  RUBY_CFLAGS="$RUBY_CFLAGS $rbconfig_CFLAGS "
  RUBY_LIBS="$rbconfig_LIBRUBYARG $rbconfig_LIBS"
  RUBY_LDFLAGS="-L$rbconfig_libdir"

  AC_SUBST(RUBY_CFLAGS)
  AC_SUBST(RUBY_LIBS)
  AC_SUBST(RUBY_LDFLAGS)

  _SAVE_CFLAGS=$CFLAGS
  _SAVE_LIBS=$LIBS
  ruby_undef_package_macros="
#undef PACKAGE_NAME
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_VERSION
"
  CFLAGS="$CFLAGS $RUBY_CFLAGS"
  AC_CHECK_HEADERS(ruby.h, [],
                   [ruby_available="no"],
                   [$ruby_undef_package_macros])
  if test "x$ruby_available" = "xyes"; then
    AC_CHECK_LIB($rbconfig_RUBY_SO_NAME,
                 ruby_init,
                 [],
                 [ruby_available="no"],
                 $RUBY_LIBS)
  fi
  if test "x$ruby_available" != "xyes"; then
    AC_MSG_ERROR([Ruby is required.])
  fi

  if test "x$ruby_available" = "xyes"; then
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
    AC_CHECK_HEADERS(rbgobject.h, [],
		     [ruby_available="no"],
		     [$ruby_undef_package_macros])
  fi
  if test "x$ruby_available" != "xyes"; then
    AC_MSG_ERROR([Ruby/GLib2 is required.])
  fi

  AC_CHECK_FUNC(rb_errinfo)
  CFLAGS=$_SAVE_CFLAGS
  LIBS=$_SAVE_LIBS
fi

AM_CONDITIONAL([WITH_RUBY], [test "$ruby_available" = "yes"])

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

AC_PROG_INTLTOOL([0.35.0])

AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=milter-manager
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package])

dnl **************************************************************
dnl Check for package platform.
dnl **************************************************************

AC_MSG_CHECKING(package-platform)
AC_ARG_WITH([package-platform],
            AS_HELP_STRING([--with-package-platform=PLATFORM],
                           [Package platform.
                            e.g. debian, redhat, freebsd, pkgsrc, ...
                            (default: auto-detect)]),
            [MILTER_MANAGER_PACKAGE_PLATFORM="$withval"])

if test "$MILTER_MANAGER_PACKAGE_PLATFORM" = ""; then
  case `uname` in
  Linux)
    if test -f /etc/debian_version -o -x /usr/bin/dpkg; then
      MILTER_MANAGER_PACKAGE_PLATFORM="debian"
    elif test -f /etc/redhat-release; then
      MILTER_MANAGER_PACKAGE_PLATFORM="redhat"
    fi
    ;;
  FreeBSD)
    MILTER_MANAGER_PACKAGE_PLATFORM="freebsd"
    ;;
  NetBSD|DragonFly)
    MILTER_MANAGER_PACKAGE_PLATFORM="pkgsrc"
    ;;
  esac
fi

if test "$MILTER_MANAGER_PACKAGE_PLATFORM" = ""; then
  MILTER_MANAGER_PACKAGE_PLATFORM="unknown"
fi
AC_MSG_RESULT($MILTER_MANAGER_PACKAGE_PLATFORM)

AC_MSG_CHECKING(package-options)
AC_ARG_WITH([package-options],
            AS_HELP_STRING([--with-package-options=OPTIONS],
                           [Package platform dependent options.
			    OPTIONS format is VAR1=VAL1,VAR2=VAL2,....
                            (default: none)]),
            [MILTER_MANAGER_PACKAGE_OPTIONS="$withval"])
AC_MSG_RESULT($MILTER_MANAGER_PACKAGE_OPTIONS)


MILTER_MANAGER_PACKAGE_PLATFORM_C_STRING="\\\"\$(MILTER_MANAGER_PACKAGE_PLATFORM)\\\""

if test "$MILTER_PACKAGE_OPTIONS" = ""; then
  MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING="NULL"
else
  MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING="\\\"\$(MILTER_MANAGER_PACKAGE_OPTIONS)\\\""
fi

AC_SUBST(MILTER_MANAGER_PACKAGE_PLATFORM)
AC_SUBST(MILTER_MANAGER_PACKAGE_PLATFORM_C_STRING)
AC_SUBST(MILTER_MANAGER_PACKAGE_OPTIONS)
AC_SUBST(MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING)

dnl **************************************************************
dnl Check for path.
dnl **************************************************************

moduledir='$(pkglibdir)/module'
module_configurationdir='$(moduledir)/configuration'
AC_SUBST(moduledir)
AC_SUBST(module_configurationdir)

bindingdir='$(pkglibdir)/binding'
binding_extdir='$(bindingdir)/ext'
binding_libdir='$(bindingdir)/lib'
AC_SUBST(bindingdir)
AC_SUBST(binding_extdir)
AC_SUBST(binding_libdir)

tool_libdir='$(pkglibdir)/tool'
AC_SUBST(tool_libdir)

pkgsysconfdir="\$(sysconfdir)/$PACKAGE_NAME"
AC_SUBST(pkgsysconfdir)

CONFIG_FILE_NAME="milter-manager.conf"
CUSTOM_CONFIG_FILE_NAME="milter-manager.custom.conf"
AC_SUBST(CONFIG_FILE_NAME)
AC_SUBST(CUSTOM_CONFIG_FILE_NAME)

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

SF_HTDOCS_PATH="/home/groups/m/mi/milter-manager/htdocs"
AC_SUBST(SF_HTDOCS_PATH)

SF_HTDOCS="$SF_HOST:$SF_HTDOCS_PATH"
AC_SUBST(SF_HTDOCS)

SF_REPOS="https://milter-manager.svn.sourceforge.net/svnroot/milter-manager/milter-manager"
AC_SUBST(SF_REPOS)

MILTER_TOOLKIT_CFLAGS="\$(GLIB_CFLAGS) -DG_DISABLE_DEPRECATED"
MILTER_RUBY_CFLAGS="\$(MILTER_TOOLKIT_CFLAGS) \$(RUBY_CFLAGS)"
MILTER_MANAGER_CFLAGS="\$(MILTER_TOOLKIT_CFLAGS)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCONFIGURATION_MODULE_DIR=\\\"\$(module_configurationdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DBINDING_EXT_DIR=\\\"\$(binding_extdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DBINDING_LIB_DIR=\\\"\$(binding_libdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCONFIG_DIR=\\\"\$(pkgsysconfdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCONFIG_FILE_NAME=\\\"\$(CONFIG_FILE_NAME)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCUSTOM_CONFIG_FILE_NAME=\\\"\$(CUSTOM_CONFIG_FILE_NAME)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_PACKAGE_PLATFORM=\$(MILTER_MANAGER_PACKAGE_PLATFORM_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_PACKAGE_OPTIONS=\$(MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING)"
AC_SUBST(MILTER_TOOLKIT_CFLAGS)
AC_SUBST(MILTER_RUBY_CFLAGS)
AC_SUBST(MILTER_MANAGER_CFLAGS)

MILTER_TOOLKIT_LIBS="\$(GLIB_LIBS)"
MILTER_RUBY_LIBS="\$(MILTER_TOOLKIT_LIBS) \$(RUBY_LIBS)"
MILTER_MANAGER_LIBS="\$(MILTER_TOOLKIT_LIBS)"
AC_SUBST(MILTER_TOOLKIT_LIBS)
AC_SUBST(MILTER_RUBY_LIBS)
AC_SUBST(MILTER_MANAGER_LIBS)

AC_CONFIG_FILES([Makefile
		 milter-core.pc
		 milter-client.pc
		 libmilter.pc
		 milter-server.pc
		 milter-manager.pc
		 milter/Makefile
		 milter/core/Makefile
		 milter/core/milter-version.h
		 milter/client/Makefile
		 libmilter/Makefile
		 milter/server/Makefile
		 milter/manager/Makefile
		 binding/Makefile
		 binding/ruby/Makefile
		 binding/ruby/src/Makefile
		 binding/ruby/src/toolkit/Makefile
		 binding/ruby/src/manager/Makefile
		 binding/ruby/lib/Makefile
		 binding/ruby/lib/milter/Makefile
		 binding/ruby/lib/milter/manager/Makefile
		 binding/ruby/test/Makefile
		 binding/ruby/test/toolkit/Makefile
		 binding/ruby/test/manager/Makefile
		 module/Makefile
		 module/configuration/Makefile
		 module/configuration/ruby/Makefile
		 src/Makefile
		 data/Makefile
		 data/applicable-conditions/Makefile
		 data/defaults/Makefile
		 data/packet/Makefile
		 data/rc.d/Makefile
		 data/init.d/Makefile
		 data/init.d/debian/Makefile
		 data/init.d/redhat/Makefile
		 data/cron.d/Makefile
		 data/cron.d/debian/Makefile
		 data/cron.d/redhat/Makefile
		 data/cron.d/freebsd/Makefile
		 tool/Makefile
		 admin/Makefile
		 test/Makefile
		 test/lib/Makefile
		 test/fixtures/Makefile
		 test/fixtures/children/Makefile
		 test/fixtures/configuration/Makefile
		 test/fixtures/leader/Makefile
		 test/fixtures/manager/Makefile
		 test/core/Makefile
		 test/client/Makefile
		 test/libmilter/Makefile
		 test/server/Makefile
		 test/tool/Makefile
		 test/tool/fixtures/Makefile
		 test/manager/Makefile
		 po/Makefile.in
		 doc/Makefile
		 doc/reference/Makefile
		 doc/reference/version.xml
		 misc/Makefile
		 html/Makefile
		 license/Makefile
		 build/Makefile])

AC_OUTPUT

glib_version="$glib_config_major_version.$glib_config_minor_version.$glib_config_micro_version"

echo
echo "Configure Result:"
echo
echo "  Package Platform : $MILTER_MANAGER_PACKAGE_PLATFORM"
echo "  Package Options  : $MILTER_MANAGER_PACKAGE_OPTIONS"
echo
echo "  GLib             : $glib_version"
echo "  Ruby             : $RUBY"
echo
echo "  GTK-Doc          : $GTKDOC_CHECK"
echo
echo "  Cutter           : $CUTTER"
echo "  LCOV             : $LCOV"
