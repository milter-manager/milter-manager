INCLUDES = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)		\
	 -I$(top_srcdir)/milter-core	\
	 -I$(top_srcdir)/milter-client

AM_CFLAGS = 				\
	$(GLIB_CFLAGS)			\
	$(COVERAGE_CFLAGS)

libmilter_includedir=$(pkgincludedir)/libmilter/libmilter
libmilter_include_HEADERS =	\
	mfapi.h			\
	mfdef.h

pkglib_LTLIBRARIES =		\
	libmilter.la

libmilter_la_SOURCES =		\
	libmilter-compatible.c

libmilter_la_LIBADD =						\
	$(top_builddir)/milter-client/libmilter-client.la

