#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

m4_define([milter_manager_version_major], [2])
m4_define([milter_manager_version_minor], [1])
m4_define([milter_manager_version_micro], [6])
m4_define([milter_manager_version],
          [milter_manager_version_major.milter_manager_version_minor.milter_manager_version_micro])

AC_INIT(milter-manager, [milter_manager_version],
        milter-manager-users-en@lists.osdn.me)
AC_CONFIG_AUX_DIR([config])

AC_CONFIG_SRCDIR([milter/manager.h])
AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE(tar-pax)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

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

ORIGINAL_CFLAGS="$CFLAGS"
AC_SUBST(ORIGINAL_CFLAGS)

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
GLIB_REQUIRED=2.24.0
AC_SUBST(GLIB_REQUIRED)
AM_PATH_GLIB_2_0($GLIB_REQUIRED,
                 [],
                 [AC_MSG_ERROR([GLib >= $GLIB_REQUIRED required.])],
                 [gobject gmodule gthread])

dnl **************************************************************
dnl Check for Cutter
dnl **************************************************************
m4_ifdef([AC_CHECK_GCUTTER],
         [AC_CHECK_GCUTTER(>= 1.1.6)],
         [cutter_use_cutter="no"])

AM_CONDITIONAL([WITH_CUTTER], [test "$cutter_use_cutter" = "yes"])
if test "$cutter_use_cutter" = "yes"; then
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

if echo -- "$libdir" | grep '\$' > /dev/null 2>&1; then
  if test "$exec_prefix" = "NONE"; then
    if test "$prefix" = "NONE"; then
      milter_manager_exec_prefix=$ac_default_prefix
    else
      milter_manager_exec_prefix=$prefix
    fi
  else
    milter_manager_exec_prefix=$exec_prefix
  fi
  eval "milter_manager_libdir=$libdir"
  if echo -- "$milter_manager_libdir" | grep NONE > /dev/null 2>&1; then
    milter_manager_libdir=${milter_manager_exec_prefix}/lib
  fi
else
  milter_manager_libdir="${libdir}"
fi

bindingdir='$(pkglibdir)/binding'
expanded_bindingdir="${milter_manager_libdir}/${PACKAGE_NAME}/binding"
binding_extdir='$(bindingdir)/ext'
binding_libdir='$(bindingdir)/lib'
expanded_binding_extdir="${expanded_bindingdir}/ext"
expanded_binding_libdir="${expanded_bindingdir}/lib"
AC_SUBST(bindingdir)
AC_SUBST(binding_extdir)
AC_SUBST(binding_libdir)
AC_SUBST(expanded_binding_extdir)
AC_SUBST(expanded_binding_libdir)

tool_libdir='$(pkglibdir)/tool'
AC_SUBST(tool_libdir)

AC_ARG_WITH(launchpad-uploader-pgp-key,
            [AS_HELP_STRING([--with-launchpad-uploader-pgp-key=KEY],
            [specify PGP key UID to upload Groonga packages to Launchpad.])],
            [LAUNCHPAD_UPLOADER_PGP_KEY="$withval"],
            [LAUNCHPAD_UPLOADER_PGP_KEY=""])
AC_SUBST(LAUNCHPAD_UPLOADER_PGP_KEY)

GPG_UID=m4_include(gpg_uid)
AC_SUBST(GPG_UID)

pkgsysconfdir="\$(sysconfdir)/$PACKAGE_NAME"
AC_SUBST(pkgsysconfdir)

CONFIG_FILE_NAME="milter-manager.conf"
CUSTOM_CONFIG_FILE_NAME="milter-manager.custom.conf"
AC_SUBST(CONFIG_FILE_NAME)
AC_SUBST(CUSTOM_CONFIG_FILE_NAME)

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

if test -z "$MILTER_MANAGER_PACKAGE_PLATFORM"; then
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
  SunOS)
    MILTER_MANAGER_PACKAGE_PLATFORM="solaris"
    ;;
  esac
fi

if test "$MILTER_MANAGER_PACKAGE_PLATFORM" = "redhat"; then
  distribution=$(cut -d " " -f 1 /etc/redhat-release | tr "A-Z" "a-z")
  case "$distribution" in
    centos)
      if grep -q Linux /etc/redhat-release; then
        distribution_version=$(cut -d " " -f 4 /etc/redhat-release)
      else
        distribution_version=$(cut -d " " -f 3 /etc/redhat-release)
      fi
    ;;
    *)
      : # do nothing
    ;;
  esac
  distribution_version=$(echo "$distribution_version" | cut -d "." -f 1)
  MILTER_MANAGER_PACKAGE_PLATFORM="${distribution}${distribution_version}"
fi

if test "$MILTER_MANAGER_PACKAGE_PLATFORM" = ""; then
  MILTER_MANAGER_PACKAGE_PLATFORM="unknown"
fi
AC_MSG_RESULT($MILTER_MANAGER_PACKAGE_PLATFORM)

AM_CONDITIONAL(DEBIAN_PLATFORM,
               [test "$MILTER_MANAGER_PACKAGE_PLATFORM" = "debian"])
AM_CONDITIONAL(REDHAT_PLATFORM,
               [test "$MILTER_MANAGER_PACKAGE_PLATFORM" = "redhat" \
                  -o "$MILTER_MANAGER_PACKAGE_PLATFORM" = "centos7" \
                  -o "$MILTER_MANAGER_PACKAGE_PLATFORM" = "centos8"])
AM_CONDITIONAL(REDHAT_SYSTEMD_PLATFORM,
               [test "$MILTER_MANAGER_PACKAGE_PLATFORM" = "centos7" \
                  -o "$MILTER_MANAGER_PACKAGE_PLATFORM" = "centos8"])
