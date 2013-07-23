AM_CPPFLAGS = 			\
	 -I$(top_builddir)	\
	 -I$(top_srcdir)

AM_CFLAGS = 					\
	-DLOCALEDIR=\""$(localedir)"\"		\
	-DMILTER_LOG_DOMAIN=\""milter-core"\"	\
	$(GLIB_CFLAGS)				\
	$(COVERAGE_CFLAGS)

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_core_public_headers =		\
	milter-protocol.h  		\
	milter-decoder.h    		\
	milter-command-decoder.h	\
	milter-reply-decoder.h		\
	milter-encoder.h		\
	milter-command-encoder.h	\
	milter-reply-encoder.h		\
	milter-error-emittable.h	\
	milter-finished-emittable.h	\
	milter-agent.h			\
	milter-protocol-agent.h		\
	milter-macros-requests.h	\
	milter-option.h			\
	milter-reader.h			\
	milter-writer.h			\
	milter-headers.h		\
	milter-logger.h			\
	milter-syslog-logger.h		\
	milter-reply-signals.h		\
	milter-utils.h			\
	milter-connection.h		\
	milter-esmtp.h			\
	milter-message-result.h		\
	milter-session-result.h		\
	milter-memory-profile.h		\
	milter-event-loop.h		\
	milter-libev-event-loop.h	\
	milter-glib-event-loop.h

enum_source_prefix = milter-enum-types
enum_sources_h =			\
	$(milter_core_public_headers)

marshal_source_prefix = milter-marshalers
marshal_prefix = _milter_marshal

milter_core_includedir=$(pkgincludedir)/milter/core
milter_core_include_HEADERS =		\
	milter-enum-types.h		\
	milter-version.h		\
	$(milter_core_public_headers)

lib_LTLIBRARIES =		\
	libmilter-core.la

libmilter_core_la_SOURCES =		\
	milter-enum-types.c		\
	milter-marshalers.c		\
	milter-marshalers.h		\
	milter-core.c			\
	milter-protocol.c  		\
	milter-decoder.c		\
	milter-command-decoder.c	\
	milter-reply-decoder.c		\
	milter-encoder.c		\
	milter-command-encoder.c	\
	milter-reply-encoder.c		\
	milter-error-emittable.c	\
	milter-finished-emittable.c	\
	milter-agent.c			\
	milter-protocol-agent.c		\
	milter-macros-requests.c	\
	milter-option.c			\
	milter-reader.c			\
	milter-writer.c			\
	milter-headers.c		\
	milter-logger.c			\
	milter-syslog-logger.c		\
	milter-reply-signals.c		\
	milter-utils.c			\
	milter-connection.c		\
	milter-esmtp.c			\
	milter-message-result.c		\
	milter-session-result.c		\
	milter-memory-profile.c		\
	milter-event-loop.c		\
	milter-glib-event-loop.c

libmilter_core_la_LIBADD =		\
	$(MILTER_CORE_LIBS)
libmilter_core_la_CFLAGS =		\
	$(AM_CFLAGS)
libmilter_core_la_DEPENDENCIES =

if WITH_LIBEV
libmilter_core_la_SOURCES += milter-libev-event-loop.c
libmilter_core_la_DEPENDENCIES += $(LIBEV_LA)
libmilter_core_la_LIBADD += $(LIBEV_LIBS)
libmilter_core_la_CFLAGS += $(LIBEV_CFLAGS)
AM_CPPFLAGS += $(LIBEV_INCLUDES)
endif

include $(top_srcdir)/build/enum-types.mk
include $(top_srcdir)/build/marshal.mk
