INCLUDES = 			\
	 -I$(top_builddir)	\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(GLIB_CFLAGS)			\
	$(COVERAGE_CFLAGS)

milter_client_public_headers =		\
	mc-parser.h

milter_client_includedir=$(pkgincludedir)/milter-client
milter_client_include_HEADERS =		\
	$(milter_client_public_headers)

lib_LTLIBRARIES =		\
	libmilter-client.la

libmilter_client_la_SOURCES =	\
	mc-parser.c