AM_CONDITIONAL(FREEBSD_PLATFORM,
               [test "$MILTER_MANAGER_PACKAGE_PLATFORM" = "freebsd"])
AM_CONDITIONAL(SOLARIS_PLATFORM,
               [test "$MILTER_MANAGER_PACKAGE_PLATFORM" = "solaris"])

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

bundled_latest_ruby_glib2_version=3.4.9
bundled_available_ruby_glib2_versions="3.4.9"
bundled_latest_ruby_gobject_introspection_version=3.4.9
bundled_available_ruby_gobject_introspection_="3.4.9"
bundled_latest_ruby_gio2_version=3.4.9
bundled_available_ruby_gio2_versions="3.4.9"
if test "$ruby_available" = "yes"; then
  rbconfig="$RUBY -rrbconfig -rshellwords -e "

  changequote(<<, >>)
  for var_name in archdir sitearchdir vendorarchdir arch \
      	       	  libdir sitelibdir vendorlibdir \
      	       	  CFLAGS LIBRUBYARG LIBS RUBY_SO_NAME \
                  rubyarchhdrdir sitearchhdrdir vendorarchhdrdir \
                  rubyhdrdir sitehdrdir vendorhdrdir; do
    rbconfig_value=`$rbconfig "print Shellwords.shellwords(RbConfig::CONFIG['$var_name'] || '').join(' ')"`
    eval "rbconfig_$var_name=\"\$rbconfig_value\""
  done
  # XXX: RbConfig::CONFIG["LIBRUBYARG"] is the following on Travis CI:
  #   "-Wl,-rpath,'${ORIGIN}/../lib' -Wl,-R -Wl,'${ORIGIN}/../lib' -L'${ORIGIN}/../lib' -lruby"
  #   https://travis-ci.org/milter-manager/milter-manager/jobs/37108226#L431
  # We don't know who sets "${ORIGIN}".
  #  The following code is just a workaround. :<
  rbconfig_LIBRUBYARG="`echo \"$rbconfig_LIBRUBYARG\" | sed -e \"s,\\\${ORIGIN},$rbconfig_libdir,g\"`"
  ruby_major_version="`$RUBY -e 'print RUBY_VERSION.split(/\\./)[0]'`"
  ruby_minor_version="`$RUBY -e 'print RUBY_VERSION.split(/\\./)[1]'`"
  ruby_micro_version="`$RUBY -e 'print RUBY_VERSION.split(/\\./)[2]'`"
  changequote([, ])
  ruby_version="${ruby_major_version}${ruby_minor_version}${ruby_micro_version}"

  if test "$rbconfig_rubyhdrdir" = ""; then
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitearchdir"
    if test "$rbconfig_vendorarchdir" != ""; then
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorarchdir"
    fi
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_archdir"
  else
    if test -d "$rbconfig_sitearchhdrdir"; then
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitearchhdrdir"
    fi
    if test -d "$rbconfig_vendorarchhdrdir"; then
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorarchhdrdir"
    fi
    if test -d "$rbconfig_rubyarchhdrdir"; then
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_rubyarchhdrdir"
    fi
    if test -d "$rbconfig_sitehdrdir"; then
      if test -d "$rbconfig_sitehdrdir/$rbconfig_arch"; then
        RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitehdrdir/$rbconfig_arch"
      fi
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_sitehdrdir"
    fi
    if test -d "$rbconfig_vendorhdrdir"; then
      if test -d "$rbconfig_vendorhdrdir/$rbconfig_arch"; then
        RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorhdrdir/$rbconfig_arch"
      fi
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_vendorhdrdir"
    fi
    if test -d "$rbconfig_rubyhdrdir/$rbconfig_arch"; then
      RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_rubyhdrdir/$rbconfig_arch"
    fi
    RUBY_CFLAGS="$RUBY_CFLAGS -I$rbconfig_rubyhdrdir"
  fi

  RUBY_CFLAGS="$RUBY_CFLAGS $rbconfig_CFLAGS"
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

  case $srcdir in
    /*) # Absolute path
     ac_abs_ruby_srcdir="$srcdir/binding/ruby"
     ;;
    *)
     ac_abs_ruby_srcdir="$ac_pwd/$srcdir/binding/ruby"
     ;;
  esac

  AC_ARG_WITH([bundled-ruby-glib2],
	      AS_HELP_STRING([--with-bundled-ruby-glib2],
			     [Use bundled Ruby/GLib2
                              (default: try system Ruby/GLib2 and them
                                            bundled Ruby/GLib2)]),
	      [use_bundled_ruby_glib2="$withval"],
              [use_bundled_ruby_glib2="auto"])

  RUBY_GLIB2_CFLAGS=""
  ruby_glib2_available=no
  if test "x$use_bundled_ruby_glib2" != "xyes"; then
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
    if test "$ruby_glib2_available" = "no"; then
      RUBY_GLIB2_CFLAGS=$($RUBY -e '
        begin
          require "rubygems"
          ruby_glib2_gem = Gem::Specification.find_by_name("glib2")
        rescue LoadError
          ruby_glib2_gem = nil
        end
	if ruby_glib2_gem.nil?
	  begin
	    require "glib2"
	    ruby_glib2_path = $".grep(/glib2\.(?!rb\z)\w+\z/).first
	    exit(false) if ruby_glib2_path.nil?
	    print("-I ", File.dirname(ruby_glib2_path))
	    exit(true)
	  rescue LoadError
	    exit(false)
	  end
	end
	ruby_glib2_path = ruby_glib2_gem.full_gem_path
	print("-I ", File.join(ruby_glib2_path, "ext", "glib2"))')
      if test $? -eq 0; then
        CFLAGS="$CFLAGS $RUBY_GLIB2_CFLAGS"
      else
        RUBY_GLIB2_CFLAGS=""
      fi
      AC_CHECK_HEADERS(rbgobject.h,
		       [ruby_glib2_available="yes"],
		       [ruby_glib2_available="no"],
		       [$ruby_undef_package_macros])
    fi
    if test "$ruby_glib2_available" = "yes"; then
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
		     [ruby_glib2_available="no"])
    fi
  fi

  if test "x$use_bundled_ruby_glib2" != "xno" -a \
          "x$ruby_glib2_available" != "xyes"; then
    ac_used_bundled_ruby_glib2_version=${bundled_latest_ruby_glib2_version}
    if test "x$use_bundled_ruby_glib2" != "xauto" -a \
            "x$use_bundled_ruby_glib2" != "xyes" -a \
            "x$use_bundled_ruby_glib2" != "xno"; then
      ac_used_bundled_ruby_glib2_version=$use_bundled_ruby_glib2
    fi
    AC_MSG_NOTICE([checking bundled Ruby/GLib2 $ac_used_bundled_ruby_glib2_version])
    ac_ruby_glib2_dir="binding/ruby/glib2-$ac_used_bundled_ruby_glib2_version"
    if $RUBY -run -e mkdir -- -p $ac_ruby_glib2_dir && \
       (cd $ac_ruby_glib2_dir && \
        RUBYLIB=$ac_abs_ruby_srcdir/pkg-config/lib:$ac_abs_ruby_srcdir/native-package-installer/lib \
          $RUBY ../../../$srcdir/$ac_ruby_glib2_dir/extconf.rb ); then
      ruby_glib2_available=yes
      ac_ruby_glib2_source_dir="$ac_ruby_glib2_dir/ext/glib2"
      RUBY_GLIB2_CFLAGS="-I\$(top_builddir)/$ac_ruby_glib2_source_dir"
      RUBY_GLIB2_CFLAGS="$RUBY_GLIB2_CFLAGS -I\$(top_srcdir)/$ac_ruby_glib2_source_dir"
      sed -e '/^install-headers:/a\
xxx	$(MAKEDIRS) $(RUBYARCHDIR)' \
	  $ac_ruby_glib2_dir/ext/glib2/Makefile | \
          sed -e 's/^xxx//' > \
	  $ac_ruby_glib2_dir/ext/glib2/Makefile.fix-install-headers
      mv $ac_ruby_glib2_dir/ext/glib2/Makefile.fix-install-headers \
	 $ac_ruby_glib2_dir/ext/glib2/Makefile
      sed -e '/^pre-install-rb:/a\
xxx	$(MAKEDIRS) $(RUBYLIBDIR)/gnome2/rake' \
          $ac_ruby_glib2_dir/Makefile.lib | \
          sed -e 's/^xxx//' > \
          $ac_ruby_glib2_dir/Makefile.lib.fix-pre-install-rb
      mv $ac_ruby_glib2_dir/Makefile.lib.fix-pre-install-rb \
         $ac_ruby_glib2_dir/Makefile.lib
      sed -e "s,^CPPFLAGS = ,CPPFLAGS = ${CPPFLAGS} ," \
          $ac_ruby_glib2_source_dir/Makefile > \
          $ac_ruby_glib2_source_dir/Makefile.fix_cppflags
      mv $ac_ruby_glib2_source_dir/Makefile.fix_cppflags \
         $ac_ruby_glib2_source_dir/Makefile
      sed -e 's/ install/ RUBYARCHDIR=$(binding_extdir) RUBYLIBDIR=$(binding_libdir) libdir=$(libdir) install/' \
          $ac_ruby_glib2_dir/Makefile > \
          $ac_ruby_glib2_dir/Makefile.fix_install
      mv $ac_ruby_glib2_dir/Makefile.fix_install \
         $ac_ruby_glib2_dir/Makefile
      cat <<EOM >> $ac_ruby_glib2_dir/Makefile

srcdir_prefix = `test "${srcdir}" = . || echo "../../../$srcdir/$ac_ruby_glib2_dir/"`
prefix = $prefix
exec_prefix = $milter_manager_exec_prefix
libdir = \$(DESTDIR)$libdir
pkglibdir = \$(libdir)/$PACKAGE_NAME
bindingdir = $bindingdir
binding_extdir = $binding_extdir
binding_libdir = $binding_libdir
RUBY = $RUBY

check:
# 	\$(RUBY) \$(srcdir_prefix)test/run-test.rb

distdir:

dist:

uninstall:
EOM
    fi
  fi

  for bundled_available_ruby_glib2_version in ${bundled_available_ruby_glib2_versions}; do
    if test "$ac_used_bundled_ruby_glib2_version" != "${bundled_available_ruby_glib2_version}"; then
      $RUBY -run -e mkdir -- -p binding/ruby/glib2-${bundled_available_ruby_glib2_version}
      cat <<EOM > binding/ruby/glib2-${bundled_available_ruby_glib2_version}/Makefile
all:
distdir:
distclean:
uninstall:
EOM
    fi
  done

  if test "$ruby_glib2_available" != "yes"; then
    AC_MSG_ERROR([Ruby/GLib2 is required.])
  fi

  AC_SUBST(RUBY_GLIB2_CFLAGS)

  AC_ARG_WITH([bundled-ruby-gobject-introspection],
	      AS_HELP_STRING([--with-bundled-ruby-gobject-introspection],
			     [Use bundled Ruby/GObjectIntrospection
                              (default: try system Ruby/GObjectIntrospection and then
                                            bundled Ruby/GObjectIntrospection)]),
	      [use_bundled_ruby_gobject_introspection="$withval"],
              [use_bundled_ruby_gobject_introspection="auto"])

  RUBY_GOBJECT_INTROSPECTION_CFLAGS="$RUBY_GLIB2_CFLAGS"
  ruby_gobject_introspection_available=no
  if test "x$use_bundled_ruby_gobject_introspection" != "xyes"; then
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
    if test "$ruby_gobject_introspection_available" = "no"; then
      RUBY_GOBJECT_INTROSPECTION_CFLAGS=$($RUBY -e '
        begin
          require "rubygems"
          ruby_gobject_introspection_gem = Gem::Specification.find_by_name("gobject-introspection")
        rescue LoadError
          ruby_gobject_introspection_gem = nil
        end
	if ruby_gobject_introspection_gem.nil?
	  begin
	    require "gobject_introspection"
	    ruby_gobject_introspection_path = $".grep(/gobject-introspection\.(?!rb\z)\w+\z/).first
	    exit(false) if ruby_gobject_introspection_path.nil?
	    print("-I ", File.dirname(ruby_gobject_introspection_path))
	    exit(true)
	  rescue LoadError
	    exit(false)
	  end
	end
	ruby_gobject_introspection_path = ruby_gobject_introspection_gem.full_gem_path
	print("-I ", File.join(ruby_gobject_introspection_path, "ext", "gobject-introspection"))')
      if test $? -eq 0; then
        CFLAGS="$CFLAGS $RUBY_GOBJECT_INTROSPECTION_CFLAGS"
      else
        RUBY_GOBJECT_INTROSPECTION_CFLAGS=""
      fi
      AC_CHECK_HEADERS(rbgiversion.h,
		       [ruby_gobject_introspection_available="yes"],
		       [ruby_gobject_introspection_available="no"],
		       [$ruby_undef_package_macros])
    fi
    if test "$ruby_gobject_introspection_available" = "yes"; then
      AC_TRY_COMPILE([
		      $ruby_undef_package_macros
		      #include <rbgobject.h>
		      #include <rb-gobject_introspection.h>
		     ],
		     [
		      #if RBGLIB_MAJOR_VERSION == 3 && RBGLIB_MINOR_VERSION == 4 && RBGLIB_MICRO_VERSION == 9
  		      #  if GI_MAJOR_VERSION 1 && GI_MINOR_VERSION == 46 && RBGLIB_MICRO_VERSION == 0
		      #  else
		      #    error "Ruby/GObjectIntrospection must be 1.46.0"
		      #  endif
		      #else
		      #  error "Ruby/GOBJECT_INTROSPECTION must be 3.4.9"
		      #endif
		     ],
		     [],
		     [ruby_gobject_introspection_available="no"])
    fi
  fi

  if test "x$use_bundled_ruby_gobject_introspection" != "xno" -a \
          "x$ruby_gobject_introspection_available" != "xyes"; then
    ac_used_bundled_ruby_gobject_introspection_version=${bundled_latest_ruby_gobject_introspection_version}
    if test "x$use_bundled_ruby_gobject_introspection" != "xauto" -a \
            "x$use_bundled_ruby_gobject_introspection" != "xyes" -a \
            "x$use_bundled_ruby_gobject_introspection" != "xno"; then
      ac_used_bundled_ruby_gobject_introspection_version=$use_bundled_ruby_gobject_introspection
    fi
    AC_MSG_NOTICE([checking bundled Ruby/GObjectIntrospection $ac_used_bundled_ruby_gobject_introspection_version])
    ac_ruby_gobject_introspection_dir="binding/ruby/gobject-introspection-$ac_used_bundled_ruby_gobject_introspection_version"
    if $RUBY -run -e mkdir -- -p $ac_ruby_gobject_introspection_dir && \
       (cd $ac_ruby_gobject_introspection_dir && \
        RUBYLIB=$ac_abs_ruby_srcdir/pkg-config/lib:$ac_abs_ruby_srcdir/native-package-installer/lib \
          $RUBY ../../../$srcdir/$ac_ruby_gobject_introspection_dir/extconf.rb); then
      ruby_gobject_introspection_available=yes
      ac_ruby_gobject_introspection_source_dir="$ac_ruby_gobject_introspection_dir/ext/gobject-introspection"
      RUBY_GOBJECT_INTROSPECTION_CFLAGS="-I \$(top_srcdir)/$ac_ruby_gobject_introspection_source_dir"
      sed -e '/^install-headers:/a\
xxx	$(MAKEDIRS) $(RUBYARCHDIR)' \
	  $ac_ruby_gobject_introspection_dir/ext/gobject-introspection/Makefile | \
          sed -e 's/^xxx//' > \
	  $ac_ruby_gobject_introspection_dir/ext/gobject-introspection/Makefile.fix-install-headers
      mv $ac_ruby_gobject_introspection_dir/ext/gobject-introspection/Makefile.fix-install-headers \
	 $ac_ruby_gobject_introspection_dir/ext/gobject-introspection/Makefile
      sed -e '/^pre-install-rb:/a\
xxx	$(MAKEDIRS) $(RUBYLIBDIR)/gnome2/rake' \
          $ac_ruby_gobject_introspection_dir/Makefile.lib | \
          sed -e 's/^xxx//' > \
          $ac_ruby_gobject_introspection_dir/Makefile.lib.fix-pre-install-rb
      mv $ac_ruby_gobject_introspection_dir/Makefile.lib.fix-pre-install-rb \
         $ac_ruby_gobject_introspection_dir/Makefile.lib
      sed -e "s,^CPPFLAGS = ,CPPFLAGS = ${CPPFLAGS} ," \
          $ac_ruby_gobject_introspection_source_dir/Makefile > \
          $ac_ruby_gobject_introspection_source_dir/Makefile.fix_cppflags
      mv $ac_ruby_gobject_introspection_source_dir/Makefile.fix_cppflags \
         $ac_ruby_gobject_introspection_source_dir/Makefile
      sed -e 's/ install/ RUBYARCHDIR=$(binding_extdir) RUBYLIBDIR=$(binding_libdir) libdir=$(libdir) install/' \
          $ac_ruby_gobject_introspection_dir/Makefile > \
          $ac_ruby_gobject_introspection_dir/Makefile.fix_install
      mv $ac_ruby_gobject_introspection_dir/Makefile.fix_install \
         $ac_ruby_gobject_introspection_dir/Makefile
      cat <<EOM >> $ac_ruby_gobject_introspection_dir/Makefile

srcdir_prefix = `test "${srcdir}" = . || echo "../../../$srcdir/$ac_ruby_gobject_introspection_dir/"`
prefix = $prefix
exec_prefix = $milter_manager_exec_prefix
libdir = \$(DESTDIR)$libdir
pkglibdir = \$(libdir)/$PACKAGE_NAME
bindingdir = $bindingdir
binding_extdir = $binding_extdir
binding_libdir = $binding_libdir
RUBY = $RUBY

check:
# 	\$(RUBY) \$(srcdir_prefix)test/run-test.rb

distdir:

dist:

uninstall:
EOM
    fi
  fi

  for bundled_available_ruby_gobject_introspection_version in ${bundled_available_ruby_gobject_introspection_versions}; do
    if test "$ac_used_bundled_ruby_gobject_introspection_version" != "${bundled_available_ruby_gobject_introspection_version}"; then
      $RUBY -run -e mkdir -- -p binding/ruby/gobject-introspection-${bundled_available_ruby_gobject_introspection_version}
      cat <<EOM > binding/ruby/gobject-introspection-${bundled_available_ruby_gobject_introspection_version}/Makefile
all:
distdir:
distclean:
uninstall:
EOM
    fi
  done

  if test "$ruby_gobject_introspection_available" != "yes"; then
    AC_MSG_ERROR([Ruby/GObjectIntrospection is required.])
  fi

  AC_SUBST(RUBY_GOBJECT_INTROSPECTION_CFLAGS)

  AC_ARG_WITH([bundled-ruby-gio2],
	      AS_HELP_STRING([--with-bundled-ruby-gio2],
			     [Use bundled Ruby/GIO2
                              (default: try system Ruby/GIO2 and then
                                            bundled Ruby/GIO2)]),
	      [use_bundled_ruby_gio2="$withval"],
              [use_bundled_ruby_gio2="auto"])

  RUBY_GIO2_CFLAGS="$RUBY_GLIB2_CFLAGS"
  ruby_gio2_available=no
  if test "x$use_bundled_ruby_gio2" != "xyes"; then
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
    if test "$ruby_gio2_available" = "no"; then
      RUBY_GIO2_CFLAGS=$($RUBY -e '
        begin
          require "rubygems"
          ruby_gio2_gem = Gem::Specification.find_by_name("gio2")
        rescue LoadError
          ruby_gio2_gem = nil
        end
	if ruby_gio2_gem.nil?
	  begin
	    require "gio2"
	    ruby_gio2_path = $".grep(/gio2\.(?!rb\z)\w+\z/).first
	    exit(false) if ruby_gio2_path.nil?
	    print("-I ", File.dirname(ruby_gio2_path))
	    exit(true)
	  rescue LoadError
	    exit(false)
	  end
	end
	ruby_gio2_path = ruby_gio2_gem.full_gem_path
	print("-I ", File.join(ruby_gio2_path, "ext", "gio2"))')
      if test $? -eq 0; then
        CFLAGS="$CFLAGS $RUBY_GIO2_CFLAGS"
      else
        RUBY_GIO2_CFLAGS=""
      fi
      AC_CHECK_HEADERS(rbgobject.h,
		       [ruby_gio2_available="yes"],
		       [ruby_gio2_available="no"],
		       [$ruby_undef_package_macros])
    fi
    if test "$ruby_gio2_available" = "yes"; then
      AC_TRY_COMPILE([
		      $ruby_undef_package_macros
		      #include <rbgobject.h>
		      #include <rb-gio2.h>
		     ],
		     [
		      #if RBGLIB_MAJOR_VERSION == 3 && RBGLIB_MINOR_VERSION == 4 && RBGLIB_MICRO_VERSION == 9
		      #else
		      #  error "Ruby/GIO2 must be 3.4.9"
		      #endif
		     ],
		     [],
		     [ruby_gio2_available="no"])
    fi
  fi

  if test "x$use_bundled_ruby_gio2" != "xno" -a \
          "x$ruby_gio2_available" != "xyes"; then
    ac_used_bundled_ruby_gio2_version=${bundled_latest_ruby_gio2_version}
    if test "x$use_bundled_ruby_gio2" != "xauto" -a \
            "x$use_bundled_ruby_gio2" != "xyes" -a \
            "x$use_bundled_ruby_gio2" != "xno"; then
      ac_used_bundled_ruby_gio2_version=$use_bundled_ruby_gio2
    fi
    AC_MSG_NOTICE([checking bundled Ruby/GIO2 $ac_used_bundled_ruby_gio2_version])
    ac_ruby_gio2_dir="binding/ruby/gio2-$ac_used_bundled_ruby_gio2_version"
    if $RUBY -run -e mkdir -- -p $ac_ruby_gio2_dir && \
       (cd $ac_ruby_gio2_dir && \
        RUBYLIB=$ac_abs_ruby_srcdir/pkg-config/lib:$ac_abs_ruby_srcdir/native-package-installer/lib \
          $RUBY ../../../$srcdir/$ac_ruby_gio2_dir/extconf.rb); then
      ruby_gio2_available=yes
      ac_ruby_gio2_source_dir="$ac_ruby_gio2_dir/ext/gio2"
      RUBY_GIO2_CFLAGS="-I \$(top_srcdir)/$ac_ruby_gio2_source_dir"
      sed -e '/^install-headers:/a\
xxx	$(MAKEDIRS) $(RUBYARCHDIR)' \
	  $ac_ruby_gio2_dir/ext/gio2/Makefile | \
          sed -e 's/^xxx//' > \
	  $ac_ruby_gio2_dir/ext/gio2/Makefile.fix-install-headers
      mv $ac_ruby_gio2_dir/ext/gio2/Makefile.fix-install-headers \
	 $ac_ruby_gio2_dir/ext/gio2/Makefile
      sed -e '/^pre-install-rb:/a\
xxx	$(MAKEDIRS) $(RUBYLIBDIR)/gnome2/rake' \
          $ac_ruby_gio2_dir/Makefile.lib | \
          sed -e 's/^xxx//' > \
          $ac_ruby_gio2_dir/Makefile.lib.fix-pre-install-rb
      mv $ac_ruby_gio2_dir/Makefile.lib.fix-pre-install-rb \
         $ac_ruby_gio2_dir/Makefile.lib
      sed -e "s,^CPPFLAGS = ,CPPFLAGS = ${CPPFLAGS} ," \
          $ac_ruby_gio2_source_dir/Makefile > \
          $ac_ruby_gio2_source_dir/Makefile.fix_cppflags
      mv $ac_ruby_gio2_source_dir/Makefile.fix_cppflags \
         $ac_ruby_gio2_source_dir/Makefile
      sed -e 's/ install/ RUBYARCHDIR=$(binding_extdir) RUBYLIBDIR=$(binding_libdir) libdir=$(libdir) install/' \
          $ac_ruby_gio2_dir/Makefile > \
          $ac_ruby_gio2_dir/Makefile.fix_install
      mv $ac_ruby_gio2_dir/Makefile.fix_install \
         $ac_ruby_gio2_dir/Makefile
      cat <<EOM >> $ac_ruby_gio2_dir/Makefile

srcdir_prefix = `test "${srcdir}" = . || echo "../../../$srcdir/$ac_ruby_gio2_dir/"`
prefix = $prefix
exec_prefix = $milter_manager_exec_prefix
libdir = \$(DESTDIR)$libdir
pkglibdir = \$(libdir)/$PACKAGE_NAME
bindingdir = $bindingdir
binding_extdir = $binding_extdir
binding_libdir = $binding_libdir
RUBY = $RUBY

check:
# 	\$(RUBY) \$(srcdir_prefix)test/run-test.rb

distdir:

dist:

uninstall:
EOM
    fi
  fi

  for bundled_available_ruby_gio2_version in ${bundled_available_ruby_gio2_versions}; do
    if test "$ac_used_bundled_ruby_gio2_version" != "${bundled_available_ruby_gio2_version}"; then
      $RUBY -run -e mkdir -- -p binding/ruby/gio2-${bundled_available_ruby_gio2_version}
      cat <<EOM > binding/ruby/gio2-${bundled_available_ruby_gio2_version}/Makefile
all:
distdir:
distclean:
uninstall:
EOM
    fi
  done

  if test "$ruby_gio2_available" != "yes"; then
    AC_MSG_ERROR([Ruby/GIO2 is required.])
  fi

  AC_SUBST(RUBY_GIO2_CFLAGS)

  AC_CHECK_FUNCS(rb_errinfo rb_fork rb_last_status_set)
  AC_CHECK_FUNCS(rb_thread_check_ints)
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
AM_CONDITIONAL([WITH_BUNDLED_RUBY_GLIB2_2_2_5],
               [test "$ac_used_bundled_ruby_glib2_version" = "2.2.5"])
AM_CONDITIONAL([WITH_BUNDLED_RUBY_GLIB2_LATEST],
               [test "$ac_used_bundled_ruby_glib2_version" = "${bundled_latest_ruby_glib2_version}"])
AM_CONDITIONAL([WITH_RUBY_MILTER], [test -n "$ruby_milterdir"])

dnl **************************************************************
dnl Checks for RD2
dnl **************************************************************
rd2_available="no"
AC_ARG_WITH([rd2],
            AS_HELP_STRING([--with-rd2=PATH],
                           [rd2 command path. (default: auto-detect)]),
            [RD2="$withval"])
if test "$RD2" = "no"; then
  : # ignore
elif test "$RD2" = ""; then
  AC_PATH_PROG(RD2, rd2, rd2-not-found)
  if test "$RD2" != "rd2-not-found"; then
    rd2_available="yes"
  fi
else
  AC_CHECK_FILE([$RD2],
                [rd2_available="yes"],
                [AC_MSG_WARN([$RD2 is not found.])])
fi

if test "$rd2_available" = "yes" -o -f "$srcdir/doc/man-build.stamp"; then
  man_available=yes
else
  man_available=no
fi
AM_CONDITIONAL([MAN_AVAILABLE], [test "$man_available" = "yes"])

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
GTK_DOC_CHECK([1.8], [--flavour no-tmpl])
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
libev_version=""
m4_define([BUNDLED_LIBEV_VERSION], [4.19])
LIBEV_CFLAGS='-DEV_COMPAT3=0'
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
    AC_CHECK_FILE([$libev_prefix/include/ev.h],
                  [],
                  [
                    CFLAGS="$_SAVE_CFLAGS"
                    LDFLAGS="$_SAVE_LDFLAGS"
                    continue
                  ])
    CFLAGS="$CFLAGS -I$libev_prefix/include ${LIBEV_CFLAGS}"
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
      LDFLAGS="$LDFLAGS -lev"
      AC_MSG_CHECKING([libev version])
      AC_RUN_IFELSE(
        [AC_LANG_PROGRAM(
          [
          @%:@include <stdio.h>
          @%:@include <ev.h>
          ],
          [printf("%d.%d\n", ev_version_major(), ev_version_minor());])
        ],
        [libev_version=`./conftest$EXEEXT`],
        [AC_MSG_RESULT(no)])
    fi
    CFLAGS="$_SAVE_CFLAGS"
    LDFLAGS="$_SAVE_LDFLAGS"
    if test "x$libev_available" != "xno"; then
      LIBEV_INCLUDES="-I$libev_prefix/include"
      LIBEV_LA=""
      LIBEV_LIBS="-L$libev_prefix/lib -lev"
      break
    fi
  done
  if test "$libev_available" = no; then
    libev_available=yes
    libev_prefix=bundled
    libev_version=BUNDLED_LIBEV_VERSION
    m4_include([libev-]BUNDLED_LIBEV_VERSION[/libev.m4])
    AC_DEFINE(HAVE_LIBEV, [1], [Define to 1 if libev is available.])
    LIBEV_INCLUDES='-I$(top_srcdir)/libev-'BUNDLED_LIBEV_VERSION
    LIBEV_LA='$(top_builddir)/libev-'BUNDLED_LIBEV_VERSION'/libev.la'
    LIBEV_LIBS='$(LIBEV_LA)'
  fi
  AC_DEFINE(HAVE_LIBEV, [1], [Define to 1 if libev is available.])
fi

if test "x$libev_available" = "xno"; then
  AC_MSG_ERROR([libev is required.])
fi

AM_CONDITIONAL(WITH_BUNDLED_LIBEV, [test "x$libev_prefix" = xbundled])
AC_SUBST(LIBEV_INCLUDES)
AC_SUBST(LIBEV_CFLAGS)
AC_SUBST(LIBEV_LA)
AC_SUBST(LIBEV_LIBS)

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
                           [Default connection spec. (default: auto)]),
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
  case "$MILTER_MANAGER_PACKAGE_PLATFORM" in
  debian)
    case `lsb_release -c --short` in
    precise)
      MILTER_MANAGER_DEFAULT_CONNECTION_SPEC=['unix:/var/run/milter-manager/milter-manager.sock']
      ;;
    *)
      MILTER_MANAGER_DEFAULT_CONNECTION_SPEC=['unix:/run/milter-manager/milter-manager.sock']
    ;;
    esac
    ;;
  redhat|freebsd|pkgsrc)
    MILTER_MANAGER_DEFAULT_CONNECTION_SPEC=['unix:/var/run/milter-manager/milter-manager.sock']
    ;;
  *)
    MILTER_MANAGER_DEFAULT_CONNECTION_SPEC=['inet:10025@[127.0.0.1]']
    ;;
  esac
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
dnl Check for misc.
dnl **************************************************************

AC_ARG_WITH([cutter-source-path],
            AS_HELP_STRING([--with-cutter-source-path=PATH],
                             [Specify Cutter source path for
                             milter-manager's release manager.]),
              [CUTTER_SOURCE_PATH="$withval"],
              [CUTTER_SOURCE_PATH="auto"])
case "$CUTTER_SOURCE_PATH" in
auto)
  CUTTER_SOURCE_PATH="\$(top_srcdir)/vendor/cutter-source"
  ;;
""|/*)
  : # do nothing
  ;;
*)
  CUTTER_SOURCE_PATH="\$(top_builddir)/${CUTTER_SOURCE_PATH}"
  ;;
esac
AC_SUBST(CUTTER_SOURCE_PATH)

AC_ARG_WITH([groonga-source-path],
            AS_HELP_STRING([--with-groonga-source-path=PATH],
                             [Specify Groonga source path for
                             milter-manager's release manager.]),
              [GROONGA_SOURCE_PATH="$withval"],
              [GROONGA_SOURCE_PATH=""])
case "$GROONGA_SOURCE_PATH" in
""|/*)
  : # do nothing
  ;;
*)
  GROONGA_SOURCE_PATH="\$(top_builddir)/${GROONGA_SOURCE_PATH}"
  ;;
esac
AC_SUBST(GROONGA_SOURCE_PATH)

dnl **************************************************************
dnl Check for osdn.net.
dnl **************************************************************

OSDN_PROJECT_ID="milter-manager"
AC_SUBST(OSDN_PROJECT_ID)
OSDN_PROJECT_NAME="milter manager"
AC_SUBST(OSDN_PROJECT_NAME)

AC_ARG_WITH([osdn-user],
            AS_HELP_STRING([--with-osdn-user=NAME],
                           [Use NAME to access osdn.net]),
            [OSDN_USER="$withval"])
AC_SUBST(OSDN_USER)

OSDN_USER_OPTION=""
if test -n "$OSDN_USER"; then
    OSDN_USER_OPTION="--username $OSDN_USER"
fi
AC_SUBST(OSDN_USER_OPTION)

OSDN_WEB_HOST="shell.osdn.net"
if test -n "$OSDN_USER"; then
    OSDN_WEB_HOST="$OSDN_USER@$OSDN_WEB_HOST"
fi
AC_SUBST(OSDN_WEB_HOST)

OSDN_PROJECT_ID_HEAD_1=`echo $OSDN_PROJECT_ID | sed -e 's/^\(.\).*/\1/'`
OSDN_PROJECT_ID_HEAD_2=`echo $OSDN_PROJECT_ID | sed -e 's/^\(..\).*/\1/'`
OSDN_HTDOCS_PATH="/home/groups/${OSDN_PROJECT_ID_HEAD_1}/${OSDN_PROJECT_ID_HEAD_2}/${OSDN_PROJECT_ID}/htdocs"
AC_SUBST(OSDN_HTDOCS_PATH)
OSDN_HTDOCS="$OSDN_WEB_HOST:$OSDN_HTDOCS_PATH"
AC_SUBST(OSDN_HTDOCS)


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
MILTER_RUBY_LIBS="\$(GLIB_LIBS) \$(RUBY_LDFLAGS) \$(RUBY_LIBS)"
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
		 ruby-milter-core.pc
		 ruby-milter-client.pc
		 ruby-milter-server.pc
		 package/rpm/centos/milter-manager.spec
		 libev-]BUNDLED_LIBEV_VERSION[/Makefile
		 milter/Makefile
		 milter/core/Makefile
		 milter/core/milter-version.h
		 milter/client/Makefile
		 libmilter/Makefile
		 milter/server/Makefile
		 milter/manager/Makefile
		 binding/Makefile
		 binding/ruby/Makefile
		 binding/ruby/ext/Makefile
		 binding/ruby/ext/core/Makefile
		 binding/ruby/ext/client/Makefile
		 binding/ruby/ext/server/Makefile
		 binding/ruby/ext/manager/Makefile
		 binding/ruby/lib/Makefile
		 binding/ruby/lib/milter/Makefile
		 binding/ruby/lib/milter/core/Makefile
		 binding/ruby/lib/milter/client/Makefile
		 binding/ruby/lib/milter/server/Makefile
		 binding/ruby/lib/milter/manager/Makefile
		 binding/ruby/ruby-milter/Makefile
		 binding/ruby/ruby-milter/milter.rb
		 binding/ruby/ruby-milter/milter/Makefile
		 binding/ruby/ruby-milter/milter/client.rb
		 binding/ruby/ruby-milter/milter/core.rb
		 binding/ruby/ruby-milter/milter/server.rb
		 binding/ruby/test/Makefile
		 binding/ruby/test/core/Makefile
		 binding/ruby/test/client/Makefile
		 binding/ruby/test/server/Makefile
		 binding/ruby/test/manager/Makefile
		 binding/ruby/test/manager/applicable-conditions/Makefile
		 binding/ruby/sample/Makefile
		 module/Makefile
		 module/configuration/Makefile
		 module/configuration/ruby/Makefile
		 src/Makefile
		 data/Makefile
		 data/applicable-conditions/Makefile
		 data/defaults/Makefile
		 data/models/Makefile
		 data/packet/Makefile
		 data/rc.d/Makefile
		 data/init.d/Makefile
		 data/init.d/redhat/Makefile
		 data/init.d/redhat/sysconfig/Makefile
		 data/init.d/solaris/Makefile
		 data/cron.d/Makefile
		 data/cron.d/redhat/Makefile
		 data/cron.d/freebsd/Makefile
		 data/munin/Makefile
		 data/smf/Makefile
		 data/smf/manifest/Makefile
		 data/smf/manifest/milter-manager.xml
		 data/smf/method/Makefile
		 data/systemd/Makefile
		 data/systemd/redhat/Makefile
		 package/solaris/Makefile
		 package/solaris/prototypes/Makefile
		 package/solaris/prototypes/milter-manager/Makefile
		 package/solaris/prototypes/milter-manager/pkginfo
		 package/solaris/prototypes/cutter/Makefile
		 package/solaris/prototypes/cutter/pkginfo
		 package/solaris/prototypes/gettext/Makefile
		 package/solaris/prototypes/gettext/pkginfo
		 package/solaris/prototypes/glib/Makefile
		 package/solaris/prototypes/glib/pkginfo
		 package/solaris/prototypes/iconv/Makefile
		 package/solaris/prototypes/iconv/pkginfo
		 package/solaris/prototypes/mysql/Makefile
		 package/solaris/prototypes/mysql/pkginfo
		 package/solaris/prototypes/ruby/Makefile
		 package/solaris/prototypes/ruby/pkginfo
		 tool/Makefile
		 test/Makefile
		 test/lib/Makefile
		 test/fixtures/Makefile
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
		 html/Makefile
		 license/Makefile
		 build/Makefile
		 package/Makefile
		 package/apt/Makefile
		 package/ubuntu/Makefile
		 package/rpm/Makefile
		 package/rpm/centos/Makefile
		 package/yum/Makefile
                 vendor/Makefile])

