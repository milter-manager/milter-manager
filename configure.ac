#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

m4_define([milter_manager_version_major], [1])
m4_define([milter_manager_version_minor], [6])
m4_define([milter_manager_version_micro], [3])
m4_define([milter_manager_version],
          [milter_manager_version_major.milter_manager_version_minor.milter_manager_version_micro])

AC_INIT(milter-manager, [milter_manager_version],
        milter-manager-users-en@lists.sourceforge.net)
AC_CONFIG_AUX_DIR([config])

AC_CONFIG_SRCDIR([milter/manager.h])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

if test "`expr milter_manager_version_minor % 2`" = "1"; then
  DEVELOPMENT_RELEASE=yes
else
  DEVELOPMENT_RELEASE=no
fi
AC_SUBST(DEVELOPMENT_RELEASE)

if test "$DEVELOPMENT_RELEASE" = "yes"; then
  RELEASE_TYPE=dev
  RELEASE_TYPE_SUFFIX=-dev
else
  RELEASE_TYPE=
  RELEASE_TYPE_SUFFIX=
fi
AC_SUBST(RELEASE_TYPE)
AC_SUBST(RELEASE_TYPE_SUFFIX)

if test -n $RELEASE_TYPE; then
   RELEASE_TYPE_PATH="/$RELEASE_TYPE"
else
   RELEASE_TYPE_PATH=""
fi
AC_SUBST(RELEASE_TYPE_PATH)

PACKAGE_TITLE="milter manager"
if test "$DEVELOPMENT_RELEASE" = "yes"; then
   PACKAGE_TITLE="$PACKAGE_TITLE (dev)"
fi
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

case "$host_os" in
solaris*)
  CPPFLAGS="$CPPFLAGS -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS"
  ;;
darwin*)
  : ${RC_ARCHS=`arch`}
  export RC_ARCHS
esac

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

AC_CHECK_TYPE([long long])
AC_CHECK_TYPE([long double])

dnl **************************************************************
dnl Check for network libraries.
dnl **************************************************************
NETWORK_LIBS=
AC_CHECK_FUNC(socket, [],
              [AC_SEARCH_LIBS(socket, nsl,
                              [NETWORK_LIBS="-lsocket -lnsl"],
                              [AC_MSG_ERROR([socket() required.])],
                              [-lsocket])])
AC_SUBST(NETWORK_LIBS)

AC_CHECK_FUNCS(sendmsg recvmsg)
if test "$ac_cv_func_sendmsg" = yes -o "$ac_cv_func_recvmsg" = yes; then
    includes="AC_INCLUDES_DEFAULT([@%:@include <sys/types.h>
@%:@include <sys/socket.h>])"
    AC_CHECK_MEMBERS([struct msghdr.msg_control], [], [], $includes)
    AC_CHECK_MEMBERS([struct msghdr.msg_accrights], [], [], $includes)
fi

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

use_bundled_ruby_glib2=no
if test "$ruby_available" = "yes"; then
  rbconfig="$RUBY -rrbconfig -rshellwords -e "

  changequote(<<, >>)
  for var_name in archdir sitearchdir vendorarchdir arch \
      	       	  libdir sitelibdir vendorlibdir \
      	       	  CFLAGS LIBRUBYARG LIBS RUBY_SO_NAME \
                  rubyhdrdir sitehdrdir vendorhdrdir; do
    rbconfig_value=`$rbconfig "print Shellwords.shellwords(RbConfig::CONFIG['$var_name'] || '').join(' ')"`
    eval "rbconfig_$var_name=\"$rbconfig_value\""
  done
  ruby_minor_version="`$RUBY -e 'print RUBY_VERSION.split(/\\./)[1]'`"
  ruby_micro_version="`$RUBY -e 'print RUBY_VERSION.split(/\\./)[2]'`"
  changequote([, ])

  if test "$rbconfig_rubyhdrdir" = ""; then
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitearchdir "
    if test "$rbconfig_vendorarchdir" != ""; then
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorarchdir "
    fi
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_archdir "
  else
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitehdrdir/$rbconfig_arch "
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitehdrdir "
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorhdrdir/$rbconfig_arch "
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorhdrdir "
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_rubyhdrdir/$rbconfig_arch "
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_rubyhdrdir "
  fi

  RUBY_CFLAGS="$RUBY_CFLAGS $rbconfig_CFLAGS "
  RUBY_LIBS="$rbconfig_LIBRUBYARG $rbconfig_LIBS"
  RUBY_LDFLAGS="-L$rbconfig_libdir"

  AC_SUBST(RUBY_CFLAGS)
  AC_SUBST(RUBY_LIBS)
  AC_SUBST(RUBY_LDFLAGS)

  _SAVE_CFLAGS=$CFLAGS
  _SAVE_LIBS=$LIBS
  _SAVE_LDFLAGS=$LDFLAGS
  ruby_undef_package_macros="
