INCLUDES = 			\
	 -I$(top_builddir)	\
	 -I$(top_srcdir)

AM_CFLAGS = 					\
	-DLOCALEDIR=\""$(localedir)"\"		\
	$(GLIB_CFLAGS)				\
	$(COVERAGE_CFLAGS)

LIBS = $(top_builddir)/milter/core/libmilter-core.la

bin_PROGRAMS =			\
	milter-test-client	\
	milter-test-server

milter_test_client_SOURCE = milter-test-client.c
milter_test_client_LDADD = $(top_builddir)/milter/client/libmilter-client.la
milter_test_client_CFLAGS =				\
	$(AM_CFLAGS)					\
	-DMILTER_LOG_DOMAIN=\""milter-test-client"\"

milter_test_server_SOURCE = milter-test-server.c
milter_test_server_LDADD = $(top_builddir)/milter/server/libmilter-server.la
milter_test_server_CFLAGS =				\
	$(AM_CFLAGS)					\
	-DMILTER_LOG_DOMAIN=\""milter-test-server"\"

