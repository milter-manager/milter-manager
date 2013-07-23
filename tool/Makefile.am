AM_CPPFLAGS = 			\
	 -I$(top_builddir)	\
	 -I$(top_srcdir)

AM_CFLAGS = 					\
	-DLOCALEDIR=\""$(localedir)"\"		\
	$(GLIB_CFLAGS)				\
	$(COVERAGE_CFLAGS)

bin_PROGRAMS =					\
	milter-test-client			\
	milter-test-client-libmilter		\
	milter-test-server

milter_test_client_SOURCE = milter-test-client.c
milter_test_client_LDADD = 					\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(GLIB_LIBS)
milter_test_client_CFLAGS =				\
	$(AM_CFLAGS)					\
	-DMILTER_LOG_DOMAIN=\""milter-test-client"\"

milter_test_client_libmilter_SOURCE = milter-test-client-libmilter.c
milter_test_client_libmilter_LDADD =				\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(top_builddir)/libmilter/libmilter.la			\
	$(GLIB_LIBS)
milter_test_client_libmilter_CFLAGS =				\
	$(AM_CFLAGS)						\
	-DMILTER_LOG_DOMAIN=\""milter-test-client-libmilter"\"

milter_test_server_SOURCE = milter-test-server.c
milter_test_server_LDADD = 					\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(GLIB_LIBS)
milter_test_server_CFLAGS =				\
	$(AM_CFLAGS)					\
	-DMILTER_LOG_DOMAIN=\""milter-test-server"\"

dist_bin_SCRIPTS =			\
	milter-performance-check	\
	milter-manager-log-analyzer	\
	milter-report-statistics

#	milter-performance-analyzer