#undef PACKAGE_NAME
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
"
  CFLAGS="$CFLAGS $RUBY_CFLAGS"
  LDFLAGS="$LDFLAGS $RUBY_LDFLAGS"
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

  RUBY_GLIB2_CFLAGS=""
  if test "x$ruby_available" = "xyes"; then
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
    AC_CHECK_HEADERS(rbgobject.h, [],
		     [ruby_available="no"],
		     [$ruby_undef_package_macros])
  fi
  if test "$ruby_available" = "yes"; then
    AC_TRY_COMPILE([
                    $ruby_undef_package_macros
                    #include <rbgobject.h>
                   ],
                   [
                    #if RBGLIB_MINOR_VERSION == 17
                    #  error "Ruby/GLib2 0.17.0 has GC related bug."
                    #endif
                    #if RBGLIB_MINOR_VERSION == 18
                    #  error "Ruby/GLib2 0.18.x has memory leak bugs."
                    #endif
                    #if RBGLIB_MINOR_VERSION == 19 && RBGLIB_MICRO_VERSION < 4
                    #  error "Ruby/GLib2 0.19.[0123] have memory leak bugs."
                    #endif
                   ],
                   [],
		   [ruby_available="no"
                    try_to_use_bundled_ruby_glib2_0_90_5="yes"])
  else
    if test $ruby_minor_version$ruby_micro_version -gt 85; then
      try_to_use_bundled_ruby_glib2_0_90_5="yes"
    fi
  fi
  if test "$ruby_available" != "yes"; then
    if test "$exec_prefix" = "NONE"; then
      if test "$prefix" = "NONE"; then
        ac_ruby_glib2_exec_prefix=$ac_default_prefix
      else
        ac_ruby_glib2_exec_prefix=$prefix
      fi
    else
      ac_ruby_glib2_exec_prefix=$exec_prefix
    fi

    if test "$try_to_use_bundled_ruby_glib2_0_90_5" = "yes"; then
      ac_ruby_glib2_version=0.90.5
    else
      ac_ruby_glib2_version=0.16.0
    fi
    AC_MSG_NOTICE([checking bundled Ruby/GLib2 $ac_ruby_glib2_version])
    ac_ruby_glib2_dir="binding/ruby/glib-$ac_ruby_glib2_version"
    if $RUBY -run -e mkdir -- -p $ac_ruby_glib2_dir && \
       (cd $ac_ruby_glib2_dir && \
        $RUBY ../../../$srcdir/$ac_ruby_glib2_dir/extconf.rb); then
      ruby_available=yes
      if test "$try_to_use_bundled_ruby_glib2_0_90_5" = "yes"; then
        ac_use_bundled_ruby_glib2_0_90_5=yes
	ac_ruby_glib2_source_dir="$ac_ruby_glib2_dir/ext/glib2"
      else
        ac_use_bundled_ruby_glib2_0_16_0=yes
        ac_ruby_glib2_source_dir="$ac_ruby_glib2_dir/src"
      fi
      RUBY_GLIB2_CFLAGS="-I \$(top_srcdir)/$ac_ruby_glib2_source_dir"
      sed -i'' \
      	  -e "s,^CPPFLAGS = ,CPPFLAGS = ${CPPFLAGS} ," \
      	  $ac_ruby_glib2_source_dir/Makefile
      sed -i'' \
      	  -e 's/ install/ RUBYARCHDIR=$(binding_extdir) RUBYLIBDIR=$(binding_libdir) libdir=$(libdir) install/' \
      	  $ac_ruby_glib2_dir/Makefile
      cat <<EOM >> $ac_ruby_glib2_dir/Makefile

