AM_CPPFLAGS = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)

AM_CFLAGS =					\
	-DMILTER_LOG_DOMAIN=\""libmilter"\"	\
	$(GLIB_CFLAGS)				\
	$(COVERAGE_CFLAGS)

libmilter_includedir=$(pkgincludedir)/libmilter/libmilter
libmilter_include_HEADERS =	\
	mfapi.h			\
	mfdef.h

pkglib_LTLIBRARIES =		\
	libmilter.la

libmilter_la_SOURCES =		\
	libmilter-compatible.c	\
	libmilter-compatible.h

libmilter_la_LIBADD =						\
	$(top_builddir)/milter/client/libmilter-client.la

libmilter_la_LDFLAGS =					\
	-version-info $(LIBMILTER_LT_VERSION_INFO)
