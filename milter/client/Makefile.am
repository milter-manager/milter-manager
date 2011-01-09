INCLUDES = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)

AM_CFLAGS =					\
	-DLOCALEDIR=\""$(localedir)"\"		\
	-DMILTER_LOG_DOMAIN=\""milter-client"\"	\
	$(GLIB_CFLAGS)				\
	$(COVERAGE_CFLAGS)

if GIO_UNIX_2_24
AM_CFLAGS +=					\
	$(GIO_UNIX_CFLAGS)
else
INCLUDES +=					\
	-I$(top_srcdir)/gio-unix-compatible
endif

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_client_public_headers =		\
	milter-client.h			\
	milter-client-context.h		\
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

libmilter_client_la_SOURCES =		\
	milter-client-enum-types.c	\
	milter-client.c			\
	milter-client-init.c		\
	milter-client-context.c

libmilter_client_la_LIBADD =					\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(MILTER_CLIENT_LIBS)

if GIO_UNIX_2_24
libmilter_client_la_LIBADD +=			\
	$(GIO_UNIX_LIBS)
else
libmilter_client_la_LIBADD +=							\
	$(top_builddir)/gio-unix-compatible/gio/libgio-unix-compatible.la
endif


include $(top_srcdir)/build/enum-types.mk