srcdir_prefix = `test "${srcdir}" = . || echo "../../../$srcdir/$ac_ruby_glib2_dir/"`
exec_prefix = $ac_ruby_glib2_exec_prefix
libdir = \$(DESTDIR)$libdir
pkglibdir = \$(libdir)/$PACKAGE_NAME
bindingdir = $bindingdir
binding_extdir = $binding_extdir
binding_libdir = $binding_libdir
RUBY = $RUBY

check:
	\$(RUBY) \$(srcdir_prefix)test/run-test.rb

distdir:

dist:

uninstall:
EOM
    fi
  fi

  if test "$ac_use_bundled_ruby_glib2_0_16_0" != "yes"; then
    $RUBY -run -e mkdir -- -p binding/ruby/glib-0.16.0
    cat <<EOM > binding/ruby/glib-0.16.0/Makefile
all:
distdir:
distclean:
uninstall:
EOM
  fi
  if test "$ac_use_bundled_ruby_glib2_0_90_5" != "yes"; then
    $RUBY -run -e mkdir -- -p binding/ruby/glib-0.90.5
    cat <<EOM > binding/ruby/glib-0.90.5/Makefile
all:
distdir:
distclean:
uninstall:
EOM
  fi

  if test "$ruby_available" != "yes"; then
    AC_MSG_ERROR([Ruby/GLib2 is required.])
  fi

  AC_SUBST(RUBY_GLIB2_CFLAGS)

  AC_CHECK_FUNCS(rb_errinfo)
  CFLAGS=$_SAVE_CFLAGS
  LIBS=$_SAVE_LIBS
  LDFLAGS=$_SAVE_LDFLAGS
fi

AC_ARG_ENABLE([ruby-milter],
              AS_HELP_STRING([--enable-ruby-milter[=DIR]],
                             [Enable milter development with Ruby.
                              DIR should be one of the Ruby's LOAD_PATH.
                              (default: vendor_ruby or site_ruby directoy)]),
              [ruby_milterdir="$enableval"],
              [ruby_milterdir="no"])

if test "$ruby_available" = "yes" -a "$ruby_milterdir" != "no"; then
  AC_MSG_CHECKING([where to Ruby milter install])
  if test "$ruby_milterdir" = "yes" ; then
     if test -n "$rbconfig_vendorlibdir"; then
       ruby_milterdir="$rbconfig_vendorlibdir"
     else
       ruby_milterdir="$rbconfig_sitelibdir"
     fi
  fi
  AC_MSG_RESULT([$ruby_milterdir])
else
  ruby_milterdir=""
fi
AC_SUBST(ruby_milterdir)

sample_rubydir="\$(pkgdatadir)/sample/ruby"
AC_SUBST(sample_rubydir)

AM_CONDITIONAL([WITH_RUBY], [test "$ruby_available" = "yes"])
AM_CONDITIONAL([WITH_BUNDLED_RUBY_GLIB2_0_16_0],
               [test "$ac_use_bundled_ruby_glib2_0_16_0" = "yes"])
AM_CONDITIONAL([WITH_BUNDLED_RUBY_GLIB2_0_90_5],
               [test "$ac_use_bundled_ruby_glib2_0_90_5" = "yes"])
AM_CONDITIONAL([WITH_RUBY_MILTER], [test -n "$ruby_milterdir"])

dnl **************************************************************
dnl Checks for RD2
dnl **************************************************************

