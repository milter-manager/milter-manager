AM_CPPFLAGS = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)

AM_CFLAGS =					\
	-DLOCALEDIR=\""$(localedir)"\"		\
	-DMILTER_LOG_DOMAIN=\""milter-client"\"	\
	$(GLIB_CFLAGS)				\
	$(COVERAGE_CFLAGS)

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_client_public_headers =			\
	milter-client.h				\
	milter-client-context.h			\
	milter-client-runner.h			\
	milter-client-single-thread-runner.h	\
	milter-client-objects.h

enum_source_prefix = milter-client-enum-types
enum_sources_h =			\
	$(milter_client_public_headers)

milter_client_includedir=$(pkgincludedir)/milter/client
milter_client_include_HEADERS =		\
	milter-client-enum-types.h	\
	milter-client-private.h		\
	$(milter_client_public_headers)

lib_LTLIBRARIES =		\
	libmilter-client.la

libmilter_client_la_SOURCES =			\
	milter-client-enum-types.c		\
	milter-client.c				\
	milter-client-main.c			\
	milter-client-context.c			\
	milter-client-runner.c			\
	milter-client-single-thread-runner.c

libmilter_client_la_LIBADD =					\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(MILTER_CLIENT_LIBS)

include $(top_srcdir)/build/enum-types.mk
