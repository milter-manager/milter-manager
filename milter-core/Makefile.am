INCLUDES = 			\
	 -I$(top_builddir)	\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(GLIB_CFLAGS)			\
	$(COVERAGE_CFLAGS)

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_core_public_headers =	\
	milter-protocol.h	\
	milter-decoder.h	\
	milter-encoder.h	\
	milter-handler.h	\
	milter-option.h		\
	milter-writer.h		\
	milter-utils.h

enum_source_prefix = milter-enum-types
enum_sources_h =			\
	$(milter_core_public_headers)

marshal_source_prefix = milter-marshalers
marshal_prefix = _milter_marshal

pkginclude_HEADERS =	\
	milter-core.h

milter_core_includedir=$(pkgincludedir)/milter-core
milter_core_include_HEADERS =		\
	milter-enum-types.h		\
	$(milter_core_public_headers)

lib_LTLIBRARIES =		\
	libmilter-core.la

libmilter_core_la_SOURCES =		\
	milter-enum-types.c		\
	milter-marshalers.c		\
	milter-marshalers.h		\
	milter-decoder.c		\
	milter-encoder.c		\
	milter-handler.c		\
	milter-option.c			\
	milter-writer.c			\
	milter-utils.c

libmilter_core_la_LIBADD =		\
	$(GLIB_LIBS)

include $(top_srcdir)/enum-types.mk
include $(top_srcdir)/marshal.mk
