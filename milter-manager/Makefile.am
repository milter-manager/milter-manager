INCLUDES = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)		\
	 -I$(top_srcdir)/milter-core	\
	 -I$(top_srcdir)/milter-client	\
	 -I$(top_srcdir)/milter-server

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(GLIB_CFLAGS)			\
	$(RUBY_CFLAGS)			\
	$(COVERAGE_CFLAGS)

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_manager_public_headers =		\
	milter-manager-context.h	\
	milter-manager-configure.h	\
	milter-manager.h

enum_source_prefix = milter-manager-enum-types
enum_sources_h =			\
	$(milter_manager_public_headers)

marshal_source_prefix = milter-manager-marshalers
marshal_prefix = _milter_manager_marshal

pkginclude_HEADERS =	\
	milter-manager.h

milter_manager_includedir=$(pkgincludedir)/milter-manager
milter_manager_include_HEADERS =	\
	milter-manager-enum-types.h	\
	$(milter_manager_public_headers)

bin_PROGRAMS =		\
	milter-manager

milter_manager_SOURCES =		\
	milter-manager-enum-types.c	\
	milter-manager-marshalers.c	\
	milter-manager-marshalers.h	\
	milter-manager.c		\
	milter-manager-context.c	\
	milter-manager-main.c		\
	milter-manager-configure.c

milter_manager_LDADD =						\
	$(top_builddir)/milter-client/libmilter-client.la	\
	$(top_builddir)/milter-server/libmilter-server.la	\
	$(RUBY_LIBS)

include $(top_srcdir)/enum-types.mk
include $(top_srcdir)/marshal.mk
