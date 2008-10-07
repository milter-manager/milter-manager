INCLUDES = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(GLIB_CFLAGS)			\
	$(COVERAGE_CFLAGS)

MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_server_public_headers =		\
	milter-server-context.h

enum_source_prefix = milter-server-enum-types
enum_sources_h =		\
	$(milter_server_public_headers)

milter_server_includedir=$(pkgincludedir)/milter/server
milter_server_include_HEADERS =		\
	milter-server-enum-types.h	\
	$(milter_server_public_headers)

lib_LTLIBRARIES =		\
	libmilter-server.la

libmilter_server_la_SOURCES =		\
	milter-server-enum-types.c	\
	milter-server-context.c

libmilter_server_la_LIBADD =					\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/core/libmilter-core.la


include $(top_srcdir)/enum-types.mk