AC_PATH_PROG(RD2, rd2, rd2-not-found)
AM_CONDITIONAL([WITH_RD2], [test "$RD2" != "rd2-not-found"])
if test "$RD2" != "rd2-not-found"; then
    RD2RUBY=`sed -e '1!{' -e 's/.*//' -e q -e '}' -e '/^@%:@! */!d' -e 's///' -e 's|^/usr/bin/env *||' -e q $RD2`
    if test -z "$RD2RUBY"; then
	RD2RUBY='$(RUBY)'
    fi
    AC_SUBST(RD2RUBY)
fi

dnl **************************************************************
dnl Configure for maintainer mode.
dnl **************************************************************

AM_MAINTAINER_MODE
AM_CONDITIONAL([MAINTAINER_MODE],
               [test "x$USE_MAINTAINER_MODE" = "xyes"])

dnl **************************************************************
dnl Checks for gtk-doc
dnl **************************************************************

m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.8])
], [enable_gtk_doc=no])
AM_CONDITIONAL([ENABLE_GTK_DOC], [test "$enable_gtk_doc" != no])
AM_CONDITIONAL([GTK_DOC_USE_LIBTOOL], [test "$enable_gtk_doc" != no])

dnl **************************************************************
dnl Check for gettext.
dnl **************************************************************

AC_PROG_INTLTOOL([0.35.0])

AM_GLIB_GNU_GETTEXT

GETTEXT_PACKAGE=milter-manager
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package])

dnl **************************************************************
dnl Check for libev.
dnl **************************************************************

libev_available=no
AC_ARG_WITH(libev,
  [AS_HELP_STRING([--with-libev],
    [Specify libev installed prefix. [default=auto]])],
  [with_libev="$withval"],
  [with_libev="auto"])
if test "x$with_libev" != "xno"; then
  _SAVE_CFLAGS="$CFLAGS"
  _SAVE_LDFLAGS="$LDFLAGS"
  if test "x$with_libev" = "xauto" -o "x$with_libev" = "xyes"; then
    libev_prefixes="/usr/local /usr"
  else
    libev_prefixes="$with_libev"
  fi
  for libev_prefix in $libev_prefixes; do
    CFLAGS="$CFLAGS -I$libev_prefix/include"
    LDFLAGS="$LDFLAGS -L$libev_prefix/lib"
    libev_available=no
    AC_CHECK_HEADERS(ev.h,
                     [AC_CHECK_LIB(ev, ev_loop_new,
                                   [libev_available=yes])])
    if test "x$libev_available" != "xno"; then
      AC_COMPILE_IFELSE(
        [AC_LANG_BOOL_COMPILE_TRY([@%:@include <ev.h>],
	  [EV_VERSION_MAJOR > 4 || (EV_VERSION_MAJOR == 4 && EV_VERSION_MINOR >= 1)])],
	[],
	[libev_available=no])
    fi
    CFLAGS="$_SAVE_CFLAGS"
    LDFLAGS="$_SAVE_LDFLAGS"
    if test "x$libev_available" != "xno"; then
      LIBEV_CFLAGS="-I$libev_prefix/include"
      LIBEV_LIBS="-L$libev_prefix/lib -lev"
      break
    fi
  done
  if test "$libev_available" = no; then
    libev_available=yes
    libev_prefix=bundled
    m4_include([libev-4.03/libev.m4])
    AC_DEFINE(HAVE_LIBEV, [1], [Define to 1 if libev is available.])
    LIBEV_CFLAGS='-I$(top_srcdir)/libev-4.03'
    LIBEV_LIBS='$(top_builddir)/libev-4.03/libev.la'
  fi
  AC_DEFINE(HAVE_LIBEV, [1], [Define to 1 if libev is available.])
fi
AM_CONDITIONAL(WITH_LIBEV, [test "x$libev_available" != xno])
AC_SUBST(LIBEV_CFLAGS)
AC_SUBST(LIBEV_LIBS)

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

