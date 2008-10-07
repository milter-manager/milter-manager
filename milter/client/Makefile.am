INCLUDES = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(GLIB_CFLAGS)			\
	$(COVERAGE_CFLAGS)

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_client_public_headers =		\
	milter-client-context.h

enum_source_prefix = milter-client-enum-types
enum_sources_h =			\
	$(milter_client_public_headers)

marshal_source_prefix = milter-client-marshalers
marshal_prefix = _milter_client_marshal

milter_client_includedir=$(pkgincludedir)/milter/client
milter_client_include_HEADERS =		\
	milter-client-enum-types.h	\
	$(milter_client_public_headers)

lib_LTLIBRARIES =		\
	libmilter-client.la

libmilter_client_la_SOURCES =		\
	milter-client-enum-types.c	\
	milter-client-marshalers.c	\
	milter-client-marshalers.h	\
	milter-client.c			\
	milter-client-context.c

libmilter_client_la_LIBADD =				\
	$(top_builddir)/milter/core/libmilter-core.la

include $(top_srcdir)/enum-types.mk
include $(top_srcdir)/marshal.mk