AC_OUTPUT

glib_version="$glib_config_major_version.$glib_config_minor_version.$glib_config_micro_version"

if test "$libev_available" = "yes"; then
   libev_configure_result="$libev_version ($libev_prefix)"
else
   libev_configure_result="$libev_version ($libev_available)"
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
echo "  Ruby version              : `$RUBY -v`"
echo "  Ruby/GLib2                : $RUBY_GLIB2_CFLAGS"
echo "  Ruby/GObjectIntrospection : $RUBY_GOBJECT_INTROSPECTION_CFLAGS"
echo "  Ruby/GIO2                 : $RUBY_GIO2_CFLAGS"
echo "  Ruby milter PATH          : $ruby_milterdir"
echo
echo "  rd2                     : $RD2"
echo "  GTK-Doc                 : $enable_gtk_doc"
echo
echo "  Cutter                  : $CUTTER"
echo "  LCOV                    : $LCOV"
echo "  coverage                : $cutter_enable_coverage"
echo "  coverage report by LCOV : $cutter_enable_coverage_report_lcov"
echo
echo "  Cutter Source Path      : $CUTTER_SOURCE_PATH"
echo
echo "  Launchpad PGP key       : ${LAUNCHPAD_UPLOADER_PGP_KEY}"
echo "  GPG UID                 : ${GPG_UID}"