AC_ARG_WITH([rcddir],
             AS_HELP_STRING([--with-rcddir=DIR],
                            [Synonym for --with-packege-options=rcddir=DIR.
                             This option is for pkgsrc platform.
                             (default: none)]))
AC_MSG_CHECKING(package-options)
AC_ARG_WITH([package-options],
            AS_HELP_STRING([--with-package-options=OPTIONS],
                           [Package platform dependent options.
			    OPTIONS format is VAR1=VAL1,VAR2=VAL2,....
                            (default: none)]),
            [MILTER_MANAGER_PACKAGE_OPTIONS="$withval"])
if test -n "$with_rcddir"; then
  if test -n "$MILTER_MANAGER_PACKAGE_OPTIONS"; then
    MILTER_MANAGER_PACKAGE_OPTIONS="rcddir=$with_rcddir,$MILTER_MANAGER_PACKAGE_OPTIONS"
  else
    MILTER_MANAGER_PACKAGE_OPTIONS="rcddir=$with_rcddir"
  fi
fi
AC_MSG_RESULT($MILTER_MANAGER_PACKAGE_OPTIONS)


MILTER_MANAGER_PACKAGE_PLATFORM_C_STRING="\\\"\$(MILTER_MANAGER_PACKAGE_PLATFORM)\\\""

if test "$MILTER_MANAGER_PACKAGE_OPTIONS" = ""; then
  MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING="NULL"
else
  MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING="\\\"\$(MILTER_MANAGER_PACKAGE_OPTIONS)\\\""
fi

AC_SUBST(MILTER_MANAGER_PACKAGE_PLATFORM)
AC_SUBST(MILTER_MANAGER_PACKAGE_PLATFORM_C_STRING)
AC_SUBST(MILTER_MANAGER_PACKAGE_OPTIONS)
AC_SUBST(MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING)

dnl **************************************************************
dnl Check for default configuration.
dnl **************************************************************
AC_ARG_WITH([default-effective-user],
            AS_HELP_STRING([--with-default-effective-user=USER],
                           [Default effective user. (default: none)]),
            [MILTER_MANAGER_DEFAULT_EFFECTIVE_USER="$withval"])

AC_ARG_WITH([default-effective-group],
            AS_HELP_STRING([--with-default-effective-group=GROUP],
                           [Default effective group. (default: none)]),
            [MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP="$withval"])

AC_ARG_WITH([default-socket-group],
            AS_HELP_STRING([--with-default-socket-group=GROUP],
                           [Default UNIX socket group. (default: none)]),
            [MILTER_MANAGER_DEFAULT_SOCKET_GROUP="$withval"])

AC_ARG_WITH([default-connection-spec],
            AS_HELP_STRING([--with-default-connection-spec=SPEC],
                           [Default connection spec. (default: none)]),
            [MILTER_MANAGER_DEFAULT_CONNECTION_SPEC="$withval"])

AC_ARG_WITH([default-pid-file],
            AS_HELP_STRING([--with-default-pid-file=FILE],
                           [Default PID file. (default: none)]),
            [MILTER_MANAGER_DEFAULT_PID_FILE="$withval"])

AC_SUBST(MILTER_MANAGER_DEFAULT_EFFECTIVE_USER)
AC_SUBST(MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP)
AC_SUBST(MILTER_MANAGER_DEFAULT_SOCKET_GROUP)
AC_SUBST(MILTER_MANAGER_DEFAULT_CONNECTION_SPEC)
AC_SUBST(MILTER_MANAGER_DEFAULT_PID_FILE)

if test -z "$MILTER_MANAGER_DEFAULT_EFFECTIVE_USER"; then
  MILTER_MANAGER_DEFAULT_EFFECTIVE_USER_C_STRING=NULL
else
  MILTER_MANAGER_DEFAULT_EFFECTIVE_USER_C_STRING="\\\"\$(MILTER_MANAGER_DEFAULT_EFFECTIVE_USER)\\\""
fi
if test -z "$MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP"; then
  MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP_C_STRING=NULL
else
  MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP_C_STRING="\\\"\$(MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP)\\\""
fi
if test -z "$MILTER_MANAGER_DEFAULT_SOCKET_GROUP"; then
  MILTER_MANAGER_DEFAULT_SOCKET_GROUP_C_STRING=NULL
else
  MILTER_MANAGER_DEFAULT_SOCKET_GROUP_C_STRING="\\\"\$(MILTER_MANAGER_DEFAULT_SOCKET_GROUP)\\\""
fi
if test -z "$MILTER_MANAGER_DEFAULT_CONNECTION_SPEC"; then
  MILTER_MANAGER_DEFAULT_CONNECTION_SPEC=['inet:10025@[127.0.0.1]']
fi
MILTER_MANAGER_DEFAULT_CONNECTION_SPEC_C_STRING="\\\"\$(MILTER_MANAGER_DEFAULT_CONNECTION_SPEC)\\\""
if test -z "$MILTER_MANAGER_DEFAULT_PID_FILE"; then
  MILTER_MANAGER_DEFAULT_PID_FILE_C_STRING=NULL
else
  MILTER_MANAGER_DEFAULT_PID_FILE_C_STRING="\\\"\$(MILTER_MANAGER_DEFAULT_PID_FILE)\\\""
fi

AC_SUBST(MILTER_MANAGER_DEFAULT_EFFECTIVE_USER_C_STRING)
AC_SUBST(MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP_C_STRING)
AC_SUBST(MILTER_MANAGER_DEFAULT_SOCKET_GROUP_C_STRING)
AC_SUBST(MILTER_MANAGER_DEFAULT_CONNECTION_SPEC_C_STRING)
AC_SUBST(MILTER_MANAGER_DEFAULT_PID_FILE_C_STRING)

dnl **************************************************************
dnl Check for SF.net.
dnl **************************************************************

SF_PROJECT_ID="milter-manager"
AC_SUBST(SF_PROJECT_ID)
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

SF_HOST="web.sourceforge.net"
if test -n "$SF_USER"; then
   SF_HOST="$SF_USER,milter-manager@$SF_HOST"
fi
AC_SUBST(SF_HOST)

SF_HTDOCS_PATH="/home/groups/m/mi/milter-manager/htdocs"
AC_SUBST(SF_HTDOCS_PATH)

SF_HTDOCS="$SF_HOST:$SF_HTDOCS_PATH"
AC_SUBST(SF_HTDOCS)

SF_REPOS="https://milter-manager.svn.sourceforge.net/svnroot/milter-manager/milter-manager"
AC_SUBST(SF_REPOS)

MILTER_TOOLKIT_CFLAGS="\$(GLIB_CFLAGS) -DG_DISABLE_DEPRECATED"
MILTER_RUBY_CFLAGS="\$(RUBY_GLIB2_CFLAGS) \$(RUBY_CFLAGS)"
MILTER_MANAGER_CFLAGS="\$(MILTER_TOOLKIT_CFLAGS)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCONFIGURATION_MODULE_DIR=\\\"\$(module_configurationdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DBINDING_EXT_DIR=\\\"\$(binding_extdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DBINDING_LIB_DIR=\\\"\$(binding_libdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCONFIG_DIR=\\\"\$(pkgsysconfdir)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCONFIG_FILE_NAME=\\\"\$(CONFIG_FILE_NAME)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DCUSTOM_CONFIG_FILE_NAME=\\\"\$(CUSTOM_CONFIG_FILE_NAME)\\\""
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_PACKAGE_PLATFORM=\$(MILTER_MANAGER_PACKAGE_PLATFORM_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_PACKAGE_OPTIONS=\$(MILTER_MANAGER_PACKAGE_OPTIONS_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_DEFAULT_EFFECTIVE_USER=\$(MILTER_MANAGER_DEFAULT_EFFECTIVE_USER_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP=\$(MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_DEFAULT_SOCKET_GROUP=\$(MILTER_MANAGER_DEFAULT_SOCKET_GROUP_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_DEFAULT_CONNECTION_SPEC=\$(MILTER_MANAGER_DEFAULT_CONNECTION_SPEC_C_STRING)"
MILTER_MANAGER_CFLAGS="$MILTER_MANAGER_CFLAGS -DMILTER_MANAGER_DEFAULT_PID_FILE=\$(MILTER_MANAGER_DEFAULT_PID_FILE_C_STRING)"
AC_SUBST(MILTER_TOOLKIT_CFLAGS)
AC_SUBST(MILTER_RUBY_CFLAGS)
AC_SUBST(MILTER_MANAGER_CFLAGS)

MILTER_CORE_LIBS="\$(NETWORK_LIBS) \$(GLIB_LIBS)"
MILTER_CLIENT_LIBS="\$(GLIB_LIBS)"
MILTER_SERVER_LIBS="\$(NETWORK_LIBS) \$(GLIB_LIBS)"
MILTER_RUBY_LIBS="\$(GLIB_LIBS) \$(RUBY_LIBS)"
MILTER_MANAGER_LIBS="\$(GLIB_LIBS)"
AC_SUBST(MILTER_CORE_LIBS)
AC_SUBST(MILTER_CLIENT_LIBS)
AC_SUBST(MILTER_SERVER_LIBS)
AC_SUBST(MILTER_RUBY_LIBS)
AC_SUBST(MILTER_MANAGER_LIBS)

AC_CONFIG_FILES([Makefile
		 milter-core.pc
		 milter-client.pc
		 libmilter.pc
		 milter-server.pc
		 milter-manager.pc
		 ruby-milter.pc
		 rpm/centos/milter-manager.spec
		 libev-4.03/Makefile
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
		 binding/ruby/lib/milter/client/Makefile
		 binding/ruby/lib/milter/manager/Makefile
		 binding/ruby/ruby-milter/Makefile
		 binding/ruby/test/Makefile
		 binding/ruby/test/toolkit/Makefile
		 binding/ruby/test/manager/Makefile
		 binding/ruby/sample/Makefile
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
		 data/init.d/debian/default/Makefile
		 data/init.d/redhat/Makefile
		 data/init.d/redhat/sysconfig/Makefile
		 data/cron.d/Makefile
		 data/cron.d/debian/Makefile
		 data/cron.d/redhat/Makefile
		 data/cron.d/freebsd/Makefile
		 data/munin/Makefile
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
		 build/Makefile
		 apt/Makefile
		 rpm/Makefile
		 rpm/centos/Makefile
		 yum/Makefile])

AC_OUTPUT

glib_version="$glib_config_major_version.$glib_config_minor_version.$glib_config_micro_version"

if test "$libev_available" = "yes"; then
   libev_configure_result="$libev_available ($libev_prefix)"
else
   libev_configure_result="$libev_available"
fi

echo
echo "Configure Result:"
echo
echo "  Package Platform        : $MILTER_MANAGER_PACKAGE_PLATFORM"
echo "  Package Options         : $MILTER_MANAGER_PACKAGE_OPTIONS"
echo
echo "  Default Effective User  : $MILTER_MANAGER_DEFAULT_EFFECTIVE_USER"
echo "  Default Effective Group : $MILTER_MANAGER_DEFAULT_EFFECTIVE_GROUP"
echo "  Default Socket Group    : $MILTER_MANAGER_DEFAULT_SOCKET_GROUP"
echo "  Default Connection Spec : $MILTER_MANAGER_DEFAULT_CONNECTION_SPEC"
echo "  Default PID File        : $MILTER_MANAGER_DEFAULT_PID_FILE"
echo
echo "  GLib                    : $glib_version"
echo "  libev                   : $libev_configure_result"
echo "  Ruby                    : $RUBY"
echo "  Ruby milter PATH        : $ruby_milterdir"
echo
echo "  GTK-Doc                 : $enable_gtk_doc"
echo
echo "  Cutter                  : $CUTTER"
echo "  LCOV                    : $LCOV"
